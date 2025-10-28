#ifndef _ZT_INC_H_
#define _ZT_INC_H_

typedef enum {
    SOCKET_OK = 0,
    ERR_SOCKET_INIT,
    ERR_SOCKET_BIND,
    ERR_SOCKET_CREATE,
    ERR_SOCKET_CONNECT,
}

int SET_NONBLOCKING (int socket );
int SOCKET_Bind ( int socket, int port );
int SOCKET_Init ( int *socket );
int SOCKET_Accept ( int socket, int epfd );
int EventLoop ( int socket );

#endif
