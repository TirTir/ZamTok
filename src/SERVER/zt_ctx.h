#ifndef _ZT_CTX_H_
#define _ZT_CTX_H_

#define MAX_CLIENTS 100

typedef struct socket_addr {
	sa_family_t family;
	socklen_t socklen;
	union {
		struct sockaddr_in ipv4;
		struct sockaddr_in ipv6;
	} addr;
} socket_addr_t;

typedef struct http_conn_ctx {
	int client_fd;
	socket_addr_t client_addr;
	req_t req;
	res_t res;
	struct http_conn_ctx *next;
} http_conn_ctx_t;

typedef struct zt_server_ctx {
	http_conn_ctx_t *http_conn_head;
	int client_count;
	pthread_mutex_t mutex;
} zt_server_ctx_t;


#endif
