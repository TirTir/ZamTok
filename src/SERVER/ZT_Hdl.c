#include "ZT_hdl.h"

/*------------------------------------------------
 * Global Variable
-------------------------------------------------*/

extern ZT_CTX_t gt_ctx_info;
extern unsigned char g_client_fd[MAX_CLIENTS/8];

/*=================================================
 * name : hdl_send_http
 * return :
 * param : socket, ResType_t *t_response
 ===================================================*/
static int hdl_send_http( int socket, st_res_t *t_response )
{
	char wire[HEADER_MAX_LEN + DATA_MAX_LEN + 128];
	size_t blen;
	int n, rc;

	if ( socket < 0 || t_response == NULL )
		return ERR_ARG_INVALID;

	blen = strlen( t_response->t_result.data );
	n = snprintf( wire, sizeof( wire ),
		"HTTP/1.1 %u %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s",
		t_response->ui_status_code,
		t_response->reason,
		blen,
		t_response->t_result.data );
	if ( n < 0 || (size_t)n >= sizeof( wire ) )
		return ERR_ARG_INVALID;

	rc = socket_send( socket, wire, n );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
}

/*=================================================
 * name : hdl_header
 * return : 
 * param : *pheader, nStatus, llen, *pType
 ===================================================*/
int hdl_header( char *p_header, char *p_buf, int status, st_req_t *t_msg )
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

int hdl_header_mime( char *p_content_type, int size, const char *p_uri )
{
	if( p_content_type == NULL )
	{
		LOG_MSG("[HDL_HEADER_MIME] Content Type is NULL\n");
		return ERR_ARG_INVALID;
	}
	
	if( p_uri == NULL )
	{
		LOG_MSG("[HDL_HEADER_MIME] URI is NULL\n");
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
 * name : HDL_SOCKET
 * return :
 * param :
 ===================================================*/
int hdl_socket( int epfd, int socket )
{
	st_req_t t_request = {0};

	char parse[BUF_MAX_LEN] = {0};
	char buf[BUF_MAX_LEN] = {0};
	
	int n, rc, status;
	int retry_cnt = 0;

	if( epfd < 0 || socket < 0 )
	{
		LOG_ERR("[HDL_SOCKET] Epoll FD or Socket FD is Wrong <%d:%s>\n", errno, strerror(errno));
		return ERR_ARG_INVALID;
	}

	while( retry_cnt < RETRY_MAX_CNT )
	{
		char header[HEADER_MAX_LEN] = "";

		errno = 0;
		n = read( socket, buf, sizeof(buf) ); 
		
		if( n < 0 && ( errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR ) )
		{
			LOG_MSG("[HDL_SOCKET] Read Socket Fail FD=%d <%d:%s>\n", socket, errno, strerror(errno));
			retry_cnt++;
			continue;
		}

		if ( n <= 0 )
		{
			/* 클라이언트 연결 종료 또는 read 오류 → 해당 클라이언트만 정리 */
			if ( n == 0 )
				LOG_MSG("[HDL_SOCKET] FD=%d closed (client disconnect)\n", socket);
			else
				LOG_MSG("[HDL_SOCKET] Read error FD=%d <%d:%s>\n", socket, errno, strerror(errno));
			return SOCKET_CLIENT_DISCONNECT;
		}

		buf[n] = '\0';

		LOG_FMT_CENTER("HDL_SOCKET_Request");
		LOG_MSG("%s\n", buf);
		fflush(stdout);

		snprintf( t_request.body, sizeof( t_request.body ), "%s", buf );
		snprintf( parse, sizeof( parse ), "%s", buf );

		char *method = strtok( parse, " " );
		char *uri = strtok( NULL, " " );
		char *version = strtok( NULL, "\r\n" );

		if( !method || !uri || !version )
		{
			LOG_MSG("[HDL_SOCKET] Invalid Request Line FD=%d\n", socket);
			HDL_400( socket );
			return SOCKET_CLIENT_DISCONNECT;
		}

		/* Start Line Parsing */
		snprintf( t_request.method, sizeof( t_request.method ), "%s", method );
		snprintf( t_request.uri, sizeof( t_request.uri ), "%s", uri );
		snprintf( t_request.version, sizeof( t_request.version ), "%s", version );

		/* API 라우팅: /join → Redis user:{user_id} HASH 저장 */
		if ( !strcmp( t_request.uri, "/join" ) )
		{
			rc = hdl_join( socket, buf, &t_request );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: /login → Redis user:{user_id}에서 pwd 조회 후 검증 */
		if ( !strcmp( t_request.uri, "/login" ) )
		{
			rc = hdl_login( socket, buf, &t_request );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: POST /room → 채팅방 생성 */
		if ( !strcmp( t_request.uri, "/room" ) && !strncmp( t_request.method, "POST", 4 ) )
		{
			rc = hdl_create_room( socket, buf, &t_request );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: GET /room?id=... → room_id로 채팅방 검색 */
		if ( !strncmp( t_request.uri, "/room?id=", 9 ) && !strncmp( t_request.method, "GET", 3 ) )
		{
			rc = hdl_search_room( socket, buf, &t_request );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: POST /room/join → room_id + pw로 입장 검증 */
		if ( !strcmp( t_request.uri, "/room/join" ) && !strncmp( t_request.method, "POST", 4 ) )
		{
			rc = hdl_join_room( socket, buf, &t_request );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: GET /rooms → 채팅방 목록 조회 */
		if ( !strcmp( t_request.uri, "/rooms" ) && !strncmp( t_request.method, "GET", 3 ) )
		{
			rc = hdl_list_rooms( socket, buf, &t_request );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		if( !strcmp( t_request.uri, "/"))
			snprintf( t_request.uri, sizeof( t_request.uri ), "%s", "/indx.html" );

		rc = hdl_header_mime( t_request.t_content_header[t_request.content_cnt].value, VALUE_MAX_LEN, t_request.uri );
		if( rc == SOCKET_OK )
        {
            status = 200;
        }
        else
		{
            status = 400;
		}

        rc = hdl_header( header, buf, status, &t_request );
		if( rc < 0 )
		{
			LOG_ERR("HDL_HEADER fail\n");
	        return ERR_SOCKET_READ;
		}

		LOG_FMT_CENTER("HDL_SOCKET_Request Parsing");
		LOG_MSG("Method: %s, URI: %s\n", t_request.method, t_request.uri);

		/* 클라이언트는 HDL_ACCEPT에서 이미 CTX에 등록됨 */
		break;
	}

	return SOCKET_OK;
}

int hdl_socket_accept( int socket, int epfd )
{
	struct sockaddr_in t_client_addr = {0};
    socklen_t un_socket_len = sizeof(t_client_addr);
	struct epoll_event tEv;

    int rc = 0;
    int client_fd = -1;
	
    client_fd = accept( socket, (struct sockaddr *)&t_client_addr, &un_socket_len );
	if( client_fd < 0 )
	{
		LOG_MSG("[HDL_ACCEPT] Socket Accept Fail\n");
		return ERR_SOCKET_ACCEPT;
	}

	LOG_MSG("[HDL_ACCEPT] Client connected FD=%d from %s:%d\n",
		client_fd, inet_ntoa(t_client_addr.sin_addr), ntohs(t_client_addr.sin_port));

    rc = ctx_http_insert( &gt_ctx_info, client_fd, t_client_addr ); 
    if( rc < 0 )
    {
        LOG_MSG("[HDL_ACCEPT] Insert Client Info Fail\n");
        close(client_fd);
        return ERR_CTX_INSERT;
    }

	/* 클라이언트 소켓을 epoll에 등록 (데이터 수신 이벤트 수신 가능) */
	socket_set_nonblocking(client_fd);
	tEv.events = EPOLLIN;
	tEv.data.fd = client_fd;
	rc = epoll_ctl( epfd, EPOLL_CTL_ADD, client_fd, &tEv );
	if( rc < 0 )
	{
		LOG_MSG("[HDL_ACCEPT] epoll_ctl Add Fail <%d:%s>\n", errno, strerror(errno));
		close(client_fd);
		return ERR_CTX_INSERT;
	}
	
	/* FD pool에 클라이언트 등록 */
	g_client_fd[client_fd/8] |= ( 1 << ( client_fd % 8 ));

	return SOCKET_OK;
}