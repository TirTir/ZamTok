#include <stdio.h>


int main( int argc, char **argv )
{
	int rc = -1, fd = -1;
	int port = 0;

	port = ATOI(argv[1]);
	printf( "[INFO] The Server Will Listen to Port: %d\n", port );

	rc = SOCKET_Bind( rc, port );
	if( rc < 0)
	{
		printf("[ERROR] Socket_Bind Fail\n");
		return -1;
	}
	
	rc = SOCKET_Init( AF_INET, SOCK_STREAM, 0 );
	if( rc < 0 )
	{
		printf("[ERROR] Socket_Init Fail\n");
		return -1;
	}

	while(1)
	{
		
	}

	return;
}
