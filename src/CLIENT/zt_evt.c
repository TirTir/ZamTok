#include "zt_common.h"
#include "zt_sock.h"
#include "zt_hdl.h"
#include "zt_ctrl.h"
#include "zt_log.h"
#include "zt_log_fmt.h"

int EventLoop(int socket, int is_client)
{
	struct epoll_event tEv, tEvents[MAX_EVENTS];
	int fd, epfd;
	int i, n, rc = 0;

	if (socket < 0) {
		LOG_MSG("[EventLoop] Socket FD is Wrong\n");
		return ZT_RC_ARG_INVALID;
	}

	epfd = epoll_create1(0);
	if (epfd < 0) {
		LOG_MSG("[EventLoop] Epoll Create Fail\n");
		return ZT_RC_SOCKET;
	}

	tEv.events = EPOLLIN;
	tEv.data.fd = socket;
	rc = epoll_ctl(epfd, EPOLL_CTL_ADD, socket, &tEv);
	if (rc < 0) {
		LOG_MSG("[EventLoop] Epoll Control Fail\n");
		goto close_event;
	}

	rc = SET_NONBLOCKING(socket);
	if (rc != ZT_RC_OK) {
		LOG_MSG("[EventLoop] Set Nonblocking Fail\n");
		goto close_event;
	}

	LOG_FMT_CENTER("Waiting for data");

	while (1) {
		n = epoll_wait(epfd, tEvents, MAX_EVENTS, -1);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			LOG_MSG("[EventLoop] Epoll Wait Fail\n");
			goto close_event;
		}

		for (i = 0; i < n; i++) 
		{
			fd = tEvents[i].data.fd;
			if (fd == socket && !is_client) 
			{
				/* Server only: new client connection */
				rc = HDL_ACCEPT(socket);
				if (rc != ZT_RC_OK) 
				{
					LOG_MSG("[EventLoop] HDL_ACCEPT fail\n");
					goto close_event;
				}
			} 
			else 
			{
				if (tEvents[i].events & EPOLLIN) {
					if (is_client) 
					{
						rc = HDL_CLIENT_RECV(fd);
					} 
					else 
					{
						rc = HDL_SOCKET(epfd, fd);
					}
					
					if (rc != ZT_RC_OK) {
						LOG_MSG("[EventLoop] %s fail\n", is_client ? "HDL_CLIENT_RECV" : "HDL_SOCKET");
						goto close_event;
					}
				}
			}
		}
	}

	return ZT_RC_OK;

close_event:

	if (epfd >= 0)
		close(epfd);
	
	return ZT_RC_SOCKET;
}
