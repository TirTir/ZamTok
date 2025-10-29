#include "ZT_Inc.h"

/*=================================================
 * name : HDL_HEADER
 * return : 
 * param : *pheader, nStatus, llen, *pType
 ===================================================*/

void HDL_HEADER( char *pHeader, int nStatus, long llen, char *pType )
{
	if(pHeader == NULL || nStatus < 100 || llen <= 0 || pType == NULL ) return;

	char msg[MAX_STATUS_MSG_LEN];

	switch (nStatus)
	{
		case 200:
			snprintf(msg, MAX_STATUS_MSG_LEN, "OK");
			break;

		case 400:
			snprintf(msg, MAX_STATUS_MSG_LEN, "Not Found");
			break;

		case 500:
		default:
			snprintf(msg, MAX_STATUS_MSG_LEN, "Internal Server Error");
			break;
	}

	snprintf( pHeader, HEADER_FMT, nStatus, msg, llen, pType );
}


/*=================================================
 * name : HDL_HEADER_MIME
 * return :
 * param : *pContentType, *pUri, tContentLen 
 ===================================================*/

int HDL_HEADER_MIME( char *pContentType, const char *pUri, size_t tContentLen )
{
	if( pContentType == NULL )
	{
		printf("[HDL_HEADER_MIME] Content Type is NULL\n");
		return HDL_HEADER_MIME_ARGS;
	}
	
	if ( pUri == NULL )
	{
		printf("[HDL_HEADER_MIME] URI is NULL\n");
		return HDL_HEADER_MIME_ARGS;
	}

	char *ext = strrchr( pUri, '.' );

	if( !strcmp( ext, ".html") )
	{
		snprintf( pContentType, CONTENT"text/html", tContentLen );
	}
	else if( !strcmp( ext, ".jpg") || !strcmp( ext, ".jpeg" ))
	{
		strncpy( pContentType, "image/jpeg",  tContentLen );
	}
	else if( !strcmp( ext, ".png") )
	{
		strncpy( pContentType, "image/png",  tContentLen );
	}
	else if( !strcmp( ext, ".CSS") )
	{
		strncpy( pContentType, "text/CSS",  tContentLen );
	}
	else if( !strcmp( ext, ".js") )
	{
		strncpy( pContentType, "text/javascript",  tContentLen );
	}
	else 
		strncpy( pContentType, "text/plain",  tContentLen );

	return SOCKET_OK;
}

/*=================================================
 * name : HDL_SOCKET
 * return :
 * param :
 ===================================================*/
int HDL_SOCKET ( int epfd, int socket )
{
	ReqType_t tMsg;

	char buf[BUF_MAX_LEN] = {0};
	size_t n = 0;
	int nRetryCnt = 0;

	if( epfd < 0 )
	{
		printf("[HDL_SOCKET] Epoll FD is Wrong\n");
		return ERR_SOCKET_ARG;
	}

	if( socket < 0 )
	{
		printf("[HDL_SOCKET] Socket FD is Wrong\n");
		return ERR_SOCKET_ARG;
	}

	nRetryCnt = 0;
	while( nRetryCnt < RETRY_MAX_CNT )
	{
		errno = 0;
		n = read( socket, buf, sizeof(buf) ); 
		
		if( n < 0 && ( errno == EWOULDBLOCK ) || ( errno == EAGAIN ) || ( errno == EINTR ) )
		{
			printf("[HDL_SOCKET] Read Socket Fail FD=%d <%d:%s>\n", socket, errno, strerror(errno));
			nRetryCnt++;
			continue;
		}

		buf[n] = '\0';

		printf("====================== HDL_SOCKET_Request ======================\n");
		printf("%s\n", buf);

		snprintf( parse, sizeof(parse), "%s", buf );

		char *method = strtok( parse, " " );
		char *url = strtok( NULL, " " );
		char *version = strtok( NULL, "\r\n" );

		if( !method || !url || !version )
		{
			printf("[HDL_SOCKET] Invalid Request Line\n");
			HDL_400( socket );
			return;
		}

		/* Data Parsing */
		snprintf( tMsg.method, sizeof(tMsg.method), "%s", method );
		snprintf( tMsg.uri, sizeof(tMsg.uri), "%s", url );
		snprintf( tMsg.version, sizeof(tMsg.version), "%s", version );


		/* 요청 경로 설정 */
		if( !strcmp( tMsg.uri, "/"))
		{
			snprintf( tMsg.uri, sizeof(tMsg.uri), "%s", "/indx.html" );
		}


		HDL_HEADER_MIME( &tMsg.tContentHeader, localUrl );
		HDL_HEADER( header, 200, nContentLen, contentType );


		printf("====================== HDL_SOCKET_Request Parsing ======================\n");
		printf("Method: %s, URI: %s\n", tMsg.method, tMsg.uri);
		
		write( socket, header, strlen( header ));

	}

	if( n == 0 )
	{
		printf("[DICONNECT] FD=%d closed\n", socket );
		return SOCKET_OK;
	}
	else if( n < 0 && nRetryCnt == RETRY_MAX_CNT )
	{
		printf("[HDL_SOCKET] Read Socket Fail\n", socket, errno, strerror(errno));
		return ERR_SOCKET_READ;
	}


	return SOCKET_OK;
}

void HDL_400( int socket )
{
	const char *msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
	write( socket, msg, strlen(msg) );
}


void HDL_500( int socket ) {
	const char *msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
	write( socket, msg, strlen(msg) );
}
