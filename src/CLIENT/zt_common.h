#ifndef __ZT_COMMON_H__
#define __ZT_COMMON_H__

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENTS 64
#define MAX_CLIENTS 64
#define CONTENT_TYPE_MAX_LEN 16

typedef enum {
	ZT_RC_OK = 0,
	ZT_RC_FAIL,
	ZT_RC_ARG_INVALID,
	ZT_RC_SOCKET,
	ZT_RC_CTX,
	ZT_RC_REDIS,
} return_code_t;

#endif
