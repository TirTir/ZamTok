#include "zt_inc.h"

static redisContext *g_redis_ctx = NULL;

/*--------------------------------------------------------------------------
 *   STATIC FUNCTION PROTOTYPE
 *-------------------------------------------------------------------------*/
static int redis_exists_key(const char *str_key);
static char *redis_trim_token(char *str);
static int redis_add_room_member(const char *room_id, const char *user_id);
static int redis_add_room_members(const char *room_id, const char *members);

/*--------------------------------------------------------------------------
 *   EXTERN FUNCTION 
 *-------------------------------------------------------------------------*/
int redis_connect(const char *str_host, int i_port)
{
	if (g_redis_ctx != NULL) {
		redisFree(g_redis_ctx);
		g_redis_ctx = NULL;
	}

	if (str_host == NULL)
		str_host = "127.0.0.1";

	if (i_port <= 0)
		i_port = 6379;

	g_redis_ctx = redisConnect(str_host, i_port);
	if (g_redis_ctx == NULL || g_redis_ctx->err) 
	{
		if (g_redis_ctx)
			LOG_MSG("[redis] connect fail: %s\n", g_redis_ctx->errstr);
		else
			LOG_MSG("[redis] connect fail: out of memory\n");
		goto err_return;
	}

	return ZT_RC_OK;

err_return:
	if (g_redis_ctx)
		redis_disconnect();

	return ZT_RC_REDIS;
}

void redis_disconnect(void)
{
	if (g_redis_ctx) {
		redisFree(g_redis_ctx);
		g_redis_ctx = NULL;
	}
}

int redis_user_exists(const char *user_id)
{
	char str_key[KEY_MAX_LEN];

	if (user_id == NULL || user_id[0] == '\0')
		return -1;

	snprintf(str_key, sizeof(str_key), USER_KEY_FMT, user_id);
	return redis_exists_key(str_key);
}

int redis_mdn_exists(const char *mdn)
{
	char str_key[KEY_MAX_LEN];

	if (mdn == NULL || mdn[0] == '\0')
		return -1;

	snprintf(str_key, sizeof(str_key), MDN_KEY_FMT, mdn);
	return redis_exists_key(str_key);
}

int redis_user_save(const user_t *user)
{
	redisReply *pt_reply = NULL;
	char str_key[KEY_MAX_LEN];
	char str_mdn_key[KEY_MAX_LEN];
	char str_created[CREATED_MAX_LEN];

	if (g_redis_ctx == NULL || user == NULL || user->user_id[0] == '\0' ||
	    user->mdn[0] == '\0')
		return ZT_RC_ARG_INVALID;

	snprintf(str_key, sizeof(str_key), USER_KEY_FMT, user->user_id);
	snprintf(str_mdn_key, sizeof(str_mdn_key), MDN_KEY_FMT, user->mdn);
	snprintf(str_created, sizeof(str_created), "%ld", (long)time(NULL));

	pt_reply = redisCommand(g_redis_ctx, REDIS_USER_SAVE_CMD, str_key, user->name,
				user->password, user->mdn, str_created);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}
	freeReplyObject(pt_reply);

	pt_reply = redisCommand(g_redis_ctx, REDIS_MDN_SAVE_CMD, str_mdn_key, user->user_id);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}
	freeReplyObject(pt_reply);

	LOG_MSG("[redis] user save: %s, name: %s, mdn: %s, created: %s\n", user->user_id,
		 user->name, user->mdn, str_created);

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

	/* reply error */
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}

	if (pt_reply->type != REDIS_REPLY_ARRAY || pt_reply->elements < 3 ||
	    pt_reply->element[0]->type == REDIS_REPLY_NIL || pt_reply->element[0]->str == NULL) {
		freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}

	snprintf(out_user->user_id, sizeof(out_user->user_id), "%s", user_id);
	snprintf(out_user->password, sizeof(out_user->password), "%s", pt_reply->element[0]->str);

	if (pt_reply->element[1]->type != REDIS_REPLY_NIL && pt_reply->element[1]->str != NULL)
		snprintf(out_user->name, sizeof(out_user->name), "%s", pt_reply->element[1]->str);
	if (pt_reply->element[2]->type != REDIS_REPLY_NIL && pt_reply->element[2]->str != NULL)
		snprintf(out_user->mdn, sizeof(out_user->mdn), "%s", pt_reply->element[2]->str);

	freeReplyObject(pt_reply);
	LOG_MSG("[redis] user get: %s, name: %s, mdn: %s\n", user_id, out_user->name,
		 out_user->mdn);
	return ZT_RC_OK;
}

int redis_user_is_friend(const char *user_id, const char *friend_id)
{
	redisReply *pt_reply = NULL;
	char str_key[KEY_MAX_LEN];
	int is_friend = -1;

	if (g_redis_ctx == NULL || user_id == NULL || friend_id == NULL)
		return -1;

	if (!strcmp(user_id, friend_id))
		return 1;

	snprintf(str_key, sizeof(str_key), FRIEND_KEY_FMT, user_id);
	pt_reply = redisCommand(g_redis_ctx, REDIS_IS_FRIEND_CMD, str_key, friend_id);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR)
		goto done;

	is_friend = (pt_reply->integer == 1) ? 1 : 0;

done:
	if (pt_reply)
		freeReplyObject(pt_reply);
	return is_friend;
}

int redis_friend_list(const char *user_id, friend_t *out_friends, size_t max_friends)
{
	redisReply *pt_reply = NULL;
	char str_key[KEY_MAX_LEN];
	size_t count = 0;

	if (g_redis_ctx == NULL || user_id == NULL || out_friends == NULL || max_friends == 0)
		return ZT_RC_ARG_INVALID;

	memset(out_friends, 0, sizeof(*out_friends) * max_friends);
	snprintf(str_key, sizeof(str_key), FRIEND_KEY_FMT, user_id);

	pt_reply = redisCommand(g_redis_ctx, REDIS_FRIEND_LIST_CMD, str_key);

	/* reply error */
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}

	if (pt_reply->type == REDIS_REPLY_ARRAY) {
		for (size_t i = 0; i < pt_reply->elements && count < max_friends; i++) {
			user_t t_user = {0};
			const char *friend_id = pt_reply->element[i]->str;

			if (friend_id == NULL)
				continue;

			if (redis_user_get(friend_id, &t_user) == ZT_RC_OK) {
				snprintf(out_friends[count].user_id, sizeof(out_friends[count].user_id),
					 "%s", t_user.user_id);
				snprintf(out_friends[count].name, sizeof(out_friends[count].name), "%s",
					 t_user.name);
				snprintf(out_friends[count].mdn, sizeof(out_friends[count].mdn), "%s",
					 t_user.mdn);
			} else {
				snprintf(out_friends[count].user_id, sizeof(out_friends[count].user_id),
					 "%s", friend_id);
			}
			count++;
		}
	}

	freeReplyObject(pt_reply);
	LOG_MSG("[redis] friend list user=%s count=%zu\n", user_id, count);
	return ZT_RC_OK;
}

int redis_room_save(const room_t *room)
{
	redisReply *pt_reply = NULL;
	char str_key[KEY_MAX_LEN];
	char str_created[CREATED_MAX_LEN];
	int exists;
	int rc;

	if (g_redis_ctx == NULL || room == NULL || room->room_id[0] == '\0' ||
	    room->creator_id[0] == '\0')
		return ZT_RC_ARG_INVALID;

	snprintf(str_key, sizeof(str_key), ROOM_KEY_FMT, room->room_id);
	exists = redis_exists_key(str_key);
	if (exists < 0)
		return ZT_RC_REDIS;
	if (exists > 0)
		return ZT_RC_CTX;

	snprintf(str_created, sizeof(str_created), "%ld", (long)time(NULL));

	pt_reply = redisCommand(g_redis_ctx, REDIS_ROOM_SAVE_CMD, str_key, room->password,
				str_created, room->creator_id, room->room_type, room->members,
				room->channel_id);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}
	freeReplyObject(pt_reply);

	rc = redis_add_room_member(room->room_id, room->creator_id);
	if (rc != ZT_RC_OK)
		return rc;

	rc = redis_add_room_members(room->room_id, room->members);
	if (rc != ZT_RC_OK)
		return rc;

	LOG_MSG("[redis] room save: %s, user_id: %s, type: %s, members: %s, channel: %s\n",
		 room->room_id, room->creator_id, room->room_type, room->members,
		 room->channel_id);

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

	if (pt_reply->type != REDIS_REPLY_ARRAY || pt_reply->elements < 5 ||
	    pt_reply->element[0]->type == REDIS_REPLY_NIL || pt_reply->element[0]->str == NULL) {
		freeReplyObject(pt_reply);
		LOG_MSG("[redis] room get: %s, get data fail\n", room_id);
		return ZT_RC_REDIS;
	}

	snprintf(out_room->room_id, sizeof(out_room->room_id), "%s", room_id);
	snprintf(out_room->password, sizeof(out_room->password), "%s", pt_reply->element[0]->str);

	if (pt_reply->element[1]->type != REDIS_REPLY_NIL && pt_reply->element[1]->str != NULL)
		snprintf(out_room->creator_id, sizeof(out_room->creator_id), "%s",
			 pt_reply->element[1]->str);
	if (pt_reply->element[2]->type != REDIS_REPLY_NIL && pt_reply->element[2]->str != NULL)
		snprintf(out_room->room_type, sizeof(out_room->room_type), "%s",
			 pt_reply->element[2]->str);
	if (pt_reply->element[3]->type != REDIS_REPLY_NIL && pt_reply->element[3]->str != NULL)
		snprintf(out_room->members, sizeof(out_room->members), "%s",
			 pt_reply->element[3]->str);
	if (pt_reply->element[4]->type != REDIS_REPLY_NIL && pt_reply->element[4]->str != NULL)
		snprintf(out_room->channel_id, sizeof(out_room->channel_id), "%s",
			 pt_reply->element[4]->str);

	LOG_MSG("[redis] room get: %s, user_id: %s, type: %s\n", room_id, out_room->creator_id,
		 out_room->room_type);

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

	memset(out_rooms, 0, sizeof(*out_rooms) * max_rooms);

	do {
		pt_reply = redisCommand(g_redis_ctx, REDIS_ROOM_LIST_CMD, cursor);
		
		/* reply error  */
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

				if (!key)
					continue;

				p = strchr(key, ':');
				if (!p || *(p + 1) == '\0')
					continue;

				if (redis_room_get(p + 1, &out_rooms[count]) == ZT_RC_OK)
					count++;
			}
		}

		freeReplyObject(pt_reply);
		pt_reply = NULL;
	} while (cursor[0] != '0' && count < max_rooms);

	LOG_MSG("[redis] room list count: %zu\n", count);
	return ZT_RC_OK;
}

int redis_room_invite(const char *room_id, const char *user_id, const char *members)
{
	room_t t_room = {0};
	int rc;

	if (g_redis_ctx == NULL || room_id == NULL || user_id == NULL || members == NULL)
		return ZT_RC_ARG_INVALID;

	rc = redis_room_get(room_id, &t_room);
	if (rc != ZT_RC_OK)
		return rc;

	rc = redis_add_room_members(room_id, members);
	if (rc != ZT_RC_OK)
		return rc;

	LOG_MSG("[redis] room invite: room=%s by=%s members=%s\n", room_id, user_id, members);
	return ZT_RC_OK;
}

/*--------------------------------------------------------------------------
 *   LOCAL FUNCTION
 *-------------------------------------------------------------------------*/
static int redis_exists_key(const char *str_key)
{
	redisReply *pt_reply = NULL;
	int exists = -1;

	if (g_redis_ctx == NULL || str_key == NULL)
		return -1;

	pt_reply = redisCommand(g_redis_ctx, REDIS_EXISTS_CMD, str_key);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR)
		goto done;

	exists = (pt_reply->integer > 0) ? 1 : 0;

done:
	if (pt_reply)
		freeReplyObject(pt_reply);

	return exists;
}

static char *redis_trim_token(char *str)
{
	char *end;

	if (str == NULL)
		return NULL;

	while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')
		str++;

	end = str + strlen(str);
	while (end > str && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' ||
			     end[-1] == '\n')) {
		end--;
	}
	*end = '\0';

	return str;
}

static int redis_add_room_member(const char *room_id, const char *user_id)
{
	redisReply *pt_reply = NULL;
	char str_member_key[KEY_MAX_LEN];
	char str_user_rooms_key[KEY_MAX_LEN];

	if (room_id == NULL || user_id == NULL || user_id[0] == '\0')
		return ZT_RC_ARG_INVALID;

	snprintf(str_member_key, sizeof(str_member_key), ROOM_MEMBERS_KEY_FMT, room_id);
	snprintf(str_user_rooms_key, sizeof(str_user_rooms_key), USER_ROOMS_KEY_FMT, user_id);

	pt_reply = redisCommand(g_redis_ctx, REDIS_ROOM_MEMBER_ADD_CMD, str_member_key, user_id);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}
	freeReplyObject(pt_reply);

	pt_reply = redisCommand(g_redis_ctx, REDIS_USER_ROOM_ADD_CMD, str_user_rooms_key, room_id);
	if (pt_reply == NULL || pt_reply->type == REDIS_REPLY_ERROR) {
		if (pt_reply)
			freeReplyObject(pt_reply);
		return ZT_RC_REDIS;
	}
	freeReplyObject(pt_reply);

	return ZT_RC_OK;
}

static int redis_add_room_members(const char *room_id, const char *members)
{
	char str_members[ROOM_MEMBERS_MAX_LEN];
	char *saveptr = NULL;
	char *token;
	int rc;

	if (members == NULL || members[0] == '\0')
		return ZT_RC_OK;

	snprintf(str_members, sizeof(str_members), "%s", members);

	for (token = strtok_r(str_members, ",", &saveptr); token != NULL;
	     token = strtok_r(NULL, ",", &saveptr)) {
		token = redis_trim_token(token);
		if (token == NULL || token[0] == '\0')
			continue;

		rc = redis_add_room_member(room_id, token);
		if (rc != ZT_RC_OK)
			return rc;
	}

	return ZT_RC_OK;
}

