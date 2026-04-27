#ifndef _ZT_CTX_H_
#define _ZT_CTX_H_

#define METHOD_MAX_LEN 8
#define URI_MAX_LEN 256
#define VERSION_MAX_LEN 16
#define BODY_MAX_LEN 4096
#define HEADER_MAX_COUNT 16

#include "ZT_Inc.h"

typedef enum _HeaderType_e {
    HEADER_TYPE_CONTENT_TYPE = 0,
    HEADER_TYPE_CONTENT_LENGTH = 1,
} HeaderType_e;

#define NAME_MAX_LEN 16
#define VALUE_MAX_LEN 128
typedef struct _Header {
    char name[NAME_MAX_LEN];
    char value[VALUE_MAX_LEN];
} Header_t;

typedef struct _ReqType {
    
	char method[METHOD_MAX_LEN];
    char uri[URI_MAX_LEN];
    char version[VERSION_MAX_LEN];

    /* Header (request) */
	Header_t t_content_header[HEADER_MAX_COUNT];
    int content_cnt;

    /* Body */
	char body[BODY_MAX_LEN];
    
} ReqType_t;

typedef enum {
    RESULT_SUCCESS = 0,
    RESULT_FAIL = 1,
} eResultType_e;

#define CODE_MAX_LEN 64
#define MESSAGE_MAX_LEN 128
#define DATA_MAX_LEN 1024
typedef struct _ResultType {
    int result;
    char code[CODE_MAX_LEN];
    char message[MESSAGE_MAX_LEN];
    char data[DATA_MAX_LEN];
} Result_t;

#define REASON_MAX_LEN 64
typedef struct _ResType {
    
	char version[VERSION_MAX_LEN];
    unsigned int ui_status_code;
    char reason[REASON_MAX_LEN];

    /* Header */
	Header_t t_header[HEADER_MAX_COUNT];
    int header_cnt;

    /* Body */
    Result_t t_result;

} ResType_t;


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

#define HEADER_FMT "HTTP/%.15s %d %.63s\r\nContent-Length: %.31s\r\nContent-Type: %.63s\r\n\r\n"

int CTX_Init( ZT_CTX_t *pt_ctx );
int CTX_Http_Insert( ZT_CTX_t *pt_ctx, const int client_fd, struct sockaddr_in t_client_addr );
int CTX_Http_Free( ZT_CTX_t *pt_ctx );

#endif
