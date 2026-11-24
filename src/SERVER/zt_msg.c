#include "zt_inc.h"

#define MSG_LIST_BUDGET (DATA_MAX_LEN - 256)
#define MSG_PAYLOAD_MAX_LEN (MSG_TEXT_MAX_LEN * 6 + 256)

static int msg_respond(int sock_fd, req_t *request, int status, zt_err_code_t err_code,
		       const char *data_json)
{
	res_t *res = generate_response(request, status);
	int rc;

	if (res == NULL)
		return ZT_RC_REDIS;

	generate_response_body(res, (err_code == ZT_ERR_SUCCESS) ? RESULT_SUCCESS : RESULT_FAIL,
			       err_code, (err_code == ZT_ERR_SUCCESS) ? data_json : NULL);
	rc = hdl_send_http_json(sock_fd, res);
	free(res);

	return (rc == ZT_RC_OK) ? ZT_RC_OK : ZT_RC_SOCKET;
}

static int msg_hexval(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int msg_query_value(const char *uri, const char *key, char *out, size_t out_size)
{
	const char *p = strchr(uri, '?');
	size_t klen = strlen(key);

	if (p == NULL || out_size == 0)
		return ZT_RC_ARG_INVALID;
	p++;

	while (*p) {
		const char *end = p + strcspn(p, "&#");

		if ((size_t)(end - p) > klen && !strncmp(p, key, klen) && p[klen] == '=') {
			const char *v = p + klen + 1;
			size_t o = 0;

			while (v < end) {
				char c = *v;

				if (c == '%' && v + 2 < end && msg_hexval(v[1]) >= 0 && msg_hexval(v[2]) >= 0) {
					c = (char)(msg_hexval(v[1]) * 16 + msg_hexval(v[2]));
					v += 2;
				} else if (c == '+') {
					c = ' ';
				}
				if (o + 1 >= out_size)
					return ZT_RC_ARG_INVALID;
				out[o++] = c;
				v++;
			}
			out[o] = '\0';
			return (o == 0 || strpbrk(out, "\"\\") != NULL) ? ZT_RC_ARG_INVALID : ZT_RC_OK;
		}

		if (*end != '&')
			break;
		p = end + 1;
	}

	return ZT_RC_ARG_INVALID;
}

static int msg_check_access(const char *session, const char *room_id, char *user_id,
			    size_t user_id_size, int *status, zt_err_code_t *err)
{
	room_t room;
	int rc;

	rc = session_get_user(session, user_id, user_id_size);
	if (rc != ZT_RC_OK) {
		*status = (rc == ZT_RC_FAIL) ? HTTP_UNAUTHORIZED : HTTP_INTERNAL_SERVER_ERROR;
		*err = (rc == ZT_RC_FAIL) ? ZT_ERR_INVALID_SESSION : ZT_ERR_SERVER_ERROR;
		return ZT_RC_FAIL;
	}

	if (redis_room_get(room_id, &room) != ZT_RC_OK) {
		*status = HTTP_NOT_FOUND;
		*err = ZT_ERR_ROOM_NOT_FOUND;
		return ZT_RC_FAIL;
	}

	rc = room_is_member(room_id, user_id);
	if (rc <= 0) {
		*status = (rc == 0) ? HTTP_FORBIDDEN : HTTP_INTERNAL_SERVER_ERROR;
		*err = (rc == 0) ? ZT_ERR_NOT_MEMBER : ZT_ERR_SERVER_ERROR;
		return ZT_RC_FAIL;
	}

	return ZT_RC_OK;
}

int room_is_member(const char *room_id, const char *user_id)
{
	redisContext *ctx = redis_get_ctx();
	redisReply *reply;
	char key[KEY_MAX_LEN];
	int rc = -1;

	if (ctx == NULL || room_id == NULL || user_id == NULL)
		return -1;

	snprintf(key, sizeof(key), ROOM_MEMBERS_KEY_FMT, room_id);
	reply = redisCommand(ctx, "SISMEMBER %s %s", key, user_id);
	if (reply && reply->type == REDIS_REPLY_INTEGER)
		rc = (reply->integer > 0) ? 1 : 0;
	if (reply)
		freeReplyObject(reply);

	return rc;
}

int room_member_add(const char *room_id, const char *user_id)
{
	redisContext *ctx = redis_get_ctx();
	char members_key[KEY_MAX_LEN];
	char rooms_key[KEY_MAX_LEN];
	redisReply *reply;

	if (ctx == NULL || room_id == NULL || user_id == NULL || user_id[0] == '\0')
		return ZT_RC_ARG_INVALID;

	snprintf(members_key, sizeof(members_key), ROOM_MEMBERS_KEY_FMT, room_id);
	snprintf(rooms_key, sizeof(rooms_key), USER_ROOMS_KEY_FMT, user_id);

	reply = redisCommand(ctx, "SADD %s %s", members_key, user_id);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if (reply)
			freeReplyObject(reply);
		return ZT_RC_REDIS;
	}
	freeReplyObject(reply);

	reply = redisCommand(ctx, "SADD %s %s", rooms_key, room_id);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if (reply)
			freeReplyObject(reply);
		return ZT_RC_REDIS;
	}
	freeReplyObject(reply);

	LOG_MSG("[msg] room member add: room=%s user=%s\n", room_id, user_id);
	return ZT_RC_OK;
}

int room_members_json(const char *room_id, char *out, size_t out_size)
{
	redisContext *ctx = redis_get_ctx();
	redisReply *reply;
	char key[KEY_MAX_LEN];
	size_t o = 0;
	size_t i;

	if (ctx == NULL || room_id == NULL || out == NULL || out_size < 3)
		return ZT_RC_ARG_INVALID;

	snprintf(key, sizeof(key), ROOM_MEMBERS_KEY_FMT, room_id);
	reply = redisCommand(ctx, "SMEMBERS %s", key);
	if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
		if (reply)
			freeReplyObject(reply);
		return ZT_RC_REDIS;
	}

	out[o++] = '[';
	for (i = 0; i < reply->elements; i++) {
		redisReply *e = reply->element[i];
		char esc[USER_ID_MAX_LEN * 6 + 1];
		int n;

		if (!ZT_REPLY_IS_STR(e) || msg_json_escape(e->str, esc, sizeof(esc)) < 0)
			continue;

		n = snprintf(out + o, out_size - o, "%s\"%s\"", (o > 1) ? "," : "", esc);
		if (n < 0 || (size_t)n >= out_size - o - 1)
			break;
		o += (size_t)n;
	}
	out[o++] = ']';
	out[o] = '\0';

	freeReplyObject(reply);
	return ZT_RC_OK;
}

int http_send_message(int sock_fd, const char *buf, req_t *request)
{
	char room_id[ROOM_ID_MAX_LEN] = {0};
	char session[SESSION_TOKEN_SIZE + 1] = {0};
	char user_id[USER_ID_MAX_LEN] = {0};
	char text[MSG_TEXT_MAX_LEN + 1];
	char text_esc[MSG_TEXT_MAX_LEN * 6 + 1];
	char payload[MSG_PAYLOAD_MAX_LEN];
	char data_json[96];
	char key[KEY_MAX_LEN];
	char channel[KEY_MAX_LEN];
	redisContext *ctx = redis_get_ctx();
	redisReply *reply;
	const char *body;
	long long msg_id;
	long ts;
	int status = HTTP_BAD_REQUEST;
	zt_err_code_t err = ZT_ERR_INVALID_PARAMS;

	if (sock_fd < 0 || buf == NULL || request == NULL)
		return ZT_RC_ARG_INVALID;

	if (strcmp(request->str_method, "POST") != 0)
		return msg_respond(sock_fd, request, HTTP_METHOD_NOT_ALLOWED, ZT_ERR_METHOD_NOT_ALLOWED, NULL);

	if (ctx == NULL)
		return msg_respond(sock_fd, request, HTTP_INTERNAL_SERVER_ERROR, ZT_ERR_SERVER_ERROR, NULL);

	body = strstr(buf, "\r\n\r\n");
	body = body ? body + 4 : buf;

	if (msg_copy_json_string_field(body, ROOM_ID_KEY, room_id, sizeof(room_id)) != ZT_RC_OK ||
	    msg_copy_json_string_field(body, SESSION_ID_KEY, session, sizeof(session)) != ZT_RC_OK ||
	    msg_copy_json_text_field(body, TEXT_KEY, text, sizeof(text)) != ZT_RC_OK)
		return msg_respond(sock_fd, request, HTTP_BAD_REQUEST, ZT_ERR_INVALID_PARAMS, NULL);

	if (msg_check_access(session, room_id, user_id, sizeof(user_id), &status, &err) != ZT_RC_OK)
		return msg_respond(sock_fd, request, status, err, NULL);

	snprintf(key, sizeof(key), ROOM_MSG_SEQ_KEY_FMT, room_id);
	reply = redisCommand(ctx, "INCR %s", key);
	if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
		if (reply)
			freeReplyObject(reply);
		return msg_respond(sock_fd, request, HTTP_INTERNAL_SERVER_ERROR, ZT_ERR_SERVER_ERROR, NULL);
	}
	msg_id = reply->integer;
	freeReplyObject(reply);

	ts = (long)time(NULL);
	if (msg_json_escape(text, text_esc, sizeof(text_esc)) < 0 ||
	    snprintf(payload, sizeof(payload),
		     "{\"type\":\"message\",\"id\":%lld,\"room_id\":\"%s\",\"from\":\"%s\",\"text\":\"%s\",\"ts\":%ld}",
		     msg_id, room_id, user_id, text_esc, ts) >= (int)sizeof(payload))
		return msg_respond(sock_fd, request, HTTP_BAD_REQUEST, ZT_ERR_TOO_LARGE, NULL);

	/* Store first so history survives a failed publish. */
	snprintf(key, sizeof(key), ROOM_MSGS_KEY_FMT, room_id);
	reply = redisCommand(ctx, "LPUSH %s %s", key, payload);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if (reply)
			freeReplyObject(reply);
		return msg_respond(sock_fd, request, HTTP_INTERNAL_SERVER_ERROR, ZT_ERR_SERVER_ERROR, NULL);
	}
	freeReplyObject(reply);

	reply = redisCommand(ctx, "LTRIM %s 0 %d", key, ROOM_MSGS_KEEP - 1);
	if (reply)
		freeReplyObject(reply);

	snprintf(channel, sizeof(channel), "%s%s", ROOM_CHANNEL_PREFIX, room_id);
	reply = redisCommand(ctx, "PUBLISH %s %s", channel, payload);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR)
		LOG_MSG("[msg] publish fail: channel=%s\n", channel);
	if (reply)
		freeReplyObject(reply);

	LOG_MSG("[msg] room=%s from=%s id=%lld len=%zu\n", room_id, user_id, msg_id, strlen(text));

	snprintf(data_json, sizeof(data_json), "{\"id\":%lld,\"ts\":%ld}", msg_id, ts);
	return msg_respond(sock_fd, request, HTTP_OK, ZT_ERR_SUCCESS, data_json);
}

int http_list_messages(int sock_fd, const char *buf, req_t *request)
{
	char room_id[ROOM_ID_MAX_LEN] = {0};
	char session[SESSION_TOKEN_SIZE + 1] = {0};
	char user_id[USER_ID_MAX_LEN] = {0};
	char key[KEY_MAX_LEN];
	char *data_json;
	redisContext *ctx = redis_get_ctx();
	redisReply *reply;
	size_t used = 2; /* "[]" */
	size_t keep = 0;
	size_t i, o;
	int status = HTTP_BAD_REQUEST;
	zt_err_code_t err = ZT_ERR_INVALID_PARAMS;
	int rc;

	(void)buf;

	if (sock_fd < 0 || request == NULL)
		return ZT_RC_ARG_INVALID;

	if (strcmp(request->str_method, "GET") != 0)
		return msg_respond(sock_fd, request, HTTP_METHOD_NOT_ALLOWED, ZT_ERR_METHOD_NOT_ALLOWED, NULL);

	if (ctx == NULL)
		return msg_respond(sock_fd, request, HTTP_INTERNAL_SERVER_ERROR, ZT_ERR_SERVER_ERROR, NULL);

	if (msg_query_value(request->str_uri, "room_id", room_id, sizeof(room_id)) != ZT_RC_OK ||
	    msg_query_value(request->str_uri, "session_id", session, sizeof(session)) != ZT_RC_OK)
		return msg_respond(sock_fd, request, HTTP_BAD_REQUEST, ZT_ERR_INVALID_PARAMS, NULL);

	if (msg_check_access(session, room_id, user_id, sizeof(user_id), &status, &err) != ZT_RC_OK)
		return msg_respond(sock_fd, request, status, err, NULL);

	snprintf(key, sizeof(key), ROOM_MSGS_KEY_FMT, room_id);
	reply = redisCommand(ctx, "LRANGE %s 0 %d", key, ROOM_MSGS_HISTORY - 1);
	if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
		if (reply)
			freeReplyObject(reply);
		return msg_respond(sock_fd, request, HTTP_INTERNAL_SERVER_ERROR, ZT_ERR_SERVER_ERROR, NULL);
	}

	/* LRANGE is newest-first: count how many fit, then emit oldest-first. */
	for (i = 0; i < reply->elements; i++) {
		redisReply *e = reply->element[i];

		if (!ZT_REPLY_IS_STR(e))
			break;
		if (used + e->len + 1 > MSG_LIST_BUDGET)
			break;
		used += e->len + 1;
		keep++;
	}

	data_json = malloc(used + 1);
	if (data_json == NULL) {
		freeReplyObject(reply);
		return msg_respond(sock_fd, request, HTTP_INTERNAL_SERVER_ERROR, ZT_ERR_SERVER_ERROR, NULL);
	}

	o = 0;
	data_json[o++] = '[';
	for (i = keep; i > 0; i--) {
		redisReply *e = reply->element[i - 1];

		if (o > 1)
			data_json[o++] = ',';
		memcpy(data_json + o, e->str, e->len);
		o += e->len;
	}
	data_json[o++] = ']';
	data_json[o] = '\0';
	freeReplyObject(reply);

	rc = msg_respond(sock_fd, request, HTTP_OK, ZT_ERR_SUCCESS, data_json);
	free(data_json);
	return rc;
}
