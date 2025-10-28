#ifndef _ZT_HDL_H_
#define _ZT_HDL_H_

void HDL_HEADER( char *pheader, int nStatus, long llen, char *pType );
int HDL_HEADER_MIME( char *pContentType, const char *pUri, size_t tContentLen );
int HDL_SOCKET ( int epfd, int socket );

#endif
