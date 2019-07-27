
#include "esp_common.h"
#include "user_common.h"
#include "mqtt/MQTTClient.h"



#define MQTT_BROKER  "a1OzgtZp5Ep.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define MQTT_PORT    1883

#define MQTT_SENDSIZE 256
#define MQTT_RECVSIZE 256


UINT MQTT_ParsePowerSwitchData( CHAR* pData );
UINT PLUG_MarshalJsonPowerSwitch( CHAR* pcBuf, UINT uiBufLen );

static CHAR* MQTT_GetMqttAddress( CHAR* pcAddr, UINT uiLen )
{
	snprintf( pcAddr, uiLen, "%s.iot-as-mqtt.cn-shanghai.aliyuncs.com", "a1OzgtZp5Ep");
	return pcAddr;
}

static CHAR* MQTT_GetMqttClientID( CHAR* pcClientId, UINT uiLen )
{
	snprintf( pcClientId, uiLen, "%s|securemode=3,signmethod=hmacsha1|", "test001");
	return pcClientId;
}

static CHAR* MQTT_GetMqttUserName( CHAR* pcUserName, UINT uiLen )
{
	snprintf( pcUserName, uiLen, "test001&a1OzgtZp5Ep");
	return pcUserName;
}

static CHAR* MQTT_GetMqttPassWord( CHAR* pcPassWord, UINT uiLen )
{
	snprintf( pcPassWord, uiLen, "B7DC8EF1D3D88AC9698F0420D79AB0833C1136A2");
	return pcPassWord;
}

static void MQTT_RecvMessage(MessageData* data)
{
	MQTT_ParsePowerSwitchData( data->message->payload);
}

static void MQTT_MqttClientTask(void* pvParameters)
{
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
	UINT uiRet = 0;
    MQTTClient client;
    Network network;
    CHAR *pcSendBuf = NULL;
    CHAR *pcRecvBuf = NULL;
    CHAR szAddr[50];
    CHAR szClientID[50];
    CHAR szUserName[50];
    CHAR szPassWord[50];
    CHAR payload[256];
    UINT uiRelayStatus = TRUE;
    UINT uiLastRelayStatus = FALSE;
    MQTTMessage message;
    static UINT8 MqttTaskRunningFlag = FALSE;

    LOG_OUT(LOGOUT_INFO, "mqtt client start");

    pcSendBuf = ( CHAR* )malloc( MQTT_SENDSIZE );
    if ( NULL == pcSendBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "malloc pcSendBuf failed.");
		goto end;
    }

    pcRecvBuf = ( CHAR* )malloc( MQTT_RECVSIZE );
    if ( NULL == pcRecvBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "malloc pcRecvBuf failed.");
		goto end;
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

	while ( 1 )
	{
		uiRet = MQTTConnect(&client, &connectData);
	    if ( uiRet == OK )
	    {
	    	LOG_OUT(LOGOUT_DEBUG, "MQTTConnect success");
	    	break;
	    }
	    LOG_OUT(LOGOUT_ERROR, "MQTTConnect failed, uiRet:%d, client.isconnected:%d", uiRet, client.isconnected);
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

	while ( 1 )
	{
	    uiRet = MQTTSubscribe(&client, "/sys/a1OzgtZp5Ep/test001/thing/service/property/set", QOS0, MQTT_RecvMessage);
	    if ( uiRet == OK )
	    {
	    	LOG_OUT(LOGOUT_DEBUG, "MQTTSubscribe success");
	    	break;
	    }
	    LOG_OUT(LOGOUT_ERROR, "MQTTSubscribe failed, uiRet:%d", uiRet);
    	vTaskDelay(1000 / portTICK_RATE_MS);
	}

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

			LOG_OUT(LOGOUT_DEBUG, "ready to MQTTPublish");
			uiRet = MQTTPublish(&client, "/sys/a1OzgtZp5Ep/test001/thing/event/property/post", &message);
			if ( uiRet != OK )
			{
				LOG_OUT(LOGOUT_ERROR, "MQTTPublish failed, uiRet:%d", uiRet);
				break;
			}
			LOG_OUT(LOGOUT_DEBUG, "MQTTPublish success");

			uiLastRelayStatus = uiRelayStatus;
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
    MQTTDisconnect( &client );
	NetworkDisconnect( &network );
	goto reConnect;

end:
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



