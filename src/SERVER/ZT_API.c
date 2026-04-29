#include "ZT_API.h"

static int is_valid_link(const char *inupt)
{   
    static const char *http = "http://";
    static const char *https = "https://";
    if (strncmp(input, http, strlen(http)) == 0 || strncmp(input, https, strlen(https)) == 0)
        return 1;
    else
        return 0;
}

/*=================================================
 * name : api_join
 * return : SOCKET_OK / ERR_*
 * param : socket, buf(전체 HTTP 요청), t_msg
 * desc  : POST /join JSON body {"user_id":"xxx","name":"yyy","password":"zzz"}
 *         Redis user:{user_id} HASH에 name, pwd, created_at 저장
 ===================================================*/
 static int api_join( int socket, const char *buf, ReqType_t *t_request )
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
     if ( msg_copy_json_string_field( p_body, USER_ID_KEY, t_user.str_user_id, sizeof( t_user.str_user_id ) ) != SOCKET_OK
         || msg_copy_json_string_field( p_body, NAME_KEY, t_user.str_name, sizeof( t_user.str_name ) ) != SOCKET_OK
         || msg_copy_json_string_field( p_body, PASSWORD_KEY, t_user.str_pwd, sizeof( t_user.str_pwd ) ) != SOCKET_OK )
     {
         status = 400;
         e_code = ZT_ERR_INVALID_PARAMS;
         goto err_return;
     }
 
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
     {
         char data_json[USER_ID_MAX_LEN + 32];
 
         snprintf( data_json, sizeof( data_json ), "{\"user_id\":\"%s\"}", t_user.str_user_id );
         t_response = Generate_Response( t_request, 200 );
         Generate_Response_Body( &t_response, RESULT_SUCCESS, ZT_ERR_SUCCESS, data_json );
         rc = HDL_SendHttpJson( socket, &t_response );
         return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
     }
 
 err_return:
     t_response = Generate_Response( t_request, status );
     Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
     rc = HDL_SendHttpJson( socket, &t_response );
     return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
 }
 
 /*=================================================
  * name : HDL_Login
  * return : SOCKET_OK / ERR_*
  * param : socket, buf(전체 HTTP 요청), t_request
  * desc  : POST /login JSON body {"user_id":"xxx","password":"yyy"}
  *         Redis user:{user_id}에서 pwd 조회 후 비교
  ===================================================*/
 static int api_login( int socket, const char *buf, ReqType_t *t_request )
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
     if ( Copy_Json_String_Field( p_body, USER_ID_KEY, t_user.str_user_id, sizeof( t_user.str_user_id ) ) != SOCKET_OK
         || Copy_Json_String_Field( p_body, PASSWORD_KEY, t_user.str_pwd, sizeof( t_user.str_pwd ) ) != SOCKET_OK )
     {
         status = 400;
         e_code = ZT_ERR_INVALID_PARAMS;
         goto err_return;
     }
 
     /* 3. User Check */
     if ( t_user.str_user_id[0] == '\0' || t_user.str_pwd[0] == '\0' )
     {
         status = 400;
         e_code = ZT_ERR_INVALID_PARAMS;
         goto err_return;
     }
 
    /* 4. User Filtering(수정필요) */
    regcomp(&unsecurePattern, "[^[:alnum:]]|select|delete|update|insert|create|alter|drop", 
        REG_ICASE | REG_EXTENDED); 
    if (unsecurePattern.re_nsub == 0)
    {
        status = 400;
        e_code = ZT_ERR_INVALID_PARAMS;
        goto err_return;
    }

    /* 5. User Get From Redis */
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
 
     /* 5. Response */
     t_response = Generate_Response( t_request, 200 );
     Generate_Response_Body( &t_response, RESULT_SUCCESS, ZT_ERR_SUCCESS, "null" );
     rc = HDL_SendHttpJson( socket, &t_response );
     return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
 
 err_return:
     t_response = Generate_Response( t_request, status );
     Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
     rc = HDL_SendHttpJson( socket, &t_response );
     return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
 }
 
 /*=================================================
  * name : HDL_CreateRoom
  * return : SOCKET_OK / ERR_*
  * param : socket, buf(전체 HTTP 요청), t_msg
  * desc  : POST /room JSON body {"room_id":"xxx","password":"yyy"}
  *         Redis room:{room_id} HASH에 pwd, created_at, creator_id 저장
  ===================================================*/
 static int api_create_room( int socket, const char *buf, ReqType_t *t_request )
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
     if ( Copy_Json_String_Field( p_body, ROOM_ID_KEY, t_room.str_room_id, sizeof( t_room.str_room_id ) ) != SOCKET_OK
         || Copy_Json_String_Field( p_body, PASSWORD_KEY, t_room.str_pwd, sizeof( t_room.str_pwd ) ) != SOCKET_OK
         || Copy_Json_String_Field( p_body, USER_ID_KEY, t_room.str_creator_id, sizeof( t_room.str_creator_id ) ) != SOCKET_OK )
     {
         status = 400;
         e_code = ZT_ERR_INVALID_PARAMS;
         goto err_return;
     }
 
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
     {
         char data_json[ROOM_ID_MAX_LEN + 32];
 
         snprintf( data_json, sizeof( data_json ), "{\"room_id\":\"%s\"}", t_room.str_room_id );
         t_response = Generate_Response( t_request, 200 );
         Generate_Response_Body( &t_response, RESULT_SUCCESS, ZT_ERR_SUCCESS, data_json );
         rc = HDL_SendHttpJson( socket, &t_response );
         return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
     }
 
 err_return:
     t_response = Generate_Response( t_request, status );
     Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
     rc = HDL_SendHttpJson( socket, &t_response );
     return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
 }
 
 /*=================================================
  * name : HDL_SearchRoom
  * return : SOCKET_OK / ERR_*
  * param : socket, buf(전체 HTTP 요청), t_request
  * desc  : GET /room?id={room_id}
  *         room_id로 채팅방 존재 여부 조회, 있으면 room_id 반환
  ===================================================*/
 static int api_search_room( int socket, const char *buf, ReqType_t *t_request )
 {
     room_t t_room = {0};
     ResType_t t_response = {0};
     int rc = 0;
     int status = 0;
     ZtErrCode e_code = ZT_ERR_INVALID_PARAMS;
     const char *q;
     size_t i;
 
     (void)buf;
 
     if ( socket < 0 || t_request == NULL )
         return ERR_ARG_INVALID;
 
     if ( strncmp( t_request->method, "GET", 3 ) )
     {
         status = 405;
         e_code = ZT_ERR_METHOD_NOT_ALLOWED;
         goto err_return;
     }
 
     q = strstr( t_request->uri, "?id=" );
     if ( !q )
     {
         status = 400;
         e_code = ZT_ERR_INVALID_PARAMS;
         goto err_return;
     }
     q += 4;
     for ( i = 0; i < sizeof( t_room.str_room_id ) - 1 && q[i] && q[i] != '&' && q[i] != ' '; i++ )
         t_room.str_room_id[i] = q[i];
     t_room.str_room_id[i] = '\0';
 
     if ( t_room.str_room_id[0] == '\0' )
     {
         status = 400;
         e_code = ZT_ERR_INVALID_PARAMS;
         goto err_return;
     }
 
     rc = ZT_REDIS_RoomGet( t_room.str_room_id, &t_room );
     if ( rc == -1 )
     {
         status = 404;
         e_code = ZT_ERR_ROOM_NOT_FOUND;
         goto err_return;
     }
     if ( rc != 0 )
     {
         status = 500;
         e_code = ZT_ERR_SERVER_ERROR;
         goto err_return;
     }
 
     {
         char data_json[ROOM_ID_MAX_LEN + 32];
 
         snprintf( data_json, sizeof( data_json ), "{\"room_id\":\"%s\"}", t_room.str_room_id );
         t_response = Generate_Response( t_request, 200 );
         Generate_Response_Body( &t_response, RESULT_SUCCESS, ZT_ERR_SUCCESS, data_json );
         rc = HDL_SendHttpJson( socket, &t_response );
         return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
     }
 
 err_return:
     t_response = Generate_Response( t_request, status );
     Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
     rc = HDL_SendHttpJson( socket, &t_response );
     return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
 }
 
 /*=================================================
  * name : HDL_JoinRoom
  * return : SOCKET_OK / ERR_*
  * param : socket, buf(전체 HTTP 요청), t_request
  * desc  : POST /room/join JSON body {"room_id":"xxx","password":"yyy"}
  *         room_id로 채팅방 조회 후 pw 검증, 일치하면 입장 허가
  ===================================================*/
 static int api_join_room( int socket, const char *buf, ReqType_t *t_request )
 {
     room_t t_room = {0};
     room_t stored_room = {0};
     ResType_t t_response = {0};
 
     int rc = 0;
     int status = 0;
     const char *p_body = NULL;
     ZtErrCode e_code = ZT_ERR_INVALID_PARAMS;
 
     if ( socket < 0 || buf == NULL || t_request == NULL )
         return ERR_ARG_INVALID;
 
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
 
     if ( Copy_Json_String_Field( p_body, ROOM_ID_KEY, t_room.str_room_id, sizeof( t_room.str_room_id ) ) != SOCKET_OK
         || Copy_Json_String_Field( p_body, PASSWORD_KEY, t_room.str_pwd, sizeof( t_room.str_pwd ) ) != SOCKET_OK )
     {
         status = 400;
         e_code = ZT_ERR_INVALID_PARAMS;
         goto err_return;
     }
 
     if ( t_room.str_room_id[0] == '\0' || t_room.str_pwd[0] == '\0' )
     {
         status = 400;
         e_code = ZT_ERR_INVALID_PARAMS;
         goto err_return;
     }
 
     rc = ZT_REDIS_RoomGet( t_room.str_room_id, &stored_room );
     if ( rc == -1 )
     {
         status = 404;
         e_code = ZT_ERR_ROOM_NOT_FOUND;
         goto err_return;
     }
     if ( rc != 0 )
     {
         status = 500;
         e_code = ZT_ERR_SERVER_ERROR;
         goto err_return;
     }
 
     if ( strcmp( stored_room.str_pwd, t_room.str_pwd ) != 0 )
     {
         status = 401;
         e_code = ZT_ERR_WRONG_PASSWORD;
         goto err_return;
     }
 
     t_response = Generate_Response( t_request, 200 );
     Generate_Response_Body( &t_response, RESULT_SUCCESS, ZT_ERR_SUCCESS, "null" );
     rc = HDL_SendHttpJson( socket, &t_response );
     return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
 
 err_return:
     t_response = Generate_Response( t_request, status );
     Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
     rc = HDL_SendHttpJson( socket, &t_response );
     return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
 }
 
 /*=================================================
  * name : api_list_rooms
  * return : SOCKET_OK / ERR_*
  * param : socket, buf(전체 HTTP 요청), t_request
  * desc  : GET /rooms
  *         Redis에서 room:* 키를 스캔해서 room_id, creator_id(user_id) 리스트 반환
  *         {"result":"ok","rooms":[{"room_id":"r1","user_id":"u1"},...]}
  * ===================================================*/
 static int api_list_rooms( int socket, const char *buf, ReqType_t *t_request )
 {
     room_t rooms[64];
     ResType_t t_response = {0};
     char listdata[DATA_MAX_LEN];
     int rc = 0;
     int status = 0;
     int pos = 0;
     int ret;
     int i;
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
 
     rc = ZT_REDIS_RoomList( rooms, (size_t)( sizeof( rooms ) / sizeof( rooms[0] ) ) );
     if ( rc < 0 )
     {
         status = 500;
         e_code = ZT_ERR_SERVER_ERROR;
         goto err_return;
     }
 
     ret = snprintf( listdata + pos, sizeof( listdata ) - (size_t)pos, "[" );
     if ( ret < 0 || ret >= (int)( sizeof( listdata ) - (size_t)pos ) )
         goto list_overflow;
     pos += ret;
 
     for ( i = 0; i < rc; i++ )
     {
         ret = snprintf( listdata + pos, sizeof( listdata ) - (size_t)pos,
             "%s{\"room_id\":\"%s\",\"user_id\":\"%s\"}",
             ( i > 0 ) ? "," : "",
             rooms[i].str_room_id,
             rooms[i].str_creator_id );
         if ( ret < 0 || ret >= (int)( sizeof( listdata ) - (size_t)pos ) )
             goto list_overflow;
         pos += ret;
     }
 
     ret = snprintf( listdata + pos, sizeof( listdata ) - (size_t)pos, "]" );
     if ( ret < 0 || ret >= (int)( sizeof( listdata ) - (size_t)pos ) )
         goto list_overflow;
 
     t_response = Generate_Response( t_request, 200 );
     Generate_Response_Body( &t_response, RESULT_SUCCESS, ZT_ERR_SUCCESS, listdata );
     rc = HDL_SendHttpJson( socket, &t_response );
     return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
 
 list_overflow:
     status = 500;
     e_code = ZT_ERR_SERVER_ERROR;
 
 err_return:
     t_response = Generate_Response( t_request, status );
     Generate_Response_Body( &t_response, RESULT_FAIL, e_code, NULL );
     rc = HDL_SendHttpJson( socket, &t_response );
     return ( rc < 0 ) ? ERR_SOCKET_WRITE : SOCKET_OK;
 }