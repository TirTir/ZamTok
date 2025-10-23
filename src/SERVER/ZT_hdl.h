#ifndef _ZT_HDL_H_
#define _ZT_HDL_H_

typedef struct {
	char name[NAME_LEN_MAX];
	char value[VALUE_LEN_MAX];
} HeaderType_t;

typedef struct {
	char method[METHOD_LEN_MAX];
	char uri[URI_LEN_MAX];
	char version[VERSION_LEN_MAX];
	HeaderType_t tHeaderType;
	int nHeaderCnt;	
	char body[BODY_LEN_MAX];
} ReqType_t;

typedef struct {
	char version[VERSION_LEN_MAX];
	unsigned int unStatusCode;
	char reason[REASON_LEN_MAX];
	HeaderType_t tHeaderType;
	int nHeaderCnt;	
	char body[BODY_LEN_MAX];
} ResType_t;

int HDL_HEADER( char *pheader, int nStatus, long llen, char *pType );
int HDL_HEADER_MIME( char *pContentType, const char *pUri, size_t tContentLen );

#endif
