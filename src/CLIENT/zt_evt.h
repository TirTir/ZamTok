#ifndef _ZT_EVT_H_
#define _ZT_EVT_H_

/* socket: 연결된 소켓 fd. is_client: 1=클라이언트(accept 없음), 0=서버(accept 사용) */
int EventLoop(int socket, int is_client);

#endif
