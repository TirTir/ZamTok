#include "zt_inc.h"

static redisContext *g_redis_ctx = NULL;

int redis_connect(const char *str_host, int i_port)
{
	if (g_redis_ctx != NULL) {
		redisFree(g_redis_ctx);
		g_redis_ctx = NULL;
	}

	/* default host and port */
	if (str_host == NULL)
		str_host = "127.0.0.1";
	if (i_port <= 0)
		i_port = 6379;

	/* redis library connect */
	g_redis_ctx = redisConnect(str_host, i_port);
	
	/* connect fail */
	/* if connect fail, free redis context and return error */
	if (g_redis_ctx == NULL || g_redis_ctx->err) {
		if (g_redis_ctx)
			LOG_MSG("[redis] connect fail: %s\n", g_redis_ctx->errstr);
		else
			LOG_MSG("[redis] connect fail: out of memory\n");

		goto err_return;
	}

	return ZT_RC_OK;

err_return:
	if (g_redis_ctx) {
		redis_disconnect();
	}
	return ZT_RC_REDIS;
}

void redis_disconnect(void)
{
	if (g_redis_ctx) {
		redisFree(g_redis_ctx);
		g_redis_ctx = NULL;
	}
}

int redis_user_save(const user_t *user)
{
	redisReply *pt_reply = NULL;

	char str_key[KEY_MAX_LEN];
	char str_created[CREATED_MAX_LEN];

	if (g_redis_ctx == NULL || user == NULL)
		return ZT_RC_ARG_INVALID;

	snprintf(str_key, sizeof(str_key), USER_KEY_FMT, user->user_id);
	snprintf(str_created, sizeof(str_created), "%ld", (long)time(NULL));

	pt_reply = redisCommand(g_redis_ctx, REDIS_USER_SAVE_CMD, str_key, user->name,
				user->password, str_created);

	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}

	LOG_MSG("[redis] user save: %s, name: %s, pwd: %s, created: %s\n", user->user_id,
		 user->name, user->password, str_created);

	freeReplyObject(pt_reply);

	return ZT_RC_OK;
}

int redis_user_get(const char *user_id, user_t *out_user)
{
	redisReply *pt_reply = NULL;
	char str_key[KEY_MAX_LEN];

	if (g_redis_ctx == NULL || user_id == NULL || out_user == NULL)
		return ZT_RC_ARG_INVALID;

	memset(out_user, 0, sizeof(*out_user));

	snprintf(str_key, sizeof(str_key), USER_KEY_FMT, user_id);

	pt_reply = redisCommand(g_redis_ctx, REDIS_USER_GET_CMD, str_key);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}
	if (pt_reply->type != REDIS_REPLY_ARRAY || pt_reply->elements < 2
	    || pt_reply->element[0]->type == REDIS_REPLY_NIL
	    || pt_reply->element[0]->str == NULL) {
		freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}

	snprintf(out_user->user_id, sizeof(out_user->user_id), "%s", user_id);
	snprintf(out_user->password, sizeof(out_user->password), "%s", pt_reply->element[0]->str);

	if (pt_reply->element[1]->type != REDIS_REPLY_NIL && pt_reply->element[1]->str != NULL)
		snprintf(out_user->name, sizeof(out_user->name), "%s", pt_reply->element[1]->str);

	freeReplyObject(pt_reply);
	LOG_MSG("[redis] user get: %s, pwd: %s, name: %s\n", user_id, out_user->password,
		 out_user->name);
	return ZT_RC_OK;
}

int redis_room_save(const room_t *room)
{
	redisReply *pt_reply = NULL;

	char str_key[KEY_MAX_LEN];
	char str_created[CREATED_MAX_LEN];

	if (g_redis_ctx == NULL || room == NULL || room->room_id[0] == '\0')
		return ZT_RC_ARG_INVALID;

	snprintf(str_key, sizeof(str_key), ROOM_KEY_FMT, room->room_id);

	pt_reply = redisCommand(g_redis_ctx, REDIS_ROOM_SAVE_CMD, str_key);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}

	if (pt_reply->integer == 1) {
		freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}

	freeReplyObject(pt_reply);

	snprintf(str_created, sizeof(str_created), "%ld", (long)time(NULL));

	pt_reply = redisCommand(g_redis_ctx, REDIS_ROOM_SAVE_CMD, str_key, room->password,
				str_created, room->creator_id);

	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}

	LOG_MSG("[redis] room save: %s, pwd: %s, created: %s, creator_id: %s\n", room->room_id,
		 room->password, str_created, room->creator_id);

	freeReplyObject(pt_reply);

	return ZT_RC_OK;
}

int redis_room_get(const char *room_id, room_t *out_room)
{
	redisReply *pt_reply = NULL;
	char str_key[KEY_MAX_LEN];

	if (g_redis_ctx == NULL || room_id == NULL || out_room == NULL)
		return ZT_RC_ARG_INVALID;

	memset(out_room, 0, sizeof(*out_room));

	snprintf(str_key, sizeof(str_key), ROOM_KEY_FMT, room_id);

	pt_reply = redisCommand(g_redis_ctx, REDIS_ROOM_GET_CMD, str_key);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}

	if (pt_reply->type != REDIS_REPLY_ARRAY || pt_reply->elements < 2
	    || pt_reply->element[0]->type == REDIS_REPLY_NIL
	    || pt_reply->element[0]->str == NULL) {
		freeReplyObject(pt_reply);
		LOG_MSG("[redis] room get: %s, get data fail\n", room_id);
		return ZT_RC_REDIS;
	}
	snprintf(out_room->room_id, sizeof(out_room->room_id), "%s", room_id);
	snprintf(out_room->password, sizeof(out_room->password), "%s", pt_reply->element[0]->str);

	if (pt_reply->element[1]->type != REDIS_REPLY_NIL && pt_reply->element[1]->str != NULL)
		snprintf(out_room->creator_id, sizeof(out_room->creator_id), "%s",
			 pt_reply->element[1]->str);

	LOG_MSG("[redis] room get: %s, pwd: %s, creator_id: %s\n", room_id, out_room->password,
		 out_room->creator_id);

	freeReplyObject(pt_reply);

	return ZT_RC_OK;
}

int redis_room_list(room_t *out_rooms, size_t max_rooms)
{
	redisReply *pt_reply = NULL;
	size_t count = 0;
	char cursor[32] = "0";

	if (g_redis_ctx == NULL || out_rooms == NULL || max_rooms == 0)
		return ZT_RC_ARG_INVALID;

	do {
		pt_reply = redisCommand(g_redis_ctx, REDIS_ROOM_LIST_CMD, cursor);
		if (pt_reply == NULL || pt_reply->type != REDIS_REPLY_ARRAY || pt_reply->elements < 2) {
			if (pt_reply)
				freeReplyObject(pt_reply);
			LOG_MSG("[redis] room list: list data fail\n");
			return ZT_RC_REDIS;
		}

		snprintf(cursor, sizeof(cursor), "%s",
			 pt_reply->element[0]->str ? pt_reply->element[0]->str : "0");

		redisReply *keys = pt_reply->element[1];
		if (keys->type == REDIS_REPLY_ARRAY) {
			for (size_t i = 0; i < keys->elements && count < max_rooms; i++) {
				const char *key = keys->element[i]->str;
				const char *p = NULL;
				redisReply *pt_hreply = NULL;

				if (!key)
					continue;

				p = strchr(key, ':');
				if (!p || *(p + 1) == '\0')
					continue;

				memset(&out_rooms[count], 0, sizeof(room_t));
				snprintf(out_rooms[count].room_id, sizeof(out_rooms[count].room_id), "%s",
					 p + 1);

				pt_hreply = redisCommand(g_redis_ctx, REDIS_ROOM_GET_CMD, key);
				if (pt_hreply && pt_hreply->type == REDIS_REPLY_ARRAY && pt_hreply->elements >= 2
				    && pt_hreply->element[1]->type != REDIS_REPLY_NIL
				    && pt_hreply->element[1]->str) {
					snprintf(out_rooms[count].creator_id,
						 sizeof(out_rooms[count].creator_id), "%s",
						 pt_hreply->element[1]->str);
				}
				if (pt_hreply) {
					freeReplyObject(pt_hreply);
					pt_hreply = NULL;
				}

				count++;
			}
		}

		freeReplyObject(pt_reply);
		pt_reply = NULL;

	} while (cursor[0] != '0' && count < max_rooms);

	LOG_MSG("[redis] room list count: %zu\n", count);
	return ZT_RC_OK;
}
