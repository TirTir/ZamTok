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

    struct stat st;

    if( socket < 0 )
	{
        printf("[HDL_SOCKET] Socket is Wrong\n");
        return ERR_SOCKET_ARG;
    }

	
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

void HDL_404( )
{

}
