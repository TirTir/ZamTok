#include "ZT_msg.h"
#include <stdio.h>
#include <string.h>

ResType_t msg_generate_response(ReqType_t *t_request, int status_code)
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

int msg_generate_response_body(ResType_t *t_response, eResultType_e e_result,
		ZtErrCode e_code, const char *data_json)
{
	const char *success_lit;
	const char *data_str;
	int n;

	if (t_response == NULL)
		return ERR_ARG_INVALID;

	t_response->t_result.result = (e_result == RESULT_SUCCESS) ? 0 : 1;

	snprintf(t_response->t_result.code, sizeof(t_response->t_result.code), "%s",
		ZT_ERR_TO_STRING(e_code));

	snprintf(t_response->t_result.message, sizeof(t_response->t_result.message), "%s",
		ZT_ERR_DEFAULT_MESSAGE(e_code));

	success_lit = (e_result == RESULT_SUCCESS) ? "true" : "false";
	data_str = (data_json != NULL && data_json[0] != '\0') ? data_json : "null";

	n = snprintf(t_response->t_result.data, sizeof(t_response->t_result.data), BODY_FMT,
		success_lit,
		t_response->t_result.code,
		t_response->t_result.message,
		data_str);

	if (n < 0 || (size_t)n >= sizeof(t_response->t_result.data))
		return ERR_ARG_INVALID;

	snprintf(t_response->t_header[HEADER_TYPE_CONTENT_LENGTH].value,
		sizeof(t_response->t_header[HEADER_TYPE_CONTENT_LENGTH].value),
		"%d", n);

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

static int msg_copy_json_string_field( const char *p_body, const char *p_key, char *out, size_t out_sz )
{
	char *off = NULL;
	int L = 0;

	if ( out == NULL || out_sz == 0 )
		return ERR_ARG_INVALID;

	if ( Get_Value_From_Body( p_body, p_key, &off, &L ) != SOCKET_OK )
		return ERR_ARG_INVALID;
	if ( L < 0 || (size_t)L >= out_sz )
		return ERR_ARG_INVALID;

	memcpy( out, off, (size_t)L );
	out[L] = '\0';
	return SOCKET_OK;
}

static int msg_get_value_from_body( const char *p_body, const char *p_key, char **p_offset, int *len )
{
	const char *p;
	const char *p_start;
	const char *p_end;

	if ( p_body == NULL || p_key == NULL || p_offset == NULL || len == NULL )
		return ERR_ARG_INVALID;

	p = strstr( p_body, p_key );
	if ( !p )
		return ERR_ARG_INVALID;

	p = p + strlen( p_key );
	p = strchr( p, ':' );
	if ( !p )
		return ERR_ARG_INVALID;

	p++;
	while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' )
		p++;
	if ( *p != '"' )
		return ERR_ARG_INVALID;

	p++;
	p_start = p;
	p_end = strchr( p_start, '"' );
	if ( !p_end || p_end < p_start )
		return ERR_ARG_INVALID;

	*p_offset = (char *)p_start;
	*len = (int)( p_end - p_start );
	
	return SOCKET_OK;
}