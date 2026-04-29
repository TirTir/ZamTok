#include "ZT_Inc.h"

extern unsigned char g_client_fd[MAX_CLIENTS/8];

int socket_set_nonblocking(int socket )
{
	int flags = -1;
	int rc = 0;

	if( socket < 0 )
	{
		LOG_MSG("[SET_NONBLOCKING] Socket is Wrong\n");
		return ERR_ARG_INVALID;
	}

	/* Get Original FD Status */
	flags = fcntl( socket, F_GETFL, 0 );
	if( flags < 0 )
	{
		LOG_MSG("[SET_NONBLOCKING] Fnctl Get Fail\n");
		return ERR_NONBLOCKING;
	}

	rc = fcntl( socket, F_SETFL, flags | O_NONBLOCK );
	if( rc < 0 )
	{
		LOG_MSG("[SET_NONBLOCKING] Fnctl Nonblock Set Fail\n");
		return ERR_NONBLOCKING;
	}

	return SOCKET_OK;
}

int socket_bind( int socket, int port )
{
	int rc = 0;
	struct sockaddr_in sin;

	if( socket < 0 )
	{
		LOG_MSG("[SOCKET_Bind] Socket is Wrong\n");
		return ERR_ARG_INVALID;
	}

	if( port < 0 )
	{
		LOG_MSG("[SOCKET_Bind] Port is Wrong\n");
		return ERR_ARG_INVALID;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons( port );

	rc = bind( socket, (struct sockaddr *)&sin, sizeof(sin));
	if( rc < 0 )
	{
		LOG_MSG("[SOCKET_Bind] Socket Bind Fail <%d:%s>\n", errno, strerror(errno));
 		return ERR_SOCKET_BIND;
	}
	
	rc = listen( socket, MAX_CLIENTS );
	if( rc < 0 )
	{
		LOG_MSG("[SOCKET_Bind] Socket listen Fail <%d:%s>\n", errno, strerror(errno));
 		return ERR_SOCKET_LISTEN;
	}

	return SOCKET_OK;

}

int socket_init( int *pSocket )
{
	int rc = 0;
	int fd = -1, opt = 1;
	
	if( pSocket == NULL )
	{
		LOG_MSG("[SOCKET_Init] Socket is NULL\n");
	}

	/* 
	*) create socket
	*) domain / type / protocol 
	*/
	fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( fd < 0 )
	{
		LOG_MSG("[SOCKET_Init] Socket Create Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_CREATE;
	}

	rc = setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int) );
	if( rc < 0 )
	{
		LOG_MSG("[SOCKET_Init] Socket Set Opt Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_INIT;
	}

	*pSocket = fd;
	
	return SOCKET_OK;

}

int socket_accept( int socket, int epfd )
{
	struct sockaddr_in tClientAddr;
	socklen_t addrlen = sizeof(tClientAddr);
	struct epoll_event tEv;

	int nClientFD = -1;
	int rc = 0;

	if ( socket < 0 || epfd < 0 )
	{
		LOG_MSG("[SOCKET_Accept] Socket FD or Epoll FD is Wrong <%d:%s>\n", errno, strerror(errno));
		return ERR_ARG_INVALID;
	}

	while ( (nClientFD = accept( socket, (struct sockaddr *)&tClientAddr, &addrlen )) > 0 )
	{
		LOG_FMT_CENTER("Connecting");
		LOG_MSG("FD=%d from %s:%d\n", nClientFD, inet_ntoa(tClientAddr.sin_addr), ntohs(tClientAddr.sin_port));
		
        tEv.events = EPOLLIN | EPOLLET;
		tEv.data.fd = nClientFD;
		
		rc = epoll_ctl( epfd, EPOLL_CTL_ADD, nClientFD, &tEv );
	    if( rc < 0 )
	    	break;
    }

	if( rc < 0 || nClientFD < 0 || ( errno != EAGAIN && errno != EWOULDBLOCK ))
	{
		LOG_MSG("[SOCKET_Accept] Socket Accept Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_ACCEPT;
	}

	return SOCKET_OK;
}

int socket_send( int socket, const char *buf, int len )
{
	int rc = 0;
	
	if( socket < 0 || buf == NULL || len <= 0 )
	{
		LOG_MSG("[SOCKET_Send] Socket FD or Buffer is Wrong\n");
		return ERR_ARG_INVALID;
	}

	rc = write( socket, buf, len );
	if( rc < 0 )
	{
		LOG_ERR("[SOCKET_Send] Write Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_WRITE;
	}
	
	LOG_MSG("===============Send Response===============\n");
	LOG_MSG("===============Send Response===============\n");
	LOG_MSG("%s\n", buf);
	LOG_MSG("===============Send Response===============\n");
	LOG_MSG("===============Send Response===============\n");

	return rc;
}

int socket_event_loop( int socket )
{
	struct epoll_event tEv, tEvents[MAX_EVENTS];

	int fd = -1, epfd;
	int i, n, rc = 0;

	/* create epoll instance */

	epfd = epoll_create1(0);
	if( epfd < 0 )
	{
		LOG_MSG("[EventLoop] Epoll Create Fail\n");
		return ERR_EPOLL_CREATE;
	}

	/* Server Socket 연결 요청 Event */

	tEv.events = EPOLLIN;
	tEv.data.fd = socket;
	
	rc = epoll_ctl( epfd, EPOLL_CTL_ADD, socket, &tEv );
	if( rc < 0 )
	{
		LOG_MSG("[EventLoop] Epoll Control Fail\n");
		goto close_event;
	}

	rc = socket_set_nonblocking( socket );
	if( rc < 0 )
	{
		LOG_MSG("[EventLoop] Set Nonblocking Fail\n");
		goto close_event;
	}

	LOG_FMT_CENTER("Waiting Connecting");

	while(1)
	{
		n = epoll_wait( epfd, tEvents, MAX_EVENTS, -1 );
		if( n < 0 )
		{
			if( errno == EINTR )
				continue;
			else
			{
				LOG_MSG("[EventLoop] Epoll Wait Fail\n");
				goto close_event;
			}
        }

		for( i = 0; i < n; i++ )
		{
			fd = tEvents[i].data.fd;
			
			if( fd == socket )
			{
				/* New Clients Connection */
				rc = hdl_accept( socket, epfd );
				if( rc < 0 )
				{
					LOG_MSG("[EventLoop] HDL_ACCEPT fail\n");
					goto close_event;
				}
			}
			else
			{		
				if( tEvents[i].events & EPOLLIN )
				{
					rc = hdl_socket( epfd, fd );
					if( rc != SOCKET_OK )
					{
						/* 클라이언트 끊김/잘못된 요청 → 해당 fd만 정리하고 서버는 계속 동작 */
						if( rc == SOCKET_CLIENT_DISCONNECT )
							LOG_MSG("[EventLoop] Client FD=%d disconnect, closing\n", fd);
						else
							LOG_MSG("[EventLoop] HDL_SOCKET fail FD=%d (rc=%d), closing\n", fd, rc);
						(void)epoll_ctl( epfd, EPOLL_CTL_DEL, fd, NULL );
						close( fd );
						if ( fd >= 0 && fd < MAX_CLIENTS )
							g_client_fd[fd/8] &= (unsigned char)~( 1 << ( fd % 8 ) );
					}
				}
			}
		}
	}

	return SOCKET_OK;

close_event:
	/* 치명적 오류(리스너/epoll 설정 실패 등) 시에만 진입 */
	if ( fd >= 0 )
		close(fd);
	return ERR_EVENTLOOP;
}
