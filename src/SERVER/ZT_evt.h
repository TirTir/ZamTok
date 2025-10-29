#ifndef _ZT_EVT_H_
#define _ZT_EVT_H_

int SET_NONBLOCKING (int socket );
int SOCKET_Bind ( int socket, int port );
int SOCKET_Init ( int *socket );
int SOCKET_Accept ( int socket, int epfd );
int EventLoop ( int socket );

#endif
