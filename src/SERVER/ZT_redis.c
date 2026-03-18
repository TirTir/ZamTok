#include "ZT_Inc.h"
#include "ZT_redis.h"
#include "ZT_log.h"
#include <hiredis/hiredis.h>
#include <time.h>

static redisContext *g_redis = NULL;

int ZT_REDIS_Connect(const char *host, int port)
{
	if (g_redis != NULL) {
		redisFree(g_redis);
		g_redis = NULL;
	}
	if (host == NULL)
		host = "127.0.0.1";
	if (port <= 0)
		port = 6379;

	g_redis = redisConnect(host, port);
	if (g_redis == NULL || g_redis->err) {
		if (g_redis)
			LOG_ERR("[ZT_REDIS] connect fail: %s\n", g_redis->errstr);
		else
			LOG_ERR("[ZT_REDIS] connect fail: out of memory\n");
		if (g_redis) {
			redisFree(g_redis);
			g_redis = NULL;
		}
		return -1;
	}
	return 0;
}

void ZT_REDIS_Disconnect(void)
{
	if (g_redis) {
		redisFree(g_redis);
		g_redis = NULL;
	}
}

#define USER_KEY_FMT "user:%s"
#define ROOM_KEY_FMT "room:%s"
#define ROOM_KEY_PATTERN "room:*"

int ZT_REDIS_UserSave(const user_t *pt_user)
{
	redisReply *reply = NULL;

	char key[64];
	char created[32];

	if (g_redis == NULL || pt_user == NULL || pt_user->str_user_id[0] == '\0')
		return -2;

	snprintf(key, sizeof(key), USER_KEY_FMT, pt_user->str_user_id);

	reply = redisCommand(g_redis, "EXISTS %s", key);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if (reply) freeReplyObject(reply);
		return -2;
	}
	if (reply->integer == 1) {
		freeReplyObject(reply);
		return -1; /* already exists */
	}

	freeReplyObject(reply);

	snprintf(created, sizeof(created), "%ld", (long)time(NULL));

	reply = redisCommand(g_redis, "HSET %s name %s pwd %s created_at %s",
		key, pt_user->str_name, pt_user->str_pwd, created);

	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if (reply) freeReplyObject(reply);
		return -2;
	}

	LOG_INFO("[ZT_REDIS] user save: %s, name: %s, pwd: %s, created: %s\n", pt_user->str_user_id, pt_user->str_name, pt_user->str_pwd, created);
	
	freeReplyObject(reply);
	return 0;
}

int ZT_REDIS_UserGet(const char *user_id, user_t *out_user)
{
	redisReply *reply = NULL;
	char key[64];

	if (g_redis == NULL || user_id == NULL || out_user == NULL)
		return -2;
	
	memset(out_user, 0, sizeof(*out_user));

	snprintf(key, sizeof(key), USER_KEY_FMT, user_id);

	reply = redisCommand(g_redis, "HMGET %s pwd name", key);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if (reply) freeReplyObject(reply);
		return -2;
	}
	if (reply->type != REDIS_REPLY_ARRAY || reply->elements < 2
		|| reply->element[0]->type == REDIS_REPLY_NIL
		|| reply->element[0]->str == NULL) {
		freeReplyObject(reply);
		return -1; /* not found */
	}

	snprintf(out_user->str_user_id, sizeof(out_user->str_user_id), "%s", user_id);
	snprintf(out_user->str_pwd,     sizeof(out_user->str_pwd),     "%s", reply->element[0]->str);
	
	if (reply->element[1]->type != REDIS_REPLY_NIL && reply->element[1]->str != NULL)
		snprintf(out_user->str_name, sizeof(out_user->str_name), "%s", reply->element[1]->str);
	
	freeReplyObject(reply);
	LOG_INFO("[ZT_REDIS] user get: %s, pwd: %s, name: %s\n", user_id, out_user->str_pwd, out_user->str_name);
	return 0;
}

int ZT_REDIS_RoomSave(const room_t *pt_room)
{
	redisReply *reply = NULL;

	char key[64];
	char created[32];

	if (g_redis == NULL || pt_room == NULL || pt_room->str_room_id[0] == '\0')
		return -2;

	snprintf(key, sizeof(key), ROOM_KEY_FMT, pt_room->str_room_id);

	/* 중복 방 id 체크 */
	reply = redisCommand(g_redis, "EXISTS %s", key);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) 
	{
		if (reply) freeReplyObject(reply);
		return -2;
	}

	if (reply->integer == 1) 
	{
		freeReplyObject(reply);
		return -1; /* already exists */
	}

	freeReplyObject(reply);

	snprintf(created, sizeof(created), "%ld", (long)time(NULL));

	reply = redisCommand(g_redis, "HSET %s pwd %s created_at %s creator_id %s",
		key, pt_room->str_pwd, created, pt_room->str_creator_id);

	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) 
	{
		if (reply) freeReplyObject(reply);
		return -2;
	}

	LOG_INFO("[ZT_REDIS] room save: %s, pwd: %s, created: %s, creator_id: %s\n", pt_room->str_room_id, pt_room->str_pwd, created, pt_room->str_creator_id);
	freeReplyObject(reply);
	return 0;
}

int ZT_REDIS_RoomGet(const char *room_id, room_t *out_room)
{
	redisReply *reply = NULL;
	char key[64];

	if (g_redis == NULL || room_id == NULL || out_room == NULL)
		return -2;
	memset(out_room, 0, sizeof(*out_room));

	snprintf(key, sizeof(key), ROOM_KEY_FMT, room_id);

	/* pwd, creator_id 한 번에 조회 */
	reply = redisCommand(g_redis, "HMGET %s pwd creator_id", key);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if (reply) freeReplyObject(reply);
		return -2;
	}
	
	if (reply->type != REDIS_REPLY_ARRAY || reply->elements < 2
		|| reply->element[0]->type == REDIS_REPLY_NIL
		|| reply->element[0]->str == NULL) {
		freeReplyObject(reply);
		return -1; /* not found */
	}
	snprintf(out_room->str_room_id,    sizeof(out_room->str_room_id),    "%s", room_id);
	snprintf(out_room->str_pwd,        sizeof(out_room->str_pwd),        "%s", reply->element[0]->str);
	
	if (reply->element[1]->type != REDIS_REPLY_NIL && reply->element[1]->str != NULL)
		snprintf(out_room->str_creator_id, sizeof(out_room->str_creator_id), "%s", reply->element[1]->str);
	
	LOG_INFO("[ZT_REDIS] room get: %s, pwd: %s, creator_id: %s\n", room_id, out_room->str_pwd, out_room->str_creator_id);
	freeReplyObject(reply);
	return 0;
}

int ZT_REDIS_RoomList(room_t *out_rooms, size_t max_rooms)
{
	redisReply *reply = NULL;
	size_t count = 0;
	char cursor[32] = "0";

	if (g_redis == NULL || out_rooms == NULL || max_rooms == 0)
		return -2;

	do {
		reply = redisCommand(g_redis, "SCAN %s MATCH " ROOM_KEY_PATTERN " COUNT 50", cursor);
		if (reply == NULL || reply->type != REDIS_REPLY_ARRAY || reply->elements < 2) {
			if (reply) freeReplyObject(reply);
			return -2;
		}

		/* 다음 커서 */
		snprintf(cursor, sizeof(cursor), "%s", reply->element[0]->str ? reply->element[0]->str : "0");

		/* 키 리스트 */
		redisReply *keys = reply->element[1];
		if (keys->type == REDIS_REPLY_ARRAY) {
			for (size_t i = 0; i < keys->elements && count < max_rooms; i++) {
				const char *key = keys->element[i]->str;
				const char *p = NULL;
				redisReply *hreply = NULL;

				if (!key) continue;

				p = strchr(key, ':');
				if (!p || *(p + 1) == '\0')
					continue;

				memset(&out_rooms[count], 0, sizeof(room_t));
				snprintf(out_rooms[count].str_room_id, sizeof(out_rooms[count].str_room_id), "%s", p + 1);

				/* 각 방의 creator_id 조회 */
				hreply = redisCommand(g_redis, "HGET %s creator_id", key);
				if (hreply && hreply->type == REDIS_REPLY_STRING && hreply->str) {
					snprintf(out_rooms[count].str_creator_id,
					         sizeof(out_rooms[count].str_creator_id),
					         "%s", hreply->str);
				}
				if (hreply) {
					freeReplyObject(hreply);
					hreply = NULL;
				}

				count++;
			}
		}

		freeReplyObject(reply);
		reply = NULL;

	} while (cursor[0] != '0' && count < max_rooms);

	LOG_INFO("[ZT_REDIS] room list count: %zu\n", count);
	return (int)count;
}
