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

static char *http_trim_token(char *str)
{
	char *end;

	if (str == NULL)
		return NULL;

	while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')
		str++;

	end = str + strlen(str);
	while (end > str && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' ||
			     end[-1] == '\n')) {
		end--;
	}
	*end = '\0';

	return str;
}

static int http_query_value(const char *uri, const char *key, char *out, size_t out_size)
{
	char pattern[64];
	const char *pstr_query;
	const char *pstr_start;
	size_t sz_len;

	if (uri == NULL || key == NULL || out == NULL || out_size == 0)
		return ZT_RC_ARG_INVALID;

	snprintf(pattern, sizeof(pattern), "%s=", key);
	pstr_query = strstr(uri, pattern);
	if (pstr_query == NULL)
		return ZT_RC_ARG_INVALID;

	pstr_start = pstr_query + strlen(pattern);
	sz_len = strcspn(pstr_start, "& ");
	if (sz_len >= out_size)
		sz_len = out_size - 1;

	memcpy(out, pstr_start, sz_len);
	out[sz_len] = '\0';

	return (out[0] == '\0') ? ZT_RC_ARG_INVALID : ZT_RC_OK;
}

static int http_member_count(const char *members)
{
	char str_members[ROOM_MEMBERS_MAX_LEN];
	char *saveptr = NULL;
	char *token;
	int count = 0;

	if (members == NULL || members[0] == '\0')
		return 0;

	snprintf(str_members, sizeof(str_members), "%s", members);

	for (token = strtok_r(str_members, ",", &saveptr); token != NULL;
	     token = strtok_r(NULL, ",", &saveptr)) {
		token = http_trim_token(token);
		if (token && token[0] != '\0')
			count++;
	}

	return count;
}

static int http_validate_room_type_members(const char *room_type, const char *members)
{
	int count;

	if (room_type == NULL || members == NULL)
		return ZT_RC_ARG_INVALID;

	count = http_member_count(members);
	if (!strcmp(room_type, "1:1"))
		return (count == 1) ? ZT_RC_OK : ZT_RC_ARG_INVALID;
	if (!strcmp(room_type, "1:N"))
		return (count >= 1) ? ZT_RC_OK : ZT_RC_ARG_INVALID;

	return ZT_RC_ARG_INVALID;
}

static int http_members_are_friends(const char *user_id, const char *members)
{
	char str_members[ROOM_MEMBERS_MAX_LEN];
	char *saveptr = NULL;
	char *token;
	int rc;

	if (user_id == NULL || members == NULL)
		return ZT_RC_ARG_INVALID;

	snprintf(str_members, sizeof(str_members), "%s", members);

	for (token = strtok_r(str_members, ",", &saveptr); token != NULL;
	     token = strtok_r(NULL, ",", &saveptr)) {
		token = http_trim_token(token);
		if (token == NULL || token[0] == '\0')
			continue;

		rc = redis_user_is_friend(user_id, token);
		if (rc < 0)
			return ZT_RC_REDIS;
		if (rc == 0)
			return ZT_RC_FAIL;
	}

	return ZT_RC_OK;
}

static int http_make_room_id(const char *room_id, const char *user_id, char *out, size_t out_size)
{
	int n;

	if (room_id == NULL || user_id == NULL || out == NULL || out_size == 0)
		return ZT_RC_ARG_INVALID;

	n = snprintf(out, out_size, "%s_%s", room_id, user_id);
	if (n < 0 || (size_t)n >= out_size)
		return ZT_RC_ARG_INVALID;

	return ZT_RC_OK;
}

int http_join(int socket, const char *buf, req_t *t_request)
{
	user_t t_user = {0};
	res_t *pt_response = NULL;
	char str_data_json[USER_ID_MAX_LEN + MDN_MAX_LEN + 48] = {0};
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
			{ MDN_KEY, t_user.mdn, sizeof(t_user.mdn) },
		};

		if (http_bind_json_fields(pstr_body, t_fields,
					  (int)(sizeof(t_fields) / sizeof(t_fields[0])), &status,
					  &err_code) < 0) {
			goto next;
		}
	}

	rc = redis_user_exists(t_user.user_id);
	if (rc < 0) {
		status = HTTP_INTERNAL_SERVER_ERROR;
		err_code = ZT_ERR_SERVER_ERROR;
		goto next;
	}
	if (rc > 0) {
		status = HTTP_CONFLICT;
		err_code = ZT_ERR_USER_ALREADY_EXISTS;
		goto next;
	}

	rc = redis_mdn_exists(t_user.mdn);
	if (rc < 0) {
		status = HTTP_INTERNAL_SERVER_ERROR;
		err_code = ZT_ERR_SERVER_ERROR;
		goto next;
	}
	if (rc > 0) {
		status = HTTP_CONFLICT;
		err_code = ZT_ERR_MDN_ALREADY_EXISTS;
		goto next;
	}

	rc = redis_user_save(&t_user);
	if (rc != ZT_RC_OK) {
		status = HTTP_INTERNAL_SERVER_ERROR;
		err_code = ZT_ERR_SERVER_ERROR;
		goto next;
	}

	snprintf(str_data_json, sizeof(str_data_json), JOIN_DATA_FMT, t_user.user_id, t_user.mdn);
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
	char str_raw_room_id[ROOM_ID_MAX_LEN] = {0};
	char str_data_json[(ROOM_ID_MAX_LEN * 2) + 64] = {0};
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
			{ ROOM_ID_KEY, str_raw_room_id, sizeof(str_raw_room_id) },
			{ PASSWORD_KEY, t_room.password, sizeof(t_room.password) },
			{ USER_ID_KEY, t_room.creator_id, sizeof(t_room.creator_id) },
			{ ROOM_TYPE_KEY, t_room.room_type, sizeof(t_room.room_type) },
			{ MEMBERS_KEY, t_room.members, sizeof(t_room.members) },
		};

		if (http_bind_json_fields(pstr_body, t_fields,
					  (int)(sizeof(t_fields) / sizeof(t_fields[0])), &status,
					  &err_code) < 0) {
			goto next;
		}
	}

	rc = redis_user_exists(t_room.creator_id);
	if (rc <= 0) {
		status = (rc == 0) ? HTTP_UNAUTHORIZED : HTTP_INTERNAL_SERVER_ERROR;
		err_code = (rc == 0) ? ZT_ERR_USER_NOT_FOUND : ZT_ERR_SERVER_ERROR;
		goto next;
	}

	if (http_validate_room_type_members(t_room.room_type, t_room.members) != ZT_RC_OK) {
		status = HTTP_BAD_REQUEST;
		err_code = ZT_ERR_INVALID_PARAMS;
		goto next;
	}

	rc = http_members_are_friends(t_room.creator_id, t_room.members);
	if (rc != ZT_RC_OK) {
		status = (rc == ZT_RC_FAIL) ? HTTP_FORBIDDEN : HTTP_INTERNAL_SERVER_ERROR;
		err_code = (rc == ZT_RC_FAIL) ? ZT_ERR_NOT_FRIEND : ZT_ERR_SERVER_ERROR;
		goto next;
	}

	if (http_make_room_id(str_raw_room_id, t_room.creator_id, t_room.room_id,
			      sizeof(t_room.room_id)) != ZT_RC_OK) {
		status = HTTP_BAD_REQUEST;
		err_code = ZT_ERR_INVALID_PARAMS;
		goto next;
	}
	snprintf(t_room.channel_id, sizeof(t_room.channel_id), "%s", t_room.room_id);

	rc = redis_room_save(&t_room);
	if (rc != ZT_RC_OK) {
		status = (rc == ZT_RC_CTX) ? HTTP_CONFLICT : HTTP_INTERNAL_SERVER_ERROR;
		err_code = (rc == ZT_RC_CTX) ? ZT_ERR_ROOM_ALREADY_EXISTS : ZT_ERR_SERVER_ERROR;
		goto next;
	}

	snprintf(str_data_json, sizeof(str_data_json), CREATE_ROOM_DATA_FMT, t_room.room_id,
		 t_room.channel_id);
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

	if (http_query_value(t_request->str_uri, "id", t_room.room_id, sizeof(t_room.room_id)) !=
	    ZT_RC_OK) {
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
			       t_rooms[i_idx].room_id, t_rooms[i_idx].creator_id,
			       t_rooms[i_idx].room_type);
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

int http_list_friends(int socket, const char *buf, req_t *t_request)
{
	friend_t t_friends[FRIEND_MAX_COUNT] = {0};
	res_t *pt_response = NULL;
	char str_user_id[USER_ID_MAX_LEN] = {0};
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

	if (http_query_value(t_request->str_uri, "user_id", str_user_id, sizeof(str_user_id)) !=
	    ZT_RC_OK) {
		status = HTTP_BAD_REQUEST;
		err_code = ZT_ERR_INVALID_PARAMS;
		goto next;
	}

	rc = redis_user_exists(str_user_id);
	if (rc <= 0) {
		status = (rc == 0) ? HTTP_NOT_FOUND : HTTP_INTERNAL_SERVER_ERROR;
		err_code = (rc == 0) ? ZT_ERR_USER_NOT_FOUND : ZT_ERR_SERVER_ERROR;
		goto next;
	}

	rc = redis_friend_list(str_user_id, t_friends, sizeof(t_friends) / sizeof(t_friends[0]));
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

	for (i_idx = 0; i_idx < FRIEND_MAX_COUNT && t_friends[i_idx].user_id[0] != '\0'; i_idx++) {
		i_n = snprintf(str_list_data + i_pos, sizeof(str_list_data) - (size_t)i_pos,
			       "%s" LIST_FRIENDS_DATA_FMT, (i_idx > 0) ? "," : "",
			       t_friends[i_idx].user_id, t_friends[i_idx].name,
			       t_friends[i_idx].mdn);
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

int http_invite_room(int socket, const char *buf, req_t *t_request)
{
	room_t t_room = {0};
	char str_user_id[USER_ID_MAX_LEN] = {0};
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
			{ USER_ID_KEY, str_user_id, sizeof(str_user_id) },
			{ MEMBERS_KEY, t_room.members, sizeof(t_room.members) },
		};

		if (http_bind_json_fields(pstr_body, t_fields,
					  (int)(sizeof(t_fields) / sizeof(t_fields[0])), &status,
					  &err_code) < 0) {
			goto next;
		}
	}

	rc = redis_user_exists(str_user_id);
	if (rc <= 0) {
		status = (rc == 0) ? HTTP_UNAUTHORIZED : HTTP_INTERNAL_SERVER_ERROR;
		err_code = (rc == 0) ? ZT_ERR_USER_NOT_FOUND : ZT_ERR_SERVER_ERROR;
		goto next;
	}

	rc = http_members_are_friends(str_user_id, t_room.members);
	if (rc != ZT_RC_OK) {
		status = (rc == ZT_RC_FAIL) ? HTTP_FORBIDDEN : HTTP_INTERNAL_SERVER_ERROR;
		err_code = (rc == ZT_RC_FAIL) ? ZT_ERR_NOT_FRIEND : ZT_ERR_SERVER_ERROR;
		goto next;
	}

	rc = redis_room_invite(t_room.room_id, str_user_id, t_room.members);
	if (rc != ZT_RC_OK) {
		status = HTTP_NOT_FOUND;
		err_code = ZT_ERR_ROOM_NOT_FOUND;
		goto next;
	}

	snprintf(str_data_json, sizeof(str_data_json), INVITE_ROOM_DATA_FMT, t_room.room_id);
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
