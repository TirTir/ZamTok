#ifndef _ZT_MSG_H_
#define _ZT_MSG_H_

#include "ZT_Http.h"

#define USER_ID_MAX_LEN 16
#define TOKEN_MAX_LEN 128
#define TYPE_MAX_LEN 16
typedef struct _Token_t {
    char user_id[USER_ID_MAX_LEN];
	char token[TOKEN_MAX_LEN];
    char type[TYPE_MAX_LEN];
	time_t t_expiration_time;
} Token_t;

#endif
