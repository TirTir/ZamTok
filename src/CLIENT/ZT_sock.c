#include "ZT_Inc.h"
#include "ZT_sock.h"
#include <stddef.h>

int SET_NONBLOCKING(int socket)
{
	int flags;
	int rc;

	if (socket < 0) {
		printf("[SET_NONBLOCKING] Socket is Wrong\n");
		return ERR_ARG_INVALID;
	}

	flags = fcntl(socket, F_GETFL, 0);

	if (flags < 0) {
		printf("[SET_NONBLOCKING] fcntl Get Fail\n");
		return ERR_NONBLOCKING;
	}

	/* file status flag setting */	
	rc = fcntl(socket, F_SETFL, flags | O_NONBLOCK);
	
	if (rc < 0) {
		printf("[SET_NONBLOCKING] fcntl Nonblock Set Fail\n");
		return ERR_NONBLOCKING;
	}

	return SOCKET_OK;
}

int SOCKET_Init(int *pSocket)
{
	int fd;

	if (pSocket == NULL) {
		printf("[SOCKET_Init] Socket is NULL\n");
		return ERR_ARG_INVALID;
	}

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("[SOCKET_Init] Socket Create Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_CREATE;
	}

	*pSocket = fd;
	return SOCKET_OK;
}

int SOCKET_Connect(int socket, const char *host, int port)
{
	struct sockaddr_in sin;
	int rc;

	if (socket < 0) {
		printf("[SOCKET_Connect] Socket is Wrong\n");
		return ERR_ARG_INVALID;
	}
	if (host == NULL) {
		printf("[SOCKET_Connect] Host is NULL\n");
		return ERR_ARG_INVALID;
	}
	if (port <= 0 || port > 65535) {
		printf("[SOCKET_Connect] Port is Wrong\n");
		return ERR_ARG_INVALID;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((unsigned short)port);

	if (inet_pton(AF_INET, host, &sin.sin_addr) <= 0) {
		printf("[SOCKET_Connect] Invalid address: %s\n", host);
		return ERR_SOCKET_CONNECT;
	}

	rc = connect(socket, (struct sockaddr *)&sin, sizeof(sin));
	if (rc < 0) {
		printf("[SOCKET_Connect] Connect Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_CONNECT;
	}

	return SOCKET_OK;
}

int SOCKET_SendHttpRequest(int socket, const char *host, int port,
	const char *method, const char *path)
{
	char req_buf[2048];
	int len, n;

	if (socket < 0 || host == NULL || method == NULL || path == NULL) {
		printf("[SOCKET_SendHttpRequest] Invalid argument\n");
		return ERR_ARG_INVALID;
	}

	len = snprintf(req_buf, sizeof(req_buf), HTTP_REQUEST_FMT,
		method, path, host, "");
	if (len < 0 || (size_t)len >= sizeof(req_buf)) {
		printf("[SOCKET_SendHttpRequest] Request too long\n");
		return ERR_ARG_INVALID;
	}

	n = (int)write(socket, req_buf, (size_t)len);
	if (n < 0) {
		printf("[SOCKET_SendHttpRequest] Write Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_WRITE;
	}
	if (n != len) {
		printf("[SOCKET_SendHttpRequest] Partial write\n");
		return ERR_SOCKET_WRITE;
	}

	return SOCKET_OK;
}

int SOCKET_SendRequestBuf(int socket, const char *req_buf, size_t len)
{
	int n;

	if (socket < 0 || req_buf == NULL) {
		printf("[SOCKET_SendRequestBuf] Invalid argument\n");
		return ERR_ARG_INVALID;
	}

	n = (int)write(socket, req_buf, len);
	if (n < 0) {
		printf("[SOCKET_SendRequestBuf] Write Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_WRITE;
	}
	if ((size_t)n != len) {
		printf("[SOCKET_SendRequestBuf] Partial write\n");
		return ERR_SOCKET_WRITE;
	}

	return SOCKET_OK;
}
