#include "esp_common.h"
#include "user_common.h"

#include "ssl/ssl_crypto.h"
#include "base64.h"
//#include "ssl/ssl_os_port.h"
//#include "ssl/ssl_ssl.h"

#define MQTT_PORT         1883

#define MQTT_SENDSIZE     512
#define MQTT_RECVSIZE     1024

MQTT_CTX *pstMqttCtx = NULL;

static void ReportSmartConfigResult(MQTT_CTX *pstCtx);
static void MQTT_MqttClientTask(void* pvPara);
static CHAR* MQTT_GetMqttAddress( CHAR* pcAddr, UINT uiLen );
static CHAR* MQTT_GetMqttClientID( CHAR* pcClientId, UINT uiLen );
static CHAR* MQTT_GetMqttUserName( CHAR* pcUserName, UINT uiLen );
static CHAR* MQTT_GetMqttPassWord( CHAR* pcPassWord, UINT uiLen );
UINT MQTT_ParseDeviceSecret(VOID *pPara);
void MQTT_StartHandleUpgradeTheard(MessageData* data);
static void MQTT_HandleServerCommond(MessageData* pstMsg);
UINT PLUG_MarshalJsonDevStatus( CHAR* pcBuf, UINT uiBufLen );
UINT MQTT_ParseToken( CHAR* pData, MQTT_CTX *pstCtx );
UINT MQTT_ParseFirmWare(MQTT_CTX *pstMqttCtx, CHAR* pcData);
static INT MQTT_ReportUpdateProgress(MQTT_CTX* pstMqttCtx, CHAR* pcState, UINT8 ucPercent, INT iCode, CHAR* pcMsg);
void MQTT_StartDownloadTheard(void* data);
UINT MQTT_RegistDevice();
VOID MQTT_SetConnectStatus(MQTT_CTX* pstCtx, UINT8 ucStatus);
static VOID MQTT_ClearTopic(MQTT_CTX* pstMqttCtx);

void MQTT_StartMqttTheard(void)
{
    xTaskCreate( MQTT_MqttClientTask, "MQTT_MqttClientTask", 2048, NULL, 5, NULL);
}

static MQTT_CTX* MQTT_NewMqttCtx()
{
    CHAR *pcSendBuf = NULL;
    CHAR *pcRecvBuf = NULL;
    MQTT_CTX *pstMqttCtx = NULL;

    pcSendBuf = ( CHAR* )malloc( MQTT_SENDSIZE );
    if ( NULL == pcSendBuf )
    {
        LOG_OUT(LOGOUT_ERROR, "malloc pcSendBuf failed.");
        goto err;
    }

    pcRecvBuf = ( CHAR* )malloc( MQTT_RECVSIZE );
    if ( NULL == pcRecvBuf )
    {
        LOG_OUT(LOGOUT_ERROR, "malloc pcRecvBuf failed.");
        goto err;
    }

    pstMqttCtx = ( MQTT_CTX* )malloc(sizeof(MQTT_CTX));
    if ( NULL == pstMqttCtx )
    {
        LOG_OUT(LOGOUT_ERROR, "malloc pstMqttCtx failed.");
        goto err;
    }
    memset(pstMqttCtx, 0, sizeof(MQTT_CTX));

    pstMqttCtx->pcRecvBuf = pcRecvBuf;
    pstMqttCtx->pcSendBuf = pcSendBuf;
    pstMqttCtx->uiSendBufSize = MQTT_SENDSIZE;
    pstMqttCtx->uiRecvBufSize = MQTT_RECVSIZE;
    pstMqttCtx->uiTimeOut = 3000;

    MQTT_GetMqttAddress(pstMqttCtx->szMqttAddr, sizeof(pstMqttCtx->szMqttAddr)-1);
    pstMqttCtx->iMqttPort = MQTT_PORT;

    pstMqttCtx->stConnectData.keepAliveInterval = 30;
    pstMqttCtx->stConnectData.MQTTVersion = 3;
    pstMqttCtx->stConnectData.clientID.cstring = MQTT_GetMqttClientID(pstMqttCtx->szClientID, sizeof(pstMqttCtx->szClientID));
    pstMqttCtx->stConnectData.username.cstring = MQTT_GetMqttUserName(pstMqttCtx->szUserName, sizeof(pstMqttCtx->szUserName));
    pstMqttCtx->stConnectData.password.cstring = MQTT_GetMqttPassWord(pstMqttCtx->szPassWord, sizeof(pstMqttCtx->szPassWord));
    pstMqttCtx->stConnectData.cleansession = TRUE;

    strcpy(pstMqttCtx->szCurSoftWareVer, SOFTWARE_VERSION);

    pstMqttCtx->eConnectStatus = MQTT_CONSTATUS_Unknown;

    return pstMqttCtx;

err:
    FREE_MEM(pcSendBuf);
    FREE_MEM(pcRecvBuf);
    FREE_MEM(pstMqttCtx);

	return NULL;
}

static INT MQTT_SetToken(MQTT_CTX* pstCtx, CHAR* pcToken)
{
	if ( pstCtx == NULL || pcToken == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstMqttCtx or pcToken is NULL");
		return 1;
	}

	if (strlen(pcToken) > sizeof(pstCtx->szToken)-1)
	{
		LOG_OUT(LOGOUT_ERROR, "szToken buf lenth not enought, need %d, but give %d",
				strlen(pcToken)+1, sizeof(pstCtx->szToken));
		return 2;
	}
	strcpy(pstCtx->szToken, pcToken);
	return 0;
}

static  CHAR* MQTT_GetToken(MQTT_CTX* pstCtx)
{
	if ( pstCtx == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstMqttCtx is NULL");
		return NULL;
	}
	return pstCtx->szToken;
}

UINT MQTT_GetConnectStatus()
{
	if ( pstMqttCtx == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstMqttCtx is NULL");
		return 0;
	}
    return pstMqttCtx->eConnectStatus;
}

VOID MQTT_SetConnectStatus(MQTT_CTX* pstCtx, UINT8 ucStatus)
{
	if ( pstMqttCtx == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstMqttCtx is NULL");
		return;
	}

	if ( ucStatus >= MQTT_CONSTATUS_Buff )
	{
		LOG_OUT(LOGOUT_ERROR, "unknown ucStatus:%d", ucStatus);
		return;
	}

    pstMqttCtx->eConnectStatus = ucStatus;
}

VOID MQTT_DestroyMqttCtx(MQTT_CTX* pstCtx)
{
	if ( pstCtx == NULL )
	{
		return;
	}

    FREE_MEM(pstCtx->pcRecvBuf);
    FREE_MEM(pstCtx->pcSendBuf);
    FREE_MEM(pstCtx);
}

static CHAR* MQTT_GetMqttAddress( CHAR* pcAddr, UINT uiLen )
{
    if ( pcAddr == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "pcAddr is NULL");
        return NULL;
    }

    snprintf( pcAddr, uiLen, "%s.iotcloud.tencentdevices.com", PLUG_GetMqttProductKey());
    return pcAddr;
}

static CHAR* MQTT_GetMqttClientID( CHAR* pcClientId, UINT uiLen )
{
	// ClientId: ${ProductId}${DeviceName}；
    if ( pcClientId == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "pcClientId is NULL");
        return NULL;
    }

    snprintf( pcClientId, uiLen, "%s%s", PLUG_GetMqttProductKey(),PLUG_GetMqttDevName());
    return pcClientId;
}


static CHAR* MQTT_GetMqttUserName( CHAR* pcUserName, UINT uiLen )
{
	// username 字段的格式为：${productid}${devicename};${sdkappid};${connid};${expiry}
    CHAR ucMac[20];
    snprintf( pcUserName, uiLen, "%s%s;12010126;%s;4593645000",
    		PLUG_GetMqttProductKey(),
			PLUG_GetMqttDevName(),
			WIFI_GetMacAddr(ucMac, sizeof(ucMac)));
    return pcUserName;
}

static CHAR* MQTT_GetMqttPassWord( CHAR* pcPassWord, UINT uiLen )
{
	UINT i = 0;
    CHAR digest[64] = {0};
    CHAR ucUserName[100];
    CHAR* ucDeDevSecret = NULL;

    if ( pcPassWord == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "pcPassWord is NULL");
        return NULL;
    }

    if ( uiLen < 20 )
    {
        LOG_OUT(LOGOUT_ERROR, "pcPassWord size not enough");
        return NULL;
    }

    ucDeDevSecret = base64Decode(PLUG_GetMqttDevSecret(), PLUG_GetMqttDevSecretLenth());
    MQTT_GetMqttUserName(ucUserName, sizeof(ucUserName)-1);
    ssl_hmac_sha1( ucUserName, strlen(ucUserName), (UINT8*)ucDeDevSecret, strlen(ucDeDevSecret), digest);

    for ( i = 0; i < 20; i++ )
    {
    	snprintf(pcPassWord+i*2, uiLen, "%02x", digest[i]);
    }
    snprintf(pcPassWord+i*2, uiLen, ";hmacsha1");

    FREE_MEM(ucDeDevSecret);
    return pcPassWord;
}


static INT MQTT_ReportToken(MQTT_CTX* pstMqttCtx)
{
    MQTTMessage message;
    CHAR szTopic[50];
    CHAR szPayload[100];
    UINT uiRet = 0;

    // 上报token
    message.qos = QOS0;
    message.retained = 0;
    message.payloadlen = sprintf(szPayload, "{\"method\":\"app_bind_token\",\"params\":{\"token\":\"%s\"}}",
    		pstMqttCtx->szToken);
    message.payload = szPayload;

    snprintf( szTopic, sizeof(szTopic), "$thing/up/service/%s/%s", PLUG_GetMqttProductKey(), PLUG_GetMqttDevName());
    uiRet = MQTTPublish(&pstMqttCtx->stClient, szTopic, &message);
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "report token %s failed, uiRet:%d", szTopic, uiRet);
    }
    else
    {
    	LOG_OUT(LOGOUT_DEBUG, "report token success, topic: %s", szTopic);
    }

    return uiRet;
}


static INT MQTT_ReportDevStatus(MQTT_CTX* pstMqttCtx)
{
    MQTTMessage message;
    CHAR szTopic[50];
    CHAR szPayload[300];
    UINT uiRet = 0;

    // 上报设备状态
    message.qos = QOS0;
    message.retained = 0;
    message.payloadlen = PLUG_MarshalJsonDevStatus(szPayload, sizeof(szPayload));
    message.payload = szPayload;

    snprintf( szTopic, sizeof(szTopic), "$thing/up/property/%s/%s", PLUG_GetMqttProductKey(), PLUG_GetMqttDevName());
    uiRet = MQTTPublish(&pstMqttCtx->stClient, szTopic, &message);
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "report device status %s failed, uiRet:%d", szTopic, uiRet);
    }
    else
    {
    	LOG_OUT(LOGOUT_INFO, "report device status success, msg: %s", szPayload);
    }

    return uiRet;
}


static INT MQTT_ReportSoftWareVersion(MQTT_CTX* pstMqttCtx)
{
    MQTTMessage message;
    CHAR szTopic[50];
    CHAR szPayload[100];
    UINT uiRet = 0;

    // 上报token
    message.qos = QOS1;
    message.retained = 0;
    message.payloadlen = sprintf(szPayload, "{\"type\":\"report_version\",\"report\":{\"version\":\"%s\"}}}",
    		pstMqttCtx->szCurSoftWareVer);
    message.payload = szPayload;

    snprintf( szTopic, sizeof(szTopic), "$ota/report/%s/%s", PLUG_GetMqttProductKey(), PLUG_GetMqttDevName());
    uiRet = MQTTPublish(&pstMqttCtx->stClient, szTopic, &message);
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "report software version failed, topic:%s, uiRet:%d", szTopic, uiRet);
    }
    else
    {
    	LOG_OUT(LOGOUT_INFO, "report software version success, topic:%s, version:%s",
    			szTopic, pstMqttCtx->szCurSoftWareVer);
    }

    return uiRet;
}

static CHAR* MQTT_FindEmptyTopic(MQTT_CTX* pstMqttCtx)
{
    UINT8 i = 0;

    for ( i = 0; i < MQTT_TOPIC_NUM; i++)
    {
    	if ( strlen(pstMqttCtx->aszSubscribeTopic[i]) == 0 )
    	{
    		return pstMqttCtx->aszSubscribeTopic[i];
    	}
    }

    return NULL;
}

static VOID MQTT_ClearTopic(MQTT_CTX* pstMqttCtx)
{
	UINT8 i = 0;

	if ( pstMqttCtx == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstMqttCtx is NULL");
		return;
	}


    for ( i = 0; i < MQTT_TOPIC_NUM; i++)
    {
    	pstMqttCtx->aszSubscribeTopic[i][0] = 0;
    }
}

static INT MQTT_SubscribeUpdate(MQTT_CTX* pstMqttCtx)
{
    CHAR *pcTopic = NULL;
    UINT uiRet = 0;

    pcTopic = MQTT_FindEmptyTopic(pstMqttCtx);
    if ( pcTopic == NULL )
    {
    	LOG_OUT(LOGOUT_ERROR, "aszSubscribeTopic is full, size:%d", MQTT_TOPIC_NUM);
    	return 1;
    }

    snprintf( pcTopic, MQTT_TOPIC_LEN, "$ota/update/%s/%s", PLUG_GetMqttProductKey(), PLUG_GetMqttDevName());
    uiRet = MQTTSubscribe(&pstMqttCtx->stClient, pcTopic, QOS1, MQTT_StartHandleUpgradeTheard);
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "Subscribe update failed, pcTopic:%s, uiRet:%d", pcTopic, uiRet);
    }
    else
    {
    	LOG_OUT(LOGOUT_INFO, "Subscribe update success, pcTopic:%s", pcTopic);
    }

    return uiRet;
}

static INT MQTT_SubscribeCommand(MQTT_CTX* pstMqttCtx)
{
    CHAR *pcTopic = NULL;
    UINT uiRet = 0;

    pcTopic = MQTT_FindEmptyTopic(pstMqttCtx);
    if ( pcTopic == NULL )
    {
    	LOG_OUT(LOGOUT_ERROR, "aszSubscribeTopic is full, size:%d", MQTT_TOPIC_NUM);
    	return 1;
    }

    snprintf( pcTopic, MQTT_TOPIC_LEN, "$thing/down/property/%s/%s", PLUG_GetMqttProductKey(), PLUG_GetMqttDevName());
    uiRet = MQTTSubscribe(&pstMqttCtx->stClient, pcTopic, QOS0, MQTT_HandleServerCommond);
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "Subscribe commond failed, pcTopic:%s, uiRet:%d", pcTopic, uiRet);
    }
    else
    {
    	LOG_OUT(LOGOUT_INFO, "Subscribe commond success, pcTopic:%s", pcTopic);
    }

    return uiRet;
}

static void MQTT_MqttClientTask(void* pvPara)
{
    UINT uiRet = 0;
    UINT8 uiRetry = 0;
    UINT8 uiRelayStatus = TRUE;
    UINT8 uiLastRelayStatus = FALSE;
    UINT8 ucCount = 0;
    UINT8 ucRegistType = 0;

    ucRegistType = PLUG_GetMqttRegistType();
    if ( ucRegistType == REGISTETYPE_Static )
    {
    	if (PLUG_GetMqttProductKeyLenth() == 0 || PLUG_GetMqttDevNameLenth() == 0 || PLUG_GetMqttDevSecretLenth() == 0 )
    	{
    		LOG_OUT(LOGOUT_ERROR, "can not connect to tencent platfrom, ProductKey(%d), DevName(%d), "
    				"DevSecret(%d), length all should not be 0 with static regist type",
    				PLUG_GetMqttProductKeyLenth(),
					PLUG_GetMqttDevNameLenth(),
					PLUG_GetMqttDevSecretLenth());
    		goto exit;
    	}
    	LOG_OUT(LOGOUT_INFO, "regist type is Static");
    }
    // 只有当动态注册方式且DevSecret为空才进行设备注册
    else if ( ucRegistType == REGISTETYPE_Dynamic )
    {
    	if ( PLUG_GetMqttProductKeyLenth() == 0 || PLUG_GetMqttProductSecretLenth() == 0 )
    	{
    		LOG_OUT(LOGOUT_ERROR, "can not connect to tencent platfrom, ProductKey(%d), ProductSecret(%d), "
    				"DevSecret(%d), length all should not be 0 with dynamic regist type",
    				PLUG_GetMqttProductKeyLenth(),
					PLUG_GetMqttProductSecretLenth());
    		goto exit;
    	}

    	if ( PLUG_GetMqttDevSecretLenth() == 0 )
    	{
    		LOG_OUT(LOGOUT_INFO, "DevSecret length is 0, will regist device to tencent platfrom");

    		while( PLUG_GetTimeSyncFlag() == TIME_SYNC_NONE )
    		{
    	        LOG_OUT(LOGOUT_DEBUG, "wait for get time from internet");
    	        vTaskDelay(1000 / portTICK_RATE_MS);
    		}

        	uiRet = MQTT_RegistDevice();
        	if( uiRet != OK )
        	{
        		LOG_OUT(LOGOUT_ERROR, "MQTT_RegistDevice failed");
        		goto exit;
        	}
        	LOG_OUT(LOGOUT_INFO, "regist device to tencent platfrom success");
    	}
    	LOG_OUT(LOGOUT_INFO, "regist type is Dynamic");
    }
    else
    {
    	LOG_OUT(LOGOUT_ERROR, "regist type(%d) is unsupport", ucRegistType);
    	goto exit;
    }

    pstMqttCtx = MQTT_NewMqttCtx();
    if ( NULL == pstMqttCtx )
    {
        LOG_OUT(LOGOUT_ERROR, "MQTT_NewMqttCtx failed");
        goto exit;
    }
    LOG_OUT(LOGOUT_INFO, "MQTT_NewMqttCtx success");

    // 如果通过smartconfig配置wifi，需上报smartconfig的配置结果，否则小程序会认为配网失败
    ReportSmartConfigResult(pstMqttCtx);

    NetworkInit( &(pstMqttCtx->stNetwork) );
    MQTTClientInit(&(pstMqttCtx->stClient), &(pstMqttCtx->stNetwork), pstMqttCtx->uiTimeOut,
    		pstMqttCtx->pcSendBuf, pstMqttCtx->uiSendBufSize,
			pstMqttCtx->pcRecvBuf, pstMqttCtx->uiRecvBufSize);

    LOG_OUT(LOGOUT_INFO, "MQTTClientInit success");

reConnect:
	MQTT_SetConnectStatus(pstMqttCtx, MQTT_CONSTATUS_Connectting);

    //等待wifi连接就绪
    while ( STATION_GOT_IP != wifi_station_get_connect_status() )
    {
        LOG_OUT(LOGOUT_DEBUG, "wait for connect");
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    while ( 1 )
    {
        uiRet = NetworkConnect(&(pstMqttCtx->stNetwork), pstMqttCtx->szMqttAddr, pstMqttCtx->iMqttPort);
        if ( uiRet == OK )
        {
            LOG_OUT(LOGOUT_DEBUG, "NetworkConnect success");
            break;
        }
        LOG_OUT(LOGOUT_ERROR, "NetworkConnect failed, uiRet:%d", uiRet);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    uiRetry = 0;
    while ( 1 )
    {
        uiRet = MQTTConnect(&pstMqttCtx->stClient, &pstMqttCtx->stConnectData);
        if ( uiRet == OK )
        {
        	MQTT_SetConnectStatus(pstMqttCtx, MQTT_CONSTATUS_Connected);
            LOG_OUT(LOGOUT_DEBUG, "MQTTConnect success");
            break;
        }

        LOG_OUT(LOGOUT_DEBUG, "MQTTConnect failed, uiRet:%d, retry %d times", uiRet, uiRetry);
        if ( uiRetry++ >= 5 )
        {
            LOG_OUT(LOGOUT_ERROR, "MQTTConnect failed, uiRet:%d, retry %d times", uiRet, uiRetry);
            goto end;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

#ifdef MQTT_TASK
    uiRet = MQTTStartTask(&pstMqttCtx->stClient);
    if ( uiRet != TRUE )
    {
        LOG_OUT(LOGOUT_ERROR, "MQTTStartTask failed, uiRet:%d", uiRet);
        MQTT_SetConnectStatus(pstMqttCtx, MQTT_CONSTATUS_Failed);
        goto end;
    }
    LOG_OUT(LOGOUT_DEBUG, "MQTTStartTask success");
#endif

    // 上报token
    MQTT_ReportToken(pstMqttCtx);

    // 上报当前软件版本
    MQTT_ReportSoftWareVersion(pstMqttCtx);

    // 订阅升级信息
    MQTT_SubscribeUpdate(pstMqttCtx);

    // 订阅服务器下发的控制指令
    MQTT_SubscribeCommand(pstMqttCtx);

    for (;;)
    {
        if ( STATION_GOT_IP != wifi_station_get_connect_status() )
        {
            LOG_OUT(LOGOUT_INFO, "wifi has disconnect");
            break;
        }

        if ( !MQTTIsConnected(&pstMqttCtx->stClient) )
        {
        	LOG_OUT(LOGOUT_INFO, "MQTT disconnect");
            MQTT_SetConnectStatus(pstMqttCtx, MQTT_CONSTATUS_Failed);
            break;
        }
        else
        {
        	MQTT_SetConnectStatus(pstMqttCtx, MQTT_CONSTATUS_Connected);
        }

        uiRelayStatus = PLUG_GetRelayStatus();
        if ( ucCount++ >= 30 || uiRelayStatus != uiLastRelayStatus )
        {
        	ucCount = 0;
        	MQTT_ReportDevStatus(pstMqttCtx);
            uiLastRelayStatus = uiRelayStatus;
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }

end:
    MQTTDisconnect( &pstMqttCtx->stClient );
    NetworkDisconnect( &pstMqttCtx->stNetwork );
    MQTT_ClearTopic(pstMqttCtx);
    LOG_OUT(LOGOUT_INFO, "MQTTDisconnect will be reconnect");
    goto reConnect;

exit:
	MQTT_SetConnectStatus(pstMqttCtx, MQTT_CONSTATUS_Failed);
    LOG_OUT(LOGOUT_INFO, "MQTT_MqttClientTask stop");
    MQTT_DestroyMqttCtx(pstMqttCtx);
    vTaskDelete(NULL);
    return;
}


static void MQTT_HandleServerCommond(MessageData* pstMsg)
{
    CHAR *pcTopic = NULL;
    CHAR *pcData = NULL;
    cJSON *pJsonRoot = NULL;
    cJSON *pJsonIteam = NULL;
    UINT8 PowerSwitch = 0;

    if ( pstMsg == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "pData is NULL.");
        return;
    }

    pcTopic = pstMsg->topicName->lenstring.data;
    pcData = pstMsg->message->payload;

    //LOG_OUT(LOGOUT_INFO, "Recv msg from server, topic:%s",  pcTopic);
    LOG_OUT(LOGOUT_DEBUG, "Recv msg from server, msg:%s",  pcData);

    pJsonRoot = cJSON_Parse( pcData );
    if ( pJsonRoot == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s", pcData);
        goto end;
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "params");
    if (pJsonIteam && pJsonIteam->type == cJSON_Object)
    {
        pJsonIteam = cJSON_GetObjectItem( pJsonIteam, "power_switch");
        if (pJsonIteam && pJsonIteam->type == cJSON_Number)
        {
            if ( pJsonIteam->valueint == 1 )
            {
                PLUG_SetRelayByStatus( TRUE, TRUE );
                LOG_OUT(LOGOUT_INFO, "power_switch:1");
            }
            else if ( pJsonIteam->valueint == 0 )
            {
                PLUG_SetRelayByStatus( FALSE, TRUE );
                LOG_OUT(LOGOUT_INFO, "power_switch:0");
            }
            else
            {
                LOG_OUT(LOGOUT_ERROR, "unknow power_switch:%d", pJsonIteam->valueint);
                goto end;
            }
        }
    }

end:
    cJSON_Delete(pJsonRoot);
    return;
}

UINT PLUG_MarshalJsonDevStatus( CHAR* pcBuf, UINT uiBufLen )
{
    cJSON  *pJson, *pJsonsub = NULL;
    CHAR *pJsonStr = NULL;


    pJson = cJSON_CreateObject();
    cJSON_AddStringToObject( pJson,        "method",     "report");

    pJsonsub = cJSON_CreateObject();
    cJSON_AddNumberToObject( pJsonsub,     "power_switch",     PLUG_GetRelayStatus());
    cJSON_AddNumberToObject( pJsonsub,     "temperature",      TEMP_GetTemperature());

#if IS_CHANG_XIN
    cJSON_AddNumberToObject( pJsonsub, "Voltage",          METER_GetMeterVoltage());
    cJSON_AddNumberToObject( pJsonsub, "Current",          METER_GetMeterCurrent());
    cJSON_AddNumberToObject( pJsonsub, "Power",            METER_GetMeterPower());
    cJSON_AddNumberToObject( pJsonsub, "Electricity",      METER_GetMeterElectricity());
#endif

    cJSON_AddItemToObject(pJson, "params", pJsonsub);

    pJsonStr = cJSON_PrintUnformatted(pJson);
    if ( pJsonStr != NULL )
    {
    	strncpy(pcBuf, pJsonStr, uiBufLen);
    }
    else
    {
    	snprintf(pcBuf, uiBufLen, "{}");
    }

    cJSON_Delete(pJson);
    FREE_MEM(pJsonStr);

    return strlen(pcBuf);
}

static void MQTT_HandleUpgrade(void* pPara)
{
    UINT uiRet = 0;
    CHAR *pcBase64Url = NULL;
    UINT uiTimeout = 0;

    if (pstMqttCtx->enUpgradeStatus == MQTT_UPGRADE_DOWNLOADING || pstMqttCtx->enUpgradeStatus == MQTT_UPGRADE_BURNING )
    {
    	// result_code：错误码，-1：下载超时；-2：文件不存在；-3：签名过期；-4:MD5不匹配；-5：更新固件失败
    	MQTT_ReportUpdateProgress(pstMqttCtx, "fail", 0, -5, "device is upgrading");
    	goto end;
    }

    if ( pPara == NULL )
    {
    	MQTT_ReportUpdateProgress(pstMqttCtx, "fail", 0, -5, "fail to get firmware");
    	goto end;
    }

    // 获取固件的下载地址、版本号、大小等信息
    uiRet = MQTT_ParseFirmWare(pstMqttCtx, pPara);
    if ( uiRet != 0 )
    {
    	MQTT_ReportUpdateProgress(pstMqttCtx, "fail", 0, -5, "parse firmware failed");
    	goto end;
    }
    LOG_OUT(LOGOUT_INFO, "UpdateSoftWareVer: %s, SoftWareSize: %d", pstMqttCtx->szUpdateSoftWareVer, pstMqttCtx->uiSoftWareSize);

    // 打印固件下载地址，地址包含特殊符号已经过base64编码
    pcBase64Url = base64Encode(pstMqttCtx->szSoftWareUrl, strlen(pstMqttCtx->szSoftWareUrl));
    LOG_OUT(LOGOUT_DEBUG, "url encodeed with base64: %s", pcBase64Url);
    FREE_MEM(pcBase64Url);

    pstMqttCtx->uiDownloadSize = 0;

    for (;;)
    {
    	switch(pstMqttCtx->enUpgradeStatus)
    	{
    		case MQTT_UPGRADE_IDEA :
    			MQTT_StartDownloadTheard(pstMqttCtx);
    			LOG_OUT(LOGOUT_INFO, "start download software");
    			pstMqttCtx->enUpgradeStatus = MQTT_UPGRADE_DOWNLOADING;
    			break;

    		case MQTT_UPGRADE_DOWNLOADING :
    			if ( pstMqttCtx->iDownloadProcess == -1 )
    			{
    				LOG_OUT(LOGOUT_INFO, "download software failed");
    				MQTT_ReportUpdateProgress(pstMqttCtx, "fail", 0, -5, "download software failed");
    				pstMqttCtx->enUpgradeStatus = MQTT_UPGRADE_IDEA;
        		    goto end;
    			}
    			else if ( pstMqttCtx->iDownloadProcess >= 100 )
    			{
    				LOG_OUT(LOGOUT_INFO, "download software success");
        		    MQTT_ReportUpdateProgress(pstMqttCtx, "downloading", 100, 0, "");
        		    pstMqttCtx->enUpgradeStatus = MQTT_UPGRADE_BURNING;
    			} else {
    				//LOG_OUT(LOGOUT_INFO, "downloading software, progress %d", pstMqttCtx->iDownloadProcess);
        		    MQTT_ReportUpdateProgress(pstMqttCtx, "downloading", pstMqttCtx->iDownloadProcess, 0, "");
    			}

    		    break;
    		case MQTT_UPGRADE_BURNING :
    			LOG_OUT(LOGOUT_INFO, "burning software");
    		    MQTT_ReportUpdateProgress(pstMqttCtx, "burning", 100, 0, "");
    		    pstMqttCtx->enUpgradeStatus = MQTT_UPGRADE_DONE;
    		    break;
    		case MQTT_UPGRADE_DONE :
    			LOG_OUT(LOGOUT_INFO, "prepare reboot with new software version: %s", pstMqttCtx->szUpdateSoftWareVer);
    		    MQTT_ReportUpdateProgress(pstMqttCtx, "done", 100, 0, "");
    		    pstMqttCtx->enUpgradeStatus = MQTT_UPGRADE_IDEA;
    	        UPGRADE_StartUpgradeRebootTimer();
    		    goto end;
    	}

    	if ( uiTimeout++ > 300 )
    	{
    		MQTT_ReportUpdateProgress(pstMqttCtx, "fail", 0, -5, "download software timeout");
		    LOG_OUT(LOGOUT_INFO, "download software timeout");
		    pstMqttCtx->enUpgradeStatus = MQTT_UPGRADE_TIMEOUT;
		    goto end;
    	}
    	//LOG_OUT(LOGOUT_INFO, "downloading software, progress %d", pstMqttCtx->iDownloadProcess);
    	vTaskDelay(1000/portTICK_RATE_MS );
    }

end:
    vTaskDelete(NULL);
    return;
}


static INT MQTT_ReportUpdateProgress(MQTT_CTX* pstMqttCtx, CHAR* pcState, UINT8 ucPercent, INT iCode, CHAR* pcMsg)
{
    MQTTMessage message;
    CHAR szTopic[50];
    CHAR szPayload[256];
    UINT uiRet = 0;

    message.qos = QOS1;
    message.retained = 0;
    message.payloadlen = sprintf(szPayload, "{\"type\":\"report_progress\",\"report\":"
    		"{\"progress\":{\"state\":\"%s\",\"percent\":\"%d\",\"result_code\":\"%d\",\"result_msg\":\"%s\"},\"version\":\"%s\"}}",
			pcState, ucPercent, iCode, pcMsg, pstMqttCtx->szUpdateSoftWareVer);
    message.payload = szPayload;

    snprintf( szTopic, sizeof(szTopic), "$ota/report/%s/%s", PLUG_GetMqttProductKey(), PLUG_GetMqttDevName());
    uiRet = MQTTPublish(&pstMqttCtx->stClient, szTopic, &message);
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "report software version failed, topic:%s, uiRet:%d", szTopic, uiRet);
    }
    else
    {
    	LOG_OUT(LOGOUT_INFO, "report software update progress success, topic:%s, state:%s, percent:%d, "
    			"result_code:%d, result_msg:%s, CurVer:%s, UpdateVer:%s",
    			szTopic, pcState, ucPercent, iCode, pcMsg, pstMqttCtx->szCurSoftWareVer, pstMqttCtx->szUpdateSoftWareVer);
    }

    return uiRet;
}


void MQTT_StartHandleUpgradeTheard(MessageData* data)
{
    xTaskCreate( MQTT_HandleUpgrade, "MQTT_HandleUpgrade", 512, data->message->payload, 7, NULL);
}


static int CreateUDPSocket()
{
    struct sockaddr_in saddr = { 0 };
    int sock = -1;
    int err = 0;

    LOG_OUT(LOGOUT_DEBUG, "socket...");
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0)
    {
    	LOG_OUT(LOGOUT_ERROR, "Failed to create socket, errno:%d", errno);
        return -1;
    }
    LOG_OUT(LOGOUT_DEBUG, "socket ok");

    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(8266);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    LOG_OUT(LOGOUT_DEBUG, "bind...");
    err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (err < 0)
    {
    	LOG_OUT(LOGOUT_ERROR, "Failed to bind socket. errno:%d", errno);
        goto err;
    }
    LOG_OUT(LOGOUT_DEBUG, "bind ok");
    return sock;

err:
    close(sock);
    return -1;
}

void ReportSmartConfigResult(MQTT_CTX *pstCtx)
{
    struct sockaddr_in client = { 0 };
    socklen_t  client_len=sizeof(struct sockaddr_in);
    struct timeval stRdTimeOut = {1, 0};
    fd_set stFdRead;
    char *pcData = NULL;
    int iRet = 0;
    int retry = 0;

    if ( pstCtx == NULL )
    {
    	LOG_OUT(LOGOUT_ERROR, "pstCtx is NULL");
    	return;
    }

    int sock = CreateUDPSocket();
    if ( sock < 0 )
    {
    	LOG_OUT(LOGOUT_ERROR, "Failed to create IPv4 multicast socket");
    	return;
    }

    pcData = malloc(128);
    if ( pcData == NULL )
    {
    	LOG_OUT(LOGOUT_ERROR, "malloc failed");
    	close(sock);
    	return;
    }

    FD_ZERO( &stFdRead );
    for(;;)
    {
    	FD_SET( sock, &stFdRead );
        iRet = select( sock + 1, &stFdRead, NULL, NULL, &stRdTimeOut );
        if ( iRet < 0 )
        {
            LOG_OUT(LOGOUT_ERROR, "recv error, errno:%d, iRet:%d", errno, iRet);
        	break;
        }
        //等待接收超时
        else if ( 0 == iRet )
        {
        	if ( retry++ >= 10 )
        	{
            	LOG_OUT(LOGOUT_DEBUG, "recv timeout, errno:%d", errno);
            	break;
        	}
            continue;
        }

        iRet = recvfrom( sock, pcData, 100, 0, (struct sockaddr *)&client, &client_len);
        if( iRet < 0 )
        {
        	LOG_OUT(LOGOUT_ERROR, "recvfrom error, errno:%d", errno);
        	continue;
        }
        pcData[iRet] = 0;
        iRet = MQTT_ParseToken(pcData, pstCtx);
        if ( iRet != 0 )
		{
			LOG_OUT(LOGOUT_ERROR, "parse token failed, iRet", iRet);
			break;
		}

        sprintf(pcData, "{\"cmdType\":2,\"productId\":\"%s\",\"deviceName\":\"%s\",\"protoVersion\":\"2.0\"}",
        		PLUG_GetMqttProductKey(), PLUG_GetMqttDevName());
        sendto(sock, pcData, strlen(pcData), 0, (struct sockaddr*)&client, client_len);
        LOG_OUT(LOGOUT_INFO, "report device connect wifi success");

        break;
    }

    FREE_MEM(pcData);
    close(sock);
}



UINT MQTT_ParseToken( CHAR* pData, MQTT_CTX *pstCtx )
{
    cJSON *pJsonRoot = NULL;
    cJSON *pJsonIteam = NULL;
    UINT uiRet = 0;

    if ( pData == NULL || pstCtx == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "pData(%p) or pstCtx(%p) is NULL", pData, pstCtx);
        return FAIL;
    }

    pJsonRoot = cJSON_Parse( pData );
    if ( pJsonRoot == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s", pData);
        goto error;
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "token");
    if (pJsonIteam && pJsonIteam->type == cJSON_String)
    {
    	uiRet = MQTT_SetToken(pstCtx, pJsonIteam->valuestring);
        if ( uiRet != 0 )
        {
        	LOG_OUT(LOGOUT_ERROR, "save token failed, iRet", uiRet);
        	goto error;
        }
    }

    cJSON_Delete(pJsonRoot);
    return 0;

error:
    cJSON_Delete(pJsonRoot);
    return 1;
}

UINT MQTT_ParseFirmWare(MQTT_CTX *pstMqttCtx, CHAR* pcData)
{
    cJSON *pJsonRoot = NULL;
    cJSON *pJsonIteam = NULL;
    UINT uiRet = 0;

    if ( pcData == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "pData is NULL.");
        return FAIL;
    }

    pJsonRoot = cJSON_Parse( pcData );
    if ( pJsonRoot == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pcData:%s", pcData);
        goto error;
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "version");
    if (pJsonIteam && pJsonIteam->type == cJSON_String)
    {
        strncpy(pstMqttCtx->szUpdateSoftWareVer, pJsonIteam->valuestring, sizeof(pstMqttCtx->szUpdateSoftWareVer)-1);
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "file_size");
    if (pJsonIteam && pJsonIteam->type == cJSON_Number)
    {
    	pstMqttCtx->uiSoftWareSize = pJsonIteam->valueint;
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "url");
    if (pJsonIteam && pJsonIteam->type == cJSON_String)
    {
    	if ( strlen(pJsonIteam->valuestring) > sizeof(pstMqttCtx->szSoftWareUrl)-1 )
    	{
    		pstMqttCtx->szSoftWareUrl[0] = 0;
    		LOG_OUT(LOGOUT_ERROR, "szSoftWareUrl lenth too short, need %d but give %d",
    				strlen(pJsonIteam->valuestring)+1, sizeof(pstMqttCtx->szSoftWareUrl));
    		goto error;
    	}
    	else{
    		strncpy(pstMqttCtx->szSoftWareUrl, pJsonIteam->valuestring, sizeof(pstMqttCtx->szSoftWareUrl)-1);
    	}
    }

    cJSON_Delete(pJsonRoot);
    return 0;

error:
    cJSON_Delete(pJsonRoot);
    return 1;
}


UINT MQTT_WriteSoftWare(VOID *pPara)
{
    UINT uiRet = 0;
    STATIC UINT32 uiAddr = 0;
    MQTT_WriteSoftWarePara_S *pstWriteSoftWare = pPara;
    HTTP_CLIENT_S *pstCli = NULL;
    MQTT_CTX* pstMqttCtx = NULL;

    if ( pstWriteSoftWare == NULL )
    {
    	LOG_OUT(LOGOUT_ERROR, "pstWriteSoftWare is NULL");
    	goto err;
    }

    pstCli = pstWriteSoftWare->pstCli;
    pstMqttCtx = pstWriteSoftWare->pstMqttCtx;
    if ( pstCli == NULL || pstMqttCtx == NULL )
    {
    	LOG_OUT(LOGOUT_ERROR, "pstCli:%p,  pstMqttCtx:%p", pstCli, pstMqttCtx);
    	goto err;
    }

    if (pstCli->stReson.eHttpCode != HTTP_CODE_Ok && pstCli->stReson.eHttpCode != HTTP_CODE_Partial_Content)
    {
    	LOG_OUT(LOGOUT_ERROR, "get response code: %s", szHttpCodeMap[pstCli->stReson.eHttpCode]);
    	goto err;
    }

    //取要升级的user.bin的地址
    if ( uiAddr == 0 )
    {
        if ( pstMqttCtx->uiSoftWareSize != pstCli->stReson.uiContentLength + pstMqttCtx->uiDownloadSize)
        {
        	LOG_OUT(LOGOUT_ERROR, "SoftWareSize[%d] != ContentLength[%d]",
        			pstMqttCtx->uiSoftWareSize, pstCli->stReson.uiContentLength + pstMqttCtx->uiDownloadSize);
        	goto err;
        }

        uiAddr = UPGRADE_GetUpgradeUserBinAddr();
        if ( uiAddr != 0 )
        {
            LOG_OUT(LOGOUT_INFO, "new bin uiAddr:0x%X, length:%d", uiAddr,  pstMqttCtx->uiSoftWareSize);
        }
    }

    //将要升级的bin文件写入对应flash地址中
    if ( uiAddr != 0 )
    {
        uiRet = FlASH_Write( uiAddr + pstMqttCtx->uiDownloadSize,
        		pstCli->stReson.pcBody,
				pstCli->stReson.uiRecvCurLen);
        //写失败直接返回500
        if ( uiRet != OK  )
        {
            LOG_OUT(LOGOUT_ERROR, "upgread failed, FlASH_Write failed.");
            goto err;
        }
        else
        {
        	pstMqttCtx->uiDownloadSize += pstCli->stReson.uiRecvCurLen;
            if (pstCli->stReson.uiRecvTotalLen > 0)
            {
                //输出user.bin下载进度
            	pstMqttCtx->iDownloadProcess = pstMqttCtx->uiDownloadSize*100 / pstMqttCtx->uiSoftWareSize;
                LOG_OUT(LOGOUT_INFO, "upgrade process:%d", pstMqttCtx->iDownloadProcess);
            }
        }
    }
    //user.bin的地址无效
    else
    {
        LOG_OUT(LOGOUT_ERROR, "Get user bin addr failed.");
        goto err;
    }

    //bin数据接收完成
    if ( pstCli->stReson.eProcess == RES_Process_Finished )
    {

        uiAddr = 0;
        LOG_OUT(LOGOUT_INFO, "new bin download successed.");
    }
    // 这里加延时的作用是让出该任务的执行权限，避免该任务长时间占用处理器导致其他任务无法被调度
    vTaskDelay(10 / portTICK_RATE_MS);
    return OK;

err:
	pstMqttCtx->iDownloadProcess = -1;
	uiAddr = 0;
	return FAIL;
}


static void MQTT_DownloadSoftWare(void* pPara)
{
	MQTT_CTX* pstMqttCtx = pPara;
	HTTP_CLIENT_S* pstCli = NULL;
	CHAR szBuf[120] = {0};
	CHAR szTmp[20] = {0};
	CHAR* pcPoint = NULL;
	CHAR* pcUri = NULL;
	MQTT_WriteSoftWarePara_S stWriteSoftWare;
	INT iRet = 0;
	UINT8 uiRetry = 0;

    if ( pstMqttCtx == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "iDownloadProcess is NULL.");
        goto end;
    }

    pstMqttCtx->iDownloadProcess = 0;

    pcPoint = strstr(pstMqttCtx->szSoftWareUrl, "//");
    if ( pcPoint == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "Split SoftWareUrl failed with: //");
        pstMqttCtx->iDownloadProcess = -1;
        goto end;
    }
    pcPoint += 2;

    pcUri = strstr(pcPoint, "/");
    if ( pcUri == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "Split uri failed with: /");
        pstMqttCtx->iDownloadProcess = -1;
        goto end;
    }

    memset(szBuf, 0, sizeof(szBuf));
    strncpy(szBuf, pcPoint, pcUri-pcPoint);

retry:
	if ( uiRetry >= 10 )
	{
        pstMqttCtx->iDownloadProcess = -1;
        goto end;
	}

    pstCli = HTTP_NewClient("GET", szBuf, pcUri, "", 0);
	if ( pstCli == NULL )
	{
        LOG_OUT(LOGOUT_ERROR, "HTTP_NewClient failed");
        pstMqttCtx->iDownloadProcess = -1;
        goto end;
	}

	sprintf(szTmp, "bytes=%d-", pstMqttCtx->uiDownloadSize);
	HTTP_ClientSetHeader(pstCli, "Range", szTmp);

	iRet = HTTP_ClientDoRequest(pstCli);
	if ( iRet != 0 )
	{
        LOG_OUT(LOGOUT_ERROR, "HTTP_ClientDoRequest failed");
        pstMqttCtx->iDownloadProcess = -1;
        goto end;
	}

	stWriteSoftWare.pstCli = pstCli;
	stWriteSoftWare.pstMqttCtx = pstMqttCtx;
	iRet = HTTP_ClientDoResponse(pstCli, MQTT_WriteSoftWare, &stWriteSoftWare);
	if ( iRet == -1 )
	{
        LOG_OUT(LOGOUT_ERROR, "HTTP_ClientDoRequest failed");
        pstMqttCtx->iDownloadProcess = -1;
        goto end;
	}
	// 接收超时
	else if ( iRet == -2 )
	{
		HTTP_DestoryClient(pstCli);
		goto retry;
	}

end:
	HTTP_DestoryClient(pstCli);
    vTaskDelete(NULL);
    return;
}

void MQTT_StartDownloadTheard(void* data)
{
    xTaskCreate( MQTT_DownloadSoftWare, "MQTT_DownloadSoftWare", 512, data, 8, NULL);
}


UINT MQTT_RegistDevice()
{
	HTTP_CLIENT_S* pstCli = NULL;
	CHAR *pcBody = NULL;
	CHAR *pcBuf = NULL;
	CHAR szMac[15] = {};
	CHAR szDigest[20] = {};
	CHAR *pcSignature = NULL;
	UINT8 i = 0;
	INT iRet = 0;

	pcBody = malloc(256);
	if (pcBody == NULL)
	{
		LOG_OUT(LOGOUT_ERROR, "malloc pcBody failed");
		goto err;
	}

	pcBuf = malloc(100);
	if (pcBuf == NULL)
	{
		LOG_OUT(LOGOUT_ERROR, "malloc pcBuf failed");
		goto err;
	}

	// 使用mac作为设备的devicename
	WIFI_GetMacAddr(szMac, sizeof(szMac));
	PLUG_SetMqttDevName(szMac);

	snprintf(pcBuf, 100, "deviceName=%s&nonce=123456&productId=%s&timestamp=%d",
			PLUG_GetMqttDevName(),
			PLUG_GetMqttProductKey(),
			sntp_get_current_timestamp());
	LOG_OUT(LOGOUT_DEBUG, "pcBuf:%s", pcBuf);
	ssl_hmac_sha1( pcBuf, strlen(pcBuf), PLUG_GetMqttProductSecret(), PLUG_GetMqttProductSecretLenth(), szDigest);

    for ( i = 0; i < 20; i++ )
    {
    	snprintf(pcBuf+i*2, 100, "%02x", szDigest[i]);
    }

    pcSignature = base64Encode(pcBuf, strlen(pcBuf));
	snprintf(pcBody, 256, "{\"deviceName\":\"%s\",\"nonce\":123456,\"productId\":\"%s\",\"timestamp\":%d,\"signature\":\"%s\"}",
			WIFI_GetMacAddr(szMac, sizeof(szMac)),
			PLUG_GetMqttProductKey(),
			sntp_get_current_timestamp(),
			pcSignature);
	FREE_MEM(pcSignature);

    pstCli = HTTP_NewClient("GET", "ap-guangzhou.gateway.tencentdevices.com", "/register/dev", pcBody, strlen(pcBody));
	if ( pstCli == NULL )
	{
        LOG_OUT(LOGOUT_ERROR, "HTTP_NewClient failed");
        goto err;
	}
	LOG_OUT(LOGOUT_DEBUG, "pcBody:%s", pcBody);
	iRet = HTTP_ClientDoRequest(pstCli);
	if ( iRet != 0 )
	{
        LOG_OUT(LOGOUT_ERROR, "HTTP_ClientDoRequest failed");
        goto err;
	}

	iRet = HTTP_ClientDoResponse(pstCli, MQTT_ParseDeviceSecret, pstCli);
	if ( iRet == -1 )
	{
        LOG_OUT(LOGOUT_ERROR, "HTTP_ClientDoRequest failed");
        goto err;
	}

succ:
	FREE_MEM(pcBody);
	FREE_MEM(pcBuf);
	HTTP_DestoryClient(pstCli);
	return OK;
err:
	FREE_MEM(pcBody);
	FREE_MEM(pcBuf);
	HTTP_DestoryClient(pstCli);
	return FAIL;
}

UINT MQTT_ParseDeviceSecret(VOID *pPara)
{
	HTTP_CLIENT_S *pstCli = pPara;
    cJSON *pJsonRoot = NULL;
    cJSON *pJsonIteam = NULL;
    CHAR *pcPayload = NULL;
    UINT uiPayloadLen = 0;
    CHAR *pcPayloadDecode = NULL;
    AES_CTX aes_ctx;
	CHAR szIV[16] = {'0', '0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};
	CHAR szProductSecretDup[PLUG_MQTT_PRODUCTSEC_LEN+1];
	CHAR* pcDevSecret = NULL;

	if ( pstCli == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstCli is NULL");
		return FAIL;
	}

	LOG_OUT(LOGOUT_DEBUG, "recv:%s", pstCli->stReson.pcBody);
    pJsonRoot = cJSON_Parse( pstCli->stReson.pcBody );
    if ( pJsonRoot == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s", pstCli->stReson.pcBody);
        goto error;
    }

    //LOG_OUT(LOGOUT_DEBUG, "recv:%s", pstCli->stReson.pcBody);

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "code");
    if (pJsonIteam && pJsonIteam->type == cJSON_String)
    {
        if ( strcmp(pJsonIteam->valuestring, "0") != 0)
        {
            LOG_OUT(LOGOUT_ERROR, "ParseDeviceSecret failed, expect code:0 ,recv code:%s", pJsonIteam->valuestring);
            goto error;
        }
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "message");
    if (pJsonIteam && pJsonIteam->type == cJSON_String)
    {
        if ( strcmp(pJsonIteam->valuestring, "success") != 0)
        {
            LOG_OUT(LOGOUT_ERROR, "ParseDeviceSecret failed, expect message:success, recv message:%s", pJsonIteam->valuestring);
            goto error;
        }
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "len");
    if (pJsonIteam && pJsonIteam->type == cJSON_Number)
    {
    	uiPayloadLen = pJsonIteam->valueint;
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "payload");
    if (pJsonIteam && pJsonIteam->type == cJSON_String)
    {
        if ( strlen(pJsonIteam->valuestring) == 0 )
        {
            LOG_OUT(LOGOUT_ERROR, "ParseDeviceSecret failed, recv payload lenth is: %d", strlen(pJsonIteam->valuestring));
            goto error;
        }
        pcPayload = pJsonIteam->valuestring;
        //LOG_OUT(LOGOUT_INFO, "payload:%s", pcPayload);
    }

    // 加密过程将原始 JSON 格式的 payload 转为字符串后进行 AES 加密，再进行 base64 加密。
    // AES 加密算法为 CBC 模式，密钥长度128，取 productSecret 前16位，偏移量为长度16的字符'0'。
    // https://cloud.tencent.com/document/product/1081/47612
    pcPayloadDecode = base64Decode(pcPayload, strlen(pcPayload));

    strncpy(szProductSecretDup, PLUG_GetMqttProductSecret(), 16);
    AES_set_key(&aes_ctx, szProductSecretDup, szIV, AES_MODE_128);
    AES_convert_key(&aes_ctx);

    // aes算法是次加解密16和字节，所以申请的内存要是16的倍数，保险起见多申请16个字节
    pcDevSecret = malloc(uiPayloadLen+16);
    if ( pcDevSecret == NULL )
    {
    	LOG_OUT(LOGOUT_ERROR, "malloc failed for pcDevSecret, size:%d", uiPayloadLen+16);
    	goto error;
    }
    AES_cbc_decrypt(&aes_ctx, pcPayloadDecode, pcDevSecret, uiPayloadLen+16);
    pcDevSecret[uiPayloadLen] = 0;


    pJsonRoot = cJSON_Parse( pcDevSecret );
    if ( pJsonRoot == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pcDevSecret:%s", pcDevSecret);
        goto error;
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "encryptionType");
    if (pJsonIteam && pJsonIteam->type == cJSON_Number)
    {
    	if ( pJsonIteam->valueint != 2 )
    	{
    		LOG_OUT(LOGOUT_ERROR, "ParseDeviceSecret failed, expect encryptionType:2 ,recv :%d", pJsonIteam->valueint);
    		goto error;
    	}
    }

    pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "psk");
    if (pJsonIteam && pJsonIteam->type == cJSON_String)
    {
    	PLUG_SetMqttDevSecret(pJsonIteam->valuestring);
    	LOG_OUT(LOGOUT_INFO, "ParseDeviceSecret success, DevSecret:%s", PLUG_GetMqttDevSecret());
    }

succ:
	cJSON_Delete(pJsonRoot);
	FREE_MEM(pcDevSecret);
	FREE_MEM(pcPayloadDecode);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	FREE_MEM(pcDevSecret);
	FREE_MEM(pcPayloadDecode);
	return FAIL;

}
