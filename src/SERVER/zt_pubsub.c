#include "zt_inc.h"

#include <poll.h>

typedef struct sub_request {
	char channel[KEY_MAX_LEN];
	size_t channel_len;
	struct sub_request *next;
} sub_request_t;

typedef struct {
	redisContext *redis;
	pthread_t thread;
	pthread_mutex_t mutex;
	int wake_pipe[2];
	bool running;
	bool thread_started;
	sub_request_t *request_head;
	sub_request_t *request_tail;
	redis_message_handler_t handler;
	void *handler_userdata;
} pubsub_ctx_t;

static pubsub_ctx_t g_pubsub = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.wake_pipe = {-1, -1},
};

static void pubsub_wake(void)
{
	char byte = 1;

	if (g_pubsub.wake_pipe[1] >= 0 &&
	    write(g_pubsub.wake_pipe[1], &byte, sizeof(byte)) < 0 && errno != EAGAIN)
		LOG_MSG("[redis] pubsub wake fail: %s\n", strerror(errno));
}

static void pubsub_free_requests(sub_request_t *request)
{
	while (request != NULL) {
		sub_request_t *next = request->next;
		free(request);
		request = next;
	}
}

static int pubsub_send_requests(void)
{
	sub_request_t *request;
	int done = 0;

	pthread_mutex_lock(&g_pubsub.mutex);
	request = g_pubsub.request_head;
	g_pubsub.request_head = NULL;
	g_pubsub.request_tail = NULL;
	pthread_mutex_unlock(&g_pubsub.mutex);

	for (sub_request_t *node = request; node != NULL; node = node->next) {
		if (redisAppendCommand(g_pubsub.redis, "SUBSCRIBE %b", node->channel,
				       node->channel_len) != REDIS_OK) {
			pubsub_free_requests(request);
			return ZT_RC_REDIS;
		}
	}

	pubsub_free_requests(request);
	while (!done) {
		if (redisBufferWrite(g_pubsub.redis, &done) != REDIS_OK)
			return ZT_RC_REDIS;
	}

	return ZT_RC_OK;
}

static void pubsub_dispatch(redisReply *reply)
{
	redis_message_handler_t handler;
	void *userdata;
	redisReply *kind;

	if (reply == NULL || reply->type != REDIS_REPLY_ARRAY || reply->elements < 3)
		return;

	kind = reply->element[0];
	if (kind == NULL || kind->str == NULL)
		return;

	if (strcmp(kind->str, "subscribe") == 0) {
		if (reply->element[1] != NULL && reply->element[1]->str != NULL)
			LOG_MSG("[redis] subscribed: %s\n", reply->element[1]->str);
		return;
	}

	if (strcmp(kind->str, "message") != 0 || reply->element[1] == NULL ||
	    reply->element[1]->str == NULL || reply->element[2] == NULL ||
	    reply->element[2]->str == NULL)
		return;

	pthread_mutex_lock(&g_pubsub.mutex);
	handler = g_pubsub.handler;
	userdata = g_pubsub.handler_userdata;
	pthread_mutex_unlock(&g_pubsub.mutex);

	if (handler != NULL) {
		handler(reply->element[1]->str, reply->element[1]->len, reply->element[2]->str,
			reply->element[2]->len, userdata);
	} else {
		LOG_MSG("[redis] message channel=%.*s payload_len=%zu\n",
			 (int)reply->element[1]->len, reply->element[1]->str,
			 reply->element[2]->len);
	}
}

static int pubsub_read_replies(void)
{
	redisReply *reply = NULL;

	if (redisBufferRead(g_pubsub.redis) != REDIS_OK)
		return ZT_RC_REDIS;

	for (;;) {
		if (redisGetReplyFromReader(g_pubsub.redis, (void **)&reply) != REDIS_OK)
			return ZT_RC_REDIS;
		if (reply == NULL)
			break;

		pubsub_dispatch(reply);
		freeReplyObject(reply);
		reply = NULL;
	}

	return ZT_RC_OK;
}

static void *pubsub_listener(void *arg)
{
	struct pollfd fds[2];
	char wake_buf[64];

	(void)arg;
	fds[0].fd = g_pubsub.redis->fd;
	fds[0].events = POLLIN;
	fds[1].fd = g_pubsub.wake_pipe[0];
	fds[1].events = POLLIN;

	for (;;) {
		int rc = poll(fds, 2, -1);
		if (rc < 0) {
			if (errno == EINTR)
				continue;
			break;
		}

		pthread_mutex_lock(&g_pubsub.mutex);
		bool running = g_pubsub.running;
		pthread_mutex_unlock(&g_pubsub.mutex);
		if (!running)
			break;

		if (fds[1].revents & POLLIN) {
			while (read(g_pubsub.wake_pipe[0], wake_buf, sizeof(wake_buf)) > 0)
				;
			if (pubsub_send_requests() != ZT_RC_OK)
				break;
		}

		if (fds[0].revents & POLLIN) {
			if (pubsub_read_replies() != ZT_RC_OK)
				break;
		}

		if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
			break;
	}

	pthread_mutex_lock(&g_pubsub.mutex);
	g_pubsub.running = false;
	pthread_mutex_unlock(&g_pubsub.mutex);
	LOG_MSG("[redis] pubsub listener stopped\n");
	return NULL;
}

static int redis_subscribe(const char *prefix, const char *id, size_t id_len)
{
	sub_request_t *request;
	size_t prefix_len;

	if (prefix == NULL || id == NULL || id_len == 0)
		return ZT_RC_ARG_INVALID;

	prefix_len = strlen(prefix);
	if (prefix_len + id_len >= KEY_MAX_LEN)
		return ZT_RC_ARG_INVALID;

	request = calloc(1, sizeof(*request));
	if (request == NULL)
		return ZT_RC_FAIL;

	memcpy(request->channel, prefix, prefix_len);
	memcpy(request->channel + prefix_len, id, id_len);
	request->channel_len = prefix_len + id_len;

	pthread_mutex_lock(&g_pubsub.mutex);
	if (!g_pubsub.running) {
		pthread_mutex_unlock(&g_pubsub.mutex);
		free(request);
		return ZT_RC_REDIS;
	}

	if (g_pubsub.request_tail == NULL)
		g_pubsub.request_head = request;
	else
		g_pubsub.request_tail->next = request;
	g_pubsub.request_tail = request;
	pthread_mutex_unlock(&g_pubsub.mutex);

	pubsub_wake();
	return ZT_RC_OK;
}

int redis_pubsub_init(const char *host, int port)
{
	redis_pubsub_deinit();

	if (host == NULL)
		host = "127.0.0.1";
	if (port <= 0)
		port = 6379;

	g_pubsub.redis = redisConnect(host, port);
	if (g_pubsub.redis == NULL || g_pubsub.redis->err)
		goto error;

	if (pipe(g_pubsub.wake_pipe) < 0)
		goto error;
	if (fcntl(g_pubsub.wake_pipe[0], F_SETFL, O_NONBLOCK) < 0 ||
	    fcntl(g_pubsub.wake_pipe[1], F_SETFL, O_NONBLOCK) < 0)
		goto error;

	pthread_mutex_lock(&g_pubsub.mutex);
	g_pubsub.running = true;
	pthread_mutex_unlock(&g_pubsub.mutex);

	if (pthread_create(&g_pubsub.thread, NULL, pubsub_listener, NULL) != 0)
		goto error;
	g_pubsub.thread_started = true;
	return ZT_RC_OK;

error:
	if (g_pubsub.redis != NULL && g_pubsub.redis->err)
		LOG_MSG("[redis] pubsub init fail: %s\n", g_pubsub.redis->errstr);
	redis_pubsub_deinit();
	return ZT_RC_REDIS;
}

void redis_pubsub_deinit(void)
{
	pthread_mutex_lock(&g_pubsub.mutex);
	g_pubsub.running = false;
	pthread_mutex_unlock(&g_pubsub.mutex);
	pubsub_wake();

	if (g_pubsub.thread_started) {
		pthread_join(g_pubsub.thread, NULL);
		g_pubsub.thread_started = false;
	}

	if (g_pubsub.wake_pipe[0] >= 0)
		close(g_pubsub.wake_pipe[0]);
	if (g_pubsub.wake_pipe[1] >= 0)
		close(g_pubsub.wake_pipe[1]);
	g_pubsub.wake_pipe[0] = -1;
	g_pubsub.wake_pipe[1] = -1;

	if (g_pubsub.redis != NULL) {
		redisFree(g_pubsub.redis);
		g_pubsub.redis = NULL;
	}

	pthread_mutex_lock(&g_pubsub.mutex);
	pubsub_free_requests(g_pubsub.request_head);
	g_pubsub.request_head = NULL;
	g_pubsub.request_tail = NULL;
	pthread_mutex_unlock(&g_pubsub.mutex);
}

void redis_pubsub_set_message_handler(redis_message_handler_t handler, void *userdata)
{
	pthread_mutex_lock(&g_pubsub.mutex);
	g_pubsub.handler = handler;
	g_pubsub.handler_userdata = userdata;
	pthread_mutex_unlock(&g_pubsub.mutex);
}

int redis_sub_user(const char *user_id, size_t len)
{
	if (user_id == NULL || len == 0 || len >= USER_ID_MAX_LEN ||
	    strnlen(user_id, USER_ID_MAX_LEN) != len)
		return ZT_RC_ARG_INVALID;

	return redis_subscribe(USER_CHANNEL_PREFIX, user_id, len);
}

int redis_sub_room(const char *room_id, size_t len)
{
	if (room_id == NULL || len == 0 || len >= ROOM_ID_MAX_LEN ||
	    strnlen(room_id, ROOM_ID_MAX_LEN) != len)
		return ZT_RC_ARG_INVALID;

	return redis_subscribe(ROOM_CHANNEL_PREFIX, room_id, len);
}
