#ifndef _ZT_CTX_H_
#define _ZT_CTX_H_

#define NAME_MAX_LEN 16
#define VALUE_MAX_LEN 128
#define METHOD_MAX_LEN 8
#define URI_MAX_LEN 256
#define VERSION_MAX_LEN 16
#define REASON_MAX_LEN 64
#define BODY_MAX_LEN 1024
#define HEADER_MAX_COUNT 16

typedef struct _HeaderType {
    char name[NAME_MAX_LEN];
    char value[VALUE_MAX_LEN];
} HeaderType_t;

typedef struct _ReqType {
    char method[METHOD_MAX_LEN];
    char uri[URI_MAX_LEN];
    char version[VERSION_MAX_LEN];
    
	HeaderType_t tHeaderType[HEADER_MAX_COUNT];
    int nHeaderCnt;

    //HeaderType_t tContentHeader[HEADER_MAX_COUNT]; // Content 헤더 (Content-Type, Content-Length)
    //int nContentCnt;

	char body[BODY_MAX_LEN];
} ReqType_t;

typedef struct _ResType {
    char version[VERSION_MAX_LEN];
    unsigned int unStatusCode;
    char reason[REASON_MAX_LEN];
    HeaderType_t tHeaderType;
    int nHeaderCnt;
    char body[BODY_MAX_LEN];
} ResType_t;

typedef struct _SocketAddr {
    sa_familiy_t tFamily;
    socklen_t    unSocklen;
    union {
        struct sockaddr_in  ipv4;
        struct sockaddr_in6 ipv6;
    } addr;
} Socket_Addr_t;

#define BUF_MAX_LEN 1024
#define FILE_MAX_LEN 256

typedef struct {
	int nClientFD;
	struct sockaddr_in tClientAddr;

	ReqType_t tReqType;
	ResType_t tReqType;

    char recvBuf[BUF_MAX_LEN];
	char sendBuf[BUF_MAX_LEN];
	char filePath[FILE_MAX_LEN];
	char contentType;
	int  nContentLen;
    int nConnState;

    HttpCTX_t *ptNextCTX; // Linked List Pointer
} HttpCTX_t;

typedef struct _ZT_CTX {
	HttpCTX_t *ptHeadCTX;
	int nCTXCnt;
    pthread_mutex_t mutex;
} ZT_CTX_t

#define RETRY_MAX_CNT 3
#define MAX_STATUS_MSG_LEN 64
#define HEADER_FMT "%s %d %s\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n"

#endif
