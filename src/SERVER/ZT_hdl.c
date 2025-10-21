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

void HDL_404( )
{

}
