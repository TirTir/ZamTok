#ifndef _ZT_CTX_H_
#define _ZT_CTX_H_

typedef enum {
    SOCKET_OK = -1,
    ERR_SOCKET_ARG = -2,
	ERR_SOCKET_INIT = -3,
    ERR_SOCKET_BIND = -4,
    ERR_SOCKET_CREATE = -5,
    ERR_SOCKET_CONNECT = -6,
    ERR_SOCKET_ACCEPT = -7,
	ERR_SOCKET_LISTEN = -8,
	ERR_SOCKET_READ = -9,
	ERR_SOCKET_WRITE = -10,
	ERR_EPOLL_CREATE = -11,
	ERR_NONBLOCKING_ARGS = -12,
	ERR_NONBLOCKING = -13,
	ERR_EVENTLOOP = -14,
};

#define NAME_MAX_LEN 16
#define VALUE_MAX_LEN 128
#define METHOD_MAX_LEN 8
#define URI_MAX_LEN 256
#define VERSION_MAX_LEN 16
#define REASON_MAX_LEN 64
#define BODY_MAX_LEN 1024

typedef struct _HeaderType {
    char name[NAME_MAX_LEN];
    char value[VALUE_MAX_LEN];
} HeaderType_t;

typedef struct _ReqType{
    char method[METHOD_MAX_LEN];
    char uri[URI_MAX_LEN];
    char version[VERSION_MAX_LEN];
    
	HeaderType_t tGeneralHeader[HEADER_MAX_COUNT];
    int nGeneralCnt;

    HeaderType_t tContentHeader[HEADER_MAX_COUNT]; // Content 헤더 (Content-Type, Content-Length)
    int nContentCnt;

	char body[BODY_MAX_LEN];
} ReqType_t;

typedef struct {
    char version[VERSION_MAX_LEN];
    unsigned int unStatusCode;
    char reason[REASON_MAX_LEN];
    HeaderType_t tHeaderType;
    int nHeaderCnt;
    char body[BODY_MAX_LEN];
} ResType_t;

#define BUF_MAX_LEN 1024
#define FILE_MAX_LEN 256

typedef struct {
	int nClientFD;
	struct sockaddr_in client_addr;
	ReqType_t tReqType;
	char recvBuf[BUF_MAX_LEN];
	char sendBuf[BUF_MAX_LEN];
	char filePath[FILE_MAX_LEN];
	char contentType;
	int nContentLen;
	int nStatus;
} HttpCTX_t;

#define RETRY_MAX_CNT 3
#define MAX_STATUS_MSG_LEN 64
#define HEADER_FMT "%s %d %s\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n"

#endif
