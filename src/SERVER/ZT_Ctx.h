#ifndef _ZT_CTX_H_
#define _ZT_CTX_H_

#include "ZT_Inc.h"

typedef struct _SocketAddr {
    sa_family_t		t_family;
    socklen_t		un_socklen;
    
	union {
        struct sockaddr_in  ipv4;
        struct sockaddr_in6 ipv6;
    } addr;

} Socket_Addr_t;

#define BUF_MAX_LEN 1024
#define FILE_MAX_LEN 256

typedef struct {
	
	int client_fd;
	struct sockaddr_in t_client_addr;

	ReqType_t t_req_type;
	ResType_t t_res_type;

    char	recv_buf[BUF_MAX_LEN];
	char	send_buf[BUF_MAX_LEN];
	char	file_path[FILE_MAX_LEN];
	char	content_type;
	int		content_len;
    int		conn_state;

    struct HttpCTX_t *pt_next_ctx; // Linked List Pointer

} HttpCTX_t;

#define MAX_CLIENTS 100

typedef struct _ZT_CTX {

	HttpCTX_t *pt_http_ctx;
	
	int client_cnt;

    pthread_mutex_t mutex;

} ZT_CTX_t;

#define RETRY_MAX_CNT 3
#define MAX_STATUS_MSG_LEN 64

int CTX_Init( ZT_CTX_t *pt_ctx );
int CTX_Http_Insert( ZT_CTX_t *pt_ctx, const int client_fd, struct sockaddr_in t_client_addr );
int CTX_Http_Free( ZT_CTX_t *pt_ctx );

#endif
