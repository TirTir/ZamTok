#include "ZT_Inc.h"
#include "../common/ZT_chat.h"


/*------------------------------------------------
 * Global Variable
-------------------------------------------------*/

extern ZT_CTX_t gt_ctx_info;
extern unsigned char g_client_fd[MAX_CLIENTS/8];

/*------------------------------------------------
 * Extern Function
-------------------------------------------------*/

void HDL_400 ( int socket );
void HDL_500 ( int socket );

static int HDL_Login( int socket, const char *buf, ReqType_t *t_msg );


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
	
	while (line)
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

	snprintf( p_header, HEADER_MAX_LEN, HEADER_FMT, 
			t_msg->version, status, status_msg, 
			t_msg->t_content_header[0].value, 
			t_msg->t_content_header[1].value );

	return SOCKET_OK;
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

	/* API 경로(/join, /login 등)는 확장자 없음 */
	if ( p_ext == NULL )
	{
		snprintf( p_content_type, size, "%s", "application/json" );
		return SOCKET_OK;
	}

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
 * name : HDL_Login
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : POST /login JSON body {"user_id":"xxx","password":"yyy"}
 ===================================================*/
static int HDL_Login( int socket, const char *buf, ReqType_t *t_msg )
{
	const char *p_body = NULL;
	user_t t_user = {0};

	char res_buf[BUF_MAX_LEN] = {0};
	char json_body[256] = {0};
	int rc;
	const char *p_val = NULL;

	if( socket < 0 || buf == NULL || t_msg == NULL )
		return ERR_ARG_INVALID;

	/* POST 메소드 확인 */
	if( strncmp( t_msg->method, "POST" ) )
	{
		snprintf( res_buf, sizeof(res_buf),
			"HTTP/1.1 405 Method Not Allowed\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 35\r\n\r\n"
			"{\"result\":\"fail\",\"reason\":\"POST only\"}" );
		rc = write( socket, res_buf, strlen(res_buf) );
		return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
	}

	/* Body 위치 찾기 (\r\n\r\n 이후) */
	p_body = strstr( buf, "\r\n\r\n" );
	if( p_body == NULL )
		p_body = strstr( buf, "\n\n" );
	if( p_body )
		p_body += ( strstr(buf,"\r\n\r\n") ? 4 : 2 );
	else
		p_body = buf;

	p_val = strstr( p_body, "\"user_id\"" );
	if( p_val )
	{
		p_val = strchr( p_val + 9, '"' );
		if( p_val )
		{
			p_val++;
			sscanf( p_val, "%15[^\"]", t_user.str_user_id );  /* USER_ID_MAX_LEN-1 */
		}
	}
	p_val = strstr( p_body, "\"password\"" );
	if( p_val )
	{
		p_val = strchr( p_val + 11, '"' );
		if( p_val )
		{
			p_val++;
			sscanf( p_val, "%15[^\"]", t_user.str_pwd );  /* PASSWORD_MAX_LEN-1 */
		}
	}

	/* TODO: ZT_DB_QUERY 등으로 실제 DB 검증 */
	/* 현재는 user_id, password 비어있으면 실패 */
	if( t_user.str_user_id[0] == '\0' || t_user.str_pwd[0] == '\0' )
	{
		snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"invalid params\"}" );
	}
	else
	{
		snprintf( json_body, sizeof(json_body), "{\"result\":\"ok\",\"user_id\":\"%s\"}", t_user.str_user_id );
	}

	snprintf( res_buf, sizeof(res_buf),
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: %d\r\n\r\n%s",
		(int)strlen(json_body), json_body );

	rc = write( socket, res_buf, strlen(res_buf) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
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
		fflush(stdout);

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

		/* API 라우팅: /login */
		if( !strcmp( t_msg.uri, "/login" ) )
		{
			rc = HDL_Login( socket, buf, &t_msg );
			if( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

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

		/* 클라이언트는 HDL_ACCEPT에서 이미 CTX에 등록됨 */
		break;
	}

	if( n == 0 )
	{
		printf("[DICONNECT] FD=%d closed\n", socket );
		return SOCKET_OK;
	}
	
    return SOCKET_OK;
}

int HDL_ACCEPT( int socket, int epfd )
{
	struct sockaddr_in t_client_addr = {0};
    socklen_t un_socket_len = sizeof(t_client_addr);
	struct epoll_event tEv;

    int rc = 0;
    int client_fd = -1;
	
    client_fd = accept( socket, (struct sockaddr *)&t_client_addr, &un_socket_len );
	if( client_fd < 0 )
	{
		printf("[HDL_ACCEPT] Socket Accept Fail\n");
		return ERR_SOCKET_ACCEPT;
	}

	printf("[HDL_ACCEPT] Client connected FD=%d from %s:%d\n",
		client_fd, inet_ntoa(t_client_addr.sin_addr), ntohs(t_client_addr.sin_port));

    rc = CTX_Http_Insert( &gt_ctx_info, client_fd, t_client_addr ); 
    if( rc < 0 )
    {
        printf("[HDL_ACCEPT] Insert Client Info Fail\n");
        close(client_fd);
        return ERR_CTX_INSERT;
    }

	/* 클라이언트 소켓을 epoll에 등록 (데이터 수신 이벤트 수신 가능) */
	SET_NONBLOCKING(client_fd);
	tEv.events = EPOLLIN;
	tEv.data.fd = client_fd;
	rc = epoll_ctl( epfd, EPOLL_CTL_ADD, client_fd, &tEv );
	if( rc < 0 )
	{
		printf("[HDL_ACCEPT] epoll_ctl Add Fail <%d:%s>\n", errno, strerror(errno));
		close(client_fd);
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
		printf("[HDL_400] write fail\n");
	}
}

void HDL_500( int socket ) 
{
	int rc = 0;

	const char *msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
	rc = write( socket, msg, strlen(msg) );
	if( rc < 0 )
	{
		printf("[HDL_500] write fail\n");
	}

}
