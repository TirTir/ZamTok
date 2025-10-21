#ifndef _ZT_LOG_H_
#define _ZT_LOG_H_

#define LOG_ERR(fmt, ...) \
	printf ("[ERROR] (%s:%d) " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif
