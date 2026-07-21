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
	char str_key[KEY_MAX_LEN] = "";

	if (user_id == NULL || user_id[0] == '\0')
		return -1;

	snprintf(str_key, sizeof(str_key), USER_KEY_FMT, user_id);
	return redis_exists_key(str_key);
}

int redis_mdn_exists(const char *mdn)
{
	char str_key[KEY_MAX_LEN] = "";

	if (mdn == NULL || mdn[0] == '\0')
		return -1;

	snprintf(str_key, sizeof(str_key), MDN_KEY_FMT, mdn);
	return redis_exists_key(str_key);
}

int redis_sub_user(char *pstr_user_id, int i_len)
{
	char str_user_channel[CHANNEL_MAX_LEN] = "";

	if (pstr_user_id == NULL)
		return ZT_RC_ARG_INVALID;

	if (i_len <= 0 || i_len > ID_MAX_LEN)
		return ZT_RC_ARG_INVALID;

	/* user channel */
	snprintf(str_user_channel, sizeof(str_user_channel), "user:%s", pstr_user_id);
	
	return ZT_RC_OK;
}


int redis_sub_user_list(user_t *out_users)
{
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

