#include "zt_ctx.h"
#include "zt_log.h"

int CTX_Init(ZT_CTX_t *pt_ctx)
{
    if (pt_ctx == NULL)
        return ZT_RC_ARG_INVALID;
    memset(pt_ctx, 0x00, sizeof(ZT_CTX_t));
    if (pthread_mutex_init(&pt_ctx->mutex, NULL) != 0)
        return ZT_RC_CTX;
    return ZT_RC_OK;
}

int CTX_Http_Insert(ZT_CTX_t *pt_ctx, const int client_fd, struct sockaddr_in t_client_addr)
{
    int rc = 0;

    if (pt_ctx == NULL) {
        LOG_ERR("CTX is NULL");
        return ZT_RC_ARG_INVALID;
    }
    if (client_fd < 0) {
        LOG_ERR("Client FD is invalid");
        return ZT_RC_ARG_INVALID;
    }
    if (pt_ctx->client_cnt == MAX_CLIENTS) {
        LOG_ERR("CTX is Full");
        return ZT_RC_CTX;
    }

    HttpCTX_t *pt_new_ctx = calloc(1, sizeof(HttpCTX_t));
    if (pt_new_ctx == NULL) {
        LOG_ERR("Malloc Fail");
        return ZT_RC_CTX;
    }

    pt_new_ctx->client_fd = client_fd;
    memcpy(&pt_new_ctx->t_client_addr, &t_client_addr, sizeof(struct sockaddr_in));

    rc = pthread_mutex_lock(&pt_ctx->mutex);
    if (rc != 0) {
        LOG_ERR("Mutex Lock Fail");
        free(pt_new_ctx);
        return ZT_RC_CTX;
    }

    pt_new_ctx->pt_next_ctx = pt_ctx->pt_http_ctx;
    pt_ctx->pt_http_ctx = pt_new_ctx;
    pt_ctx->client_cnt++;

    rc = pthread_mutex_unlock(&pt_ctx->mutex);
    if (rc != 0) {
        LOG_ERR("Mutex Unlock Fail");
        return ZT_RC_CTX;
    }

    return ZT_RC_OK;
}
