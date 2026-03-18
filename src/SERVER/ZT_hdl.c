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

/*------------------------------------------------
 * Extern Function
-------------------------------------------------*/

void HDL_400 ( int socket );
void HDL_500 ( int socket );

static int HDL_Join( int socket, const char *buf, ReqType_t *t_msg );
static int HDL_Login( int socket, const char *buf, ReqType_t *t_msg );
static int HDL_CreateRoom( int socket, const char *buf, ReqType_t *t_msg );
static int HDL_SearchRoom( int socket, const char *buf, ReqType_t *t_msg );
static int HDL_JoinRoom( int socket, const char *buf, ReqType_t *t_msg );
static int HDL_ListRooms( int socket, const char *buf, ReqType_t *t_msg );


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
static int HDL_Join( int socket, const char *buf, ReqType_t *t_msg )
{
	const char *p_body = NULL;
	user_t t_user = {0};
	char res_buf[BUF_MAX_LEN] = {0};
	char json_body[256] = {0};
	int rc;
	const char *p_val = NULL;

	if ( socket < 0 || buf == NULL || t_msg == NULL )
		return ERR_ARG_INVALID;

	if ( strncmp( t_msg->method, "POST", 4 ) )
	{
		snprintf( res_buf, sizeof(res_buf),
			"HTTP/1.1 405 Method Not Allowed\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 35\r\n\r\n"
			"{\"result\":\"fail\",\"reason\":\"POST only\"}" );
		rc = write( socket, res_buf, strlen(res_buf) );
		return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
	}

	p_body = strstr( buf, "\r\n\r\n" );
	if ( p_body == NULL )
		p_body = strstr( buf, "\n\n" );
	if ( p_body )
		p_body += ( strstr( buf, "\r\n\r\n" ) ? 4 : 2 );
	else
		p_body = buf;

	p_val = strstr( p_body, "\"user_id\"" );
	if ( p_val ) { p_val = strchr( p_val + 9, '"' ); if ( p_val ) { p_val++; sscanf( p_val, "%15[^\"]", t_user.str_user_id ); } }
	p_val = strstr( p_body, "\"name\"" );
	if ( p_val ) { p_val = strchr( p_val + 6, '"' ); if ( p_val ) { p_val++; sscanf( p_val, "%15[^\"]", t_user.str_name ); } }
	p_val = strstr( p_body, "\"password\"" );
	if ( p_val ) { p_val = strchr( p_val + 11, '"' ); if ( p_val ) { p_val++; sscanf( p_val, "%15[^\"]", t_user.str_pwd ); } }

	rc = ZT_REDIS_UserSave( &t_user );
	if ( rc == -1 )
		snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"user_id already exists\"}" );
	else if ( rc != 0 )
		snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"server error\"}" );
	else
		snprintf( json_body, sizeof(json_body), "{\"result\":\"ok\",\"user_id\":\"%s\"}", t_user.str_user_id );

	snprintf( res_buf, sizeof(res_buf),
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: %d\r\n\r\n%s",
		(int)strlen(json_body), json_body );
	rc = write( socket, res_buf, strlen(res_buf) );
	return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
}

/*=================================================
 * name : HDL_Login
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : POST /login JSON body {"user_id":"xxx","password":"yyy"}
 *         Redis user:{user_id}에서 pwd 조회 후 비교
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
	if( strncmp( t_msg->method, "POST", 4 ) )
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

	if ( t_user.str_user_id[0] == '\0' || t_user.str_pwd[0] == '\0' )
	{
		snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"invalid params\"}" );
	}
	else
	{
		user_t stored_user = {0};
		int get_rc = ZT_REDIS_UserGet( t_user.str_user_id, &stored_user );
		if ( get_rc == -1 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"user not found\"}" );
		else if ( get_rc != 0 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"server error\"}" );
		else if ( strcmp( stored_user.str_pwd, t_user.str_pwd ) != 0 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"wrong password\"}" );
		else
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
 * name : HDL_CreateRoom
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : POST /room JSON body {"room_id":"xxx","password":"yyy"}
 *         Redis room:{room_id} HASH에 pwd, created_at, creator_id 저장
 ===================================================*/
static int HDL_CreateRoom( int socket, const char *buf, ReqType_t *t_msg )
{
	const char *p_body = NULL;
	room_t t_room = {0};
	char res_buf[BUF_MAX_LEN] = {0};
	char json_body[256] = {0};
	int rc;
	const char *p_val = NULL;

	if ( socket < 0 || buf == NULL || t_msg == NULL )
		return ERR_ARG_INVALID;

	if ( strncmp( t_msg->method, "POST", 4 ) )
	{
		snprintf( res_buf, sizeof(res_buf),
			"HTTP/1.1 405 Method Not Allowed\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 35\r\n\r\n"
			"{\"result\":\"fail\",\"reason\":\"POST only\"}" );
		rc = write( socket, res_buf, strlen(res_buf) );
		return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
	}

	p_body = strstr( buf, "\r\n\r\n" );
	if ( p_body == NULL )
		p_body = strstr( buf, "\n\n" );
	if ( p_body )
		p_body += ( strstr( buf, "\r\n\r\n" ) ? 4 : 2 );
	else
		p_body = buf;

	p_val = strstr( p_body, "\"room_id\"" );
	if ( p_val ) { p_val = strchr( p_val + 9, '"' ); if ( p_val ) { p_val++; sscanf( p_val, "%15[^\"]", t_room.str_room_id ); } }
	p_val = strstr( p_body, "\"password\"" );
	if ( p_val ) { p_val = strchr( p_val + 11, '"' ); if ( p_val ) { p_val++; sscanf( p_val, "%15[^\"]", t_room.str_pwd ); } }
	/* 옵션: 클라이언트가 user_id를 함께 전달하면 creator 기록 */
	p_val = strstr( p_body, "\"user_id\"" );
	if ( p_val ) { p_val = strchr( p_val + 9, '"' ); if ( p_val ) { p_val++; sscanf( p_val, "%15[^\"]", t_room.str_creator_id ); } }

	if ( t_room.str_room_id[0] == '\0' || t_room.str_pwd[0] == '\0' )
	{
		snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"invalid params\"}" );
	}
	else
	{
		rc = ZT_REDIS_RoomSave( &t_room );
		if ( rc == -1 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"room_id already exists\"}" );
		else if ( rc != 0 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"server error\"}" );
		else
			snprintf( json_body, sizeof(json_body), "{\"result\":\"ok\",\"room_id\":\"%s\"}", t_room.str_room_id );
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
 * name : HDL_SearchRoom
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : GET /room?id={room_id}
 *         room_id로 채팅방 존재 여부 조회, 있으면 room_id 반환
 ===================================================*/
static int HDL_SearchRoom( int socket, const char *buf, ReqType_t *t_msg )
{
	room_t t_room = {0};
	char res_buf[BUF_MAX_LEN] = {0};
	char json_body[256] = {0};
	char room_id[ROOM_ID_MAX_LEN] = {0};
	int rc;
	const char *p_val = NULL;

	if ( socket < 0 || buf == NULL || t_msg == NULL )
		return ERR_ARG_INVALID;

	if ( strncmp( t_msg->method, "GET", 3 ) )
	{
		snprintf( res_buf, sizeof(res_buf),
			"HTTP/1.1 405 Method Not Allowed\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 34\r\n\r\n"
			"{\"result\":\"fail\",\"reason\":\"GET only\"}" );
		rc = write( socket, res_buf, strlen(res_buf) );
		return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
	}

	/* URI에서 ?id= 파싱: /room?id=myroom */
	p_val = strstr( t_msg->uri, "?id=" );
	if ( p_val )
		snprintf( room_id, sizeof(room_id), "%15s", p_val + 4 );

	if ( room_id[0] == '\0' )
	{
		snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"room_id required\"}" );
	}
	else
	{
		rc = ZT_REDIS_RoomGet( room_id, &t_room );
		if ( rc == -1 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"room not found\"}" );
		else if ( rc != 0 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"server error\"}" );
		else
			snprintf( json_body, sizeof(json_body), "{\"result\":\"ok\",\"room_id\":\"%s\"}", t_room.str_room_id );
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
 * name : HDL_JoinRoom
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : POST /room/join JSON body {"room_id":"xxx","password":"yyy"}
 *         room_id로 채팅방 조회 후 pw 검증, 일치하면 입장 허가
 ===================================================*/
static int HDL_JoinRoom( int socket, const char *buf, ReqType_t *t_msg )
{
	const char *p_body = NULL;
	room_t t_room = {0};
	room_t stored_room = {0};
	char res_buf[BUF_MAX_LEN] = {0};
	char json_body[256] = {0};
	int rc;
	const char *p_val = NULL;

	if ( socket < 0 || buf == NULL || t_msg == NULL )
		return ERR_ARG_INVALID;

	if ( strncmp( t_msg->method, "POST", 4 ) )
	{
		snprintf( res_buf, sizeof(res_buf),
			"HTTP/1.1 405 Method Not Allowed\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 35\r\n\r\n"
			"{\"result\":\"fail\",\"reason\":\"POST only\"}" );
		rc = write( socket, res_buf, strlen(res_buf) );
		return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
	}

	p_body = strstr( buf, "\r\n\r\n" );
	if ( p_body == NULL )
		p_body = strstr( buf, "\n\n" );
	if ( p_body )
		p_body += ( strstr( buf, "\r\n\r\n" ) ? 4 : 2 );
	else
		p_body = buf;

	p_val = strstr( p_body, "\"room_id\"" );
	if ( p_val ) { p_val = strchr( p_val + 9, '"' ); if ( p_val ) { p_val++; sscanf( p_val, "%15[^\"]", t_room.str_room_id ); } }
	p_val = strstr( p_body, "\"password\"" );
	if ( p_val ) { p_val = strchr( p_val + 11, '"' ); if ( p_val ) { p_val++; sscanf( p_val, "%15[^\"]", t_room.str_pwd ); } }

	if ( t_room.str_room_id[0] == '\0' || t_room.str_pwd[0] == '\0' )
	{
		snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"invalid params\"}" );
	}
	else
	{
		rc = ZT_REDIS_RoomGet( t_room.str_room_id, &stored_room );
		if ( rc == -1 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"room not found\"}" );
		else if ( rc != 0 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"server error\"}" );
		else if ( strcmp( stored_room.str_pwd, t_room.str_pwd ) != 0 )
			snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"wrong password\"}" );
		else
			snprintf( json_body, sizeof(json_body), "{\"result\":\"ok\",\"room_id\":\"%s\"}", t_room.str_room_id );
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
 * name : HDL_ListRooms
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : GET /rooms
 *         Redis에서 room:* 키를 스캔해서 room_id, creator_id(user_id) 리스트 반환
 *         {"result":"ok","rooms":[{"room_id":"r1","user_id":"u1"},...]}
 * ===================================================*/
static int HDL_ListRooms( int socket, const char *buf, ReqType_t *t_msg )
{
	room_t rooms[64] = {0};
	char res_buf[BUF_MAX_LEN] = {0};
	char json_body[1024] = {0};
	int rc;
	int i, n;

	(void)buf;

	if ( socket < 0 || t_msg == NULL )
		return ERR_ARG_INVALID;

	if ( strncmp( t_msg->method, "GET", 3 ) )
	{
		snprintf( res_buf, sizeof(res_buf),
			"HTTP/1.1 405 Method Not Allowed\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 34\r\n\r\n"
			"{\"result\":\"fail\",\"reason\":\"GET only\"}" );
		rc = write( socket, res_buf, strlen(res_buf) );
		return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
	}

	n = ZT_REDIS_RoomList( rooms, (size_t)(sizeof(rooms)/sizeof(rooms[0])) );
	if ( n < 0 )
	{
		snprintf( json_body, sizeof(json_body), "{\"result\":\"fail\",\"reason\":\"server error\"}" );
	}
	else
	{
		int len = 0;
		len += snprintf( json_body + len, sizeof(json_body) - len, "{\"result\":\"ok\",\"rooms\":[");
		for ( i = 0; i < n && len < (int)sizeof(json_body) - 1; i++ )
		{
			len += snprintf( json_body + len, sizeof(json_body) - len,
				"%s{\"room_id\":\"%s\",\"user_id\":\"%s\"}",
				(i == 0 ? "" : ","),
				rooms[i].str_room_id,
				rooms[i].str_creator_id );
		}
		len += snprintf( json_body + len, sizeof(json_body) - len, "]}" );
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

		snprintf( parse, sizeof(parse), "%s", buf );

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

void HDL_400( int socket )
{
	int rc = 0;

	const char *msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
	rc = write( socket, msg, strlen(msg) );
	if( rc < 0 )
	{
		LOG_MSG("[HDL_400] write fail\n");
	}
}

void HDL_500( int socket ) 
{
	int rc = 0;

	const char *msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
	rc = write( socket, msg, strlen(msg) );
	if( rc < 0 )
	{
		LOG_MSG("[HDL_500] write fail\n");
	}

}
