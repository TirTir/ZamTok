#include <stdio.h>

typedef enum {
	SOCKET_OK = 0,
	ERR_SOCKET_INIT,
	ERR_SOCKET_CREATE,
	ERR_SOCKET_CONNECT,
}

int SOCKET_Init ( )
{
	int fd = -1, rc = 0;
	int opt = 1, flag = 0;

	fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( fd < 0 )
	{
		printf("[SOCKET_Init] Socket Create Fail\n");
		return ERR_SOCKET_CREATE;
	}

	rc = setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int) );
	if( rc < 0 )
	{
		printf("[SOCKET_Init] Socket Init Fail\n");
		return ERR_SOCKET_INIT;
	}
	
	return SOCKET_OK;

}

int __ ( int fd )
{
	char header[MAX_BUF_SIZE];
	char buf[];
	int nContentLen = 0;
	char nContentType[MAX_CONTENT_TYPE_LEN];
	
	

}
