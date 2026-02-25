#include "ZT_Inc.h"
#include "ZT_ctx.h"
#include "ZT_sock.h"
#include "ZT_evt.h"
#include "ZT_ctrl.h"

ZT_CTX_t gt_ctx_info;
unsigned char g_client_fd[MAX_CLIENTS/8];

int main(int argc, char **argv)
{
	int rc = -1, sockfd = -1;
	int port = 0;
	const char *host = NULL;

	if (argc < 3) {
		printf("[INFO] Use: %s <server_host> <port>\n", argv[0] ? argv[0] : "CLIENT");
		printf("       e.g. %s 127.0.0.1 8080\n", argv[0] ? argv[0] : "CLIENT");
		return -1;
	}

	host = argv[1];
	port = atoi(argv[2]);
	if (port <= 0 || port > 65535) {
		printf("[ERROR] Invalid port\n");
		return -1;
	}

	printf("[INFO] Connecting to %s:%d\n", host, port);

	rc = SOCKET_Init(&sockfd);
	if (rc < 0) {
		printf("[ERROR] SOCKET_Init Fail\n");
		goto close_socket;
	}

	rc = SOCKET_Connect(sockfd, host, port);
	if (rc < 0) {
		printf("[ERROR] SOCKET_Connect Fail\n");
		goto close_socket;
	}

	rc = CTRL_start(sockfd);
	if (rc < 0) {
		printf("[ERROR] CTRL_start Fail\n");
		goto close_socket;
	}

	rc = EventLoop(sockfd, 1);  /* 1 = client mode (no accept) */
	if (rc < 0) {
		printf("[ERROR] EventLoop Fail\n");
		goto close_socket;
	}

close_socket:
	if (sockfd >= 0)
		close(sockfd);

	return 0;
}
