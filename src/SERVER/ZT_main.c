#include <stdio.h>
#include "ZT_Inc.h"

int g_nClients;

int main( int argc, char **argv )
{
	int rc = -1, socket = -1;
	int port = 0;

	int nClientSockets[MAX_CLIENT];

	port = ATOI(argv[1]);
	printf( "[INFO] The Server Will Listen to Port: %d\n", port );
	
	rc = SOCKET_Init( &socket );
	if( rc < 0 )
	{
		printf("[ERROR] Socket_Init Fail\n");
		close_socket;
	}

	rc = SOCKET_Bind( socket, port );
	if( rc < 0 )
	{
		printf("[ERROR] Socket_Bind Fail\n");
		goto close_socket;
	}

	rc = SOCKET_Accept( socket,  );
	if( rc < 0 )
	{
		printf("[ERROR] Socket_Accept Fail\n");
		goto close_socket;
	}
	
	rc = HDL_SOCKET( );
	if( rc < 0 )
	{
		printf("[ERROR] HDL_SOCKET Fail\n");
		goto close_socket;
	}	

close_socket;
	if( socket >= 0 )
		close( socket );

	return 0;
}
