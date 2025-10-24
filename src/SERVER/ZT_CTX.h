#ifndef _ZT_CTX_H_
#define _ZT_CTX_H_

#define NAME_MAX_LEN 12

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
