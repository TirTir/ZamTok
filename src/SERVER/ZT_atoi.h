#ifndef _ZT_ATOI_H_
#define _ZT_ATOI_H_

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
typedef enum {
    ATOI_OK = 0,
    ERR_ATOI_ARG,
    ERR_INVALID_NUM,
    ERR_EXCESS_RANGE,
}

int ATOI( char *pStr, int *pnResult); 

#endif
