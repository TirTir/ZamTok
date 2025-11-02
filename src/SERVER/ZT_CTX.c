#include "queue.h"

#define LOG_ERR( fmt, ... ) \
    printf("[ERROR] (%s:%d) " msg "\n", __FILE_, __LINE__, ##__VA_ARGS__ )

int CTX_Init( ZT_CTX_t *pCTX )
{
        
}

int CTX_Http_Insert( ZT_CTX_t *pCTX, const int nClientFD, struct sockaddr_in tClientAddr, socklen_t unSockLen )
{
    if( pCTX == NULL )
    {
        LOG_ERR("CTX is NULL\n");
        return ERR_CTX_ARG;
    }

    if( nClientFD < 0 || tClientAddr == NULL || unSockLen == NULL )
    {
        LOG_ERR("Client Info is NULL\n");
        return ERR_CTX_ARG;
    }

    /* Full Check */
    if( nCTXCnt ==  MAX_CTX_SIZE )
    {
        LOG_ERR("CTX Count is Full");
        return ERR_CTX_FULL;
    }

    HttpCTX_t *ptNewCTX = calloc(1, sizeof(HttpCTX_t));
    if( ptNewCTX == NULL )
    {
        LOG_ERR("Malloc Fail"); 
        return ERR_CTX_ALLOC;
    }

    /* Copy Client Info */
    ptNewCTX->nClientFD = nClientFD;
    memcpy( &ptNewCTX->tClientAddr, &tClientAddr, sizeof(struct sockaddr_in) );
    ptNewCTX->tClientAddr.unSocklen = unSockLen;

    rc = pthread_mutex_lock( &pCTX->mutex );
    if( rc < 0 )
    {
        LOG_ERR("Mutex Lock Fail");
        return ERR_CTX_LOCK;
    }
    
    ptNewCTX->ptNextCTX = pCTX->ptHeadCTX;
    pCTX->ptHeadCTX = ptNewCTX;
    pCTX.nCTXCnt++;
    
close_mutex:
    rc = pthread_mutex_unlock( &pCTX->mutex );
    if( rc < 0 )
    {
        LOG_ERR("Mutex Unlock Fail");
        return ERR_CTX_UNLOCK;
    }

    return CTX_OK; 
}
