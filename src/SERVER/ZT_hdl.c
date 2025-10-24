#include <stdio.h>
#include <string.h>


/*=================================================
* name : HDL_HEADER
* return :
* param : 
===================================================*/

int HDL_HEADER( char *pheader, int nStatus, long llen, char *pType )
{
	if(pHeader == NULL || nStatus < 100 || llen <= 0 || pType == NULL ) return -1;
	
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
			snprintf(status_msg, MAX_STATUS_MSG_LEN, "Internal Server Error");
			break;
	}
	
	snprintf( pHeader, HEADER_FMT, nStatus, msg, llen, pType );
}


/*=================================================
* name : HDL_HEADER_MIME
* return :
* param : 
===================================================*/

int HDL_HEADER_MIME( char *pContentType, const char *pUri, size_t tContentLen )
{
	if( pContentType == NULL || pUri == NULL ) return -1;
	
	char *ext = strrchr( pUri, '.' );

	if( !strcmp( ext, ".html") )
	{
		strncpy( pContentType, "text/html", tContentLen );
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

	return 0;
}

/*=================================================
* name : HDL_SOCKET
* return :
* param :
===================================================*/
int HDL_SOCKET ( int socket )
{
	ReqType_t tMsg;
    
	/*
	char header[MAX_BUF_SIZE];
    char parse[MAX_BUF_SIZE], buf[MAX_BUF_SIZE];
    char *pTempUrl, *pLocalUrl;

    int cnt, nContentLen = 0;
    char contentType[MAX_CONTENT_TYPE_LEN];
	*/

    int fd = -1;
	
	if( socket < 0 )
	{
        printf("[HDL_SOCKET] Socket is Wrong\n");
        return ERR_SOCKET_ARG;
    }

	/* 요청 수신 */	
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
	snprintf( req.method, sizeof(req.method), "%s", method );
	snprintf( req.url, sizeof(req.uri), "%s", url );
	snprintf( req.version, sizeof(req.version), "%s", version );

    printf("====================== HDL_SOCKET_Request Parsing ======================\n");
	printf("Method: %s, URI: %s\n", req.method, req.uri);

	/* 요청 경로 설정 */
    snprintf( pTempUrl, sizeof(pTempUrl), "%s", url );
    if( !strcmp( pTempUrl, "/"))
    {
        snprintf( pTempUrl, sizeof(pTempUrl), "%s", "/indx.html" );
    }

	
    nContentLen = st.st_size;
    HDL_HEADER_MIME( contentType, localUrL );
    HDL_HEADER( header, 200, nContentLen, contentType );
    write( socket, header, strlen( header ));

    while(( cnt = read( fd, buf, BUF_XIZE)) > 0)
    {
        write( socket, buf, cnt );
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
