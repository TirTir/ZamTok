#include "ZT_Inc.h"
#include "ZT_redis.h"
#include "ZT_log.h"

ZT_CTX_t gt_ctx_info;
unsigned char g_client_fd[MAX_CLIENTS/8];

int main( int argc, char **argv )
{
	int rc = -1, socket = -1;
	int port = 0;

	if (argc < 2) {
		LOG_MSG("[INFO] Use: %s <port>\n", argv[0] ? argv[0] : "CLIENT");
		return -1;
	}

	port = atoi(argv[1]);
	if ( port < 0 ) {
		LOG_MSG("[ERROR] Invalid port\n");
		return -1;
	}

	LOG_MSG("[INFO] The Server Will Listen to Port: %d\n", port);

	{
		const char *redis_host = getenv("REDIS_HOST");
		const char *redis_port_str = getenv("REDIS_PORT");
		int redis_port = redis_port_str ? atoi(redis_port_str) : 6379;
		if (!redis_host) redis_host = "127.0.0.1";
		LOG_MSG("[INFO] Redis: %s:%d\n", redis_host, redis_port);
		rc = redis_connect( redis_host, redis_port );
		if ( rc != 0 )
			LOG_MSG("[WARN] Redis connect fail (join/login will fail)\n");
		else
			LOG_MSG("[INFO] Redis connected\n");
	}

	rc = socket_init( &socket );
	if ( rc < 0 ) {
		LOG_MSG("[ERROR] Socket_Init Fail\n");
		goto close_socket;
	}

	rc = socket_bind( socket, port );
	if ( rc < 0 ) {
		LOG_MSG("[ERROR] Socket_Bind Fail\n");
		goto close_socket;
	}

	rc = ctx_init( &gt_ctx_info );
	if ( rc < 0 ) {
		LOG_MSG("[ERROR] CTX_Init Fail\n");
		goto close_socket;
	}

	rc = socket_event_loop( socket );
	if ( rc < 0 ) {
		LOG_MSG("[ERROR] EventLoop Fail\n");
		goto close_socket;
	}

close_socket:
	redis_disconnect();
	if ( socket >= 0 )
		close( socket );

	return 0;
}
