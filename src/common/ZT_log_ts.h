#ifndef _ZT_LOG_TS_H_
#define _ZT_LOG_TS_H_

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

/** 표준 출력에 [HH:MM:SS.μs] 형식 타임스탬프 출력 (줄바꿈 없음) */
static inline void ZT_LOG_TIMESTAMP(void)
{
	struct timeval tv;
	struct tm *t;
	gettimeofday(&tv, NULL);
	t = localtime(&tv.tv_sec);
	printf("[%02d:%02d:%02d.%06ld] ", t->tm_hour, t->tm_min, t->tm_sec, (long)tv.tv_usec);
}

#endif
