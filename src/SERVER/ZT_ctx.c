
#include "ZT_ctx.h"

int CTX_Init( ZT_CTX_t *pt_ctx )
{
	memset(pt_ctx, 0x00, sizeof(ZT_CTX_t));
	return SOCKET_OK;	
}

int CTX_Http_Insert( ZT_CTX_t *pt_ctx, const int client_fd, struct sockaddr_in t_client_addr )
{
    int rc = 0;

	if( pt_ctx == NULL )
    {
        LOG_ERR("CTX is NULL\n");
        return ERR_ARG_INVALID;
    }

    if( client_fd < 0 )
    {
        LOG_ERR("Client Info is NULL\n");
        return ERR_ARG_INVALID;
    }

    /* Full Check */
    if( pt_ctx->client_cnt ==  MAX_CLIENTS )
    {
        LOG_ERR("CTX is Full");
        return ERR_CTX_FULL;
    }

    HttpCTX_t *pt_new_ctx = calloc(1, sizeof(HttpCTX_t));
    if( pt_new_ctx == NULL )
    {
        LOG_ERR("Malloc Fail"); 
        return ERR_CTX_ALLOC;
    }

    /* Copy Client Info */
    pt_new_ctx->client_fd = client_fd;
    memcpy( &pt_new_ctx->t_client_addr, &t_client_addr, sizeof(struct sockaddr_in) );

    rc = pthread_mutex_lock( &pt_ctx->mutex );
    if( rc < 0 )
    {
        LOG_ERR("Mutex Lock Fail");
        return ERR_CTX_LOCK;
    }

    if ( pt_ctx->pt_http_ctx == NULL )
    {
        /* First client: empty list */
        pt_ctx->pt_http_ctx = pt_new_ctx;
    }
    else
    {
        /* Append to tail */
        HttpCTX_t *pt_tail = pt_ctx->pt_http_ctx;
        while ( pt_tail->pt_next_ctx != NULL )
            pt_tail = pt_tail->pt_next_ctx;
        pt_tail->pt_next_ctx = pt_new_ctx;
    }
    pt_ctx->client_cnt++;
    
close_mutex:
    rc = pthread_mutex_unlock( &pt_ctx->mutex );
    if( rc < 0 )
    {
        LOG_ERR("Mutex Unlock Fail");
        return ERR_CTX_UNLOCK;
    }

    return CTX_OK; 
}
