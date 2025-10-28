#include "ZT_Inc.h"

typedef struct {
	char *pName;
	int cmd;
	char *pDoc;
}

stCommand_t gpCommands [] = {
	{ "help", 1, "							:This Screen" },
	{ (char*)NULL, 0, NULL }	
}

extern g_nClients;

int SET_NONBLOCKING (int socket )
{
	int flags = -1;

	if( socket < 0 )
	{
		printf("[SET_NONBLOCKING] Socket is Wrong\n");
		return ERR_NONBLOCKING_ARGS;
	}

	/* Get Original FD Status */
	flags = fnctl( socket, F_GETFL, 0 );
	if( flags < 0 )
	{
		printf("[SET_NONBLOCKING] Fnctl Get Fail\n");
		return ERR_NONBLOCKING;
	}

	rc = fnctl( socket, F_SETFL, flags | O_NONBLOCK );
	if( rc < 0 )
	{
		printf("[SET_NONBLOCKING] Fnctl Nonblock Set Fail\n");
		return ERR_NONBLOCKING;
	}
}

int SOCKET_Bind ( int socket, int port )
{
	int rc = 0;
	struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr( "127.0.0.1" );
	sin.sin_port = htons( port );

	rc = bind( socket, (struct sockaddr *)&sin, sizeof(sin));
	if( rc < 0 )
	{
		printf("[SOCKET_Bind] Socket Bind Fail <%d:%s>\n", errno, strerror(errno));
 		return ERR_SOCKET_BIND;
	}
	
	rc = listen( socket, MAX_CLIENT );
	if( rc < 0 )
	{
		printf("[SOCKET_Bind] Socket listen Fail <%d:%s>\n", errno, strerror(errno));
 		return ERR_SOCKET_LISTEN;
	}

	return SOCKET_OK;

}

int SOCKET_Init ( int *socket )
{
	int i, rc = 0;
	int fd = -1;
	int opt = 1, flag = 0;
	
	if( socket == NULL )
	{
		printf("[SOCKET_Init] Socket is NULL\n");
	}

	/* create socket
	*) domain / type / protocol */
	fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( fd < 0 )
	{
		printf("[SOCKET_Init] Socket Create Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_CREATE;
	}

	rc = setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int) );
	if( rc < 0 )
	{
		printf("[SOCKET_Init] Socket Set Opt Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_INIT;
	}
	
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		g_nClients[i] = -1;
	}

	return SOCKET_OK;

}

int SOCKET_Accept ( int socket, int epfd )
{
	struct sockaddr_in tClientAddr;
	socklen_t addrlen = sizeof(tClientAddr);

	int nClientFD = -1, 
    int rc = 0;

	if ( socket < 0 )
	{
		printf("[SOCKET_Accept] Socket FD is Wrong\n");
		return ERR_SOCKET_ARG;
	}

	if ( epfd < 0 )
	{
		printf("[SOCKET_Accept] Epoll FD is Wrong\n");
		return ERR_SOCKET_ARG;
	}
	
	while(( nClientFD = accept( socket, NULL, NULL )) > 0 )
	{
        printf("======================= Connecting =======================\n");
	    printf("FD=%d from %s:%d\n", nClientFD, inet_ntoa(tClientAddr.sin_addr), ntohs(tClientAddr.sin_port));

		/* Edge Trigger */
		/* Client Socket Event ADD */
		
        tEv.events = EPOLLIN | EPOLLET;
		tEv.data.fd = nClientFD;
		
		rc = epoll_ctl( epfd, EPOLL_CTL_ADD, nClientFD, &tEv );
	    if( rc < 0 )
	    	break;
    }

	if( rc < 0 || nClientFD < 0 || errno != EAGAIN && errno != EWOULDBLOCK )
	{
		printf("[SOCKET_Accept] Socket Accept Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_ACCEPT;
	}

	return SOCKET_OK;
}

int EventLoop ( int socket )
{
	/* tEv: fd 단일 등록 -> epoll_ctl()
	   tEvs: fd에서 발생한 이벤트들 -> epoll_wait() */
	struct epoll_event tEv, tEvents[MAX_EVENTS];
	
	int fd, epfd, flags = -1;
	int i, n, rc = 0;

	if ( socket < 0 )
	{
		printf("[EventLoop] Socket FD is Wrong\n");
		return ERR_SOCKET_ARG;
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
			if( errno == EINTER )
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
			
			switch(fd)
			{
				case (socket):
					
					/* New Clients Connection */
						
					rc = HDL_ACCEPT( epfd, socket );
					if( rc < 0 )
					{
						printf("[EventLoop] HDL_ACCEPT fail\n");
						goto close_event;
					}

					break;
	
				default:
						
					if( tEvents[i].events & EPOLLIN )
					{
						rc = HDL_SOCKET ( epfd, socket );
						if( rc < 0 )
						{
							printf("[EventLoop] HDL_SOCKET fail\n");
							goto close_event;
						}
					}

					break;
			}
		}
	}

	return SOCKET_OK;

close_event:
	close(fd);
	return ERR_EVENTLOOP;
}
