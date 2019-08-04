
#include "esp_common.h"
#include "user_common.h"
#include "mqtt/MQTTClient.h"

#include "ssl/ssl_crypto.h"

#define MQTT_PORT    	1883

#define MQTT_SENDSIZE 	256
#define MQTT_RECVSIZE 	256


UINT MQTT_ParsePowerSwitchData( CHAR* pData );
UINT PLUG_MarshalJsonPowerSwitch( CHAR* pcBuf, UINT uiBufLen );


static CHAR* MQTT_GetMqttAddress( CHAR* pcAddr, UINT uiLen )
{
	if ( pcAddr == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcAddr is NULL");
	    return NULL;
	}

	snprintf( pcAddr, uiLen, "%s.iot-as-mqtt.cn-shanghai.aliyuncs.com", PLUG_GetMqttProductKey());
	return pcAddr;
}

static CHAR* MQTT_GetMqttClientID( CHAR* pcClientId, UINT uiLen )
{
	CHAR ucMac[20];

	if ( pcClientId == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcClientId is NULL");
	    return NULL;
	}

	snprintf( pcClientId, uiLen, "%s|securemode=3,signmethod=hmacmd5|", WIFI_GetMacAddr(ucMac, sizeof(ucMac)));
	return pcClientId;
}

static CHAR* MQTT_GetMqttUserName( CHAR* pcUserName, UINT uiLen )
{
	snprintf( pcUserName, uiLen, "%s&%s", PLUG_GetMqttDevName(), PLUG_GetMqttProductKey());
	return pcUserName;
}

static CHAR* MQTT_GetMqttPassWord( CHAR* pcPassWord, UINT uiLen )
{
	UINT8 i = 0;
	UINT8 uiPos = 0;
	CHAR digest[16] = {0};
	CHAR message[100];
	CHAR ucMac[20];

	if ( pcPassWord == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcPassWord is NULL");
	    return NULL;
	}

	if ( uiLen < 33 )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcPassWord size not enough");
	    return NULL;
	}

	snprintf(message, sizeof(message), "clientId%sdeviceName%sproductKey%s",
			 WIFI_GetMacAddr(ucMac, sizeof(ucMac)),
			 PLUG_GetMqttDevName(),
			 PLUG_GetMqttProductKey());

	ssl_hmac_md5( message, strlen(message), (UINT8*)PLUG_GetMqttDevSecret(), PLUG_GetMqttDevSecretLenth(), digest);

	for ( i = 0; i < 16; i++ )
	{
		uiPos += snprintf( (pcPassWord + uiPos), uiLen, "%02X", digest[i]);
	}

	return pcPassWord;
}

static void MQTT_RecvMessage(MessageData* data)
{
	//LOG_OUT(LOGOUT_INFO, "MQTTRecv, topic:%s",  data->topicName->cstring);
	MQTT_ParsePowerSwitchData( data->message->payload);
}

static void MQTT_MqttClientTask(void* pvParameters)
{
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
	UINT uiRet = 0;
	UINT uiRetry = 0;
    MQTTClient client;
    Network network;
    CHAR *pcSendBuf = NULL;
    CHAR *pcRecvBuf = NULL;
    CHAR szAddr[50];
    CHAR szClientID[50];
    CHAR szUserName[50];
    CHAR szPassWord[50];
    CHAR payload[256];
    CHAR szSubscribeTopic[80];
    CHAR szPublishTopic[80];
    UINT uiRelayStatus = TRUE;
    UINT uiLastRelayStatus = FALSE;
    MQTTMessage message;

    LOG_OUT(LOGOUT_INFO, "mqtt client start");

    pcSendBuf = ( CHAR* )malloc( MQTT_SENDSIZE );
    if ( NULL == pcSendBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "malloc pcSendBuf failed.");
		goto exit;
    }

    pcRecvBuf = ( CHAR* )malloc( MQTT_RECVSIZE );
    if ( NULL == pcRecvBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "malloc pcRecvBuf failed.");
		goto exit;
    }

    NetworkInit(&network);
    MQTTClientInit(&client, &network, 3000, pcSendBuf, MQTT_SENDSIZE, pcRecvBuf, MQTT_RECVSIZE);

reConnect:

	//等待wifi连接就绪
	while ( STATION_GOT_IP != wifi_station_get_connect_status() )
	{
		LOG_OUT(LOGOUT_DEBUG, "wait for connect");
		vTaskDelay(1000 / portTICK_RATE_MS);
	}

	while ( 1 )
	{
	    uiRet = NetworkConnect(&network, MQTT_GetMqttAddress(szAddr, sizeof(szAddr)), MQTT_PORT);
	    if ( uiRet == OK )
	    {
	    	LOG_OUT(LOGOUT_DEBUG, "NetworkConnect success");
	    	break;
	    }
    	LOG_OUT(LOGOUT_ERROR, "NetworkConnect failed, uiRet:%d", uiRet);
    	vTaskDelay(1000 / portTICK_RATE_MS);
	}

    connectData.keepAliveInterval = 30;
    connectData.MQTTVersion = 3;
    connectData.clientID.cstring = MQTT_GetMqttClientID(szClientID, sizeof(szClientID));
    connectData.username.cstring = MQTT_GetMqttUserName(szUserName, sizeof(szUserName));
    connectData.password.cstring = MQTT_GetMqttPassWord(szPassWord, sizeof(szPassWord));
    connectData.cleansession = TRUE;

    //LOG_OUT(LOGOUT_DEBUG, "clientID:%s", connectData.clientID.cstring);
    //LOG_OUT(LOGOUT_DEBUG, "username:%s", connectData.username.cstring);
    //LOG_OUT(LOGOUT_DEBUG, "password:%s", connectData.password.cstring);

    uiRetry = 0;
	while ( 1 )
	{
		uiRet = MQTTConnect(&client, &connectData);
	    if ( uiRet == OK )
	    {
	    	LOG_OUT(LOGOUT_DEBUG, "MQTTConnect success");
	    	break;
	    }

    	LOG_OUT(LOGOUT_DEBUG, "MQTTConnect failed, uiRet:%d, retry %d times", uiRet, uiRetry);
    	if ( uiRetry++ >= 5 )
    	{
    		LOG_OUT(LOGOUT_ERROR, "MQTTConnect failed, uiRet:%d, retry %d times", uiRet, uiRetry);
    		goto exit;
    	}
    	vTaskDelay(1000 / portTICK_RATE_MS);
	}

#ifdef MQTT_TASK
	uiRet = MQTTStartTask(&client);
	if ( uiRet != TRUE )
	{
		LOG_OUT(LOGOUT_ERROR, "MQTTStartTask failed, uiRet:%d", uiRet);
		goto end;
	}
	LOG_OUT(LOGOUT_DEBUG, "MQTTStartTask success");
#endif

	snprintf( szSubscribeTopic, sizeof(szSubscribeTopic), "/sys/%s/%s/thing/service/property/set",
			  PLUG_GetMqttProductKey(),
			  PLUG_GetMqttDevName());
	uiRet = MQTTSubscribe(&client, szSubscribeTopic, QOS0, MQTT_RecvMessage);
	if ( uiRet != OK )
	{
		LOG_OUT(LOGOUT_ERROR, "MQTTSubscribe %s failed, uiRet:%d", szSubscribeTopic, uiRet);
		goto end;
	}
	LOG_OUT(LOGOUT_INFO, "MQTTSubscribe %s success", szSubscribeTopic);

	for (;;)
	{
		if ( STATION_GOT_IP != wifi_station_get_connect_status() )
		{
			LOG_OUT(LOGOUT_INFO, "wifi has disconnected");
			break;
		}

	    if ( keepalive(&client) != SUCCESS)
	    {
	        LOG_OUT(LOGOUT_INFO, "keepalive failed");
	        break;
	    }

		uiRelayStatus = PLUG_GetRelayStatus();
		if ( uiRelayStatus != uiLastRelayStatus )
		{
			message.qos = QOS0;
			message.retained = 0;
			message.payloadlen = PLUG_MarshalJsonPowerSwitch(payload, sizeof(payload));
			message.payload = payload;

			snprintf( szPublishTopic, sizeof(szPublishTopic), "/sys/%s/%s/thing/event/property/post",
					  PLUG_GetMqttProductKey(),
					  PLUG_GetMqttDevName());
			uiRet = MQTTPublish(&client, szPublishTopic, &message);
			if ( uiRet != OK )
			{
				LOG_OUT(LOGOUT_ERROR, "MQTTPublish %s failed, uiRet:%d", szPublishTopic, uiRet);
				break;
			}
			LOG_OUT(LOGOUT_INFO, "MQTTPublish %s success", szPublishTopic);

			uiLastRelayStatus = uiRelayStatus;
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
	}

end:
    MQTTDisconnect( &client );
	NetworkDisconnect( &network );
	goto reConnect;

exit:
    LOG_OUT(LOGOUT_INFO, "MQTT_MqttClientTask stop");
    FREE_MEM( pcSendBuf );
    FREE_MEM( pcRecvBuf );
    vTaskDelete(NULL);
    return;
}

void MQTT_StartMqttTheard(void)
{
    xTaskCreate( MQTT_MqttClientTask, "MQTT_MqttClientTask", 2048, NULL, 5, NULL);
}


UINT MQTT_ParsePowerSwitchData( CHAR* pData )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonIteam = NULL;
	UINT8 PowerSwitch = 0;

	if ( pData == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pData is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pData );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pData);
	    goto error;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "params");
	if (pJsonIteam && pJsonIteam->type == cJSON_Object)
	{
		pJsonIteam = cJSON_GetObjectItem( pJsonIteam, "PowerSwitch");
		if (pJsonIteam && pJsonIteam->type == cJSON_Number)
		{
			if ( pJsonIteam->valueint == 1 )
			{
				PLUG_SetRelayByStatus( TRUE, TRUE );
				LOG_OUT(LOGOUT_INFO, "PowerSwitch:1");
			}
			else if ( pJsonIteam->valueint == 0 )
			{
				PLUG_SetRelayByStatus( FALSE, TRUE );
				LOG_OUT(LOGOUT_INFO, "PowerSwitch:0");
			}
			else
			{
			    LOG_OUT(LOGOUT_ERROR, "unknow PowerSwitch:%d", pJsonIteam->valueint);
			    goto error;
			}
		}
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}


UINT PLUG_MarshalJsonPowerSwitch( CHAR* pcBuf, UINT uiBufLen )
{
	cJSON  *pJson, *pJsonsub = NULL;
	CHAR *pJsonStr = NULL;

	pJson = cJSON_CreateObject();

	cJSON_AddStringToObject( pJson, 	"id", 		"123");
	cJSON_AddStringToObject( pJson, 	"version", 	"1.0");
	cJSON_AddStringToObject( pJson, 	"method", 	"thing.service.property.set");

	pJsonsub = cJSON_CreateObject();
	cJSON_AddNumberToObject( pJsonsub, 	"PowerSwitch", 	PLUG_GetRelayStatus());
	cJSON_AddItemToObject(pJson, "params", pJsonsub);

    pJsonStr = cJSON_PrintUnformatted(pJson);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJson);
    FREE_MEM(pJsonStr);

	return strlen(pcBuf);
}



