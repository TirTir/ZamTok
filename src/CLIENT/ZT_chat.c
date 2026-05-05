#include "zt_common.h"
#include "zt_chat.h"
#include "zt_sock.h"

int user_pool[USER_POOL_MAX_COUNT];

int chat_join(int socket, const user_t *user)
{
	int len = 0;
	char req_buf[1024] = {0};
	char json_body[512] = {0};

	snprintf(json_body, sizeof(json_body),
		"{\"user_id\": \"%s\", \"name\": \"%s\", \"password\": \"%s\"}",
		user->user_id, user->name, user->password);

	len = snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/join", "localhost:8080", strlen(json_body), json_body);

	if (len <= 0 )
	{
		printf("[chat_join] Request build fail\n");
		return -1;
	}

	return SOCKET_SendRequestBuf(socket, req_buf, (size_t)len);
}

int chat_login(int socket, const char *user_id, const char *password)
{
	int len = 0;
	char req_buf[1024] = {0};
	char json_body[256] = {0};

	snprintf(json_body, sizeof(json_body),
		"{\"user_id\": \"%s\", \"password\": \"%s\"}",
		user_id, password);

	len = snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/login", "localhost:8080", strlen(json_body), json_body);

	if (len <= 0 || (size_t)len >= sizeof(req_buf)) {
		printf("[chat_login] Request build fail\n");
		return -1;
	}

	return SOCKET_SendRequestBuf(socket, req_buf, (size_t)len);
}

int chat_create_room(int socket, const char *room_id, const char *password, const char *user_id)
{
	int len = 0;
	char req_buf[1024] = {0};
	char json_body[256] = {0};

	if (user_id && user_id[0] != '\0') {
		snprintf(json_body, sizeof(json_body),
			"{\"room_id\": \"%s\", \"password\": \"%s\", \"user_id\": \"%s\"}",
			room_id, password, user_id);
	} else {
		snprintf(json_body, sizeof(json_body),
			"{\"room_id\": \"%s\", \"password\": \"%s\"}",
			room_id, password);
	}

	len = snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/room", "localhost:8080", strlen(json_body), json_body);

	if (len <= 0 || (size_t)len >= sizeof(req_buf)) {
		printf("[chat_create_room] Request build fail\n");
		return -1;
	}

	return SOCKET_SendRequestBuf(socket, req_buf, (size_t)len);
}

int chat_search_room(int socket, const char *room_id)
{
	char path[128] = {0};

	if (socket < 0 || room_id == NULL) {
		printf("[chat_search_room] Invalid argument\n");
		return -1;
	}

	snprintf(path, sizeof(path), "/room?id=%s", room_id);

	return SOCKET_SendHttpRequest(socket, "localhost", "GET", path);
}

int chat_list_rooms(int socket)
{
	if (socket < 0) {
		printf("[chat_list_rooms] Invalid socket\n");
		return -1;
	}

	return SOCKET_SendHttpRequest(socket, "localhost", "GET", "/rooms");
}

int chat_join_room(int socket, const char *room_id, const char *password)
{
	int len = 0;
	char req_buf[1024] = {0};
	char json_body[256] = {0};

	if (socket < 0 || room_id == NULL || password == NULL) {
		printf("[chat_join_room] Invalid argument\n");
		return -1;
	}

	snprintf(json_body, sizeof(json_body),
		"{\"room_id\": \"%s\", \"password\": \"%s\"}",
		room_id, password);

	len = snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/room/join", "localhost:8080", strlen(json_body), json_body);

	if (len <= 0 || (size_t)len >= sizeof(req_buf)) {
		printf("[chat_join_room] Request build fail\n");
		return -1;
	}

	return SOCKET_SendRequestBuf(socket, req_buf, (size_t)len);
}
