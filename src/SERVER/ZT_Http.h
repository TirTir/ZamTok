#ifndef _ZT_CTX_H_
#define _ZT_CTX_H_

#define METHOD_MAX_LEN 8
#define URI_MAX_LEN 256
#define VERSION_MAX_LEN 16
#define BODY_MAX_LEN 4096
#define HEADER_MAX_COUNT 16

#include "ZT_Inc.h"

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
	X(METHOD_NOT_ALLOWED,  "METHOD_NOT_ALLOWED",  "method not allowed") \
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
} e_error_code_t;

const char *ZT_STATUS_TEXT(int status_code);

const char *ZT_ERR_TO_STRING(ZtErrCode e);
#define ZT_ERR_STR(e) (ZT_ERR_TO_STRING((e)))

const char *ZT_ERR_DEFAULT_MESSAGE(ZtErrCode e);
#define ZT_ERR_DEFAULT_MSG(e) (ZT_ERR_DEFAULT_MESSAGE((e)))

typedef enum 
{
    HEADER_TYPE_CONTENT_TYPE = 0,
    HEADER_TYPE_CONTENT_LENGTH = 1,
} e_header_type_t;

#define NAME_MAX_LEN 16
#define VALUE_MAX_LEN 128
typedef struct 
{
    char name[NAME_MAX_LEN];
    char value[VALUE_MAX_LEN];
} st_header_t;

typedef struct 
{    
	char method[METHOD_MAX_LEN];
    char uri[URI_MAX_LEN];
    char version[VERSION_MAX_LEN];

    /* Header (request) */
	Header_t t_content_header[HEADER_MAX_COUNT];
    int content_cnt;

    /* Body */
	char body[BODY_MAX_LEN];
    
} st_req_t;

typedef enum 
{
    RESULT_SUCCESS = 0,
    RESULT_FAIL = 1,
} e_result_type_t;

#define CODE_MAX_LEN 64
#define MESSAGE_MAX_LEN 128
#define DATA_MAX_LEN 1024
typedef struct
{
    int result;
    char code[CODE_MAX_LEN];
    char message[MESSAGE_MAX_LEN];
    char data[DATA_MAX_LEN];
} st_result_t;

#define REASON_MAX_LEN 64
typedef struct
{
	char version[VERSION_MAX_LEN];
    unsigned int ui_status_code;
    char reason[REASON_MAX_LEN];

    /* Header */
	Header_t t_header[HEADER_MAX_COUNT];
    int header_cnt;

    /* Body */
    st_result_t t_result;
} st_res_t;

#define HEADER_FMT "HTTP/%.15s %d %.63s\r\nContent-Length: %.31s\r\nContent-Type: %.63s\r\n\r\n"
#define BODY_FMT "{\"success\":%s,\"code\":\"%s\",\"message\":\"%s\",\"data\":%s}"

st_res_t Generate_Response(ReqType_t *t_request, int status_code);
int Generate_Response_Body(st_res_t *t_response, eResultType_e e_result, ZtErrCode e_code, const char *data_json);

#endif