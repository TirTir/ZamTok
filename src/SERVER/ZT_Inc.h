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

#endif
