#include "ZT_Inc.h"

extern ZT_CTX_t gt_ctx_info;
extern unsigned char g_client_fd[MAX_CLIENTS/8];

/*=================================================
 * name : HDL_HEADER
 * return : 
 * param : *pheader, nStatus, llen, *pType
 ===================================================*/

int HDL_HEADER( char *p_header, char *p_buf, int status, ReqType_t *t_msg )
{
	if( !p_header || !p_buf || status < 100 )
		return ERR_ARG_INVALID;

	char status_msg[MAX_STATUS_MSG_LEN] = "";
	char *temp = NULL;
	
	char *line = strtok( p_buf, "\r\n\r\n" );
	
	t_msg->content_cnt = 0;
	
	while(line)
	{
		if(!strncmp( line, "Content-Type:", 13 ))
		{
			temp = strtok(line + 13, " ");
            snprintf( t_msg->t_content_header[t_msg->content_cnt].name, NAME_MAX_LEN, "%s", "Content-Type");
            snprintf( t_msg->t_content_header[t_msg->content_cnt].value, NAME_MAX_LEN, "%s", temp);
		}
		else if(!strncmp( line, "Content-Length:", 15 ))
		{
			temp = strtok(line + 15, " ");
            snprintf( t_msg->t_content_header[t_msg->content_cnt].name, NAME_MAX_LEN, "%s", "Content-Length");
            snprintf( t_msg->t_content_header[t_msg->content_cnt].value, NAME_MAX_LEN, "%s", temp);
		}

		line = strtok( NULL, "\r\n" );	/* 한 줄씩 자르기 */
	}

	switch (status)
	{
		case 200:
			snprintf(status_msg, MAX_STATUS_MSG_LEN, "OK");
			break;

		case 400:
			snprintf(status_msg, MAX_STATUS_MSG_LEN, "Not Found");
			break;

		case 500:
		default:
			snprintf(status_msg, MAX_STATUS_MSG_LEN, "Internal Server Error");
			break;
	}

	snprintf( p_header, HEADER_FMT, t_msg->version, status, status_msg, t_msg->t_content_header[0].value, t_msg->t_content_header[1].value);
}


/*=================================================
 * name : HDL_HEADER_MIME
 * return :
 * param : *pContentType, nSize, *pUri 
 ===================================================*/

int HDL_HEADER_MIME( char *p_content_type, int size, const char *p_uri )
{
	if( p_content_type == NULL )
	{
		printf("[HDL_HEADER_MIME] Content Type is NULL\n");
		return ERR_ARG_INVALID;
	}
	
	if( p_uri == NULL )
	{
		printf("[HDL_HEADER_MIME] URI is NULL\n");
		return ERR_ARG_INVALID;
	}

	char *p_ext = strrchr( p_uri, '.' );

	if( !strcmp( p_ext, ".html") )
	{
		snprintf( p_content_type, size, "%s", "text/html" );
	}
	else if( !strcmp( p_ext, ".jpg") || !strcmp( p_ext, ".jpeg" ))
	{
		snprintf( p_content_type, size, "%s", "image/jpeg" );
	}
	else if( !strcmp( p_ext, ".png") )
	{
		snprintf( p_content_type, size, "%s", "image/png" );
	}
	else if( !strcmp( p_ext, ".CSS") )
	{
		snprintf( p_content_type, size, "%s", "text/CSS" );
	}
	else if( !strcmp( p_ext, ".js") )
	{
		snprintf( p_content_type, size, "%s", "text/javascript" );
	}
	else 
		snprintf( p_content_type, size, "%s", "text/plain" );

	return SOCKET_OK;
}

/*=================================================
 * name : HDL_SOCKET
 * return :
 * param :
 ===================================================*/
int HDL_SOCKET ( int epfd, int socket )
{
	ReqType_t t_msg = {0};
    //memset( &tMsg, 0x00, sizeof(tMsg) );
	
    char parse[BUF_MAX_LEN] = {0};
	char buf[BUF_MAX_LEN] = {0};
	
	int n, rc, status;
	int retry_cnt = 0;

	if( epfd < 0 )
	{
		printf("[HDL_SOCKET] Epoll FD is Wrong\n");
		return ERR_ARG_INVALID;
	}

	if( socket < 0 )
	{
		printf("[HDL_SOCKET] Socket FD is Wrong\n");
		return ERR_ARG_INVALID;
	}

	while( retry_cnt < RETRY_MAX_CNT )
	{
		char header[HEADER_MAX_LEN] = "";

		errno = 0;
		n = read( socket, buf, sizeof(buf) ); 
		
		if( n < 0 && ( errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR ) )
		{
			printf("[HDL_SOCKET] Read Socket Fail FD=%d <%d:%s>\n", socket, errno, strerror(errno));
			retry_cnt++;
			continue;
		}

		buf[n] = '\0';

		printf("====================== HDL_SOCKET_Request ======================\n");
		printf("%s\n", buf);

		snprintf( parse, sizeof(parse), "%s", buf );

		char *method = strtok( parse, " " );
		char *uri = strtok( NULL, " " );
		char *version = strtok( NULL, "\r\n" );

		if( !method || !uri || !version )
		{
			printf("[HDL_SOCKET] Invalid Request Line\n");
			HDL_400( socket );
			return ERR_ARG_INVALID;
		}

		/* Start Line Parsing */
		snprintf( t_msg.method, sizeof(t_msg.method), "%s", method );
		snprintf( t_msg.uri, sizeof(t_msg.uri), "%s", uri );
		snprintf( t_msg.version, sizeof(t_msg.version), "%s", version );


		if( !strcmp( t_msg.uri, "/"))
			snprintf( t_msg.uri, sizeof(t_msg.uri), "%s", "/indx.html" );

		rc = HDL_HEADER_MIME( t_msg.t_content_header[t_msg.content_cnt].value, VALUE_MAX_LEN, t_msg.uri );
		if( rc == SOCKET_OK )
        {
            status = 200;
        }
        else
		{
            status = 400;
		}

        rc = HDL_HEADER( header, buf, status, &t_msg );
		if( rc < 0 )
		{
			LOG_ERR("HDL_HEADER fail\n");
	        return ERR_SOCKET_READ;
		}

		printf("====================== HDL_SOCKET_Request Parsing ======================\n");
		printf("Method: %s, URI: %s\n", t_msg.method, t_msg.uri);
		
		//write( socket, t_msg.t_general_header, strlen( t_msg.t_general_header ));

	}

	if( n == 0 )
	{
		printf("[DICONNECT] FD=%d closed\n", socket );
		return SOCKET_OK;
	}
	
    return SOCKET_OK;
}

int HDL_ACCEPT( int socket )
{
	struct sockaddr_in t_client_addr = {0};
    socklen_t un_socket_len = sizeof(t_client_addr);

    int rc = 0;
    int client_fd = -1;
	
    client_fd = accept( socket, (struct sockaddr *)&t_client_addr, &un_socket_len );
	if( client_fd < 0 )
	{
		printf("[HDL_ACCEPT] Socket Accept Fail\n");
		return ERR_SOCKET_ACCEPT;
	}

    rc = CTX_Http_Insert( &gt_ctx_info, client_fd, t_client_addr, un_socket_len ); 
    if( rc < 0 )
    {
        printf("[HDL_ACCEPT] Insert Client Info Fail\n");
        return ERR_CTX_INSERT;
    }
	
	/* FD pool에 클라이언트 등록 */
	g_client_fd[client_fd/8] |= ( 1 << ( client_fd % 8 ));


	return SOCKET_OK;
}

void HDL_400( int socket )
{
	int rc = 0;

	const char *msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
	rc = write( socket, msg, strlen(msg) );
	if( rc < 0 )
	{
		LOG_ERR("write fail\n");
	}
}

void HDL_500( int socket ) 
{
	int rc = 0;

	const char *msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
	rc = write( socket, msg, strlen(msg) );
	if( rc < 0 )
	{
		LOG_ERR("write fail\n");
	}

}
