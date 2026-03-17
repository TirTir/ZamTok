#ifndef _ZT_LOG_FMT_H_
#define _ZT_LOG_FMT_H_

#include <stdio.h>
#include <string.h>
#include "ZT_log_ts.h"

/* 고정 너비: 구분선 '=' 개수 및 한 줄 글자 수 */
#define LOG_FMT_WIDTH  52
#define LOG_FMT_LEFT  14   /* 왼쪽 컬럼(예: Command) 글자 수 */
#define LOG_FMT_RIGHT 34   /* 오른쪽 컬럼(예: Description) 글자 수 */

/** 구분선 출력 (항상 LOG_FMT_WIDTH개의 '=') */
#define LOG_FMT_SEP()  do { \
	ZT_LOG_TIMESTAMP(); \
	int _i; \
	for (_i = 0; _i < LOG_FMT_WIDTH; _i++) putchar('='); \
	putchar('\n'); \
} while (0)

/** 두 컬럼 한 줄 (왼쪽 LOG_FMT_LEFT, 오른쪽 LOG_FMT_RIGHT 너비, 공백 패딩) */
#define LOG_FMT_LINE(left, right)  do { \
	ZT_LOG_TIMESTAMP(); \
	printf("%-*s %-*s\n", LOG_FMT_LEFT, (left), LOG_FMT_RIGHT, (right)); \
} while (0)

/** 가운데 제목: "==== ... 텍스트 ... ====" 형태, 전체 너비 LOG_FMT_WIDTH */
static inline void LOG_FMT_CENTER(const char *msg)
{
	int len = (int)strlen(msg);
	int left = (LOG_FMT_WIDTH - len - 2) / 2;  /* 앞 '=' 개수 */
	int right = LOG_FMT_WIDTH - len - 2 - left; /* 뒤 '=' 개수 */
	int i;
	ZT_LOG_TIMESTAMP();
	for (i = 0; i < left; i++) putchar('=');
	putchar(' ');
	printf("%s", msg);
	putchar(' ');
	for (i = 0; i < right; i++) putchar('=');
	putchar('\n');
}

#endif
