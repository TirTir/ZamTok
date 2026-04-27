#include "ZT_Inc.h"
#include "ZT_chat.h"
#include "ZT_redis.h"
#include "ZT_log.h"
#include "ZT_log_fmt.h"

/*------------------------------------------------
 * Global Variable
-------------------------------------------------*/

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
 * name : HDL_Join
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : POST /join JSON body {"user_id":"xxx","name":"yyy","password":"zzz"}
 *         Redis user:{user_id} HASH에 name, pwd, created_at 저장
 ===================================================*/
 #define NAME_KEY "\"name\""
static int HDL_Join( int socket, const char *buf, ReqType_t *t_msg )
{
	user_t t_user = {0};
	ResType_t t_response = {0};

	int rc = 0;
	int status = 0;
	const char *p_body = NULL;
	ZtErrCode e_code = ZT_ERR_INVALID_PARAMS;

	if ( socket < 0 || buf == NULL || t_msg == NULL )
		return ERR_ARG_INVALID;

	/* 1. Method Check */
	if ( strncmp( t_request->method, "POST", 4 ) )
	{
		status = 405;
		e_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto err_return;
	}

	p_body = strstr( buf, "\r\n\r\n" );
	if ( p_body == NULL )
		p_body = strstr( buf, "\n\n" );
	if ( p_body )
		p_body += ( strstr( buf, "\r\n\r\n" ) ? 4 : 2 );
	else
		p_body = buf;

	/* 2. Body Parsing */
	Get_Value_From_Body( p_body, USER_ID_KEY, &t_user.str_user_id, &len );
	Get_Value_From_Body( p_body, NAME_KEY, &t_user.str_name, &len );
	Get_Value_From_Body( p_body, PASSWORD_KEY, &t_user.str_pwd, &len );

	/* 3. User Check */
	if ( t_user.str_user_id[0] == '\0' || t_user.str_name[0] == '\0' || t_user.str_pwd[0] == '\0' )
	{
		status = 400;
		e_code = ZT_ERR_INVALID_PARAMS;
		goto err_return;
	}

	/* 4. User Save to Redis */
	rc = ZT_REDIS_UserSave( &t_user );
	if ( rc == -1 )
	{
		status = 409;
		e_code = ZT_ERR_USER_ALREADY_EXISTS;
		goto err_return;
	}
	else if ( rc != 0 )
	{
		status = 500;
		e_code = ZT_ERR_SERVER_ERROR;
		goto err_return;
	}

	/* 5. Response */
	t_response = Generate_Response(t_request, 200);
	Generate_Response_Body( &t_response, RESULT_SUCCESS, e_code, t_user.str_user_id );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );	
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;

err_return:
	t_response = Generate_Response(t_request, status);
	Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
}

/*=================================================
 * name : HDL_Login
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_request
 * desc  : POST /login JSON body {"user_id":"xxx","password":"yyy"}
 *         Redis user:{user_id}에서 pwd 조회 후 비교
 ===================================================*/
 #define USER_ID_KEY "\"user_id\""
 #define PASSWORD_KEY "\"password\""
static int HDL_Login( int socket, const char *buf, ReqType_t *t_request )
{
	user_t t_user = {0};
	ResType_t t_response = {0};

	int rc = 0;
	int status = 0;
	const char *p_body = NULL;
	ZtErrCode e_code = ZT_ERR_INVALID_PARAMS;

	if ( socket < 0 || buf == NULL || t_request == NULL )
		return ERR_ARG_INVALID;

	/* 1. Method Check */
	if ( strncmp( t_request->method, "POST", 4 ) )
	{
		status = 405;
		e_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto err_return;
	}

	p_body = strstr( buf, "\r\n\r\n" );
	if ( p_body == NULL )
		p_body = strstr( buf, "\n\n" );
	if ( p_body )
		p_body += ( strstr( buf, "\r\n\r\n" ) ? 4 : 2 );
	else
		p_body = buf;

	/* 2. Body Parsing */
	Get_Value_From_Body( p_body, USER_ID_KEY, &t_user.str_user_id, &len );
	Get_Value_From_Body( p_body, PASSWORD_KEY, &t_user.str_pwd, &len );

	/* 3. User Check */
	if ( t_user.str_user_id[0] == '\0' || t_user.str_pwd[0] == '\0' )
	{
		status = 400;
		e_code = ZT_ERR_INVALID_PARAMS;
		goto err_return;
	}

	/* 4. User Get From Redis */
	user_t stored_user = {0};
	rc = ZT_REDIS_UserGet( t_user.str_user_id, &stored_user );
	if ( rc == -1 )
	{
		status = 401;
		e_code = ZT_ERR_USER_NOT_FOUND;
		goto err_return;
	}
	else if ( rc != 0 )
	{
		status = 500;
		e_code = ZT_ERR_SERVER_ERROR;
		goto err_return;
	}
	else if ( strcmp( stored_user.str_pwd, t_user.str_pwd ) != 0 )
	{
		status = 401;
		e_code = ZT_ERR_WRONG_PASSWORD;
		goto err_return;
	}

	/* 3. Response */
	t_response = Generate_Response(t_request, 200);
	Generate_Response_Body( &t_response, RESULT_SUCCESS, e_code, NULL );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );	
		return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;

err_return:
	t_response = Generate_Response(t_request, status);
	Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
}

/*=================================================
 * name : HDL_CreateRoom
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : POST /room JSON body {"room_id":"xxx","password":"yyy"}
 *         Redis room:{room_id} HASH에 pwd, created_at, creator_id 저장
 ===================================================*/
 #define ROOM_ID_KEY "\"room_id\""
static int HDL_CreateRoom( int socket, const char *buf, ReqType_t *t_request )
{
	room_t t_room = {0};
	ResType_t t_response = {0};

	int rc = 0;
	int status = 0;
	const char *p_body = NULL;
	ZtErrCode e_code = ZT_ERR_INVALID_PARAMS;

	if ( socket < 0 || buf == NULL || t_request == NULL )
		return ERR_ARG_INVALID;

	/* 1. Method Check */
	if ( strncmp( t_request->method, "POST", 4 ) )
	{
		status = 405;
		e_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto err_return;
	}

	p_body = strstr( buf, "\r\n\r\n" );
	if ( p_body == NULL )
		p_body = strstr( buf, "\n\n" );
	if ( p_body )
		p_body += ( strstr( buf, "\r\n\r\n" ) ? 4 : 2 );
	else
		p_body = buf;

	/* 2. Body Parsing */
	Get_Value_From_Body( p_body, ROOM_ID_KEY, &t_room.str_room_id, &len );
	Get_Value_From_Body( p_body, PASSWORD_KEY, &t_room.str_pwd, &len );
	Get_Value_From_Body( p_body, USER_ID_KEY, &t_room.str_creator_id, &len );

	/* 3. Room Check */
	if ( t_room.str_room_id[0] == '\0' || t_room.str_pwd[0] == '\0' || t_room.str_creator_id[0] == '\0' )
	{
		status = 400;
		e_code = ZT_ERR_INVALID_PARAMS;
		goto err_return;
	}

	/* 4. Room Save to Redis */
	rc = ZT_REDIS_RoomSave( &t_room );
	if ( rc == -1 )
	{
		status = 409;
		e_code = ZT_ERR_ROOM_ALREADY_EXISTS;
		goto err_return;
	}
	else if ( rc != 0 )
	{
		status = 500;
		e_code = ZT_ERR_SERVER_ERROR;
		goto err_return;
	}

	/* 5. Response */
	t_response = Generate_Response(t_request, 200);
	Generate_Response_Body( &t_response, RESULT_SUCCESS, e_code, t_room.str_room_id );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;

err_return:
	t_response = Generate_Response(t_request, status);
	Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
}

/*=================================================
 * name : HDL_SearchRoom
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : GET /room?id={room_id}
 *         room_id로 채팅방 존재 여부 조회, 있으면 room_id 반환
 ===================================================*/
static int HDL_SearchRoom( ReqType_t *t_request )
{
	ResType_t t_response = {0};

	int rc = 0;
	int status = 0;
	const char *p_body = NULL;
	ZtErrCode e_code = ZT_ERR_INVALID_PARAMS;

	if ( socket < 0 || buf == NULL || t_request == NULL )
		return ERR_ARG_INVALID;

	/* 1. Method Check */
	if ( strncmp( t_request->method, "GET", 3 ) )
	{
		status = 405;
		e_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto err_return;
	}

	/* URI에서 ?id= 파싱: /room?id=myroom */
	p_val = strstr( t_request->uri, "?id=" );
	if ( p_val )
		Get_Value_From_Body( p_body, ROOM_ID_KEY, &t_room.str_room_id, &len );

	/* 3. Room Check */
	if ( t_room.str_room_id[0] == '\0' )
	{
		status = 400;
		e_code = ZT_ERR_INVALID_PARAMS;
		goto err_return;
	}

	/* 4. Room Get From Redis */
	rc = ZT_REDIS_RoomGet( t_room.str_room_id, &t_room );
	if ( rc == -1 )
	{
		status = 404;
		e_code = ZT_ERR_ROOM_NOT_FOUND;
		goto err_return;
	}
	else if ( rc != 0 )
	{
		status = 500;
		e_code = ZT_ERR_SERVER_ERROR;
		goto err_return;
	}	

	/* 5. Response */
	t_response = Generate_Response(t_request, 200);
	Generate_Response_Body( &t_response, RESULT_SUCCESS, e_code, t_room.str_room_id );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;

err_return:
	t_response = Generate_Response(t_request, status);
	Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
}

/*=================================================
 * name : HDL_JoinRoom
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_request
 * desc  : POST /room/join JSON body {"room_id":"xxx","password":"yyy"}
 *         room_id로 채팅방 조회 후 pw 검증, 일치하면 입장 허가
 ===================================================*/
 static int HDL_JoinRoom( int socket, const char *buf, ReqType_t *t_request )
{
	user_t t_user = {0};
	ResType_t t_response = {0};

	int rc = 0;
	int status = 0;
	const char *p_body = NULL;
	ZtErrCode e_code = ZT_ERR_INVALID_PARAMS;

	if ( socket < 0 || buf == NULL || t_request == NULL )
		return ERR_ARG_INVALID;

	/* 1. Method Check */
	if ( strncmp( t_request->method, "POST", 4 ) )
	{
		status = 405;
		e_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto err_return;
	}

	p_body = strstr( buf, "\r\n\r\n" );
	if ( p_body == NULL )
		p_body = strstr( buf, "\n\n" );
	if ( p_body )
		p_body += ( strstr( buf, "\r\n\r\n" ) ? 4 : 2 );
	else
		p_body = buf;

	/* 2. Body Parsing */
	Get_Value_From_Body( p_body, ROOM_ID_KEY, &t_room.str_room_id, &len );
	Get_Value_From_Body( p_body, PASSWORD_KEY, &t_room.str_pwd, &len );

	/* 3. Room Check */
	if ( t_room.str_room_id[0] == '\0' || t_room.str_pwd[0] == '\0' )
	{
		status = 400;
		e_code = ZT_ERR_INVALID_PARAMS;	
		goto err_return;
	}

	/* 4. Room Get From Redis */
	rc = ZT_REDIS_RoomGet( t_room.str_room_id, &stored_room );
	if ( rc == -1 )
	{
		status = 404;
		e_code = ZT_ERR_ROOM_NOT_FOUND;
		goto err_return;
	}
	else if ( rc != 0 )
	{
		status = 500;
		e_code = ZT_ERR_SERVER_ERROR;
		goto err_return;
	}
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;

err_return:
	t_response = Generate_Response(t_request, status);
	Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
}

/*=================================================
 * name : HDL_ListRooms
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_request
 * desc  : GET /rooms
 *         Redis에서 room:* 키를 스캔해서 room_id, creator_id(user_id) 리스트 반환
 *         {"result":"ok","rooms":[{"room_id":"r1","user_id":"u1"},...]}
 * ===================================================*/
static int HDL_ListRooms( int socket, const char *buf, ReqType_t *t_request )
{
	room_t t_room = {0};
	ResType_t t_response = {0};

	int rc = 0;
	int status = 0;
	const char *p_body = NULL;
	ZtErrCode e_code = ZT_ERR_INVALID_PARAMS;
	(void)buf;

	if ( socket < 0 || buf == NULL || t_request == NULL )
		return ERR_ARG_INVALID;

	if ( strncmp( t_request->method, "GET", 3 ) )
	{
		status = 405;
		e_code = ZT_ERR_METHOD_NOT_ALLOWED;
		goto err_return;
	}

	p_body = strstr( buf, "\r\n\r\n" );
	if ( p_body == NULL )
		p_body = strstr( buf, "\n\n" );
	if ( p_body )
		p_body += ( strstr( buf, "\r\n\r\n" ) ? 4 : 2 );
	else
		p_body = buf;

	rc = ZT_REDIS_RoomList( t_rooms, (size_t)(sizeof(t_rooms)/sizeof(t_rooms[0])) );
	if ( rc < 0 )
	{
		status = 500;
		e_code = ZT_ERR_SERVER_ERROR;
		goto err_return;
	}
	else if ( rc != 0 )
	{
		status = 500;
		e_code = ZT_ERR_SERVER_ERROR;
		goto err_return;
	}

	/* 5. Response */
	t_response = Generate_Response(t_request, 200);
	Generate_Response_Body( &t_response, RESULT_SUCCESS, e_code, t_rooms );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;

err_return:
	t_response = Generate_Response(t_request, status);
	Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
	rc = write( socket, t_response.t_response, strlen(t_response.t_response) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
}

/*=================================================
 * name : HDL_SOCKET
 * return :
 * param :
 ===================================================*/
int HDL_SOCKET ( int epfd, int socket )
{
	ReqType_t t_request = {0};
	ResType_t t_response = {0};
	
    char parse[BUF_MAX_LEN] = {0};
	char buf[BUF_MAX_LEN] = {0};
	
	int n, rc, status;
	int retry_cnt = 0;

	if( epfd < 0 )
	{
		LOG_MSG("[HDL_SOCKET] Epoll FD is Wrong\n");
		return ERR_ARG_INVALID;
	}

	if( socket < 0 )
	{
		LOG_MSG("[HDL_SOCKET] Socket FD is Wrong\n");
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

		snprintf( t_request.body, sizeof(t_request.body), "%s", buf );

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
		snprintf( t_msg.method, sizeof(t_msg.method), "%s", method );
		snprintf( t_msg.uri, sizeof(t_msg.uri), "%s", uri );
		snprintf( t_msg.version, sizeof(t_msg.version), "%s", version );

		/* API 라우팅: /join → Redis user:{user_id} HASH 저장 */
		if ( !strcmp( t_msg.uri, "/join" ) )
		{
			rc = HDL_Join( socket, buf, &t_msg );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: /login → Redis user:{user_id}에서 pwd 조회 후 검증 */
		if ( !strcmp( t_msg.uri, "/login" ) )
		{
			rc = HDL_Login( socket, buf, &t_msg );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: POST /room → 채팅방 생성 */
		if ( !strcmp( t_msg.uri, "/room" ) && !strncmp( t_msg.method, "POST", 4 ) )
		{
			rc = HDL_CreateRoom( socket, buf, &t_msg );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: GET /room?id=... → room_id로 채팅방 검색 */
		if ( !strncmp( t_msg.uri, "/room?id=", 9 ) && !strncmp( t_msg.method, "GET", 3 ) )
		{
			rc = HDL_SearchRoom( socket, buf, &t_msg );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: POST /room/join → room_id + pw로 입장 검증 */
		if ( !strcmp( t_msg.uri, "/room/join" ) && !strncmp( t_msg.method, "POST", 4 ) )
		{
			rc = HDL_JoinRoom( socket, buf, &t_msg );
			if ( rc < 0 )
				return ERR_SOCKET_WRITE;
			break;
		}

		/* API 라우팅: GET /rooms → 채팅방 목록 조회 */
		if ( !strcmp( t_msg.uri, "/rooms" ) && !strncmp( t_msg.method, "GET", 3 ) )
		{
			rc = HDL_ListRooms( socket, buf, &t_msg );
			if ( rc < 0 )
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

		LOG_FMT_CENTER("HDL_SOCKET_Request Parsing");
		LOG_MSG("Method: %s, URI: %s\n", t_msg.method, t_msg.uri);

		/* 클라이언트는 HDL_ACCEPT에서 이미 CTX에 등록됨 */
		break;
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
		LOG_MSG("[HDL_ACCEPT] Socket Accept Fail\n");
		return ERR_SOCKET_ACCEPT;
	}

	LOG_MSG("[HDL_ACCEPT] Client connected FD=%d from %s:%d\n",
		client_fd, inet_ntoa(t_client_addr.sin_addr), ntohs(t_client_addr.sin_port));

    rc = CTX_Http_Insert( &gt_ctx_info, client_fd, t_client_addr ); 
    if( rc < 0 )
    {
        LOG_MSG("[HDL_ACCEPT] Insert Client Info Fail\n");
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
		LOG_MSG("[HDL_ACCEPT] epoll_ctl Add Fail <%d:%s>\n", errno, strerror(errno));
		close(client_fd);
		return ERR_CTX_INSERT;
	}
	
	/* FD pool에 클라이언트 등록 */
	g_client_fd[client_fd/8] |= ( 1 << ( client_fd % 8 ));

	return SOCKET_OK;
}

static int Get_Value_From_Body( const char *p_body, const char *p_key, char **p_offset, int *len )
{
	const char *p;
	const char *p_start;
	const char *p_end;

	if ( p_body == NULL || p_key == NULL || p_offset == NULL || len == NULL )
		return ERR_ARG_INVALID;

	p = strstr( p_body, p_key );
	if ( !p )
		return ERR_ARG_INVALID;

	p = p + strlen( p_key );
	p = strchr( p, ':' );
	if ( !p )
		return ERR_ARG_INVALID;

	p++;
	while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' )
		p++;
	if ( *p != '"' )
		return ERR_ARG_INVALID;

	p++;
	p_start = p;
	p_end = strchr( p_start, '"' );
	if ( !p_end || p_end < p_start )
		return ERR_ARG_INVALID;

	*p_offset = (char *)p_start;
	*len = (int)( p_end - p_start );
	
	return SOCKET_OK;
}