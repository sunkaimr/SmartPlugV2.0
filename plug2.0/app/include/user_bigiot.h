/*
 * user_bigiot.h
 *
 *  Created on: 2019Äê8ÔÂ4ÈÕ
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

typedef struct tagBigiot
{
    char* pcHostName;
    int port;
    char* pcDeviceId;
    char* pcApiKey;

    int socket;
    int iTimeOut;

    xTaskHandle xKeepLiveHandle;
    int iIsLived;
    int iBeatInterval;

    int  (*Read)(struct tagBigiot*, unsigned char*, unsigned int, unsigned int);
    int  (*Write)(struct tagBigiot*, const unsigned char*, unsigned int, unsigned int);
    int (*Connect)(struct tagBigiot*);
    void (*Disconnect)(struct tagBigiot*);

}BIGIOT_Ctx_S;


BIGIOT_Ctx_S* Bigiot_New( char* pcHostName, int iPort, char* pcDevId, char* pcApiKey );
void BIGIOT_Destroy( BIGIOT_Ctx_S **ppstCtx );
int Bigiot_Login( BIGIOT_Ctx_S *pstCtx );
int Bigiot_Logout( BIGIOT_Ctx_S *pstCtx );
int Bigiot_Cycle( BIGIOT_Ctx_S *pstCtx );
int Bigiot_KeepLive( BIGIOT_Ctx_S *pstCtx );


#endif /* __USER_BIGIOT_H__ */
