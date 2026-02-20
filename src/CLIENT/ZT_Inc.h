#ifndef _ZT_INC_H_
#define _ZT_INC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS  64
#define MAX_CLIENTS 64
#define CONTENT_TYPE_MAX_LEN 16

typedef enum {
    SOCKET_OK = 0,
    CLIENT_OK = 0,
    CTX_OK = 0,
    ERR_ARG_INVALID = -1,
    ERR_SOCKET_INIT = -2,
    ERR_SOCKET_BIND = -3,
    ERR_SOCKET_CREATE = -4,
    ERR_SOCKET_CONNECT = -5,
    ERR_SOCKET_ACCEPT = -6,
    ERR_SOCKET_LISTEN = -7,
    ERR_SOCKET_READ = -8,
    ERR_SOCKET_WRITE = -9,
    ERR_EPOLL_CREATE = -10,
    ERR_NONBLOCKING = -11,
    ERR_EVENTLOOP = -12,
    ERR_CTX_ALLOC = -20,
    ERR_CTX_LOCK = -21,
    ERR_CTX_UNLOCK = -22,
    ERR_CTX_FULL = -23,
    ERR_CTX_INSERT = -24,
} zt_client_e;

#endif
