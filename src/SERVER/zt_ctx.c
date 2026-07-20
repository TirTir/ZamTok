#include "zt_inc.h"

int ctx_init(zt_server_ctx_t *ctx)
{
	if (ctx == NULL)
		return ZT_RC_ARG_INVALID;
	
    memset(ctx, 0, sizeof(*ctx));
	
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0)
		return ZT_RC_CTX;

	return ZT_RC_OK;
}

int ctx_http_free(zt_server_ctx_t *ctx)
{
	http_conn_ctx_t *node;
	http_conn_ctx_t *next;

	if (ctx == NULL)
		return ZT_RC_ARG_INVALID;

	node = ctx->http_conn_head;
	while (node != NULL) {
		next = node->next;
		free(node);
		node = next;
	}
	ctx->http_conn_head = NULL;
	ctx->client_count = 0;

	return ZT_RC_OK;
}

int ctx_http_insert(zt_server_ctx_t *ctx, int client_fd, struct sockaddr_in client_addr)
{
	int rc = 0;
	http_conn_ctx_t *new_ctx;

	if (ctx == NULL || client_fd < 0)
		return ZT_RC_ARG_INVALID;

	if (ctx->client_count >= MAX_CLIENTS) {
		LOG_ERR("CTX is Full");
		return ZT_RC_CTX;
	}

	new_ctx = calloc(1, sizeof(*new_ctx));
	if (new_ctx == NULL) {
		LOG_ERR("Malloc Fail");
		return ZT_RC_CTX;
	}

	new_ctx->client_fd = client_fd;
	new_ctx->client_addr.family = AF_INET;
	new_ctx->client_addr.socklen = sizeof(client_addr);
	memcpy(&new_ctx->client_addr.addr.ipv4, &client_addr, sizeof(client_addr));

	rc = pthread_mutex_lock(&ctx->mutex);
	if (rc != 0) {
		LOG_ERR("Mutex Lock Fail");
		free(new_ctx);
		return ZT_RC_CTX;
	}

	if (ctx->http_conn_head == NULL) {
		ctx->http_conn_head = new_ctx;
	} else {
		http_conn_ctx_t *tail = ctx->http_conn_head;
		while (tail->next != NULL)
			tail = tail->next;
		tail->next = new_ctx;
	}
	ctx->client_count++;

	rc = pthread_mutex_unlock(&ctx->mutex);
	if (rc != 0) {
		LOG_ERR("Mutex Unlock Fail");
		return ZT_RC_CTX;
	}

	return ZT_RC_OK;
}
