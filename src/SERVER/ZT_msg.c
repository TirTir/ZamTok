#include "ZT_msg.h"
#include <stdio.h>
#include <string.h>

ResType_t Generate_Response(ReqType_t *t_request, int status_code)
{
	ResType_t t_response = {0};
	const char *st = ZT_STATUS_TEXT(status_code);

	/* 1. Version */
	if (t_request) {
		snprintf(t_response.version, sizeof(t_response.version), "%s",
			t_request->version);
	}

	/* 2. Status Code */
	t_response.ui_status_code = (unsigned int)status_code;

	/* 3. Reason */
	snprintf(t_response.reason, sizeof(t_response.reason), "%s", st);

	/* 4. Content-Type */
	if (t_response.header_cnt < HEADER_MAX_COUNT) {
		snprintf(t_response.t_header[HEADER_TYPE_CONTENT_TYPE].name,
			sizeof(t_response.t_header[HEADER_TYPE_CONTENT_TYPE].name), "%s", "Content-Type");
		snprintf(t_response.t_header[HEADER_TYPE_CONTENT_TYPE].value,
			sizeof(t_response.t_header[HEADER_TYPE_CONTENT_TYPE].value), "%s", "application/json");
		t_response.header_cnt++;
	}
	/* 5. Content-Length (본문 전 0, Body 채운 뒤 갱신) */
	if (t_response.header_cnt < HEADER_MAX_COUNT) {
		snprintf(t_response.t_header[HEADER_TYPE_CONTENT_LENGTH].name,
			sizeof(t_response.t_header[HEADER_TYPE_CONTENT_LENGTH].name), "%s", "Content-Length");
		snprintf(t_response.t_header[HEADER_TYPE_CONTENT_LENGTH].value,
			sizeof(t_response.t_header[HEADER_TYPE_CONTENT_LENGTH].value), "%d", 0);
		t_response.header_cnt++;
	}
	return t_response;
}

int Generate_Response_Body(ResType_t *t_response, eResultType_e e_result,
		ZtErrCode e_code, const char *data)
{
	const char *msg_str;

	if (t_response == NULL)
		return ERR_ARG_INVALID;

    /* 1. Result */
    t_response->t_result.result = (e_result == RESULT_SUCCESS) ? 0 : 1;

    /* 2. Code */
	snprintf(t_response->t_result.code, sizeof(t_response->t_result.code), "%s", ZT_ERR_TO_STRING(e_code));

    /* 3. Message */
	snprintf(t_response->t_result.message, sizeof(t_response->t_result.message), "%s", ZT_ERR_DEFAULT_MESSAGE(e_code));

	/* 4. data JSON */
	n = snprintf(t_response->t_result.data, sizeof(t_response->t_result.data), BODY_FMT,
		t_response->t_result.result, 
        t_response->t_result.code, 
        t_response->t_result.message, 
        data);

	return SOCKET_OK;
}

const char *ZT_STATUS_TEXT(int status_code)
{
	switch (status_code) {
	case 200: return "OK";
	case 201: return "Created";
	case 204: return "No Content";
	case 400: return "Bad Request";
	case 401: return "Unauthorized";
	case 403: return "Forbidden";
	case 404: return "Not Found";
	case 405: return "Method Not Allowed";
	case 500: return "Internal Server Error";
	case 503: return "Service Unavailable";
	default:  return "Unknown";
	}
}

const char *ZT_ERR_TO_STRING(ZtErrCode e)
{
	static const char *const code_tbl[] = {
#define X(id, code, msg) code,
		ZT_ERR_CODES(X)
#undef X
	};

	if (e >= 0 && e < ZT_ERR_COUNT && (size_t)e < (sizeof(code_tbl) / sizeof(code_tbl[0])))
		return code_tbl[e];
	return "UNKNOWN";
}

const char *ZT_ERR_DEFAULT_MESSAGE(ZtErrCode e)
{
	static const char *const msg_tbl[] = {
#define X(id, code, msg) msg,
		ZT_ERR_CODES(X)
#undef X
	};

	if (e >= 0 && e < ZT_ERR_COUNT && (size_t)e < (sizeof(msg_tbl) / sizeof(msg_tbl[0])))
		return msg_tbl[e];
	return "unknown error";
}