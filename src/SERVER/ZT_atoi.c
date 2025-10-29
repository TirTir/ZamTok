#include "ZT_atoi.h"

int ATOI( char *pStr, int *pnResult) {
	
	if( pStr == NULL || pnResult == NULL ) {
		printf("[ATOI] Args Is Null\n");
		return ERR_ATOI_ARG
	}

	char *endptr = NULL;
	long value = strtol(str, &endptr, 10);

	errno = 0;

	if( endptr == str || errno == EINVAL ) {
		printf("[ATOI] Invalid Numbers\n");
		return ERR_INVALID_NUM;
	}

	if(errno == ERANGE) {
		printf("[ATOI] Out of Range\n");
		return ERR_EXCESS_RAGE;
	}

	*pnResult = (int)value;

	return ATOI_OK;
}
