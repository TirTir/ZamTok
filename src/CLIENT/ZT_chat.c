#include "ZT_Inc.h"
#include "ZT_chat.h"

int user_pool[USER_POOL_MAX_COUNT];

int Join(int socket, const user_t *pt_user)
{
	int len, offset = 0;
	char req_buf[1024] = {0};

	len += snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/join", "localhost:8080");
    len += snprintf(req_buf + len, sizeof(req_buf) - len, 
        "{\"user_id\": \"%s\", \"name\": \"%s\", \"phone\": \"%s\", \"email\": \"%s\"}",
        pt_user->str_user_id, pt_user->str_name, pt_user->str_phone, pt_user->str_email);
    if (len <= 0) {
        printf("[Join] Request build fail\n");
        return -1;
    }

	return SOCKET_SendHttpRequest(socket, req_buf);
}

int Login(int socket, const char *str_user_id, const char *str_password)
{
	int len = -1;
	char req_buf[1024] = {0};

	len += snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		"POST", "/login", "localhost:8080");
    len += snprintf(req_buf + len, sizeof(req_buf) - len, 
        "{\"user_id\": \"%s\", \"password\": \"%s\"}",
        str_user_id, str_password);
    if (len <= 0) {
        printf("[Login] Request build fail\n");
        return -1;
    }

    return SOCKET_SendHttpRequest(socket, req_buf);
}