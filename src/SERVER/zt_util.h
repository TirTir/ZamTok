#ifndef _ZT_UTIL_H_
#define _ZT_UTIL_H_

#include <stddef.h>
#include "zt_http.h"

typedef struct {
	const char *str_json_key;
	char *str_out;
	size_t sz_out;
} http_json_field_spec_t;

res_t *generate_response(req_t *t_request, int i_status_code);
int generate_response_body(res_t *pt_response, result_kind_t e_kind, zt_err_code_t e_err_code,
			   const char *str_data_json);
int msg_copy_json_string_field(const char *str_body, const char *str_key, char *str_out,
			       size_t sz_out);

int http_bind_json_fields(const char *str_body, http_json_field_spec_t *pt_fields, int i_size,
			  int *pi_status, zt_err_code_t *pe_err_code);

#endif
