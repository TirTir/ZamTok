#include "ZT_Inc.h"
#include "ZT_chat.h"
#include "ZT_sock.h"

int user_pool[USER_POOL_MAX_COUNT];

int Join(int socket, const user_t *pt_user)
{
	int len = 0;
	char req_buf[1024] = {0};
	char json_body[512] = {0};

	snprintf(json_body, sizeof(json_body),
		"{\"user_id\": \"%s\", \"name\": \"%s\", \"password\": \"%s\"}",
		pt_user->str_user_id, pt_user->str_name, pt_user->str_pwd);

	len = snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/join", "localhost:8080", strlen(json_body), json_body);

	if (len <= 0 )
	{
		printf("[Join] Request build fail\n");
		return -1;
	}

	return SOCKET_SendRequestBuf(socket, req_buf, (size_t)len);
}

int Login(int socket, const char *str_user_id, const char *str_password)
{
	int len = 0;
	char req_buf[1024] = {0};
	char json_body[256] = {0};

	snprintf(json_body, sizeof(json_body),
		"{\"user_id\": \"%s\", \"password\": \"%s\"}",
		str_user_id, str_password);

	len = snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/login", "localhost:8080", strlen(json_body), json_body);

	if (len <= 0 || (size_t)len >= sizeof(req_buf)) {
		printf("[Login] Request build fail\n");
		return -1;
	}

	return SOCKET_SendRequestBuf(socket, req_buf, (size_t)len);
}

int CreateRoom(int socket, const char *str_room_id, const char *str_password, const char *str_user_id)
{
	int len = 0;
	char req_buf[1024] = {0};
	char json_body[256] = {0};

	if (str_user_id && str_user_id[0] != '\0') {
		snprintf(json_body, sizeof(json_body),
			"{\"room_id\": \"%s\", \"password\": \"%s\", \"user_id\": \"%s\"}",
			str_room_id, str_password, str_user_id);
	} else {
		snprintf(json_body, sizeof(json_body),
			"{\"room_id\": \"%s\", \"password\": \"%s\"}",
			str_room_id, str_password);
	}

	len = snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/room", "localhost:8080", strlen(json_body), json_body);

	if (len <= 0 || (size_t)len >= sizeof(req_buf)) {
		printf("[CreateRoom] Request build fail\n");
		return -1;
	}

	return SOCKET_SendRequestBuf(socket, req_buf, (size_t)len);
}

int SearchRoom(int socket, const char *str_room_id)
{
	char path[128] = {0};

	if (socket < 0 || str_room_id == NULL) {
		printf("[SearchRoom] Invalid argument\n");
		return -1;
	}

	snprintf(path, sizeof(path), "/room?id=%s", str_room_id);

	return SOCKET_SendHttpRequest(socket, "localhost", 8080, "GET", path);
}

int ListRooms(int socket)
{
	if (socket < 0) {
		printf("[ListRooms] Invalid socket\n");
		return -1;
	}

	return SOCKET_SendHttpRequest(socket, "localhost", 8080, "GET", "/rooms");
}

int JoinRoom(int socket, const char *str_room_id, const char *str_password)
{
	int len = 0;
	char req_buf[1024] = {0};
	char json_body[256] = {0};

	if (socket < 0 || str_room_id == NULL || str_password == NULL) {
		printf("[JoinRoom] Invalid argument\n");
		return -1;
	}

	snprintf(json_body, sizeof(json_body),
		"{\"room_id\": \"%s\", \"password\": \"%s\"}",
		str_room_id, str_password);

	len = snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/room/join", "localhost:8080", strlen(json_body), json_body);

	if (len <= 0 || (size_t)len >= sizeof(req_buf)) {
		printf("[JoinRoom] Request build fail\n");
		return -1;
	}

	return SOCKET_SendRequestBuf(socket, req_buf, (size_t)len);
}
