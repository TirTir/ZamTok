#ifndef _ZT_SOCK_H_
#define _ZT_SOCK_H_

#define HTTP_REQUEST_FMT "%s %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s"

int SET_NONBLOCKING(int socket);
int SOCKET_Init(int *pSocket);
int SOCKET_Connect(int socket, const char *host, int port);
int SOCKET_SendHttpRequest(int socket, const char *host, int port,
	const char *method, const char *path);
int SOCKET_SendRequestBuf(int socket, const char *req_buf, size_t len);

#endif
