#include "ZT_Inc.h"

ZT_CTX_t gt_ctx_info;
unsigned char g_client_fd[MAX_CLIENTS/8];

int main( int argc, char **argv )
{
	int rc = -1, socket = -1;
	int port = 0;

	if( argc < 1)
	{
		printf("[INFO] Use Format\n");
		return -1;
	}

	port = atoi(argv[1]);
	if( port < 0 )
	{
		printf("[ERROR] Invalid port\n");
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

	rc = CTX_Init( &gt_ctx_info );
	if( rc < 0 )
	{
		printf("[ERROR] CTX_Init Fail\n");
		goto close_socket;
	}

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
