/*
 * user_web.h
 *
 *  Created on: 2018��11��4��
 *      Author: lenovo
 */

#ifndef __USER_WEB_H__
#define __USER_WEB_H__

#define WEB_RECVBUF_SIZE     (1024)    //���ջ�������С
#define WEB_SENDBUF_SIZE     (1024)    //���ͻ�������С


#define WEB_MAX_FD                2    //���ɽ������������������ݿ���ջ��С������,�����齫��ֵ���󣬳������и��õ��Ż���������ʡ�ڴ棩
#define WEB_CONTINUE_TMOUT        5    //���ӳ�ʱʱ�䣬������ʱ��δ�������ݽ����������Ͽ�����

VOID WEB_StartWebServerTheard( VOID );
VOID WEB_StopWebServerTheard( VOID );

UINT8 WEB_GetWebSvcStatus( VOID );



#endif /* __USER_WEB_H__ */
