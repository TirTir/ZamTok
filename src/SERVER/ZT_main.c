#include "ZT_Inc.h"

HttpCTX_t gtCTXInfo;

int main( int argc, char **argv )
{
	int rc = -1, socket = -1;
	int port = 0;

	int nClientSockets[MAX_CLIENT];

	if( argc < 1)
	{
		printf("[INFO] Use Format\n");
		return -1;
	}

	rc = ATOI(argv[1], &port);
	if( rc < 0 )
	{
		printf("[ERROR] ATOI() Fail\n");
		return -1;
	}

	printf( "[INFO] The Server Will Listen to Port: %d\n", port );
	
	rc = SOCKET_Init( &socket );
	if( rc < 0 )
	{
		printf("[ERROR] Socket_Init Fail\n");
		goto close_socket;
	}

	rc = SOCKET_Bind( socket, port );
	if( rc < 0 )
	{
		printf("[ERROR] Socket_Bind Fail\n");
		goto close_socket;
	}

	memset( gtCTXInfo, 0x00, sizeof(gtCTXInfo) );
	rc = EventLoop( socket );
	if( rc < 0 )
	{
		printf("[ERROR] EventLoop Fail\n");
		goto close_socket;
	}

close_socket:
	if( socket >= 0 )
		close( socket );

	return 0;
}
