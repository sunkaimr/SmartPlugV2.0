
/*
 * https://www.bigiot.net/help/1.html
 *
 *
 *
 *
 * */

#include "esp_common.h"
#include "user_common.h"

#include "lwip/netdb.h"
#include "openssl/ssl.h"
#include "MQTTFreeRTOS.h"
#include "freertos/timers.h"

#define BIGIOT_HOSTNAME		"www.bigiot.net"
#define BIGIOT_PORT    		((int)8181)
#define BIGIOT_TIMEOUT 		3

#define BIGIOT_CONNECT_OK	"WELCOME TO BIGIOT"
#define BIGIOT_LOGINT_OK	"checkinok"
#define BIGIOT_ON			"play"
#define BIGIOT_OFF			"stop"
#define BIGIOT_SAY			"say"
#define BIGIOT_LOGIN		"login"
#define BIGIOT_LOGOUT		"logout"

#define BIGIOT_TIME_DEFAULT  6


static void Init( BIGIOT_Ctx_S *pstCtx, char* pcHostName, int iPort, char* pcDevId, char* pcApiKey );
static int Connect(BIGIOT_Ctx_S* pstCtx);
static void Disconnect(BIGIOT_Ctx_S* pstCtx);
static int Write( BIGIOT_Ctx_S* pstCtx, const unsigned char* buffer, unsigned int len, unsigned int timeout_s );
static int Read(BIGIOT_Ctx_S* pstCtx, unsigned char* buffer, unsigned int len, unsigned int timeout_s);
static int Bigiot_EventRegister( BIGIOT_Ctx_S *pstCtx, BIGIOT_Event_S* pstEvent );
static void Bigiot_EventCallBack( BIGIOT_Ctx_S *pstCtx );
static int Bigiot_SendMessage( BIGIOT_Ctx_S *pstCtx, char* pcIfId, char* pcValue );
static int Bigiot_StartEventTask( BIGIOT_Ctx_S *pstCtx );

static char* BigiotParseString( const char* pcData, const char* pcKey, char*pcValue, unsigned int uiLen );
static int BigiotParseInt( const char* pcData, const char* pcKey );

static int Bigiot_DeviceIDRegister( BIGIOT_Ctx_S *pstCtx );
static int Bigiot_HeartBeatCallBack( void * para );
static int Bigiot_RelayStatusCallBack( void * para );
static int Bigiot_UploadTempCallBack( void * para );
static int Bigiot_UploadHumidityCallBack( void * para );
static int Bigiot_UploadVoltageCallBack( void * para );
static int Bigiot_UploadCurrentCallBack( void * para );
static int Bigiot_UploadPowerCallBack( void * para );
static int Bigiot_UploadElectricityCallBack( void * para );


BIGIOT_Ctx_S* pstCli = NULL;

static void BIGIOT_BigiotTask( void* para )
{
	int iRet = -1;
	char* pcDevId = 0;
	char* pcApiKey = 0;

	pcDevId = PLUG_GetBigiotDevId();
	if ( pcDevId == 0 || pcDevId[0] == 0 || pcDevId[0] == 0xFF )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "pcDevId is invalid");
		goto err;
	}

	pcApiKey = PLUG_GetBigiotApiKey();
	if ( pcApiKey == 0 || pcApiKey[0] == 0 || pcApiKey[0] == 0xFF )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "pcApiKey is invalid");
		goto err;
	}

retry:

	//等待wifi连接就绪
	while ( STATION_GOT_IP != wifi_station_get_connect_status() )
	{
		BIGIOT_LOG(BIGIOT_DEBUG, "wait for connect");
		vTaskDelay(1000 / portTICK_RATE_MS);
	}

	pstCli = Bigiot_New( BIGIOT_HOSTNAME, BIGIOT_PORT, pcDevId, pcApiKey);
	if ( pstCli == 0 )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_New failed");
		goto exit;
	}
	BIGIOT_LOG(BIGIOT_INFO, "Bigiot_New success");

	iRet = Bigiot_DeviceIDRegister( pstCli );
	if ( iRet )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_DeviceIDRegister failed, iRet:%d", iRet);
		goto exit;
	}

	for (;;)
	{
		iRet = Bigiot_Login( pstCli );
		if ( iRet )
		{
			BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Login failed, iRet:%d", iRet);
			goto exit;
		}
		BIGIOT_LOG(BIGIOT_INFO, "Bigiot_Login success");

		iRet = Bigiot_StartEventTask( pstCli );
		if ( iRet )
		{
			BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_StartEventTask failed");
			goto exit;
		}
		BIGIOT_LOG(BIGIOT_INFO, "Bigiot_StartEventTask success");

		for ( ;; )
		{
			iRet = Bigiot_Cycle( pstCli );
			if ( iRet )
			{
				BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Cycle failed");
				goto exit;
			}

			if ( !pstCli->iAlived )
			{
				goto exit;
			}
			vTaskDelay( 1000/portTICK_RATE_MS );
		}
	}

exit:
	BIGIOT_LOG(BIGIOT_INFO, "BIGIOT_BigiotTask stop");
	vTaskDelay( 1000/portTICK_RATE_MS );
	BIGIOT_Destroy( &pstCli );
	goto retry;
err:
	vTaskDelete(NULL);
	return;
}

void BIGIOT_StartBigiotTheard(void)
{
    xTaskCreate( BIGIOT_BigiotTask, "BIGIOT_BigiotTask", 1024, (void*)0, 5, 0);
}

BIGIOT_Ctx_S* Bigiot_New( char* pcHostName, int iPort, char* pcDevId, char* pcApiKey )
{
	BIGIOT_Ctx_S* pstCtx = NULL;

	pstCtx = malloc( sizeof(BIGIOT_Ctx_S) );
	if ( pstCtx == 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_New failed");
    	return 0;
	}

	Init( pstCtx, pcHostName, iPort, pcDevId, pcApiKey);

	return pstCtx;
}

void BIGIOT_Destroy( BIGIOT_Ctx_S **ppstCtx )
{

	if ( ppstCtx != 0 && *ppstCtx != 0 )
	{
		(*ppstCtx)->Disconnect( *ppstCtx );
		if ( (*ppstCtx)->xEventHandle != 0 )
		{
			vTaskDelete( (*ppstCtx)->xEventHandle );
		}

		free( *ppstCtx );
		*ppstCtx = NULL;
	}
	else if ( ppstCtx != 0 && *ppstCtx == 0 )
	{
		*ppstCtx = NULL;
	}
}

static void Bigiot_EventHanderTask( void* para )
{
	BIGIOT_Ctx_S *pstCtx = para;

	for ( ;; )
	{
		Bigiot_EventCallBack( pstCtx );

		vTaskDelay( BIGIOT_TIME_DEFAULT * 1000 / portTICK_RATE_MS );
	}
}

static int Bigiot_StartEventTask( BIGIOT_Ctx_S *pstCtx )
{
	if ( pdPASS != xTaskCreate( Bigiot_EventHanderTask,
								"Bigiot_EventHanderTask",
								512,
								(void*)pstCtx,
								uxTaskPriorityGet(NULL),
								&(pstCtx->xEventHandle)))
	{
		return 1;
	}

	return 0;
}

int Bigiot_Login( BIGIOT_Ctx_S *pstCtx )
{
	char szMess[100] = { 0 };
	char szValue[100] = { 0 };
	char* pcMethod = 0;
	char* pcContent = 0;
	int iLen = 0;
	int iRet = 0;

	//先连接至贝壳物联平台
    iRet = pstCtx->Connect( pstCtx );
	if ( iRet )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "Connect failed");
		return 1;
	}

	//拼装并发送登陆指令
	iLen = snprintf(szMess, sizeof(szMess), "{\"M\":\"checkin\",\"ID\":\"%s\",\"K\":\"%s\"}\n",
					pstCtx->pcDeviceId, pstCtx->pcApiKey );

	iRet = pstCtx->Write( pstCtx, szMess, iLen, pstCtx->iTimeOut );
	if ( iRet != iLen )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Login failed");
    	return 2;
	}

	//登陆成功会返回checkinok和设备名称等字段
	iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
	if ( iRet < 0 )
	{
    	return 3;
	}
	else if ( iRet == 0 )
	{
		Bigiot_Logout(pstCtx);
		return 4;
	}

	pcMethod = BigiotParseString(szMess, "M", szValue, sizeof(szValue));
	if ( 0 != pcMethod && 0 == strcmp(pcMethod, BIGIOT_LOGINT_OK) )
	{
		pcContent = BigiotParseString(szMess, "NAME", szValue, sizeof(szValue));
		if ( 0 != pcContent )
		{
			strncpy( pstCtx->szDevName, pcContent, BIGIOT_DEVNAME_LEN );
		}
		return 0;
	}
	BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Login failed, unknown method:%s", pcMethod);
	return 6;
}


int Bigiot_Cycle( BIGIOT_Ctx_S *pstCtx )
{
	char szMess[150] = { 0 };
	char szValue[100] = { 0 };
	char* pcMethod = 0;
	char* pcContent = 0;
	int iRet = 0;

	//判断是否有数据发送过来
	iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
	if ( iRet < 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "Read failed");
    	return iRet;
	}

	if ( iRet > 0 )
	{
		//有指令发送过来
		pcMethod = BigiotParseString(szMess, "M", szValue, sizeof(szValue));
		if ( 0 != pcMethod && 0 == strcmp(pcMethod, BIGIOT_SAY) )
		{
			pcContent = BigiotParseString(szMess, "C", szValue, sizeof(szValue));
			if ( 0 != pcContent )
			{
				//打开开关的指令
				if ( 0 == strcmp(pcContent, BIGIOT_ON) )
				{
					BIGIOT_LOG(BIGIOT_INFO, "Conent: %s", pcContent);
					PLUG_SetRelayByStatus( 1, 1 );
				}
				//关闭开关的指令
				else if ( 0 == strcmp(pcContent, BIGIOT_OFF) )
				{
					BIGIOT_LOG(BIGIOT_INFO, "Conent: %s", pcContent);
					PLUG_SetRelayByStatus( 0, 1 );
				}
				//其他指令
				else
				{
					BIGIOT_LOG(BIGIOT_INFO, "recv content:%s", pcContent);
				}
			}
			else
			{
				BIGIOT_LOG(BIGIOT_ERROR, "BigiotParseString failed, szMess:%s", szMess);
			}
		}
		//有其他途径登陆到贝壳物联平台如通过页面或者微信公众号等会收到上线的通知
		else if ( 0 != pcMethod && 0 == strcmp(pcMethod, BIGIOT_LOGIN) )
		{
			pcContent = BigiotParseString(szMess, "NAME", szValue, sizeof(szValue));
			if ( 0 != pcContent )
			{
				BIGIOT_LOG(BIGIOT_INFO, "%s has login", pcContent);
			}
		}
		//有用户离线通知
		else if ( 0 != pcMethod && 0 == strcmp(pcMethod, BIGIOT_LOGOUT) )
		{
			pcContent = BigiotParseString(szMess, "NAME", szValue, sizeof(szValue));
			if ( 0 != pcContent )
			{
				BIGIOT_LOG(BIGIOT_INFO, "%s has logout", pcContent);
			}
		}
	}

	return 0;
}

int Bigiot_EventRegister( BIGIOT_Ctx_S *pstCtx, BIGIOT_Event_S* pstEvent )
{
	UINT i = 0;

	if ( pstCtx == NULL )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "pstCtx is NULL");
		return 1;
	}

	for ( i = 0; i < BIGIOT_EVENT_NUM; i++ )
	{
		if ( pstCtx->astEvent[i].cb == 0 )
		{
			strncpy(pstCtx->astEvent[i].szCbName, pstEvent->szCbName, BIGIOT_CBNAME_NUM);

			pstCtx->astEvent[i].lNextTime 	= 0;
			pstCtx->astEvent[i].pcIfId		= pstEvent->pcIfId;
			pstCtx->astEvent[i].cb 			= pstEvent->cb;
			pstCtx->astEvent[i].cbPara 		= &pstCtx->astEvent[i];
			pstCtx->astEvent[i].pstCtx  	= pstCtx;

			BIGIOT_LOG(BIGIOT_INFO, "Regist %s success", pstCtx->astEvent[i].szCbName);
			return 0;
		}
	}

	if ( i >= BIGIOT_EVENT_NUM )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "Event num(%d) upper limit", BIGIOT_EVENT_NUM);
		return 1;
	}

	return 1;
}

void Bigiot_EventCallBack( BIGIOT_Ctx_S *pstCtx )
{
	UINT8 i = 0;
	int iRet = 0;
	sntp_time_t NowTime;
	static UINT8 ucEventNum = 0;

	if ( ucEventNum == 0 )
	{
		for ( i = 0; i < BIGIOT_EVENT_NUM; i++ )
		{
			if ( pstCtx->astEvent[i].cb != NULL )
			{
				ucEventNum++;
			}
		}
	}

	for ( i = 0; i < BIGIOT_EVENT_NUM; i++ )
	{
		NowTime = sntp_get_current_timestamp();
		if ( pstCtx->astEvent[i].cb != NULL && NowTime > pstCtx->astEvent[i].lNextTime )
		{
			iRet = pstCtx->astEvent[i].cb( pstCtx->astEvent[i].cbPara );
			if ( !iRet )
			{
				pstCtx->astEvent[i].lNextTime = NowTime + ucEventNum * BIGIOT_TIME_DEFAULT;
				return;
			}
		}
	}
	return;
}

static int Bigiot_DeviceIDRegister( BIGIOT_Ctx_S *pstCtx )
{
	BIGIOT_Event_S stEvent;
	int iRet = 0;
	UINT uiDevType = 0;

	uiDevType = PLUG_GetBigiotDeviceType();

	//心跳事件注册
	stEvent.pcIfId = NULL;
	stEvent.cb = Bigiot_HeartBeatCallBack;
	strncpy(stEvent.szCbName, "Bigiot_HeartBeatCallBack", BIGIOT_CBNAME_NUM);
	iRet = Bigiot_EventRegister( pstCtx, &stEvent );
	if ( iRet )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "Registe %s failed", stEvent.szCbName);
		return 1;
	}

	//开关状态事件注册
	stEvent.pcIfId = PLUG_GetBigiotSwitchId();
	stEvent.cb = Bigiot_RelayStatusCallBack;
	strncpy(stEvent.szCbName, "Bigiot_RelayStatusCallBack", BIGIOT_CBNAME_NUM);
	if ( strlen(stEvent.pcIfId) != 0 )
	{
		iRet = Bigiot_EventRegister( pstCli, &stEvent );
		if ( iRet )
		{
			BIGIOT_LOG(BIGIOT_ERROR, "Registe %s failed", stEvent.szCbName);
			return 1;
		}
	}

	if ( uiDevType == DEVTYPE_humidifier )
	{
		stEvent.pcIfId = PLUG_GetBigiotHumidityId();
		stEvent.cb = Bigiot_UploadHumidityCallBack;
		strncpy(stEvent.szCbName, "Bigiot_UploadHumidityCallBack", BIGIOT_CBNAME_NUM);
		if ( strlen(stEvent.pcIfId) != 0 )
		{
			iRet = Bigiot_EventRegister( pstCli, &stEvent );
			if ( iRet )
			{
				BIGIOT_LOG(BIGIOT_ERROR, "Register %s failed", stEvent.szCbName);
				return 1;
			}
		}
	}
	else if ( uiDevType == DEVTYPE_socket )
	{
		stEvent.pcIfId = PLUG_GetBigiotTempId();
		stEvent.cb = Bigiot_UploadTempCallBack;
		strncpy(stEvent.szCbName, "Bigiot_UploadTempCallBack", BIGIOT_CBNAME_NUM);
		if ( strlen(stEvent.pcIfId) != 0 )
		{
			iRet = Bigiot_EventRegister( pstCli, &stEvent );
			if ( iRet )
			{
				BIGIOT_LOG(BIGIOT_ERROR, "Register %s failed", stEvent.szCbName);
				return 1;
			}
		}

		stEvent.pcIfId = PLUG_GetBigiotVoltageId();
		stEvent.cb = Bigiot_UploadVoltageCallBack;
		strncpy(stEvent.szCbName, "Bigiot_UploadVoltageCallBack", BIGIOT_CBNAME_NUM);
		if ( strlen(stEvent.pcIfId) != 0 )
		{
			iRet = Bigiot_EventRegister( pstCli, &stEvent );
			if ( iRet )
			{
				BIGIOT_LOG(BIGIOT_ERROR, "Register %s failed", stEvent.szCbName);
				return 1;
			}
		}

		stEvent.pcIfId = PLUG_GetBigiotCurrentId();
		stEvent.cb = Bigiot_UploadCurrentCallBack;
		strncpy(stEvent.szCbName, "Bigiot_UploadCurrentCallBack", BIGIOT_CBNAME_NUM);
		if ( strlen(stEvent.pcIfId) != 0 )
		{
			iRet = Bigiot_EventRegister( pstCli, &stEvent );
			if ( iRet )
			{
				BIGIOT_LOG(BIGIOT_ERROR, "Register %s failed", stEvent.szCbName);
				return 1;
			}
		}

		stEvent.pcIfId = PLUG_GetBigiotPowerId();
		stEvent.cb = Bigiot_UploadPowerCallBack;
		strncpy(stEvent.szCbName, "Bigiot_UploadPowerCallBack", BIGIOT_CBNAME_NUM);
		if ( strlen(stEvent.pcIfId) != 0 )
		{
			iRet = Bigiot_EventRegister( pstCli, &stEvent );
			if ( iRet )
			{
				BIGIOT_LOG(BIGIOT_ERROR, "Register %s failed", stEvent.szCbName);
				return 1;
			}
		}

		stEvent.pcIfId = PLUG_GetBigiotElectricityId();
		stEvent.cb = Bigiot_UploadElectricityCallBack;
		strncpy(stEvent.szCbName, "Bigiot_UploadElectricityCallBack", BIGIOT_CBNAME_NUM);
		if ( strlen(stEvent.pcIfId) != 0 )
		{
			iRet = Bigiot_EventRegister( pstCli, &stEvent );
			if ( iRet )
			{
				BIGIOT_LOG(BIGIOT_ERROR, "Register %s failed", stEvent.szCbName);
				return 1;
			}
		}
	}

	return 0;
}

int Bigiot_GetBigioStatus( void )
{
	if ( pstCli == NULL )
	{
		return 0;
	}

	return pstCli->iAlived;
}

char* Bigiot_GetBigioDeviceName( void )
{
	if ( pstCli == NULL )
	{
		return 0;
	}

	return pstCli->szDevName;
}

static char* BigiotParseString( const char* pcData, const char* pcKey, char*pcValue, unsigned int uiLen )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonIteam = NULL;

	if ( pcData == 0 || pcKey == 0 || pcValue == 0)
	{
	    BIGIOT_LOG(BIGIOT_ERROR, "pData,pcKey or pcValue is NULL");
	    return 0;
	}

	pJsonRoot = cJSON_Parse( pcData );
	if ( pJsonRoot == 0 )
	{
	    BIGIOT_LOG(BIGIOT_ERROR, "cJSON_Parse failed, pData:%s", pcData);
	    goto exit;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, pcKey );
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy( pcValue, pJsonIteam->valuestring, uiLen );
		cJSON_Delete(pJsonRoot);
		return pcValue;
	}

exit:
	cJSON_Delete(pJsonRoot);
	return 0;
}

static int BigiotParseInt( const char* pcData, const char* pcKey )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonIteam = NULL;
	int iValue = 999999;	//999999表示解析数字出错

	if ( pcData == 0 || pcKey == 0 )
	{
	    BIGIOT_LOG(BIGIOT_ERROR, "pData or pcKey is NULL");
	    return iValue;
	}

	pJsonRoot = cJSON_Parse( pcData );
	if ( pJsonRoot == 0 )
	{
	    BIGIOT_LOG(BIGIOT_ERROR, "cJSON_Parse failed, pData:%s", pcData);
	    goto exit;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, pcKey );
	if (pJsonIteam && pJsonIteam->type == cJSON_Number)
	{
		iValue = pJsonIteam->valueint;
		cJSON_Delete(pJsonRoot);
		return iValue;
	}

exit:
	cJSON_Delete(pJsonRoot);
	return iValue;
}

int Bigiot_Logout( BIGIOT_Ctx_S *pstCtx )
{
	char szMess[100] = { 0 };
	int iLen = 0;
	int iRet = 0;

	iLen = snprintf(szMess, sizeof(szMess), "{\"M\":\"checkout\",\"ID\":\"%s\",\"K\":\"%s\"}\n",
					pstCtx->pcDeviceId, pstCtx->pcApiKey);

	iRet = pstCtx->Write( pstCtx, szMess, iLen, pstCtx->iTimeOut );
	if ( iRet != iLen )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Logout failed");
    	return 1;
	}

	iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
	if ( iRet < 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Logout failed");
    	return 1;
	}

	return 0;
}

static int Bigiot_SendMessage( BIGIOT_Ctx_S *pstCtx, char* pcIfId, char* pcValue )
{
	char szMsg[100] = { 0 };
	int iLen = 0;
	int iRet = 0;

	iLen = snprintf(szMsg, sizeof(szMsg), "{\"M\":\"update\",\"ID\":\"%s\",\"V\":{\"%s\":\"%s\"}}\n",
					pstCtx->pcDeviceId, pcIfId, pcValue);

	//BIGIOT_LOG(BIGIOT_INFO, "msg: %s", szMsg);
	iRet = pstCtx->Write( pstCtx, szMsg, iLen, pstCtx->iTimeOut );
	if ( iRet != iLen )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_SendMessage failed, msg:%s", szMsg);
    	return 1;
	}

	return 0;
}

static int Bigiot_HeartBeatCallBack( void * para )
{
	BIGIOT_Event_S *pstEvn = para;
	BIGIOT_Ctx_S *pstCtx = pstEvn->pstCtx;
	const char* pcMsg = "{\"M\":\"beat\"}\n";
	static int iFailedCnt = 0;
	int iRet = 0;

	iRet = pstCtx->Write( pstCtx, pcMsg, strlen(pcMsg), pstCtx->iTimeOut );
	if ( iRet != strlen(pcMsg) )
	{
		iFailedCnt++;
		if ( iFailedCnt >= 2 )
		{
			BIGIOT_LOG(BIGIOT_ERROR, "HeartBeat send failed");
			pstCtx->iAlived = 0;
			iFailedCnt = 0;
			return 1;
		}
	}

	BIGIOT_LOG(BIGIOT_DEBUG, "send heartbeat");
	pstCtx->iAlived = 1;
	iFailedCnt = 0;

	return 0;
}

static int Bigiot_RelayStatusCallBack( void * para )
{
	BIGIOT_Event_S *pstEvn = para;
	unsigned int uiRelayStatus = 0;
	static unsigned int uiLastRelayStatus = 0xFF;
	int iRet = 0;

	uiRelayStatus = PLUG_GetRelayStatus();
	if ( uiRelayStatus != uiLastRelayStatus )
	{
		uiLastRelayStatus = uiRelayStatus;

		iRet = Bigiot_SendMessage( pstEvn->pstCtx, pstEvn->pcIfId, (uiRelayStatus ? "1" : "0" ));
		if ( iRet )
		{
	    	BIGIOT_LOG(BIGIOT_ERROR, "send uiRelayStatus(%d) failed", uiRelayStatus);
	    	return 1;
		}
		BIGIOT_LOG(BIGIOT_DEBUG, "send uiRelayStatus(%d)", uiRelayStatus);
		return 0;
	}

	return 1;
}

static int Bigiot_UploadTempCallBack( void * para )
{
	BIGIOT_Event_S *pstEvn = para;
	char szBuf[20] = {0};
	int iRet = 0;

	snprintf(szBuf, sizeof(szBuf), "%2.1f", TEMP_GetTemperature());

	iRet = Bigiot_SendMessage( pstEvn->pstCtx, pstEvn->pcIfId, szBuf );
	if ( iRet )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "send Temperature(%s) failed", szBuf);
    	return 1;
	}
	BIGIOT_LOG(BIGIOT_DEBUG, "send Temperature(%s)", szBuf);
	return 0;
}

static int Bigiot_UploadHumidityCallBack( void * para )
{
	BIGIOT_Event_S *pstEvn = para;
	int iRet = 0;

	iRet = Bigiot_SendMessage( pstEvn->pstCtx, pstEvn->pcIfId, "50");
	if ( iRet )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "send Humidity(%d) failed", "50");
    	return 1;
	}
	BIGIOT_LOG(BIGIOT_DEBUG, "send Humidity(%d)", "50");
	return 0;
}

static int Bigiot_UploadVoltageCallBack( void * para )
{
	BIGIOT_Event_S *pstEvn = para;
	char szBuf[20] = {0};
	int iRet = 0;

	snprintf(szBuf, sizeof(szBuf), "%3.1f", METER_GetMeterVoltage());
	iRet = Bigiot_SendMessage( pstEvn->pstCtx, pstEvn->pcIfId, szBuf );
	if ( iRet )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "send Voltage(%s) failed", szBuf);
    	return 1;
	}
	BIGIOT_LOG(BIGIOT_DEBUG, "send Voltage(%s)", szBuf);
	return 0;
}

static int Bigiot_UploadCurrentCallBack( void * para )
{
	BIGIOT_Event_S *pstEvn = para;
	char szBuf[20] = {0};
	float fData = 0;
	int iRet = 0;

	fData = METER_GetMeterCurrent();
	snprintf(szBuf, sizeof(szBuf), "%3.1f", fData);

	iRet = Bigiot_SendMessage( pstEvn->pstCtx, pstEvn->pcIfId, szBuf );
	if ( iRet )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "send Current(%s) failed", szBuf);
    	return 1;
	}
	BIGIOT_LOG(BIGIOT_DEBUG, "send Current(%s)", szBuf);
	return 0;
}

static int Bigiot_UploadPowerCallBack( void * para )
{
	BIGIOT_Event_S *pstEvn = para;
	char szBuf[20] = {0};
	int iRet = 0;

	snprintf(szBuf, sizeof(szBuf), "%3.1f", METER_GetMeterPower());

	iRet = Bigiot_SendMessage( pstEvn->pstCtx, pstEvn->pcIfId, szBuf );
	if ( iRet )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "send Power(%s) failed", szBuf);
    	return 1;
	}
	BIGIOT_LOG(BIGIOT_DEBUG, "send Power(%s)", szBuf);
	return 0;
}

static int Bigiot_UploadElectricityCallBack( void * para )
{
	BIGIOT_Event_S *pstEvn = para;
	char szBuf[20] = {0};
	int iRet = 0;

	snprintf(szBuf, sizeof(szBuf), "%3.1f", METER_GetMeterElectricity());

	iRet = Bigiot_SendMessage( pstEvn->pstCtx, pstEvn->pcIfId, szBuf );
	if ( iRet )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "send Electricity(%s) failed", szBuf);
    	return 1;
	}
	BIGIOT_LOG(BIGIOT_DEBUG, "send Electricity(%s)", szBuf);
	return 0;
}

static void Init( BIGIOT_Ctx_S *pstCtx, char* pcHostName, int iPort, char* pcDevId, char* pcApiKey )
{
	pstCtx->socket			= -1;
	pstCtx->pcHostName 		= pcHostName;
	pstCtx->port	   		= iPort;
	pstCtx->pcDeviceId		= pcDevId;
	pstCtx->pcApiKey		= pcApiKey;

	pstCtx->xEventHandle 	= 0;
	pstCtx->iAlived			= 0;

	pstCtx->iTimeOut 		= BIGIOT_TIMEOUT;
	pstCtx->Read 			= Read;
	pstCtx->Write 			= Write;
	pstCtx->Connect 		= Connect;
	pstCtx->Disconnect 		= Disconnect;

	memset( pstCtx->szDevName, 0, BIGIOT_DEVNAME_LEN );
	memset( pstCtx->astEvent, 0, sizeof(pstCtx->astEvent) );
}

static int Connect(BIGIOT_Ctx_S* pstCtx)
{
    struct sockaddr_in sAddr;
    int iRet = -1;
    struct hostent* ipAddress;
	char szMess[100] = { 0 };
	char szValue[100] = { 0 };
	char* pcMethod = 0;

    //获取www.bigiot.net域名对应的ip
    ipAddress = gethostbyname( pstCtx->pcHostName );
    if ( ipAddress == NULL ) {
    	BIGIOT_LOG(BIGIOT_ERROR, "gethostbyname failed, name:%s", pstCtx->pcHostName);
    	return 1;
    }

    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = ((struct in_addr*)(ipAddress->h_addr))->s_addr;
    sAddr.sin_port = htons(pstCtx->port);

    pstCtx->socket = socket(AF_INET, SOCK_STREAM, 0);
    if ( pstCtx->socket < 0 )
    {
    	BIGIOT_LOG(BIGIOT_ERROR, "socket failed, socket:%d", pstCtx->socket);
    	return 2;
    }

    iRet = connect(pstCtx->socket, (struct sockaddr*)&sAddr, sizeof(sAddr));
    if ( iRet < 0)
    {
    	BIGIOT_LOG(BIGIOT_ERROR, "connect failed, socket:%d, ret:%d", pstCtx->socket, iRet);
    	close( pstCtx->socket );
    	pstCtx->socket = -1;
    	return 3;
    }

	iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
	if ( iRet < 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "connect read failed");
    	return 4;
	}

	pcMethod = BigiotParseString(szMess, "M", szValue, sizeof(szValue));
	if ( 0 != pcMethod )
	{
		if ( 0 == strcmp(pcMethod, BIGIOT_CONNECT_OK) )
		{
			return 0;
		}
		BIGIOT_LOG(BIGIOT_ERROR, "connect failed, unknown method:%s", pcMethod);
	}

	return 5;

}

static void Disconnect(BIGIOT_Ctx_S* pstCtx)
{
	if ( pstCtx->socket >= 0 )
	{
		close( pstCtx->socket );
	}
}

static int Write( BIGIOT_Ctx_S* pstCtx, const unsigned char* buffer, unsigned int len, unsigned int timeout_s )
{
    portTickType xTicksToWait = timeout_s*1000 / portTICK_RATE_MS;
    xTimeOutType xTimeOut;
    int sentLen = 0;
    int rc = 0;
    int readysock;
    struct timeval timeout;
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(pstCtx->socket, &fdset);

    timeout.tv_sec = timeout_s;
    timeout.tv_usec = 0;

    vTaskSetTimeOutState(&xTimeOut);

	readysock = select( pstCtx->socket + 1, 0, &fdset, 0, &timeout );
	if ( readysock <= 0 )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "socket failed, socket:%d", pstCtx->socket);
		goto exit;
	}

    if ( FD_ISSET(pstCtx->socket, &fdset) )
    {
        do
        {
            rc = send(pstCtx->socket, buffer + sentLen, len - sentLen, MSG_DONTWAIT);
            if ( rc > 0 )
            {
                sentLen += rc;
            }
            else if (rc < 0)
            {
                sentLen = rc;
                BIGIOT_LOG(BIGIOT_ERROR, "send failed, socket:%d", pstCtx->socket);
                break;
            }

        }while (sentLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
    }

    return sentLen;

exit:
    return -1;
}


static int Read(BIGIOT_Ctx_S* pstCtx, unsigned char* buffer, unsigned int len, unsigned int timeout_s)
{
    portTickType xTicksToWait = timeout_s *1000 / portTICK_RATE_MS;
    xTimeOutType xTimeOut;
    int recvLen = 0;
    int rc = 0;

    struct timeval timeout;
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(pstCtx->socket, &fdset);

    timeout.tv_sec = timeout_s;
    timeout.tv_usec = 0;

    vTaskSetTimeOutState(&xTimeOut);

    rc = select(pstCtx->socket + 1, &fdset, (void*)0, (void*)0, &timeout);
    if ( rc > 0)
    {
        if ( FD_ISSET(pstCtx->socket, &fdset) )
        {
            do
            {
                rc = recv(pstCtx->socket, buffer + recvLen, len - recvLen, MSG_DONTWAIT);
                if (rc > 0)
                {
                	while( rc-- )
                	{
                		if ( buffer[recvLen] == '\n' )
                		{
                			buffer[recvLen] = 0;
                			goto exit;
                		}
                		recvLen++;
                	}
                }
                else if (rc < 0)
                {
                    recvLen = rc;
                    BIGIOT_LOG(BIGIOT_ERROR, "recv failed, socket:%d", pstCtx->socket);
                    return rc;
                }

            } while (recvLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
        }
    }
    else
    {
    	return rc;
    }

exit:
    return recvLen;
}


