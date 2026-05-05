#include "zt_inc.h"

static const char *http_get_body(const char *buf)
{
	const char *pstr_body;

	pstr_body = strstr(buf, "\r\n\r\n");
	if (pstr_body == NULL)
		pstr_body = strstr(buf, "\n\n");
	if (pstr_body != NULL)
		pstr_body += (strstr(buf, "\r\n\r\n") ? 4 : 2);
	else
		pstr_body = buf;

	return pstr_body;
}

int http_join(int socket, const char *buf, req_t *t_request)
{
	user_t t_user = {0};
	res_t *pt_response = NULL;
	char str_data_json[USER_ID_MAX_LEN + 32] = {0};
	const char *pstr_body = NULL;
	int rc = 0;
	int status = HTTP_OK;
	zt_err_code_t err_code = ZT_ERR_INVALID_PARAMS;
	result_kind_t e_result = RESULT_FAIL;

	if (socket < 0 || buf == NULL || t_request == NULL)
		return ZT_RC_ARG_INVALID;

	if (strncmp(t_request->str_method, "POST", 4) != 0) {
		status = HTTP_METHOD_NOT_ALLOWED;
		err_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto next;
	}

	pstr_body = http_get_body(buf);

	{
		http_json_field_spec_t t_fields[] = {
			{ USER_ID_KEY, t_user.user_id, sizeof(t_user.user_id) },
			{ NAME_KEY, t_user.name, sizeof(t_user.name) },
			{ PASSWORD_KEY, t_user.password, sizeof(t_user.password) },
		};

		if (http_bind_json_fields(pstr_body, t_fields,
					  (int)(sizeof(t_fields) / sizeof(t_fields[0])), &status,
					  &err_code) < 0) {
			goto next;
		}
	}

	rc = redis_user_save(&t_user);
	if (rc != ZT_RC_OK) {
		status = HTTP_INTERNAL_SERVER_ERROR;
		err_code = ZT_ERR_SERVER_ERROR;
		goto next;
	}

	snprintf(str_data_json, sizeof(str_data_json), JOIN_DATA_FMT, t_user.user_id);
	e_result = RESULT_SUCCESS;
	err_code = ZT_ERR_SUCCESS;

next:
	pt_response = generate_response(t_request, status);
	if (pt_response == NULL)
		return ZT_RC_REDIS;

	generate_response_body(pt_response, e_result, err_code,
			       (e_result == RESULT_SUCCESS) ? str_data_json : NULL);
	rc = hdl_send_http_json(socket, pt_response);
	free(pt_response);

	return (rc < 0) ? ZT_RC_SOCKET : ZT_RC_OK;
}

int http_login(int socket, const char *buf, req_t *t_request)
{
	user_t t_user = {0};
	user_t t_stored_user = {0};
	res_t *pt_response = NULL;
	const char *pstr_body = NULL;
	int rc = 0;
	int status = HTTP_OK;
	zt_err_code_t err_code = ZT_ERR_INVALID_PARAMS;
	result_kind_t e_result = RESULT_FAIL;

	if (socket < 0 || buf == NULL || t_request == NULL)
		return ZT_RC_ARG_INVALID;

	if (strncmp(t_request->str_method, "POST", 4) != 0) {
		status = HTTP_METHOD_NOT_ALLOWED;
		err_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto next;
	}

	pstr_body = http_get_body(buf);

	{
		http_json_field_spec_t t_fields[] = {
			{ USER_ID_KEY, t_user.user_id, sizeof(t_user.user_id) },
			{ PASSWORD_KEY, t_user.password, sizeof(t_user.password) },
		};

		if (http_bind_json_fields(pstr_body, t_fields,
					  (int)(sizeof(t_fields) / sizeof(t_fields[0])), &status,
					  &err_code) < 0) {
			goto next;
		}
	}

	rc = redis_user_get(t_user.user_id, &t_stored_user);
	if (rc != ZT_RC_OK) {
		status = HTTP_UNAUTHORIZED;
		err_code = ZT_ERR_USER_NOT_FOUND;
		goto next;
	}

	if (strcmp(t_stored_user.password, t_user.password) != 0) {
		status = HTTP_UNAUTHORIZED;
		err_code = ZT_ERR_WRONG_PASSWORD;
		goto next;
	}

	e_result = RESULT_SUCCESS;
	err_code = ZT_ERR_SUCCESS;

next:
	pt_response = generate_response(t_request, status);
	if (pt_response == NULL)
		return ZT_RC_REDIS;

	generate_response_body(pt_response, e_result, err_code, NULL);
	rc = hdl_send_http_json(socket, pt_response);
	free(pt_response);

	return (rc < 0) ? ZT_RC_SOCKET : ZT_RC_OK;
}

int http_create_room(int socket, const char *buf, req_t *t_request)
{
	room_t t_room = {0};
	res_t *pt_response = NULL;
	char str_data_json[ROOM_ID_MAX_LEN + 32] = {0};
	const char *pstr_body = NULL;
	int rc = 0;
	int status = HTTP_OK;
	zt_err_code_t err_code = ZT_ERR_INVALID_PARAMS;
	result_kind_t e_result = RESULT_FAIL;

	if (socket < 0 || buf == NULL || t_request == NULL)
		return ZT_RC_ARG_INVALID;

	if (strncmp(t_request->str_method, "POST", 4) != 0) {
		status = HTTP_METHOD_NOT_ALLOWED;
		err_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto next;
	}

	pstr_body = http_get_body(buf);

	{
		http_json_field_spec_t t_fields[] = {
			{ ROOM_ID_KEY, t_room.room_id, sizeof(t_room.room_id) },
			{ PASSWORD_KEY, t_room.password, sizeof(t_room.password) },
			{ USER_ID_KEY, t_room.creator_id, sizeof(t_room.creator_id) },
		};

		if (http_bind_json_fields(pstr_body, t_fields,
					  (int)(sizeof(t_fields) / sizeof(t_fields[0])), &status,
					  &err_code) < 0) {
			goto next;
		}
	}

	rc = redis_room_save(&t_room);
	if (rc != ZT_RC_OK) {
		status = HTTP_INTERNAL_SERVER_ERROR;
		err_code = ZT_ERR_SERVER_ERROR;
		goto next;
	}

	snprintf(str_data_json, sizeof(str_data_json), CREATE_ROOM_DATA_FMT, t_room.room_id);
	e_result = RESULT_SUCCESS;
	err_code = ZT_ERR_SUCCESS;

next:
	pt_response = generate_response(t_request, status);
	if (pt_response == NULL)
		return ZT_RC_REDIS;

	generate_response_body(pt_response, e_result, err_code,
			       (e_result == RESULT_SUCCESS) ? str_data_json : NULL);
	rc = hdl_send_http_json(socket, pt_response);
	free(pt_response);

	return (rc < 0) ? ZT_RC_SOCKET : ZT_RC_OK;
}

int http_search_room(int socket, const char *buf, req_t *t_request)
{
	room_t t_room = {0};
	res_t *pt_response = NULL;
	char str_data_json[ROOM_ID_MAX_LEN + 32] = {0};
	const char *pstr_query;
	const char *pstr_start;
	size_t sz_len;
	int rc = 0;
	int status = HTTP_OK;
	zt_err_code_t err_code = ZT_ERR_INVALID_PARAMS;
	result_kind_t e_result = RESULT_FAIL;

	(void)buf;

	if (socket < 0 || t_request == NULL)
		return ZT_RC_ARG_INVALID;

	if (strncmp(t_request->str_method, "GET", 3) != 0) {
		status = HTTP_METHOD_NOT_ALLOWED;
		err_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto next;
	}

	pstr_query = strstr(t_request->str_uri, "?id=");
	if (pstr_query == NULL) {
		status = HTTP_BAD_REQUEST;
		err_code = ZT_ERR_INVALID_PARAMS;
		goto next;
	}

	pstr_start = pstr_query + 4;
	sz_len = strcspn(pstr_start, "& ");
	if (sz_len >= sizeof(t_room.room_id))
		sz_len = sizeof(t_room.room_id) - 1;

	memcpy(t_room.room_id, pstr_start, sz_len);
	t_room.room_id[sz_len] = '\0';

	if (t_room.room_id[0] == '\0') {
		status = HTTP_BAD_REQUEST;
		err_code = ZT_ERR_INVALID_PARAMS;
		goto next;
	}

	rc = redis_room_get(t_room.room_id, &t_room);
	if (rc != ZT_RC_OK) {
		status = HTTP_NOT_FOUND;
		err_code = ZT_ERR_ROOM_NOT_FOUND;
		goto next;
	}

	snprintf(str_data_json, sizeof(str_data_json), SEARCH_ROOM_DATA_FMT, t_room.room_id);
	e_result = RESULT_SUCCESS;
	err_code = ZT_ERR_SUCCESS;

next:
	pt_response = generate_response(t_request, status);
	if (pt_response == NULL)
		return ZT_RC_REDIS;

	generate_response_body(pt_response, e_result, err_code,
			       (e_result == RESULT_SUCCESS) ? str_data_json : NULL);
	rc = hdl_send_http_json(socket, pt_response);
	free(pt_response);

	return (rc < 0) ? ZT_RC_SOCKET : ZT_RC_OK;
}

int http_join_room(int socket, const char *buf, req_t *t_request)
{
	room_t t_room = {0};
	room_t t_stored_room = {0};
	res_t *pt_response = NULL;
	char str_data_json[ROOM_ID_MAX_LEN + 32] = {0};
	const char *pstr_body = NULL;
	int rc = 0;
	int status = HTTP_OK;
	zt_err_code_t err_code = ZT_ERR_INVALID_PARAMS;
	result_kind_t e_result = RESULT_FAIL;

	if (socket < 0 || buf == NULL || t_request == NULL)
		return ZT_RC_ARG_INVALID;

	if (strncmp(t_request->str_method, "POST", 4) != 0) {
		status = HTTP_METHOD_NOT_ALLOWED;
		err_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto next;
	}

	pstr_body = http_get_body(buf);

	{
		http_json_field_spec_t t_fields[] = {
			{ ROOM_ID_KEY, t_room.room_id, sizeof(t_room.room_id) },
			{ PASSWORD_KEY, t_room.password, sizeof(t_room.password) },
		};

		if (http_bind_json_fields(pstr_body, t_fields,
					  (int)(sizeof(t_fields) / sizeof(t_fields[0])), &status,
					  &err_code) < 0) {
			goto next;
		}
	}

	rc = redis_room_get(t_room.room_id, &t_stored_room);
	if (rc != ZT_RC_OK) {
		status = HTTP_NOT_FOUND;
		err_code = ZT_ERR_ROOM_NOT_FOUND;
		goto next;
	}

	if (strcmp(t_stored_room.password, t_room.password) != 0) {
		status = HTTP_UNAUTHORIZED;
		err_code = ZT_ERR_WRONG_PASSWORD;
		goto next;
	}

	snprintf(str_data_json, sizeof(str_data_json), JOIN_ROOM_DATA_FMT, t_room.room_id);
	e_result = RESULT_SUCCESS;
	err_code = ZT_ERR_SUCCESS;

next:
	pt_response = generate_response(t_request, status);
	if (pt_response == NULL)
		return ZT_RC_REDIS;

	generate_response_body(pt_response, e_result, err_code,
			       (e_result == RESULT_SUCCESS) ? str_data_json : NULL);
	rc = hdl_send_http_json(socket, pt_response);
	free(pt_response);

	return (rc < 0) ? ZT_RC_SOCKET : ZT_RC_OK;
}

int http_list_rooms(int socket, const char *buf, req_t *t_request)
{
	room_t t_rooms[ROOM_MAX_COUNT] = {0};
	res_t *pt_response = NULL;
	char str_list_data[DATA_MAX_LEN] = {0};
	int rc = 0;
	int i_pos = 0;
	int i_n = 0;
	int i_idx;
	int status = HTTP_OK;
	zt_err_code_t err_code = ZT_ERR_INVALID_PARAMS;
	result_kind_t e_result = RESULT_FAIL;

	(void)buf;

	if (socket < 0 || t_request == NULL)
		return ZT_RC_ARG_INVALID;

	if (strncmp(t_request->str_method, "GET", 3) != 0) {
		status = HTTP_METHOD_NOT_ALLOWED;
		err_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto next;
	}

	rc = redis_room_list(t_rooms, sizeof(t_rooms) / sizeof(t_rooms[0]));
	if (rc != ZT_RC_OK) {
		status = HTTP_INTERNAL_SERVER_ERROR;
		err_code = ZT_ERR_SERVER_ERROR;
		goto next;
	}

	i_n = snprintf(str_list_data + i_pos, sizeof(str_list_data) - (size_t)i_pos, "[");
	if (i_n < 0 || i_n >= (int)(sizeof(str_list_data) - (size_t)i_pos)) {
		status = HTTP_INTERNAL_SERVER_ERROR;
		err_code = ZT_ERR_SERVER_ERROR;
		goto next;
	}
	i_pos += i_n;

	for (i_idx = 0; i_idx < ROOM_MAX_COUNT && t_rooms[i_idx].room_id[0] != '\0'; i_idx++) {
		i_n = snprintf(str_list_data + i_pos, sizeof(str_list_data) - (size_t)i_pos,
			       "%s" LIST_ROOMS_DATA_FMT, (i_idx > 0) ? "," : "",
			       t_rooms[i_idx].room_id, t_rooms[i_idx].creator_id);
		if (i_n < 0 || i_n >= (int)(sizeof(str_list_data) - (size_t)i_pos)) {
			status = HTTP_INTERNAL_SERVER_ERROR;
			err_code = ZT_ERR_SERVER_ERROR;
			goto next;
		}
		i_pos += i_n;
	}

	i_n = snprintf(str_list_data + i_pos, sizeof(str_list_data) - (size_t)i_pos, "]");
	if (i_n < 0 || i_n >= (int)(sizeof(str_list_data) - (size_t)i_pos)) {
		status = HTTP_INTERNAL_SERVER_ERROR;
		err_code = ZT_ERR_SERVER_ERROR;
		goto next;
	}

	e_result = RESULT_SUCCESS;
	err_code = ZT_ERR_SUCCESS;

next:
	pt_response = generate_response(t_request, status);
	if (pt_response == NULL)
		return ZT_RC_REDIS;

	generate_response_body(pt_response, e_result, err_code,
			       (e_result == RESULT_SUCCESS) ? str_list_data : NULL);
	rc = hdl_send_http_json(socket, pt_response);
	free(pt_response);

	return (rc < 0) ? ZT_RC_SOCKET : ZT_RC_OK;
}
