/*
 * user_bigiot.h
 *
 *  Created on: 2019年8月4日
 *      Author: lenovo
 */

#ifndef __USER_BIGIOT_H__
#define __USER_BIGIOT_H__

#include "MQTTFreeRTOS.h"

#define BIGIOT_NONE       4
#define BIGIOT_ERROR      3
#define BIGIOT_INFO       2
#define BIGIOT_DEBUG      1

#define BIGIOT_LOG(lev, arg...) LOG_OUT(lev, ##arg)


#define BIGIOT_DEVNAME_LEN  64
#define BIGIOT_EVENT_NUM      10
#define BIGIOT_CBNAME_NUM      64

typedef char* (*CallbackFun)(void *para);

typedef struct tagBigiotEvent
{
    char*             pcIfId;                                //贝壳物联的接口ID
    char            szCbName[BIGIOT_CBNAME_NUM];        //回调函数名称
    CallbackFun     cb;                                    //回调函数
    void*             cbPara;                                //回调函数入参
    void*             pstCtx;                                //BIGIOT_Ctx_S

}BIGIOT_Event_S;

typedef struct tagBigiot
{
    char* pcHostName;                        //贝壳物联平台服务器的域名：www.bigiot.net
    int port;                                //贝壳物联平台服务器的端口，这里用8181
    char szDevName[BIGIOT_DEVNAME_LEN];        //对应贝壳物联的设备名称
    DEVTYPE_E eDevType;                        //设备类型
    char* pcDeviceId;                        //设备ID
    char* pcApiKey;                            //APIKEY

    int socket;                                //使用tcp协议连接的socket描述符
    int iTimeOut;                            //发送和接收的超时时间，超过该时间认为发送或接收失败

    xTaskHandle xEventHandle;                //处理注册事件任务的任务句柄
    int iAlived;                            //心跳成功标志，心跳失败时该标志会被清零

    BIGIOT_Event_S astEvent[BIGIOT_EVENT_NUM]; //心跳、开关状态、温度、湿度等事件会注册到这里

    int  (*Read)(struct tagBigiot*, unsigned char*, unsigned int, unsigned int); //数据接受函数
    int  (*Write)(struct tagBigiot*, const unsigned char*, unsigned int, unsigned int); //数据发送函数
    int (*Connect)(struct tagBigiot*);        //连接函数
    void (*Disconnect)(struct tagBigiot*);    //断开连接函数

}BIGIOT_Ctx_S;



BIGIOT_Ctx_S* Bigiot_New( char* pcHostName, int iPort, char* pcDevId, char* pcApiKey );
void BIGIOT_Destroy( BIGIOT_Ctx_S **ppstCtx );
int Bigiot_Login( BIGIOT_Ctx_S *pstCtx );
int Bigiot_Logout( BIGIOT_Ctx_S *pstCtx );
int Bigiot_Cycle( BIGIOT_Ctx_S *pstCtx );
int Bigiot_GetBigioStatus( void );
char* Bigiot_GetBigioDeviceName( void );

#endif /* __USER_BIGIOT_H__ */
