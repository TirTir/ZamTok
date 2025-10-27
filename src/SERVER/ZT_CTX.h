#ifndef _ZT_CTX_H_
#define _ZT_CTX_H_

typedef enum {
    SOCKET_OK = 0,
    ERR_SOCKET_INIT,
    ERR_SOCKET_BIND,
    ERR_SOCKET_CREATE,
    ERR_SOCKET_CONNECT,
    ERR_SOCKET_ACCEPT,
}

#define NAME_MAX_LEN 16
#define VALUE_MAX_LEN
#define METHOD_MAX_LEN
#define URI_MAX_LEN
#define VERSION_MAX_LEN
#define REASON_MAX_LEN
#define BODY_MAX_LEN

typedef struct {
    char name[NAME_MAX_LEN];
    char value[VALUE_MAX_LEN];
} HeaderType_t;

typedef struct {
    char method[METHOD_MAX_LEN];
    char uri[URI_MAX_LEN];
    char version[VERSION_MAX_LEN];
    HeaderType_t tHeaderType;
    int nHeaderCnt;
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
} HttpCTX_t

#endif
