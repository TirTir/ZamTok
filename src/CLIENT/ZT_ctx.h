#ifndef _ZT_CTX_H_
#define _ZT_CTX_H_

#define NAME_MAX_LEN 16
#define VALUE_MAX_LEN 128
#define METHOD_MAX_LEN 8
#define URI_MAX_LEN 256
#define VERSION_MAX_LEN 16
#define REASON_MAX_LEN 64
#define HEADER_MAX_LEN 256
#define BODY_MAX_LEN 4096
#define HEADER_MAX_COUNT 16

#include "ZT_Inc.h"
#include <pthread.h>

typedef struct _HeaderType {
    char name[NAME_MAX_LEN];
    char value[VALUE_MAX_LEN];
} HeaderType_t;

typedef struct _ReqType {
    char method[METHOD_MAX_LEN];
    char uri[URI_MAX_LEN];
    char version[VERSION_MAX_LEN];
    HeaderType_t t_header_type[HEADER_MAX_COUNT];
    int header_cnt;
    HeaderType_t t_content_header[HEADER_MAX_COUNT];
    int content_cnt;
    char body[BODY_MAX_LEN];
} ReqType_t;

typedef struct _ResType {
    char version[VERSION_MAX_LEN];
    unsigned int un_status_code;
    char reason[REASON_MAX_LEN];
    HeaderType_t t_header_type;
    int header_Cnt;
    char body[BODY_MAX_LEN];
} ResType_t;

typedef struct _SocketAddr {
    sa_family_t t_family;
    socklen_t un_socklen;
    union {
        struct sockaddr_in  ipv4;
        struct sockaddr_in6 ipv6;
    } addr;
} Socket_Addr_t;

#define BUF_MAX_LEN 1024
#define FILE_MAX_LEN 256

typedef struct HttpCTX_t {
    int client_fd;
    struct sockaddr_in t_client_addr;
    ReqType_t t_req_type;
    ResType_t t_res_type;
    char recv_buf[BUF_MAX_LEN];
    char send_buf[BUF_MAX_LEN];
    char file_path[FILE_MAX_LEN];
    char content_type;
    int content_len;
    int conn_state;
    struct HttpCTX_t *pt_next_ctx;
} HttpCTX_t;

#ifndef MAX_CLIENTS
#define MAX_CLIENTS 100
#endif

typedef struct _ZT_CTX {
    HttpCTX_t *pt_http_ctx;
    int client_cnt;
    pthread_mutex_t mutex;
} ZT_CTX_t;

#define RETRY_MAX_CNT 3
#define MAX_STATUS_MSG_LEN 64

int CTX_Init(ZT_CTX_t *pt_ctx);
int CTX_Http_Insert(ZT_CTX_t *pt_ctx, const int client_fd, struct sockaddr_in t_client_addr);

#endif
