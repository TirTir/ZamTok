#include "ZT_Inc.h"

extern HttpCTX_t gtCTXInfo;

/*=================================================
 * name : HDL_HEADER
 * return : 
 * param : *pheader, nStatus, llen, *pType
 ===================================================*/

void HDL_HEADER( char *pHeader, int nStatus, long llen, char *pType )
{
	if( pHeader == NULL )
    {
        printf("[HDL_HEADER] Header is NULL\n");
        return ERR_SOCKET_ARG;
    }        
    
    if( pType == NULL )
    {
        printf("[HDL_HEADER] Type is NULL\n");
        return ERR_SOCKET_ARG;
    }

    if( nStatus < 100 || llen <= 0 )
        return ERR_SOCKET_ARG;

	char statusMsg[MAX_STATUS_MSG_LEN];

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

	// snprintf( pHeader, HEADER_FMT, nStatus, msg, llen, pType );
}


/*=================================================
 * name : HDL_HEADER_MIME
 * return :
 * param : *pContentType, nSize, "%s", *pUri, sizeof(pContentType) 
 ===================================================*/

int HDL_HEADER_MIME( char *pContentType, size_t nSize, const char *pUri )
{
	if( pContentType == NULL )
	{
		printf("[HDL_HEADER_MIME] Content Type is NULL\n");
		return ERR_SOCKET_ARG;
	}
	
	if( pUri == NULL )
	{
		printf("[HDL_HEADER_MIME] URI is NULL\n");
		return ERR_SOCKET_ARG;
	}

	char *ext = strrchr( pUri, '.' );

	if( !strcmp( ext, ".html") )
	{
		snprintf( pContentType, nSize, "%s", "text/html" );
	}
	else if( !strcmp( ext, ".jpg") || !strcmp( ext, ".jpeg" ))
	{
		snprintf( pContentType, nSize, "%s", "image/jpeg" );
	}
	else if( !strcmp( ext, ".png") )
	{
		snprintf( pContentType, nSize, "%s", "image/png" );
	}
	else if( !strcmp( ext, ".CSS") )
	{
		snprintf( pContentType, nSize, "%s", "text/CSS" );
	}
	else if( !strcmp( ext, ".js") )
	{
		snprintf( pContentType, nSize, "%s", "text/javascript" );
	}
	else 
		snprintf( pContentType, nSize, "%s", "text/plain" );

	return SOCKET_OK;
}

/*=================================================
 * name : HDL_SOCKET
 * return :
 * param :
 ===================================================*/
int HDL_SOCKET ( int epfd, int socket )
{
	ReqType_t tMsg = {0};
    //memset( &tMsg, 0x00, sizeof(tMsg) );
	
    char parse[BUF_MAX_LEN] = {0};
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
		
		if( n < 0 && ( errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR ) )
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
			return ERR_SOCKET_ARG;
		}

		/* Data Parsing */
		snprintf( tMsg.method, sizeof(tMsg.method), "%s", method );
		snprintf( tMsg.uri, sizeof(tMsg.uri), "%s", url );
		snprintf( tMsg.version, sizeof(tMsg.version), "%s", version );


		/* 요청 경로 설정 */
		if( !strcmp( tMsg.uri, "/"))
			snprintf( tMsg.uri, sizeof(tMsg.uri), "%s", "/indx.html" );

		rc = HDL_HEADER_MIME( &tMsg.tContentHeader[tReq.nContentCnt].value, VALUE_MAX_LEN, tMsg.Uri );
		if( rc == SOCKET_OK )
        {
            snprintf( tMsg.tContentHeader[tReq.nContentCnt].name, NAME_MAX_LEN, "%s", "Content-Type");
            nStatus = 200;
        }
        else
            nStatus = 400;
     
        HDL_HEADER( header, 200, nContentLen, tMsg.tContentHeader[tReq.nContentCnt].value );


		printf("====================== HDL_SOCKET_Request Parsing ======================\n");
		printf("Method: %s, URI: %s\n", tMsg.method, tMsg.uri);
		
		write( socket, tMsg.tGeneralHeader, strlen( tMsg.tGeneralHeader ));

	}

	if( n == 0 )
	{
		printf("[DICONNECT] FD=%d closed\n", socket );
		return SOCKET_OK;
	}
	
    return SOCKET_OK;
}

int HDL_ACCEPT( epfd, socket )
{
	struct sockaddr_in tClientAddr = {0};
    socklen_t unSocketLen;

    int rc = 0;
    int nClientFD = -1;
	
    nClientFD = accept( socket, &tClientAddr, &unSocketLen );
	if( nClientFD < 0 )
	{
		printf("[HDL_ACCEPT] Socket Accept Fail\n");
		return ERR_SOCKET_ACCEPT;
	}

    rc = CTX_Http_Insert( &gtCTXInfo, nClientFD, tClientAddr, unSocketLen ); 
    if( rc < 0 )
    {
        printf("[HDL_ACCEPT] Insert Client Info Fail\n");
        return ERR_INSERT_CTX;
    }
}

void HDL_400( int socket )
{
	const char *msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
	write( socket, msg, strlen(msg) );
}

void HDL_500( int socket ) 
{
	const char *msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
	write( socket, msg, strlen(msg) );
}
