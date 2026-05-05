#ifndef __ZT_COMMON_H__
#define __ZT_COMMON_H__

/* --- 프로젝트 전역 매크로 --- */
#define BUF_MAX_LEN 1024
#define FILE_MAX_LEN 256
#define RETRY_MAX_CNT 3
#define MAX_STATUS_MSG_LEN 64

/* --- 표준 C --- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>

/* --- POSIX / 네트워크 --- */
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* --- 외부 라이브러리 (서버 공통) --- */
#include <hiredis/hiredis.h>

/* --- 공통 디렉터리 (Makefile -I../common) --- */
#include "zt_log_ts.h"
#include "zt_chat.h"

/* --- SERVER 헤더 (의존 순서) --- */
#include "zt_log.h"
#include "zt_log_fmt.h"
#include "zt_http.h"
#include "zt_ctx.h"
#include "zt_util.h"
#include "zt_redis.h"

#define MAX_EVENTS 64
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
