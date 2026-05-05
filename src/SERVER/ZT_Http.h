#ifndef __ZT_HTTP_H__
#define __ZT_HTTP_H__

#define METHOD_MAX_LEN 8
#define URI_MAX_LEN 256
#define VERSION_MAX_LEN 16
#define BODY_MAX_LEN 4096
#define HEADER_MAX_COUNT 16

#define NAME_MAX_LEN 16
#define VALUE_MAX_LEN 128

#define HEADER_MAX_LEN 256
#define CODE_MAX_LEN 64
#define RES_JSON_MESSAGE_MAX_LEN 128
#define DATA_MAX_LEN 1024
#define REASON_MAX_LEN 64

#define ROOM_MAX_COUNT 64

#define USER_ID_KEY "\"user_id\""
#define NAME_KEY "\"name\""
#define PASSWORD_KEY "\"password\""
#define ROOM_ID_KEY "\"room_id\""

#define HTTP_REASON_MAP(X) \
	X(HTTP_OK, 200, "OK") \
	X(HTTP_BAD_REQUEST, 400, "Bad Request") \
	X(HTTP_UNAUTHORIZED, 401, "Unauthorized") \
	X(HTTP_FORBIDDEN, 403, "Forbidden") \
	X(HTTP_NOT_FOUND, 404, "Not Found") \
	X(HTTP_METHOD_NOT_ALLOWED, 405, "Method Not Allowed") \
	X(HTTP_CONFLICT, 409, "Conflict") \
	X(HTTP_INTERNAL_SERVER_ERROR, 500, "Internal Server Error") \
	X(HTTP_NOT_IMPLEMENTED, 501, "Not Implemented") \
	X(HTTP_BAD_GATEWAY, 502, "Bad Gateway") \
	X(HTTP_SERVICE_UNAVAILABLE, 503, "Service Unavailable") \
	X(HTTP_GATEWAY_TIMEOUT, 504, "Gateway Timeout")

/* HTTP_* 숫자: HTTP_REASON_MAP 한 곳에서만 정의 */
typedef enum {
#define X(name, val, reason) name = val,
	HTTP_REASON_MAP(X)
#undef X
} zt_http_status_e;

#define ZT_ERR_CODES(X) \
	X(SUCCESS, "SUCCESS", "ok") \
	X(FAIL, "FAIL", "failed") \
	X(METHOD_NOT_ALLOWED, "METHOD_NOT_ALLOWED", "method not allowed") \
	X(MISSING_USER_ID, "MISSING_USER_ID", "user_id is required") \
	X(INVALID_PARAMS, "INVALID_PARAMS", "invalid parameters") \
	X(USER_NOT_FOUND, "USER_NOT_FOUND", "user not found") \
	X(ROOM_NOT_FOUND, "ROOM_NOT_FOUND", "room not found") \
	X(WRONG_PASSWORD, "WRONG_PASSWORD", "wrong password") \
	X(USER_ALREADY_EXISTS, "USER_ALREADY_EXISTS", "user already exists") \
	X(ROOM_ALREADY_EXISTS, "ROOM_ALREADY_EXISTS", "room already exists") \
	X(SERVER_ERROR, "SERVER_ERROR", "server error")

typedef enum {
#define X(id, code, msg) ZT_ERR_##id,
	ZT_ERR_CODES(X)
#undef X
	ZT_ERR_COUNT
} zt_err_code_t;

typedef enum {
	HEADER_TYPE_CONTENT_TYPE = 0,
	HEADER_TYPE_CONTENT_LENGTH = 1,
} header_type_t;

typedef enum {
	RESULT_SUCCESS = 0,
	RESULT_FAIL = 1,
} result_kind_t;

typedef struct {
	char str_name[NAME_MAX_LEN];
	char str_value[VALUE_MAX_LEN];
} header_field_t;

typedef struct {
	char str_method[METHOD_MAX_LEN];
	char str_uri[URI_MAX_LEN];
	char str_version[VERSION_MAX_LEN];

	header_field_t t_header_fields[HEADER_MAX_COUNT];
	int i_header_count;
	header_field_t t_content_headers[HEADER_MAX_COUNT];
	int i_content_header_count;
	char str_body[BODY_MAX_LEN];
} req_t;

typedef struct {
	bool b_success;
	char str_code[CODE_MAX_LEN];
	char str_message[RES_JSON_MESSAGE_MAX_LEN];
	char str_data[DATA_MAX_LEN];
} res_body_t;

typedef struct {
	char str_version[VERSION_MAX_LEN];
	unsigned int i_status_code;
	char str_reason[REASON_MAX_LEN];
	
	header_field_t t_header_fields[HEADER_MAX_COUNT];
	int i_header_count;

	res_body_t t_body;
} res_t;

#define HEADER_FMT "HTTP/%.15s %d %.63s\r\nContent-Length: %.31s\r\nContent-Type: %.63s\r\n\r\n"
#define BODY_FMT "{\"success\":%s,\"code\":\"%s\",\"message\":\"%s\",\"data\":%s}"

#define JOIN_DATA_FMT "{\"user_id\":\"%s\"}"
#define LOGIN_DATA_FMT "{\"user_id\":\"%s\"}"
#define CREATE_ROOM_DATA_FMT "{\"room_id\":\"%s\"}"
#define SEARCH_ROOM_DATA_FMT "{\"room_id\":\"%s\"}"
#define JOIN_ROOM_DATA_FMT "{\"room_id\":\"%s\"}"
#define LIST_ROOMS_DATA_FMT "{\"room_id\":\"%s\",\"user_id\":\"%s\"}"

const char *zt_http_status_to_reason(int i_status_code);
const char *zt_err_code_str(zt_err_code_t code);
#define ZT_ERR_STR(e) (zt_err_code_str((e)))
const char *zt_err_default_msg(zt_err_code_t code);
#define ZT_ERR_DEFAULT_MSG(e) (zt_err_default_msg((e)))

#endif