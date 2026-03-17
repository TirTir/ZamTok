#ifndef _ZT_LOG_H_
#define _ZT_LOG_H_

#include <stdio.h>
#include "ZT_log_ts.h"

#define LOG_ERR(fmt, ...)  do { \
    ZT_LOG_TIMESTAMP(); \
    printf("[ERROR] (%s:%d) " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define LOG_INFO(fmt, ...) do { \
    ZT_LOG_TIMESTAMP(); \
    printf("[INFO]  (%s:%d) " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

/** 타임스탬프만 붙인 일반 로그 */
#define LOG_MSG(fmt, ...) do { ZT_LOG_TIMESTAMP(); printf(fmt, ##__VA_ARGS__); } while (0)

#endif
