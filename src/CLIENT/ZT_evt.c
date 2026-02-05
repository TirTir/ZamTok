#include "ZT_Inc.h"
#include "ZT_hdl.h"

int EventLoop ( int socket )
{
	struct epoll_event tEv, tEvents[MAX_EVENTS];
	
	int fd, epfd;
	int i, n, rc = 0;

	if ( socket < 0 )
	{
		printf("[EventLoop] Socket FD is Wrong\n");
		return ERR_ARG_INVALID;
	}

	/* create epoll instance */
	
	epfd = epoll_create1(0);
	if( epfd < 0 )
	{
		printf("[EventLoop] Epoll Create Fail\n");
		return ERR_EPOLL_CREATE;
	}

	/* Server Socket 연결 요청 Event */

	tEv.events = EPOLLIN;
	tEv.data.fd = socket;
	
	rc = epoll_ctl( epfd, EPOLL_CTL_ADD, socket, &tEv );
	if( rc < 0 )
	{
		printf("[EventLoop] Epoll Control Fail\n");
		goto close_event;
	}

	rc = SET_NONBLOCKING( socket );
	if( rc < 0 )
	{
		printf("[EventLoop] Set Nonblocking Fail\n");
		goto close_event;
	}

	printf("======================= Waiting Connecting =======================\n");

	while(1)
	{
		n = epoll_wait( epfd, tEvents, MAX_CLIENTS, -1 );
		if( n < 0 )
		{
			if( errno == EINTR )
				continue;
			else
			{
				printf("EventLoop] Epoll Wait Fail\n");
				goto close_event;
			}
        }

		for( i = 0; i < n; i++ )
		{
			fd = tEvents[i].data.fd;
			
			if( fd == socket )
			{
				/* New Clients Connection */
						
				rc = HDL_ACCEPT( socket );
				if( rc < 0 )
				{
					printf("[EventLoop] HDL_ACCEPT fail\n");
					goto close_event;
				}
			}
			else
			{		
				if( tEvents[i].events & EPOLLIN )
				{
					rc = HDL_SOCKET ( epfd, socket );
					if( rc < 0 )
					{
						printf("[EventLoop] HDL_SOCKET fail\n");
						goto close_event;
					}
				}
			}
		}
	}

	return SOCKET_OK;

close_event:
	close(fd);
	return ERR_EVENTLOOP;
}
