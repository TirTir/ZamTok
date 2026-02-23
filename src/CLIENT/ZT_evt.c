#include "ZT_Inc.h"
#include "ZT_sock.h"
#include "ZT_hdl.h"
#include "ZT_ctrl.h"

#define MAX_CMD_LEN  512
#define MAX_ARGC     32

static int parse_cmd_line(char *line, char **argv, int max_argc)
{
	int argc = 0;
	char *p = line;

	while (argc < max_argc) {
		while (*p == ' ' || *p == '\t') *p++ = '\0';
		if (!*p || *p == '\n') break;
		argv[argc++] = p;
		while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
		if (*p == '\n') { *p = '\0'; break; }
	}
	return argc;
}

int EventLoop(int socket, int is_client)
{
	struct epoll_event tEv, tEvents[MAX_EVENTS];
	int fd, epfd;
	int i, n, rc = 0;

	if (socket < 0) {
		printf("[EventLoop] Socket FD is Wrong\n");
		return ERR_ARG_INVALID;
	}

	epfd = epoll_create1(0);
	if (epfd < 0) {
		printf("[EventLoop] Epoll Create Fail\n");
		return ERR_EPOLL_CREATE;
	}

	tEv.events = EPOLLIN;
	tEv.data.fd = socket;
	rc = epoll_ctl(epfd, EPOLL_CTL_ADD, socket, &tEv);
	if (rc < 0) {
		printf("[EventLoop] Epoll Control Fail\n");
		goto close_event;
	}

	/* 클라이언트 모드: stdin도 함께 감시 → 명령어 입력 시 바로 인식 */
	if (is_client) {
		rc = SET_NONBLOCKING(STDIN_FILENO);
		if (rc == 0) {
			tEv.events = EPOLLIN;
			tEv.data.fd = STDIN_FILENO;
			rc = epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &tEv);
			if (rc == 0)
				printf("[EventLoop] stdin registered (type command and press Enter)\n");
		}
	}

	rc = SET_NONBLOCKING(socket);
	if (rc < 0) {
		printf("[EventLoop] Set Nonblocking Fail\n");
		goto close_event;
	}

	printf("======================= Waiting for data =======================\n");

	while (1) {
		n = epoll_wait(epfd, tEvents, MAX_EVENTS, -1);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			printf("[EventLoop] Epoll Wait Fail\n");
			goto close_event;
		}

		for (i = 0; i < n; i++) 
		{
			fd = tEvents[i].data.fd;
			if (fd == STDIN_FILENO && is_client) 
			{
				
				/* 터미널 입력 한 줄 읽기 → 명령으로 바로 디스패치 */
				
				static char line_buf[MAX_CMD_LEN];
				static int line_len = 0;
				char *argv[MAX_ARGC];
				int argc, nr;

				nr = read(STDIN_FILENO, line_buf + line_len, sizeof(line_buf) - line_len - 1);
				if (nr <= 0) 
				{
					if (errno == EWOULDBLOCK || errno == EAGAIN) continue;
					break;
				}

				line_len += nr;
				line_buf[line_len] = '\0';

				if (!strchr(line_buf, '\n')) 
				{
					if (line_len >= (int)sizeof(line_buf) - 1) line_len = 0;
					continue;
				}

				line_buf[line_len] = '\0';
				line_len = 0;
				
				argc = parse_cmd_line(line_buf, argv, MAX_ARGC);
				
				if (argc > 0)
					CTRL_proc(argc, argv);
			} 
			else if (fd == socket && !is_client) 
			{
				/* Server only: new client connection */
				rc = HDL_ACCEPT(socket);
				if (rc < 0) 
				{
					printf("[EventLoop] HDL_ACCEPT fail\n");
					goto close_event;
				}
			} 
			else 
			{
				if (tEvents[i].events & EPOLLIN) {
					rc = HDL_SOCKET(epfd, fd);
					if (rc < 0) {
						printf("[EventLoop] HDL_SOCKET fail\n");
						goto close_event;
					}
				}
			}
		}
	}

	return SOCKET_OK;

close_event:
	if (epfd >= 0)
		close(epfd);
	return ERR_EVENTLOOP;
}
