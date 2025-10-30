#ifndef _ZT_HDL_H_
#define _ZT_HDL_H_

void HDL_HEADER( char *pheader, int nStatus, long llen, char *pType );
int HDL_HEADER_MIME( char *pContentType, const char *pUri );
int HDL_SOCKET ( int epfd, int socket );
void HDL_400( int socket );
void HDL_500( int socket );

#endif
