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

#include "ZT_evt.h"
#include "ZT_hdl.h"
#include "ZT_CTX.h"

#define MAX_CLIENTS 10
#define MAX_EVENTS 64
#define CONTENT_TYPE_MAX_LEN 16

typedef enum {
    SOCKET_OK = 0,
    ERR_INVALID_PARAM = -1,
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
};

#endif
