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
