#include <mysql/mysql.h>

MYSQL *conn;

#define HOST localhost
#define USER yjkim
#define PASSWORD 1234

int ZT_DB_CONNECT()
{
	int rc = -1;

	/* 1. Conn 초기화 */
	conn = mysql_init(NULL);
	if(conn == NULL)
	{
		LOG_ERR("[ZT_DB_CONNECT] mysql_init fail: %s\n", mysql_error(conn));
		return DB_ERROR;
	}

	/* 2. DB 접속 */
	rc = mysql_real_connect(conn, HOST, USER, PASSWORD, "database", 0, NULL, 0);
	if(rc == NULL)
	{
		LOG_ERR("[ZT_DB_CONNECT] mysql_real_connect fail: %s\n", mysql_error(conn));
		return DB_ERROR;
	}

	return SOCKET_OK;
}

#define SELECT_USER "SELECT * FROM user WHERE userid=?;"

int ZT_DB_QUERY()
{
	
}
