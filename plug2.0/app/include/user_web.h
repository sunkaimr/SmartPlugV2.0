/*
 * user_web.h
 *
 *  Created on: 2018Äê11ÔÂ4ÈÕ
 *      Author: lenovo
 */

#ifndef __USER_WEB_H__
#define __USER_WEB_H__

#include "user_http.h"

#define USER_RECVBUF_SIZE 	(1024*4 + 10)
#define USER_SENDBUF_SIZE 	(1024*5 + 10)

#define WEB_MAX_FD 				8
#define WEB_CONTINUE_TMOUT		10


typedef struct tagWebCliSktFd
{
	INT						iClientFd;
	UINT					uiTimeOut;
	UINT					uiElapsedTime;
	HTTP_REQUEST_HEAD_S     stReqHead;

}WEB_CLISKTFD_S;




VOID WEB_StartWebServerTheard( VOID );
VOID WEB_StartWebRecvTheard( VOID );
VOID WEB_StartWebSendTheard( VOID );
VOID WEB_StopWebServerTheard( VOID );

UINT8 WEB_GetWebSvcStatus( VOID );

#endif /* __USER_WEB_H__ */
