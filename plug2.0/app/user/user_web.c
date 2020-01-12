/*
 * user_web.c
 *
 *  Created on: 2018年11月4日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"



xTaskHandle xWebServerHandle = NULL;
xTaskHandle xWebHandle = NULL;

INT32 iSocketFd = -1;
static HTTP_CTX stWebCtx[WEB_MAX_FD];

CHAR* pcRecvBuf = NULL;

BOOL bIsWebSvcRunning = FALSE;
BOOL bWebServerTaskTerminate = FALSE;


VOID WEB_StartHandleTheard( VOID *Para );


VOID WEB_WebCtxInitAll( VOID )
{
    UINT8 iLoop = 0;
    memset(stWebCtx, 0, sizeof(stWebCtx));
    for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
    {
        if( stWebCtx[iLoop].iClientFd > 0 )
        {
            close( stWebCtx[iLoop].iClientFd );
        }

        stWebCtx[iLoop].iClientFd = -1;
        stWebCtx[iLoop].uiTimeOut = WEB_CONTINUE_TMOUT;
    }
}

UINT WEB_CloseWebCtx( HTTP_CTX *pstCtx )
{
    if ( pstCtx == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "pstCtx:%p", pstCtx);
        return FAIL;
    }

    if ( pstConsoleCtx == pstCtx )
    {
    	printf("%d:pstCtx:0x%p\r\n", __LINE__, pstCtx);
    	pstConsoleCtx = NULL;
    }

    if( pstCtx->iClientFd > 0 )
    {
        LOG_OUT(LOGOUT_DEBUG, "disconnect", pstCtx->iClientFd);
        close( pstCtx->iClientFd );
    }

    HTTP_ResponInit(pstCtx);
    memset(pstCtx, 0, sizeof(HTTP_CTX));
    HTTP_RequestInit( pstCtx );
    pstCtx->iClientFd = -1;
    pstCtx->uiTimeOut = WEB_CONTINUE_TMOUT;

    return OK;
}

VOID WEB_SetWebSvcStatus( BOOL bStatus )
{
    if ( bStatus )
    {
        bIsWebSvcRunning = TRUE;
    }
    else
    {
        bIsWebSvcRunning = FALSE;
    }
}

UINT8 WEB_GetWebSvcStatus( VOID )
{
    return bIsWebSvcRunning;
}

STATIC VOID WEB_WebServerTask( VOID *Para )
{
    struct sockaddr_in stServerAddr;
    struct sockaddr_in stClientAddr;
    INT32 iClientAddrLen = sizeof( struct sockaddr_in );
    INT32 iClientFd = -1;
    INT32 iRet = 0;
    INT32 Reuseaddr = 1;
    INT32 iLoop = 0;
    fd_set stFdRead;
    struct timeval stSelectTimeOut = {1, 0};
    struct timeval stRecvTimeOut = {1, 0};

    LOG_OUT(LOGOUT_INFO, "WEB_WebServerTask started.");
    bWebServerTaskTerminate = FALSE;
    WEB_SetWebSvcStatus( TRUE );

    memset(&stServerAddr, 0, sizeof(stServerAddr));
    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_addr.s_addr = INADDR_ANY;
    stServerAddr.sin_len = sizeof(stServerAddr);
    stServerAddr.sin_port = htons(80);

    iSocketFd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( iSocketFd == -1 )
    {
        LOG_OUT(LOGOUT_ERROR, "socket failed started. iSocketFd:%d", iSocketFd);
    }
    LOG_OUT(LOGOUT_DEBUG, "socket ok, iSocketFd:%d", iSocketFd);

    //不知道为什么一直失败
    //iRet = setsockopt(iSocketFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&Reuseaddr, sizeof(Reuseaddr));
    //iRet = setsockopt(iSocketFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&stRecvTimeOut, sizeof(stRecvTimeOut));
    //if ( 0 != iRet )
    //{
    //    LOG_OUT(LOGOUT_ERROR, "setsockopt failed, iRet:%d", iRet);
    //}
    //LOG_OUT(LOGOUT_DEBUG, "setsockopt ok, iSocketFd:%d", iSocketFd);

    do
    {
        iRet = bind( iSocketFd, (struct sockaddr *)&stServerAddr, sizeof(stServerAddr));
        if ( 0 != iRet )
        {
            LOG_OUT(LOGOUT_ERROR, "bind failed, iRet:%d", iRet);
            vTaskDelay(1000/portTICK_RATE_MS);
        }
    } while ( iRet != 0 );

    LOG_OUT(LOGOUT_DEBUG, "bind ok, iSocketFd:%d", iSocketFd);

    do
    {
        iRet = listen( iSocketFd, WEB_MAX_FD );
        if (iRet != 0)
        {
            vTaskDelay(1000/portTICK_RATE_MS);
        }
    } while ( iRet != 0 );
    //LOG_OUT(LOGOUT_DEBUG, "listen ok, iSocketFd:%d", iSocketFd);

    FD_ZERO( &stFdRead );
    FD_SET( iSocketFd, &stFdRead );

    WEB_WebCtxInitAll();
    HTTP_RouterInit();

    for ( ;; )
    {
        if ( bWebServerTaskTerminate == TRUE )
        {
            goto end;
        }
        FD_ZERO( &stFdRead );
        FD_SET( iSocketFd, &stFdRead );

        iRet = select( WEB_MAX_FD+1, &stFdRead, NULL, NULL, &stSelectTimeOut );
        if ( iRet < 0 )
        {
            LOG_OUT(LOGOUT_ERROR, "select accept error, errno:%d, iRet:%d", errno, iRet);
            vTaskDelay(1000/portTICK_RATE_MS);
            continue;
        }
        else if ( 0 == iRet || errno == EINTR )
        {
            //LOG_OUT(LOGOUT_DEBUG, "WEB_WebServerTask, select timeout.");
            vTaskDelay(100/portTICK_RATE_MS);
            continue;
        }
        
        //LOG_OUT(LOGOUT_DEBUG, "WEB_WebServerTask, select ok, iRet:%d", iRet);

        //客户端已满，等待释放
        while (1)
        {
            for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
            {
                if ( stWebCtx[iLoop].iClientFd < 0 )
                {
                    break;
                }
            }

            if ( system_get_free_heap_size() > HTTP_BUF_30K  && iLoop < WEB_MAX_FD)
            {
                break;
            }

            LOG_OUT(LOGOUT_DEBUG, "connect full or heap insufficient, max free heap size: %d", WEB_MAX_FD, system_get_free_heap_size());
            vTaskDelay(100/portTICK_RATE_MS);
        }

        if ( FD_ISSET(iSocketFd, &stFdRead ))
        {
            //LOG_OUT(LOGOUT_DEBUG, "accept...");
            iClientFd = accept(iSocketFd, (struct sockaddr *)&stClientAddr, (socklen_t *)&iClientAddrLen);
            if ( -1 != iClientFd )
            {
                FD_CLR(iSocketFd, &stFdRead);
                
                for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
                {
                    if ( stWebCtx[iLoop].iClientFd < 0 )
                    {
                        LOG_OUT(LOGOUT_DEBUG, "fd:%d connect", iClientFd);
                        stWebCtx[iLoop].iClientFd = iClientFd;

                        UINT oldHeap = system_get_free_heap_size();
                        WEB_StartHandleTheard( &stWebCtx[iLoop] );
                        UINT newHeap = system_get_free_heap_size();
                        LOG_OUT(LOGOUT_DEBUG, "Free heap:%d, used:%d", newHeap, oldHeap-newHeap);

                        break;
                    }
                }
            }
            else
            {
                LOG_OUT(LOGOUT_ERROR, "accept error", iClientFd);
            }
        }
    }

end:
    LOG_OUT(LOGOUT_INFO, "WEB_WebServerTask stopped.");
    close( iSocketFd );
    iSocketFd = -1;
    bWebServerTaskTerminate = FALSE;
    vTaskDelete( NULL );
}

VOID WEB_StartWebServerTheard( VOID )
{
    xTaskCreate(WEB_WebServerTask, "WEB_WebServerTask", 512, NULL, 4, &xWebServerHandle);
}

STATIC VOID WEB_WebHandleTask( VOID *Para )
{
    CHAR* pcRecvBuf = NULL;
    struct timeval stRdTimeOut = {1, 0};
    fd_set stFdRead;
    INT iRetN = 0;
    INT32 iRet = 0;
    HTTP_CTX *pstCtx = Para;

    //LOG_OUT(LOGOUT_INFO, "WEB_WebHandleTask started", pstCtx->iClientFd);

    pcRecvBuf = ( CHAR* )malloc( WEB_RECVBUF_SIZE + 10 );
    if ( NULL == pcRecvBuf )
    {
        LOG_OUT(LOGOUT_ERROR, "malloc pcRecvBuf failed.");
        goto end;
    }

    FD_ZERO( &stFdRead );

    for ( ;; )
    {
        FD_SET( pstCtx->iClientFd, &stFdRead );

        iRet = select( pstCtx->iClientFd + 1, &stFdRead, NULL, NULL, &stRdTimeOut );
        if ( iRet < 0 )
        {
            LOG_OUT(LOGOUT_ERROR, "read error, errno:%d, iRet:%d", errno, iRet);
            goto end;
        }
        //等待接收超时
        else if ( 0 == iRet )
        {
            pstCtx->uiCostTime ++;
            //LOG_OUT(LOGOUT_DEBUG, "select timeout, uiCostTime:%d", pstCtx->uiCostTime);

            if ( pstCtx->uiCostTime >= pstCtx->uiTimeOut )
            {
                //LOG_OUT(LOGOUT_DEBUG, "recv timeout closed", pstCtx->iClientFd);
                goto end;
            }
            continue;
        }

        if ( !FD_ISSET(pstCtx->iClientFd, &stFdRead ))
        {
            LOG_OUT(LOGOUT_ERROR, "stFdRead failed");
            goto end;
        }

        FD_CLR( pstCtx->iClientFd, &stFdRead );
        pstCtx->uiCostTime = 0;

        iRetN = recv( pstCtx->iClientFd, pcRecvBuf, WEB_RECVBUF_SIZE, 0 );
        //数据接收出错
        if ( iRetN <= 0 )
        {
            LOG_OUT(LOGOUT_DEBUG, "fd:%d recv failed, client closed");
            goto end;
        }
        pcRecvBuf[iRetN] = 0;
        //LOG_OUT(LOGOUT_DEBUG, "recv:\r\n%s\r\n\r\n", pcRecvBuf);
        //LOG_OUT(LOGOUT_DEBUG, "recv:%d", iRetN );

        iRet = HTTP_ParsingHttpHead( pstCtx, pcRecvBuf, iRetN );
        if ( iRet != OK )
        {
            LOG_OUT(LOGOUT_INFO, "fd:%d Parsing http header failed");
            goto end;
        }
        //LOG_OUT(LOGOUT_DEBUG, "ParsingHttpHead");

        iRet = HTTP_RouterHandle( pstCtx );
        if ( iRet != OK )
        {
            LOG_OUT(LOGOUT_INFO, "Router handle failed");
            goto end;
        }
        //LOG_OUT(LOGOUT_DEBUG, "HTTP_RouterHandle");
        //LOG_OUT(LOGOUT_INFO, "uiSentLen:%d, uiSendTotalLen:%d", pstCtx->stResp.uiSentLen, pstCtx->stResp.uiSendTotalLen);

        printf("%s:%d[%x]# pstConsoleCtx:0x%p\r\n", __func__, __LINE__, xTaskGetCurrentTaskHandle(),pstCtx);

        if ( HTTP_IS_SEND_FINISH( pstCtx ) )
        {
            LOG_OUT(LOGOUT_INFO, "[Response] Method:%s URL:%s Code:%s",
                    szHttpMethodStr[pstCtx->stReq.eMethod],
                    pstCtx->stReq.szURL,
                    szHttpCodeMap[pstCtx->stResp.eHttpCode]);
            HTTP_RequestInit( pstCtx );
        }
    }

end:
    //LOG_OUT(LOGOUT_INFO, "WEB_WebHandleTask over", pstCtx->iClientFd);
    WEB_CloseWebCtx( pstCtx );
    FREE_MEM( pcRecvBuf );
    vTaskDelete( NULL );
}


VOID WEB_StartHandleTheard( VOID *Para )
{
    xTaskCreate(WEB_WebHandleTask, "WEB_WebHandleTask", 2048, Para, 3, &xWebHandle);
}


UINT WEB_WebSend( HTTP_CTX *pstCtx )
{
    INT iRetN = 0;
    INT32 iRet = 0;
    struct timeval stTimeOut = {1, 0};
    fd_set stFdWrite;
    UINT8 ucRetry = 0;

    //LOG_OUT(LOGOUT_DEBUG, "WEB_WebSend..", pstCtx->iClientFd);

retry:

    FD_ZERO( &stFdWrite );
    FD_SET( pstCtx->iClientFd, &stFdWrite );

    iRet = select( pstCtx->iClientFd + 1, NULL, &stFdWrite, NULL, &stTimeOut );
    if ( iRet < 0 )
    {
        LOG_OUT(LOGOUT_ERROR, "select send error, errno:%d, iRet:%d", errno, iRet);
        FREE_MEM( pstCtx->stResp.pcResponBody );
        return FAIL;
    }
    else if ( 0 == iRet )
    {
        //LOG_OUT(LOGOUT_DEBUG, "fd:%d select timeout, uiCostTime:%d", pstCtx->iClientFd， pstCtx->uiCostTime);
        pstCtx->uiCostTime ++;
        if ( pstCtx->uiCostTime > pstCtx->uiTimeOut )
        {
        	FREE_MEM( pstCtx->stResp.pcResponBody );
            LOG_OUT(LOGOUT_ERROR, "closed select send timeout", pstCtx->iClientFd);
            return FAIL;
        }
    }

    if ( !FD_ISSET(pstCtx->iClientFd, &stFdWrite ))
    {
        ucRetry ++;
        if ( ucRetry >= 10 )
        {
        	FREE_MEM( pstCtx->stResp.pcResponBody );
            LOG_OUT(LOGOUT_ERROR, "FdWrite error", pstCtx->iClientFd);
            return FAIL;
        }
        LOG_OUT(LOGOUT_INFO, "FdWrite error", pstCtx->iClientFd);
        goto retry;
    }

    FD_CLR(pstCtx->iClientFd, &stFdWrite);
    pstCtx->uiCostTime = 0;

    iRet = send( pstCtx->iClientFd,
                 pstCtx->stResp.pcResponBody,
                 pstCtx->stResp.uiSendCurLen,
                 0 );
    if ( iRet < 0 )
    {
    	FREE_MEM( pstCtx->stResp.pcResponBody );
        LOG_OUT(LOGOUT_ERROR, "send error, pstCtx:%p,fd:%d, iRet:%d, errno:%d", pstCtx, pstCtx->iClientFd, iRet, errno);
        return FAIL;
    }
    else if ( iRet != pstCtx->stResp.uiSendCurLen )
    {
    	FREE_MEM( pstCtx->stResp.pcResponBody );
        LOG_OUT(LOGOUT_ERROR, "should send %d, but send %d actually",
                pstCtx->stResp.uiSendCurLen, iRet);
        return FAIL;
    }

    pstCtx->stResp.uiSentLen += pstCtx->stResp.uiSendCurLen;
    //LOG_OUT(LOGOUT_INFO, "send process:%d",
    //        pstCtx->stResp.uiSentLen * 100 / pstCtx->stResp.uiSendTotalLen);

    //LOG_OUT(LOGOUT_DEBUG, "WEB_WebSend over", pstCtx->iClientFd);
    FREE_MEM( pstCtx->stResp.pcResponBody );
    return OK;
}


VOID WEB_StopWebServerTheard( VOID )
{
    if ( xWebServerHandle != NULL )
    {
        bWebServerTaskTerminate = TRUE;
        while( bWebServerTaskTerminate == TRUE )
        {
            LOG_OUT(LOGOUT_DEBUG, "web server task stop...");
            vTaskDelay(1000/portTICK_RATE_MS);
        }
    }

    WEB_SetWebSvcStatus( FALSE );
    LOG_OUT(LOGOUT_INFO, "stop web server successed");
}


