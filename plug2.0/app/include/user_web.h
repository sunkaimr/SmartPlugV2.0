/*
 * user_web.h
 *
 *  Created on: 2018Äê11ÔÂ4ÈÕ
 *      Author: lenovo
 */

#ifndef __USER_WEB_H__
#define __USER_WEB_H__

#define WEB_RECVBUF_SIZE 	(1024)
#define WEB_SENDBUF_SIZE 	(1024)

#define WEB_MUX 			(1024)


#define WEB_MAX_FD 				4
#define WEB_CONTINUE_TMOUT		3


VOID WEB_StartWebServerTheard( VOID );
VOID WEB_StopWebServerTheard( VOID );

UINT8 WEB_GetWebSvcStatus( VOID );

#endif /* __USER_WEB_H__ */
