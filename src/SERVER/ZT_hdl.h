#ifndef _ZT_HDL_H_
#define _ZT_HDL_H_

#include "ZT_ctx.h"

int HDL_HEADER( char *p_header, char *p_buf, int status, ReqType_t *t_msg );
int HDL_HEADER_MIME( char *p_content_type, int size, const char *p_uri );
int HDL_SOCKET ( int epfd, int socket );
int HDL_ACCEPT( int socket );
void HDL_400( int socket );
void HDL_500( int socket );

#endif
