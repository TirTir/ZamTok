
#include "zt_inc.h"

res_t *generate_response(req_t *t_request, int i_status_code)
{
    if(t_request == NULL || i_status_code < 100 || i_status_code > 599) {
        LOG_MSG("invalid parameters: request is NULL or status code is out of range\n");
        return NULL;
    }

    /* 1. response allocate */
	res_t *pt_response = malloc(sizeof(*pt_response));
	if(pt_response == NULL) {
		LOG_MSG("malloc failed: response\n");
		return NULL;
	}

	memset(pt_response, 0, sizeof(*pt_response));

    /* 2. response initialize */
	const char *str_reason_text = zt_http_status_to_reason(i_status_code);
	if(str_reason_text == NULL) {
		LOG_MSG("invalid status code: %d\n", i_status_code);
		free(pt_response);
		return NULL;
	}

	snprintf(pt_response->str_version, sizeof(pt_response->str_version), "%s",
			 t_request->str_version);

	pt_response->i_status_code = (unsigned int)i_status_code;
	snprintf(pt_response->str_reason, sizeof(pt_response->str_reason), "%s", str_reason_text);

	return pt_response;
}

const char *zt_http_status_to_reason(int i_status_code)
{
	switch (i_status_code) {
#define X(name, val, reason) case val: return reason;
		HTTP_REASON_MAP(X)
#undef X
	default:
		return "Unknown";
	}
}

int generate_response_body(res_t *pt_response, result_kind_t e_kind, zt_err_code_t e_err_code,
			   const char *str_data_json)
{
	const char *str_success_lit;
	const char *str_data_str;
	int i_n;

	if (pt_response == NULL)
		return ZT_RC_ARG_INVALID;

	pt_response->t_body.b_success = (e_kind == RESULT_SUCCESS);

	snprintf(pt_response->t_body.str_code, sizeof(pt_response->t_body.str_code), "%s",
		 zt_err_code_str(e_err_code));

	snprintf(pt_response->t_body.str_message, sizeof(pt_response->t_body.str_message), "%s",
		 zt_err_default_msg(e_err_code));

	str_success_lit = (e_kind == RESULT_SUCCESS) ? "true" : "false";
	str_data_str = (str_data_json != NULL && str_data_json[0] != '\0') ? str_data_json : "null";

	i_n = snprintf(pt_response->t_body.str_data, sizeof(pt_response->t_body.str_data), BODY_FMT,
		       str_success_lit, pt_response->t_body.str_code, pt_response->t_body.str_message,
		       str_data_str);

	if (i_n < 0 || (size_t)i_n >= sizeof(pt_response->t_body.str_data))
		return ZT_RC_ARG_INVALID;

	snprintf(pt_response->t_header_fields[HEADER_TYPE_CONTENT_LENGTH].str_value,
		 sizeof(pt_response->t_header_fields[HEADER_TYPE_CONTENT_LENGTH].str_value), "%d",
		 i_n);

	return ZT_RC_OK;
}

const char *zt_err_code_str(zt_err_code_t e_code)
{
	static const char *const astr_code_tbl[] = {
#define X(id, c, msg) c,
		ZT_ERR_CODES(X)
#undef X
	};

	if (e_code >= 0 && e_code < ZT_ERR_COUNT &&
	    (size_t)e_code < (sizeof(astr_code_tbl) / sizeof(astr_code_tbl[0])))
		return astr_code_tbl[e_code];
	return "UNKNOWN";
}

const char *zt_err_default_msg(zt_err_code_t e_code)
{
	static const char *const astr_msg_tbl[] = {
#define X(id, c, msg) msg,
		ZT_ERR_CODES(X)
#undef X
	};

	if (e_code >= 0 && e_code < ZT_ERR_COUNT &&
	    (size_t)e_code < (sizeof(astr_msg_tbl) / sizeof(astr_msg_tbl[0])))
		return astr_msg_tbl[e_code];
	return "unknown error";
}

static int msg_get_json_string_field_helper(const char *str_body, const char *str_key,
					    char **ppstr_offset_out, int *pi_len_out);

int msg_copy_json_string_field(const char *str_body, const char *str_key, char *str_out,
			       size_t sz_out)
{
	char *str_off = NULL;
	int i_len = 0;

	if (str_out == NULL || sz_out == 0)
		return ZT_RC_ARG_INVALID;

	if (msg_get_json_string_field_helper(str_body, str_key, &str_off, &i_len) != ZT_RC_OK)
		return ZT_RC_ARG_INVALID;
	
    /* 빈 문자열 확인 */
    if (i_len <= 0 || (size_t)i_len >= sz_out)
		return ZT_RC_ARG_INVALID;

	memcpy(str_out, str_off, (size_t)i_len);
	str_out[i_len] = '\0';
	return ZT_RC_OK;
}

static int msg_get_json_string_field_helper(const char *str_body, const char *str_key,
					    char **ppstr_offset_out, int *pi_len_out)
{
	const char *str_p;
	const char *str_start;
	const char *str_end;

	if (str_body == NULL || str_key == NULL || ppstr_offset_out == NULL || pi_len_out == NULL)
		return ZT_RC_ARG_INVALID;

	str_p = strstr(str_body, str_key);
	if (!str_p)
		return ZT_RC_ARG_INVALID;

	str_p = str_p + strlen(str_key);
	str_p = strchr(str_p, ':');
	if (!str_p)
		return ZT_RC_ARG_INVALID;

	str_p++;
	while (*str_p == ' ' || *str_p == '\t' || *str_p == '\r' || *str_p == '\n')
		str_p++;
	if (*str_p != '"')
		return ZT_RC_ARG_INVALID;

	str_p++;
	str_start = str_p;
	str_end = strchr(str_start, '"');
	if (!str_end || str_end < str_start)
		return ZT_RC_ARG_INVALID;

	*ppstr_offset_out = (char *)str_start;
	*pi_len_out = (int)(str_end - str_start);

	return ZT_RC_OK;
}

int http_bind_json_fields(const char *str_body, http_json_field_spec_t *pt_fields, int i_size,
			  int *pi_status, zt_err_code_t *pe_err_code)
{
	int i_idx;

	if (str_body == NULL || pt_fields == NULL || i_size == 0 || pi_status == NULL ||
	    pe_err_code == NULL)
		return ZT_RC_ARG_INVALID;

	for (i_idx = 0; i_idx < i_size; i_idx++) {
		if (msg_copy_json_string_field(str_body, pt_fields[i_idx].str_json_key,
					       pt_fields[i_idx].str_out, pt_fields[i_idx].sz_out) !=
		    ZT_RC_OK) {
			*pi_status = HTTP_BAD_REQUEST;
			*pe_err_code = ZT_ERR_INVALID_PARAMS;
			return -1;
		}
	}
	return 0;
}