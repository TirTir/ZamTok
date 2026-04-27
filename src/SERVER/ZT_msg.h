#ifndef _ZT_MSG_H_
#define _ZT_MSG_H_

#include "ZT_ctx.h"

/* --- HTTP status (numeric) --- */
#define HTTP_OK                   200
#define HTTP_BAD_REQUEST          400
#define HTTP_UNAUTHORIZED         401
#define HTTP_NOT_FOUND            404
#define HTTP_METHOD_NOT_ALLOWED  405
#define HTTP_SERVER_ERROR         500
#define HTTP_SERVICE_UNAVAILABLE  503

#define ZT_ERR_CODES(X) \
	X(SUCCESS,              "SUCCESS",              "ok") \
	X(FAIL,                 "FAIL",                 "failed") \
	X(MISSING_USER_ID,     "MISSING_USER_ID",      "user_id is required") \
	X(INVALID_PARAMS,      "INVALID_PARAMS",       "invalid parameters") \
	X(USER_NOT_FOUND,      "USER_NOT_FOUND",       "user not found") \
	X(ROOM_NOT_FOUND,      "ROOM_NOT_FOUND",       "room not found") \
	X(WRONG_PASSWORD,      "WRONG_PASSWORD",       "wrong password") \
	X(USER_ALREADY_EXISTS, "USER_ALREADY_EXISTS",  "user already exists") \
	X(ROOM_ALREADY_EXISTS, "ROOM_ALREADY_EXISTS",  "room already exists") \
	X(SERVER_ERROR,        "SERVER_ERROR",         "server error")

typedef enum {
#define X(id, code, msg) ZT_ERR_##id,
	ZT_ERR_CODES(X)
#undef X
	ZT_ERR_COUNT
} ZtErrCode;

const char *ZT_STATUS_TEXT(int status_code);

const char *ZT_ERR_TO_STRING(ZtErrCode e);
#define ZT_ERR_STR(e) (ZT_ERR_TO_STRING((e)))

const char *ZT_ERR_DEFAULT_MESSAGE(ZtErrCode e);
#define ZT_ERR_DEFAULT_MSG(e) (ZT_ERR_DEFAULT_MESSAGE((e)))

#define BODY_FMT "{\"success\":%s,\"code\":\"%s\",\"message\":\"%s\",\"data\":%s}"

ResType_t Generate_Response(ReqType_t *t_request, int status_code);

int Generate_Response_Body(ResType_t *t_response, eResultType_e e_result, ZtErrCode e_code, const char *message);
#endif
