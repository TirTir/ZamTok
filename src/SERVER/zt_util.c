
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

	/* Stored values are echoed into JSON unescaped, so reject escapes here. */
	if (memchr(str_off, '\\', (size_t)i_len) != NULL)
		return ZT_RC_ARG_INVALID;

	memcpy(str_out, str_off, (size_t)i_len);
	str_out[i_len] = '\0';
	return ZT_RC_OK;
}

static int msg_hex4(const char *p, unsigned int *out)
{
	unsigned int v = 0;
	int i;

	for (i = 0; i < 4; i++) {
		char c = p[i];

		v <<= 4;
		if (c >= '0' && c <= '9')
			v |= (unsigned int)(c - '0');
		else if (c >= 'a' && c <= 'f')
			v |= (unsigned int)(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			v |= (unsigned int)(c - 'A' + 10);
		else
			return ZT_RC_ARG_INVALID;
	}

	*out = v;
	return ZT_RC_OK;
}

static size_t msg_utf8_encode(unsigned int cp, char *out)
{
	if (cp < 0x80) {
		out[0] = (char)cp;
		return 1;
	}
	if (cp < 0x800) {
		out[0] = (char)(0xC0 | (cp >> 6));
		out[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp < 0x10000) {
		out[0] = (char)(0xE0 | (cp >> 12));
		out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	}
	out[0] = (char)(0xF0 | (cp >> 18));
	out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
	out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
	out[3] = (char)(0x80 | (cp & 0x3F));
	return 4;
}

int msg_copy_json_text_field(const char *str_body, const char *str_key, char *str_out,
			     size_t sz_out)
{
	const char *p;
	size_t o = 0;

	if (str_body == NULL || str_key == NULL || str_out == NULL || sz_out == 0)
		return ZT_RC_ARG_INVALID;

	p = strstr(str_body, str_key);
	if (p == NULL)
		return ZT_RC_ARG_INVALID;
	p = strchr(p + strlen(str_key), ':');
	if (p == NULL)
		return ZT_RC_ARG_INVALID;
	p++;
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	if (*p != '"')
		return ZT_RC_ARG_INVALID;
	p++;

	while (*p != '"') {
		char enc[4];
		size_t n = 1;

		if (*p == '\0' || (unsigned char)*p < 0x20)
			return ZT_RC_ARG_INVALID;

		if (*p != '\\') {
			enc[0] = *p++;
		} else {
			p++;
			switch (*p) {
			case '"':  enc[0] = '"';  break;
			case '\\': enc[0] = '\\'; break;
			case '/':  enc[0] = '/';  break;
			case 'b':  enc[0] = '\b'; break;
			case 'f':  enc[0] = '\f'; break;
			case 'n':  enc[0] = '\n'; break;
			case 'r':  enc[0] = '\r'; break;
			case 't':  enc[0] = '\t'; break;
			case 'u': {
				unsigned int cp, lo;

				if (msg_hex4(p + 1, &cp) != ZT_RC_OK)
					return ZT_RC_ARG_INVALID;
				p += 4;
				if (cp >= 0xD800 && cp <= 0xDBFF) { /* surrogate pair */
					if (p[1] != '\\' || p[2] != 'u' || msg_hex4(p + 3, &lo) != ZT_RC_OK ||
					    lo < 0xDC00 || lo > 0xDFFF)
						return ZT_RC_ARG_INVALID;
					cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
					p += 6;
				} else if (cp >= 0xDC00 && cp <= 0xDFFF) {
					return ZT_RC_ARG_INVALID;
				}
				if (cp == 0)
					return ZT_RC_ARG_INVALID;
				n = msg_utf8_encode(cp, enc);
				break;
			}
			default:
				return ZT_RC_ARG_INVALID;
			}
			p++;
		}

		if (o + n >= sz_out)
			return ZT_RC_ARG_INVALID;
		memcpy(str_out + o, enc, n);
		o += n;
	}

	if (o == 0)
		return ZT_RC_ARG_INVALID;

	str_out[o] = '\0';
	return ZT_RC_OK;
}

int msg_json_escape(const char *str_src, char *str_dst, size_t sz_dst)
{
	size_t o = 0;
	const unsigned char *p;

	if (str_src == NULL || str_dst == NULL || sz_dst == 0)
		return -1;

	for (p = (const unsigned char *)str_src; *p; p++) {
		char tmp[8];
		const char *s = tmp;
		size_t n;

		switch (*p) {
		case '"':  s = "\\\""; break;
		case '\\': s = "\\\\"; break;
		case '\n': s = "\\n";  break;
		case '\r': s = "\\r";  break;
		case '\t': s = "\\t";  break;
		default:
			if (*p < 0x20 || *p == 0x7F)
				snprintf(tmp, sizeof(tmp), "\\u%04x", *p);
			else {
				tmp[0] = (char)*p;
				tmp[1] = '\0';
			}
		}

		n = strlen(s);
		if (o + n >= sz_dst)
			return -1;
		memcpy(str_dst + o, s, n);
		o += n;
	}

	str_dst[o] = '\0';
	return (int)o;
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