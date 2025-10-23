#include <stdio.h>

/*
typedef enum {
	SOCKET_OK = 0,
	ERR_SOCKET_INIT,
	ERR_SOCKET_BIND,
	ERR_SOCKET_CREATE,
	ERR_SOCKET_CONNECT,
}
*/

typedef struct {
	char *pName;
	int cmd;
	char *pDoc;
}

stCommand_t gpCommands [] = {
	{ "help", 1, "							:This Screen" },
	{ (char*)NULL, 0, NULL }	
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
	int fd = -1, rc = 0;
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
	
	return SOCKET_OK;

}

int SOCKET_Accept ( int socket, int *pnNewfd )
{
	int fd = -1, rc = 0;

	if ( socket < 0 )
	{
		printf("[SOCKET_Accept] Socket FD is Wrong\n");
		return ERR_SOCKET_ARG;
	}

	if ( pnNewfd == NULL )
	{
		printf("[SOCKET_Accept] New FD is NULL\n");
		return ERR_SOCKET_ARG;
	}

	printf("======================= Waiting Connecting =======================\n");
	fd = accept( socket, NULL, NULL );
	if( fd < 0 )
	{
		printf("[SOCKET_Accept] Socket Accept Fail <%d:%s>\n", errno, strerror(errno));
		return ERR_SOCKET_ACCEPT;
	}

	return SOCKET_OK;

}
