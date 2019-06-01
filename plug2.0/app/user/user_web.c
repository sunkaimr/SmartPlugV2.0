/*
 * user_web.c
 *
 *  Created on: 2018年11月4日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"


xTaskHandle xWebServerHandle = NULL;
xTaskHandle xWebRecvHandle = NULL;
xTaskHandle xWebSendHandle = NULL;

INT32 iSocketFd = -1;
WEB_CLISKTFD_S stClientSktfd[WEB_MAX_FD];

CHAR* pcRecvBuf = NULL;

BOOL bIsWebSvcRunning = FALSE;

BOOL bSendTaskTerminate = FALSE;
BOOL bRecvTaskTerminate = FALSE;
BOOL bWebServerTaskTerminate = FALSE;


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


STATIC VOID WEB_WebSendTask( VOID *Para )
{
	INT32 iLoop = 0;
    INT iRetN = 0;
    INT32 iRet = 0;
    struct timeval stTimeOut = {1, 0};
	fd_set stFdWrite;


	LOG_OUT(LOGOUT_INFO, "WEB_WebSendTask started.");

	FD_ZERO( &stFdWrite );

	for ( ;; )
	{
		while ( 1 )
		{
			for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
			{
				if ( stClientSktfd[iLoop].iClientFd == -1 && stClientSktfd[iLoop].stReqHead.pcResponBody )
				{
					FREE_MEM(stClientSktfd[iLoop].stReqHead.pcResponBody);
				}

				if ( stClientSktfd[iLoop].stReqHead.bIsCouldSend )
				{
					LOG_OUT(LOGOUT_DEBUG, "bIsCouldSend iLoop:%d fd:%d", iLoop, stClientSktfd[iLoop].iClientFd);
					break;
				}
			}

			if ( bSendTaskTerminate == TRUE )
			{
				goto end;
			}

			if ( iLoop < WEB_MAX_FD )
			{
				break;
			}
			vTaskDelay(10/portTICK_RATE_MS);
		}

		for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
		{
			if (stClientSktfd[iLoop].iClientFd > 0 )
			{
		    	FD_SET( stClientSktfd[iLoop].iClientFd, &stFdWrite );
			}
		}

		iRet = select( WEB_MAX_FD+1, NULL, &stFdWrite, NULL, &stTimeOut );
		if ( iRet < 0 )
		{
			LOG_OUT(LOGOUT_ERROR, "select write error, errno:%d, iRet:%d.", errno, iRet);
			vTaskDelay(1000/portTICK_RATE_MS);

			for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
			{
				if (stClientSktfd[iLoop].iClientFd > 0 )
				{
					close( stClientSktfd[iLoop].iClientFd );

					stClientSktfd[iLoop].iClientFd = -1;
					stClientSktfd[iLoop].uiElapsedTime = 0;
					FREE_MEM(stClientSktfd[iLoop].stReqHead.pcResponBody);
					memset(&stClientSktfd[iLoop].stReqHead, 0 , sizeof(HTTP_REQUEST_HEAD_S));
				}
			}


			continue;
		}
		else if ( 0 == iRet )
		{
			LOG_OUT(LOGOUT_DEBUG, "WEB_WebSendTask, select timeout.");

			for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
			{
				if ( stClientSktfd[iLoop].iClientFd < 0 )
				{
					continue;
				}

				stClientSktfd[iLoop].uiElapsedTime ++;
				if ( stClientSktfd[iLoop].uiElapsedTime > stClientSktfd[iLoop].uiTimeOut )
				{
					LOG_OUT(LOGOUT_DEBUG, "select timeout, closed iClientFd:%d.", stClientSktfd[iLoop].iClientFd);

					close( stClientSktfd[iLoop].iClientFd );

					stClientSktfd[iLoop].iClientFd = -1;
					stClientSktfd[iLoop].uiElapsedTime = 0;
					FREE_MEM(stClientSktfd[iLoop].stReqHead.pcResponBody);
					memset(&stClientSktfd[iLoop].stReqHead, 0 , sizeof(HTTP_REQUEST_HEAD_S));
				}
			}
			continue;
		}

		LOG_OUT(LOGOUT_DEBUG, "WEB_WebSendTask, select ok");

		for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
		{
			if ( stClientSktfd[iLoop].iClientFd < 0 )
			{
				continue;
			}

			if ( FD_ISSET(stClientSktfd[iLoop].iClientFd, &stFdWrite ))
			{
				FD_CLR(stClientSktfd[iLoop].iClientFd, &stFdWrite);
				stClientSktfd[iLoop].uiElapsedTime = 0;

				if ( ! stClientSktfd[iLoop].stReqHead.bIsCouldSend )
				{
					continue;
				}

				LOG_OUT(LOGOUT_DEBUG, "WEB_WebSendTask, send... iClientFd:%d", stClientSktfd[iLoop].iClientFd);

				iRet = send( stClientSktfd[iLoop].iClientFd,
							 stClientSktfd[iLoop].stReqHead.pcResponBody,
							 stClientSktfd[iLoop].stReqHead.uiSendCurLength,
							 0 );
				FREE_MEM( stClientSktfd[iLoop].stReqHead.pcResponBody );
				//LOG_OUT(LOGOUT_DEBUG, "WEB_WebSendTask, send over, iClientFd:%d", stClientSktfd[iLoop].iClientFd);
				//发送出错
				if ( iRet < 0 )
				{
					if ( errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN )
					{
						LOG_OUT(LOGOUT_DEBUG, "WEB_WebSendTask, errno:%d.", errno);
						continue;
					}
					LOG_OUT(LOGOUT_ERROR, "send error, close connect, iClientFd:%d.", stClientSktfd[iLoop].iClientFd);

					close( stClientSktfd[iLoop].iClientFd );
					stClientSktfd[iLoop].iClientFd = -1;
					stClientSktfd[iLoop].uiElapsedTime = 0;
					memset(&stClientSktfd[iLoop].stReqHead, 0, sizeof(HTTP_REQUEST_HEAD_S));

					continue;
				}
				//发送长度不足
				else if ( iRet != stClientSktfd[iLoop].stReqHead.uiSendCurLength )
				{
					LOG_OUT(LOGOUT_ERROR, "WEB_WebSendTask should send %d, but send %d actually.",
							stClientSktfd[iLoop].stReqHead.uiSendCurLength, iRet);
				}

				stClientSktfd[iLoop].stReqHead.bIsCouldSend = FALSE;
				stClientSktfd[iLoop].stReqHead.uiSentLength += stClientSktfd[iLoop].stReqHead.uiSendCurLength;
				LOG_OUT(LOGOUT_INFO, "ClientFd:%d, send process:%d.", stClientSktfd[iLoop].iClientFd, stClientSktfd[iLoop].stReqHead.uiSentLength * 100 / stClientSktfd[iLoop].stReqHead.uiSendTotalLength);

				//发送完成
				if ( stClientSktfd[iLoop].stReqHead.uiSentLength >= stClientSktfd[iLoop].stReqHead.uiSendTotalLength )
				{
					LOG_OUT(LOGOUT_DEBUG, "send finished.");
					stClientSktfd[iLoop].uiElapsedTime = 0;
					memset(&stClientSktfd[iLoop].stReqHead, 0, sizeof(HTTP_REQUEST_HEAD_S));
				}
			}
		}
	}

end:
	for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
	{
		if ( stClientSktfd[iLoop].iClientFd >= 0 )
		{
			LOG_OUT(LOGOUT_DEBUG, "WEB_StopWebServerTheard iClientFd has closed. Fd:%d", stClientSktfd[iLoop].iClientFd);
			close(stClientSktfd[iLoop].iClientFd);
		}
		FREE_MEM( stClientSktfd[iLoop].stReqHead.pcResponBody );
	}
	bSendTaskTerminate = FALSE;
	LOG_OUT(LOGOUT_INFO, "WEB_WebSendTask stopped.");
    vTaskDelete( NULL );
}


VOID WEB_StartWebSendTheard( VOID )
{
	xTaskCreate(WEB_WebSendTask, "WEB_WebSendTask", 1024, NULL, 2, &xWebSendHandle);//512, 376 left,136 used
}

STATIC VOID WEB_WebRecvTask( VOID *Para )
{
	INT32 iLoop = 0;
    INT iRetN = 0;
    INT32 iRet = 0;
    struct timeval stTimeOut = {1, 0};
	fd_set stFdRead;


	LOG_OUT(LOGOUT_INFO, "WEB_WebRecvTask started.");

	bRecvTaskTerminate = FALSE;

	HTTP_RouterInit();

	pcRecvBuf = ( CHAR* )malloc( USER_RECVBUF_SIZE );
    if ( NULL == pcRecvBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "malloc pcRecvBuf failed.");
		return;
    }

	FD_ZERO( &stFdRead );

	for ( ;; )
	{
		if ( bRecvTaskTerminate == TRUE )
		{
			goto end;
		}

		for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
		{
			if (stClientSktfd[iLoop].iClientFd > 0 )
			{
		    	FD_SET( stClientSktfd[iLoop].iClientFd, &stFdRead );
			}
		}

		iRet = select( WEB_MAX_FD+1, &stFdRead, NULL, NULL, &stTimeOut );
		if ( iRet < 0 )
		{
			LOG_OUT(LOGOUT_ERROR, "select read error, errno:%d, iRet:%d.", errno, iRet);
			vTaskDelay(1000/portTICK_RATE_MS);
			continue;
		}
		else if ( 0 == iRet )
		{
			LOG_OUT(LOGOUT_DEBUG, "WEB_WebRecvTask, select timeout.");

			for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
			{
				if ( stClientSktfd[iLoop].iClientFd < 0 )
				{
					continue;
				}

				stClientSktfd[iLoop].uiElapsedTime ++;
				LOG_OUT(LOGOUT_DEBUG, "select timeout, Fd:%d, uiElapsedTime:%d", stClientSktfd[iLoop].iClientFd, stClientSktfd[iLoop].uiElapsedTime);

				if ( stClientSktfd[iLoop].uiElapsedTime >= stClientSktfd[iLoop].uiTimeOut )
				{
					LOG_OUT(LOGOUT_INFO, "select timeout, closed iClientFd:%d.", stClientSktfd[iLoop].iClientFd);

					close( stClientSktfd[iLoop].iClientFd );

					stClientSktfd[iLoop].iClientFd = -1;
					stClientSktfd[iLoop].uiElapsedTime = 0;
					FREE_MEM(stClientSktfd[iLoop].stReqHead.pcResponBody);
					memset(&stClientSktfd[iLoop].stReqHead, 0 , sizeof(HTTP_REQUEST_HEAD_S));
				}
			}
			continue;
		}

		LOG_OUT(LOGOUT_DEBUG, "WEB_WebRecvTask, select ok");

		for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
		{
			if ( stClientSktfd[iLoop].iClientFd < 0 )
			{
				continue;
			}

			LOG_OUT(LOGOUT_DEBUG, "Judege Fd:%d can read", stClientSktfd[iLoop].iClientFd);
			if ( FD_ISSET(stClientSktfd[iLoop].iClientFd, &stFdRead ))
			{
				LOG_OUT(LOGOUT_DEBUG, "Fd:%d can recv", stClientSktfd[iLoop].iClientFd);

				FD_CLR(stClientSktfd[iLoop].iClientFd, &stFdRead);
				stClientSktfd[iLoop].uiElapsedTime = 0;

				iRetN = recv( stClientSktfd[iLoop].iClientFd, pcRecvBuf, USER_RECVBUF_SIZE, 0 );
				if ( iRetN > 0 )
				{
					pcRecvBuf[iRetN] = 0;

					//LOG_OUT(LOGOUT_DEBUG, "recv:\r\n%s\r\n\r\n", pcRecvBuf);
					//printf("\r\n");
					//LOG_OUT(LOGOUT_DEBUG, "recv:%d, Fd:%d", iRetN, stClientSktfd[iLoop].iClientFd);
					HTTP_ParsingHttpHead(pcRecvBuf, iRetN, &stClientSktfd[iLoop].stReqHead );

					HTTP_RouterHandle(&stClientSktfd[iLoop].stReqHead);
				}
				else
				{
					if ( errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN )
					{
						LOG_OUT(LOGOUT_DEBUG, "WEB_WebRecvTask, errno:%d.", errno);
						continue;
					}
					LOG_OUT(LOGOUT_INFO, "ClientFd:%d has disconnect.", stClientSktfd[iLoop].iClientFd);

					close( stClientSktfd[iLoop].iClientFd );
					stClientSktfd[iLoop].iClientFd = -1;
					stClientSktfd[iLoop].uiElapsedTime = 0;
					FREE_MEM(stClientSktfd[iLoop].stReqHead.pcResponBody);
					memset(&stClientSktfd[iLoop].stReqHead, 0, sizeof(HTTP_REQUEST_HEAD_S));
				}
			}
		}

	}
end:
	LOG_OUT(LOGOUT_INFO, "WEB_WebRecvTask stopped.");
	FREE_MEM(pcRecvBuf);
	bRecvTaskTerminate = FALSE;
    vTaskDelete( NULL );
}


VOID WEB_StartWebRecvTheard( VOID )
{
	xTaskCreate(WEB_WebRecvTask, "WEB_WebRecvTask", 1024, NULL, 3, &xWebRecvHandle);//512, 376 left,136 used
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
    LOG_OUT(LOGOUT_DEBUG, "socket ok, iSocketFd:%d.", iSocketFd);

	//不知道为什么一直失败
	//iRet = setsockopt(iSocketFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&Reuseaddr, sizeof(Reuseaddr));
    //iRet = setsockopt(iSocketFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&stRecvTimeOut, sizeof(stRecvTimeOut));
    //if ( 0 != iRet )
	//{
	//	LOG_OUT(LOGOUT_ERROR, "setsockopt failed, iRet:%d.", iRet);
	//}
	//LOG_OUT(LOGOUT_DEBUG, "setsockopt ok, iSocketFd:%d.", iSocketFd);

    do
	{
        iRet = bind( iSocketFd, (struct sockaddr *)&stServerAddr, sizeof(stServerAddr));
		if ( 0 != iRet )
		{
			LOG_OUT(LOGOUT_ERROR, "bind failed, iRet:%d.", iRet);
            vTaskDelay(1000/portTICK_RATE_MS);
        }
    } while ( iRet != 0 );

    LOG_OUT(LOGOUT_DEBUG, "bind ok, iSocketFd:%d.", iSocketFd);

	do
	{
		iRet = listen( iSocketFd, WEB_MAX_FD );
		if (iRet != 0)
		{
			vTaskDelay(1000/portTICK_RATE_MS);
		}
	} while ( iRet != 0 );
	//LOG_OUT(LOGOUT_DEBUG, "listen ok, iSocketFd:%d.", iSocketFd);

	memset(&stClientSktfd, 0, sizeof(stClientSktfd));
	for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
	{
		stClientSktfd[iLoop].iClientFd = -1;
		stClientSktfd[iLoop].uiTimeOut	= WEB_CONTINUE_TMOUT;
		stClientSktfd[iLoop].uiElapsedTime = 0;
	}

	FD_ZERO( &stFdRead );
	FD_SET( iSocketFd, &stFdRead );

	WEB_StartWebRecvTheard();
	WEB_StartWebSendTheard();

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
			LOG_OUT(LOGOUT_ERROR, "select accept error, errno:%d, iRet:%d.", errno, iRet);
			vTaskDelay(1000/portTICK_RATE_MS);

			continue;
		}
		else if ( 0 == iRet || errno == EINTR )
		{
			LOG_OUT(LOGOUT_DEBUG, "WEB_WebServerTask, select timeout.");
			vTaskDelay(100/portTICK_RATE_MS);
			continue;
		}
		
		LOG_OUT(LOGOUT_DEBUG, "WEB_WebServerTask, select ok, iRet:%d.", iRet);

		if ( FD_ISSET(iSocketFd, &stFdRead ))
		{
			LOG_OUT(LOGOUT_DEBUG, "accept...");
			iClientFd = accept(iSocketFd, (struct sockaddr *)&stClientAddr, (socklen_t *)&iClientAddrLen);
			if ( -1 != iClientFd )
			{
				FD_CLR(iSocketFd, &stFdRead);
				
				for ( iLoop = 0; iLoop < WEB_MAX_FD; iLoop++ )
				{
					if ( stClientSktfd[iLoop].iClientFd < 0 )
					{
						stClientSktfd[iLoop].iClientFd = iClientFd;
						LOG_OUT(LOGOUT_INFO, "ClientFd:%d has connect.", iClientFd);
						break;
					}
				}
			}
			else
			{
				LOG_OUT(LOGOUT_ERROR, "accept error, ClientFd:%d", iClientFd);
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
	xTaskCreate(WEB_WebServerTask, "WEB_WebServerTask", 1024, NULL, 4, &xWebServerHandle);//512, 376 left,136 used
}



VOID WEB_StopWebServerTheard( VOID )
{
	if ( xWebRecvHandle != NULL )
	{
		bRecvTaskTerminate = TRUE;
		while(bRecvTaskTerminate == TRUE)
		{
			LOG_OUT(LOGOUT_DEBUG, "WEB_WebRecvTask stop...");
			vTaskDelay(1000/portTICK_RATE_MS);
		}
		LOG_OUT(LOGOUT_INFO, "WEB_WebRecvTask stop successed.");
	}

	if ( xWebSendHandle != NULL )
	{
		bSendTaskTerminate = TRUE;
		while(bSendTaskTerminate == TRUE)
		{
			LOG_OUT(LOGOUT_DEBUG, "WEB_WebSendTask stop...");
			vTaskDelay(1000/portTICK_RATE_MS);
		}
		LOG_OUT(LOGOUT_INFO, "WEB_WebSendTask stop successed.");
	}

	if ( xWebServerHandle != NULL )
	{
		bWebServerTaskTerminate = TRUE;
		while(bWebServerTaskTerminate == TRUE)
		{
			LOG_OUT(LOGOUT_DEBUG, "WEB_WebServerTask stop...");
			vTaskDelay(1000/portTICK_RATE_MS);
		}
		LOG_OUT(LOGOUT_INFO, "WEB_WebServerTask stop successed.");
	}

	WEB_SetWebSvcStatus( FALSE );
	LOG_OUT(LOGOUT_INFO, "WEB_StopWebServerTheard successed.");
}


