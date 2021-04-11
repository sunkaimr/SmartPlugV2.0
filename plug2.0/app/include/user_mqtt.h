/*
 * user_mqtt.h
 *
 *  Created on: 2019年8月4日
 *      Author: lenovo
 */

#ifndef __USER_MQTT_H__
#define __USER_MQTT_H__


#include "user_common.h"
#include "mqtt/MQTTClient.h"

#define MQTT_ADDR_LEN      		64
#define MQTT_CLIENTID_LEN  		64
#define MQTT_USERNAME_LEN  		64
#define MQTT_PASSWD_LEN    		64
#define MQTT_PASSWD_LEN    		64
#define MQTT_TOKEN_LEN     		40
#define MQTT_SOFTWAREVER_LEN    10
#define MQTT_TOPIC_NUM          3    // 最大不要超过 MAX_MESSAGE_HANDLERS, 如果确实需要必须修改MAX_MESSAGE_HANDLERS这个宏然后重新编译mqtt的三方库
#define MQTT_TOPIC_LEN          100
#define MQTT_URL_LEN            500  // 新固件的下载地址

typedef enum tagMQTT_UPGRADE_STATUS{
	MQTT_UPGRADE_IDEA = 0,            // 空闲状态
	MQTT_UPGRADE_DOWNLOADING,         // 正在下载固件
	MQTT_UPGRADE_BURNING,             // 正在烧写固件
	MQTT_UPGRADE_DONE,                // 升级完成
	MQTT_UPGRADE_TIMEOUT,             // 升级超时

	MQTT_UPGRADE_BUFF,
}MQTT_UPGRADE_STATUS;

typedef enum{
	MQTT_CONSTATUS_Unknown = 0,
	MQTT_CONSTATUS_Connectting,
	MQTT_CONSTATUS_Connected,
	MQTT_CONSTATUS_Failed,

	MQTT_CONSTATUS_Buff,
}MQTT_CONSTATUS_E;

typedef struct tagMQTT_CTX{
	MQTTPacket_connectData      stConnectData;
	Network 					stNetwork;
	MQTTClient 					stClient;
	UINT                        uiTimeOut;       	// 命令发送超时间，单位ms
	UINT						uiRecvBufSize;	    // 接收缓冲区大小
	UINT						uiSendBufSize;	    // 发送缓冲区大小
	CHAR*                       pcRecvBuf;			// 接收缓冲区
	CHAR*						pcSendBuf;          // 发送缓冲区

	INT                         iMqttPort;         // mqtt服务器端口
	CHAR						szMqttAddr[MQTT_ADDR_LEN];  		// mqtt服务器地址
	CHAR						szClientID[MQTT_CLIENTID_LEN];  	// mqtt clientID
	CHAR						szUserName[MQTT_USERNAME_LEN];  	// mqtt username
	CHAR						szPassWord[MQTT_PASSWD_LEN];  		// mqtt password
	CHAR                        szToken[MQTT_TOKEN_LEN];            // smartconfig配网时的token

	CHAR                        szCurSoftWareVer[MQTT_SOFTWAREVER_LEN];      // 当前运行的固件版本
	CHAR                        szUpdateSoftWareVer[MQTT_SOFTWAREVER_LEN];   // 要升级的固件版本
	CHAR                        szSoftWareUrl[MQTT_URL_LEN];                 // 固件下载地址
	UINT                        uiSoftWareSize;                              // 固件大小
	UINT                        uiDownloadSize;                              // 已下载固件大小
	MQTT_UPGRADE_STATUS         enUpgradeStatus;                             // 当前升级的状态
	INT                         iDownloadProcess;                            // 固件下载进度

	MQTT_CONSTATUS_E            eConnectStatus;                             // 是否连接到服务器
	CHAR                        aszSubscribeTopic[MQTT_TOPIC_NUM][MQTT_TOPIC_LEN];// 存放订阅的主题

}MQTT_CTX;

typedef struct tagWriteSoftWarePara{
	HTTP_CLIENT_S* pstCli;
	MQTT_CTX* pstMqttCtx;
}MQTT_WriteSoftWarePara_S;


#endif /* __USER_MQTT_H__ */
