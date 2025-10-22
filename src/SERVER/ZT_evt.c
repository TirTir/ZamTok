#include <stdio.h>

typedef enum {
	SOCKET_OK = 0,
	ERR_SOCKET_INIT,
	ERR_SOCKET_BIND,
	ERR_SOCKET_CREATE,
	ERR_SOCKET_CONNECT,
}

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
	sin.sin_port = htons( 51000 );

	rc = bind( socket, (struct sockaddr *)&sin, sizeof(sin));
	if( rc < 0 )
	{
		printf("[SOCKET_Bind] Socket Bind Fail\n");
 		return ERR_SOCKET_BIND;
	}

	return rc;
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



/*=================================================
* name : HDL_SOCKET
* return :
* param :
===================================================*/
int HDL_SOCKET ( int socket )
{
	
	char header[MAX_BUF_SIZE];
	char parse[MAX_BUF_SIZE], buf[MAX_BUF_SIZE];
	char *pTempUrl, *pLocalUrl;

	int cnt, nContentLen = 0;
	char contentType[MAX_CONTENT_TYPE_LEN];

	struct stat st;

	size_t n = read( socket, buf, sizeof(buf) - 1 );
	if( n <= 0 )
	{
		printf("[HDL_SOCKET] Read Socket Buf Fail\n");
		HDL_500( socket );
		return;
	}

	buf[n] = '\0';

	printf("====================== HDL_SOCKET_Request ======================\n");
	printf("%s\n", buf);

	snprintf( parse, sizeof(parse), "%s", buf );
	
	/* Data Parsing */
	char *method = strtok( parse, " " );
	char *url = strtok( NULL, " " );
	
	if( !method || !url )
	{
		printf("[HDL_SOCKET] Invalid Request Line\n");
		HDL_400( socket );
		return;
	}

	snprintf( pTempUrl, sizeof(pTempUrl), "%s", url );
	if( !strcmp( pTempUrl, "/"))
	{
		snprintf( pTempUrl, sizeof(pTempUrl), "%s", "/indx.html" );
	}

	printf("====================== HDL_SOCKET_Request Parsing ======================\n");
	printf("\nMethod: %s, URI: %s\n", method, pTempUrl );

	nContentLen = st.st_size;
	HDL_HEADER_MIME( contentType, localUrL );
	HDL_HEADER( header, 200, nContentLen, contentType );
	write( socket, header, strlen( header ));

	while(( cnt = read( fd, buf, BUF_XIZE)) > 0)
	{
		write( socket, buf, cnt );
	}
}
