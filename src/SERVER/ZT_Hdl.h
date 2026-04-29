#ifndef _ZT_HDL_H_
#define _ZT_HDL_H_

#include "ZT_Inc.h"
#include "ZT_chat.h"
#include "ZT_redis.h"
#include "ZT_log.h"
#include "ZT_log_fmt.h"

#define HEADER_MAX_LEN 256
#define USER_ID_KEY "\"user_id\""
#define NAME_KEY "\"name\""
#define PASSWORD_KEY "\"password\""
#define ROOM_ID_KEY "\"room_id\""

int HDL_ACCEPT( int socket, int epfd );
int HDL_SOCKET( int epfd, int socket );
int HDL_HEADER( char *p_header, char *p_buf, int status, ReqType_t *t_msg );
int HDL_HEADER_MIME( char *p_content_type, int size, const char *p_uri );

#endif
