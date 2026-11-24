#include "zt_inc.h"

#include <ctype.h>
#include <sys/random.h>

static bool session_token_ok(const char *token)
{
	size_t i;

	if (token == NULL || strlen(token) != SESSION_TOKEN_LEN)
		return false;

	for (i = 0; i < SESSION_TOKEN_LEN; i++) {
		if (!isxdigit((unsigned char)token[i]))
			return false;
	}

	return true;
}

int session_create(const char *user_id, char *out_token, size_t out_size)
{
	static const char hex[] = "0123456789abcdef";
	unsigned char raw[SESSION_TOKEN_LEN / 2];
	char key[KEY_MAX_LEN];
	redisContext *ctx = redis_get_ctx();
	redisReply *reply;
	size_t got = 0;
	size_t i;

	if (ctx == NULL || user_id == NULL || out_token == NULL || out_size < SESSION_TOKEN_SIZE)
		return ZT_RC_ARG_INVALID;

	while (got < sizeof(raw)) {
		ssize_t n = getrandom(raw + got, sizeof(raw) - got, 0);

		if (n < 0) {
			if (errno == EINTR)
				continue;
			LOG_MSG("[session] getrandom fail <%d:%s>\n", errno, strerror(errno));
			return ZT_RC_FAIL;
		}
		got += (size_t)n;
	}

	for (i = 0; i < sizeof(raw); i++) {
		out_token[i * 2] = hex[raw[i] >> 4];
		out_token[i * 2 + 1] = hex[raw[i] & 0x0F];
	}
	out_token[SESSION_TOKEN_LEN] = '\0';

	snprintf(key, sizeof(key), SESSION_KEY_FMT, out_token);
	reply = redisCommand(ctx, "SET %s %s EX %d", key, user_id, SESSION_TTL_SEC);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if (reply)
			freeReplyObject(reply);
		out_token[0] = '\0';
		return ZT_RC_REDIS;
	}
	freeReplyObject(reply);

	LOG_MSG("[session] created for user=%s\n", user_id);
	return ZT_RC_OK;
}

int session_get_user(const char *token, char *out_user_id, size_t out_size)
{
	char key[KEY_MAX_LEN];
	redisContext *ctx = redis_get_ctx();
	redisReply *reply;
	int rc = ZT_RC_FAIL;

	if (out_user_id == NULL || out_size == 0)
		return ZT_RC_ARG_INVALID;
	out_user_id[0] = '\0';

	if (!session_token_ok(token))
		return ZT_RC_FAIL;
	if (ctx == NULL)
		return ZT_RC_REDIS;

	snprintf(key, sizeof(key), SESSION_KEY_FMT, token);
	reply = redisCommand(ctx, "GET %s", key);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		if (reply)
			freeReplyObject(reply);
		return ZT_RC_REDIS;
	}

	if (ZT_REPLY_IS_STR(reply) && reply->len > 0 && reply->len < out_size) {
		memcpy(out_user_id, reply->str, reply->len);
		out_user_id[reply->len] = '\0';
		rc = ZT_RC_OK;
	}
	freeReplyObject(reply);

	if (rc == ZT_RC_OK) {
		/* sliding expiration */
		reply = redisCommand(ctx, "EXPIRE %s %d", key, SESSION_TTL_SEC);
		if (reply)
			freeReplyObject(reply);
	}

	return rc;
}
