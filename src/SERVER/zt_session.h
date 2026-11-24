#ifndef _ZT_SESSION_H_
#define _ZT_SESSION_H_

#include <stddef.h>

#define SESSION_TOKEN_LEN 32 /* hex chars */
#define SESSION_TOKEN_SIZE (SESSION_TOKEN_LEN + 1)
#define SESSION_TTL_SEC 86400
#define SESSION_KEY_FMT "session:%s"
#define SESSION_ID_KEY "\"session_id\""

/* Some Redis-compatible servers reply STATUS instead of STRING. */
#define ZT_REPLY_IS_STR(r) \
	((r) != NULL && ((r)->type == REDIS_REPLY_STRING || (r)->type == REDIS_REPLY_STATUS))

int session_create(const char *user_id, char *out_token, size_t out_size);

/* ZT_RC_FAIL: missing or expired */
int session_get_user(const char *token, char *out_user_id, size_t out_size);

#endif
