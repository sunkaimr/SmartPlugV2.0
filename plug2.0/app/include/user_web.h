/*
 * user_web.h
 *
 *  Created on: 2018年11月4日
 *      Author: lenovo
 */

#ifndef __USER_WEB_H__
#define __USER_WEB_H__

#define WEB_RECVBUF_SIZE     (1024)    //接收缓冲区大小
#define WEB_SENDBUF_SIZE     (1024)    //发送缓冲区大小


#define WEB_MAX_FD                5    //最大可接受连接数并发（根据空闲栈大小来调整,不建议将该值调大，除非你有更好的优化方案来节省内存）
#define WEB_CONTINUE_TMOUT        1    //连接超时时间，超过该时间未进行数据交互将主动断开连接

VOID WEB_StartWebServerTheard( VOID );
VOID WEB_StopWebServerTheard( VOID );

UINT8 WEB_GetWebSvcStatus( VOID );



#endif /* __USER_WEB_H__ */
