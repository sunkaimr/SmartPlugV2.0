
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

#define BIGIOT_IF_SW		"switch"
#define BIGIOT_IF_TEMP		"tempture"
#define BIGIOT_IF_HUMI		"HumidityId"


#define BIGIOT_TIME_HEARTBEAT		40
#define BIGIOT_TIME_SW_UPLOAD		5
#define BIGIOT_TIME_TEMP_UPLOAD		60
#define BIGIOT_TIME_HUMI_UPLOAD		60


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

static int Bigiot_JudgeHeartBeat( void );
static int Bigiot_HeartBeatCallBack( void * para );
static int Bigiot_JudgeRelayStatus( void );
static int Bigiot_RelayStatusCallBack( void * para );
static int Bigiot_JudgeUploadTemp( void );
static int Bigiot_UploadTempCallBack( void * para );
static int Bigiot_JudgeUploadHumidity( void );
static int Bigiot_UploadHumidityCallBack( void * para );

BIGIOT_Ctx_S* pstCli = NULL;


static void BIGIOT_BigiotTask( void* para )
{
	int iRet = -1;
	char* pcDevId = 0;
	char* pcApiKey = 0;
	BIGIOT_Event_S stHeartBeat = {"", "", Bigiot_JudgeHeartBeat, "Bigiot_HeartBeatCallBack", Bigiot_HeartBeatCallBack, pstCli};
	BIGIOT_Event_S stSwitch = {BIGIOT_IF_SW, "", Bigiot_JudgeRelayStatus, "Bigiot_RelayStatusCallBack", Bigiot_RelayStatusCallBack, pstCli};
	BIGIOT_Event_S stTemp = {BIGIOT_IF_TEMP, "", Bigiot_JudgeUploadTemp, "Bigiot_UploadTempCallBack", Bigiot_UploadTempCallBack, pstCli};
	BIGIOT_Event_S stHumidity = {BIGIOT_IF_HUMI, "", Bigiot_JudgeUploadHumidity, "Bigiot_UploadHumidityCallBack", Bigiot_UploadHumidityCallBack, pstCli};

	pcDevId = PLUG_GetBigiotDevId();
	if ( pcDevId == 0 || pcDevId[0] == 0 || pcDevId[0] == 0xFF )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "pcDevId is invalid");
		return;
	}

	pcApiKey = PLUG_GetBigiotApiKey();
	if ( pcApiKey == 0 || pcApiKey[0] == 0 || pcApiKey[0] == 0xFF )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "pcApiKey is invalid");
		return;
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

	stHeartBeat.cbPara = pstCli;
	iRet = Bigiot_EventRegister( pstCli, &stHeartBeat );
	if ( iRet )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "Register Bigiot_HeartBeatCallBack failed");
		goto exit;
	}
	BIGIOT_LOG(BIGIOT_INFO, "Register Bigiot_HeartBeatCallBack success");

	stSwitch.cbPara = pstCli;
	stSwitch.pcIfId = PLUG_GetBigiotSwitchId();
	if ( strlen(stSwitch.pcIfId) != 0 )
	{
		iRet = Bigiot_EventRegister( pstCli, &stSwitch );
		if ( iRet )
		{
			BIGIOT_LOG(BIGIOT_ERROR, "Register Bigiot_RelayStatusCallBack failed");
			goto exit;
		}
		BIGIOT_LOG(BIGIOT_INFO, "Register Bigiot_RelayStatusCallBack success");
	}

	stTemp.cbPara = pstCli;
	stTemp.pcIfId = PLUG_GetBigiotTempId();
	if ( strlen(stTemp.pcIfId) != 0 )
	{
		iRet = Bigiot_EventRegister( pstCli, &stTemp );
		if ( iRet )
		{
			BIGIOT_LOG(BIGIOT_ERROR, "Register Bigiot_UploadTempCallBack failed");
			goto exit;
		}
		BIGIOT_LOG(BIGIOT_INFO, "Register Bigiot_UploadTempCallBack success");
	}

	stHumidity.cbPara = pstCli;
	stHumidity.pcIfId = PLUG_GetBigiotHumidityId();
	if ( strlen(stHumidity.pcIfId) != 0 )
	{
		iRet = Bigiot_EventRegister( pstCli, &stHumidity );
		if ( iRet )
		{
			BIGIOT_LOG(BIGIOT_ERROR, "Register Bigiot_UploadHumidityCallBack failed");
			goto exit;
		}
		BIGIOT_LOG(BIGIOT_INFO, "Register Bigiot_UploadHumidityCallBack success");
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
			Bigiot_Cycle( pstCli );

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
		if ( (*ppstCtx)->xKeepLiveHandle != 0 )
		{
			vTaskDelete( (*ppstCtx)->xKeepLiveHandle );
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

		vTaskDelay( 1000 / portTICK_RATE_MS );
	}
}

static int Bigiot_StartEventTask( BIGIOT_Ctx_S *pstCtx )
{
	if ( pdPASS != xTaskCreate( Bigiot_EventHanderTask,
								"Bigiot_EventHanderTask",
								configMINIMAL_STACK_SIZE * 5,
								(void*)pstCtx,
								uxTaskPriorityGet(NULL),
								&(pstCtx->xKeepLiveHandle)))
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

    iRet = pstCtx->Connect( pstCtx );
	if ( iRet )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "Connect failed");
		return 1;
	}

	iLen = snprintf(szMess, sizeof(szMess), "{\"M\":\"checkin\",\"ID\":\"%s\",\"K\":\"%s\"}\n",
					pstCtx->pcDeviceId, pstCtx->pcApiKey );

	iRet = pstCtx->Write( pstCtx, szMess, iLen, pstCtx->iTimeOut );
	if ( iRet != iLen )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Login failed");
    	return 2;
	}

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

	iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
	if ( iRet < 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Cycle failed");
    	return iRet;
	}

	if ( iRet > 0 )
	{
		pcMethod = BigiotParseString(szMess, "M", szValue, sizeof(szValue));
		if ( 0 != pcMethod && 0 == strcmp(pcMethod, BIGIOT_SAY) )
		{
			pcContent = BigiotParseString(szMess, "C", szValue, sizeof(szValue));
			if ( 0 != pcContent )
			{
				if ( 0 == strcmp(pcContent, BIGIOT_ON) )
				{
					BIGIOT_LOG(BIGIOT_INFO, "Conent: %s", pcContent);
					PLUG_SetRelayByStatus( 1, 1 );
				}
				else if ( 0 == strcmp(pcContent, BIGIOT_OFF) )
				{
					BIGIOT_LOG(BIGIOT_INFO, "Conent: %s", pcContent);
					PLUG_SetRelayByStatus( 0, 1 );
				}
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
		else if ( 0 != pcMethod && 0 == strcmp(pcMethod, BIGIOT_LOGIN) )
		{
			pcContent = BigiotParseString(szMess, "NAME", szValue, sizeof(szValue));
			if ( 0 != pcContent )
			{
				BIGIOT_LOG(BIGIOT_INFO, "%s has login", pcContent);

				//有用户登陆马上上报开关状态
				Bigiot_RelayStatusCallBack( pstCtx );
			}
		}
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
		if ( pstCtx->astEvent[i].tf == 0 )
		{
			pstCtx->astEvent[i].pcIfName 	= pstEvent->pcIfName;
			pstCtx->astEvent[i].pcIfId		= pstEvent->pcIfId;
			pstCtx->astEvent[i].tf 			= pstEvent->tf;
			pstCtx->astEvent[i].cbName 		= pstEvent->cbName;
			pstCtx->astEvent[i].cb 			= pstEvent->cb;
			pstCtx->astEvent[i].cbPara 		= pstEvent->cbPara;

			return 0;
		}
	}

	if ( i >= BIGIOT_EVENT_NUM )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "Event is %d and full", BIGIOT_EVENT_NUM);
		return 1;
	}

	return 1;
}

void Bigiot_EventCallBack( BIGIOT_Ctx_S *pstCtx )
{
	int i = 0;
	int iRet = 0;

	for ( i = 0; i < BIGIOT_EVENT_NUM; i++ )
	{
		if ( pstCtx->astEvent[i].tf != NULL && pstCtx->astEvent[i].cb != NULL )
		{
			if ( pstCtx->astEvent[i].tf() )
			{
				iRet = pstCtx->astEvent[i].cb( pstCtx->astEvent[i].cbPara );
				if ( iRet )
				{
					BIGIOT_LOG(BIGIOT_ERROR, "EventCallBack %s failed", pstCtx->astEvent[i].cbName);
				}
			}
		}
	}
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
	    BIGIOT_LOG(BIGIOT_ERROR, "cJSON_Parse failed, pData:%s.", pcData);
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
	    BIGIOT_LOG(BIGIOT_ERROR, "cJSON_Parse failed, pData:%s.", pcData);
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

	//BIGIOT_LOG(BIGIOT_INFO, "msg: %s.", szMsg);
	iRet = pstCtx->Write( pstCtx, szMsg, iLen, pstCtx->iTimeOut );
	if ( iRet != iLen )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_SendMessage failed, msg:%s.", szMsg);
    	return 1;
	}

	return 0;
}

static int Bigiot_JudgeHeartBeat( void )
{
	//心跳间隔为40s
	static 	sntp_time_t LastTime = 0;
	sntp_time_t NowTime;

	NowTime = sntp_get_current_timestamp();
	if ( NowTime > ( LastTime + BIGIOT_TIME_HEARTBEAT ) )
	{
		LastTime = NowTime;
		return 1;
	}
	return 0;
}

static int Bigiot_HeartBeatCallBack( void * para )
{
	BIGIOT_Ctx_S *pstCtx = para;
	const char* pcMsg = "{\"M\":\"beat\"}\n";
	static int iFailedCnt = 0;
	int iRet = 0;

	if ( pstCtx == NULL )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "pstCtx is NULL");
		return 1;
	}

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


static int Bigiot_JudgeRelayStatus( void )
{
	static 	sntp_time_t LastTime = 0;
	sntp_time_t NowTime;
    unsigned int uiRelayStatus = 0;
    static unsigned int uiLastRelayStatus = 0;

	NowTime = sntp_get_current_timestamp();
	if ( NowTime > ( LastTime + BIGIOT_TIME_SW_UPLOAD ) )
	{
		uiRelayStatus = PLUG_GetRelayStatus();
		if ( uiRelayStatus != uiLastRelayStatus )
		{
			uiLastRelayStatus = uiRelayStatus;

			LastTime = NowTime;
			return 1;
		}
	}
	return 0;
}

static int Bigiot_RelayStatusCallBack( void * para )
{
	BIGIOT_Ctx_S *pstCtx = para;
	unsigned int uiRelayStatus = 0;
	int iRet = 0;
	int i = 0;

	if ( pstCtx == NULL )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "pstCtx is NULL");
		return 1;
	}

	for ( i = 0; i < BIGIOT_EVENT_NUM; i++ )
	{
		if ( pstCtx->astEvent[i].pcIfName != NULL &&
			 strcmp(pstCtx->astEvent[i].pcIfName, BIGIOT_IF_SW) == 0 )
		{
			break;
		}
	}

	if ( i >= BIGIOT_EVENT_NUM )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "unknown interface: %s.", pstCtx->astEvent[i].pcIfName);
		return 1;
	}

	uiRelayStatus = PLUG_GetRelayStatus();
	iRet = Bigiot_SendMessage( pstCtx, pstCtx->astEvent[i].pcIfId, (uiRelayStatus ? "1" : "0" ));
	if ( iRet )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "%s send failed", pstCtx->astEvent[i].pcIfName);
    	return 1;
	}
	BIGIOT_LOG(BIGIOT_INFO, "send %s data: %s", pstCtx->astEvent[i].pcIfName, (uiRelayStatus ? "1" : "0" ));
	return 0;
}

static int Bigiot_JudgeUploadTemp( void )
{
	static 	sntp_time_t LastTime = 0;
	sntp_time_t NowTime;

	NowTime = sntp_get_current_timestamp();
	if ( NowTime > ( LastTime + BIGIOT_TIME_TEMP_UPLOAD ) )
	{
		LastTime = NowTime;
		return 1;
	}
	return 0;
}

static int Bigiot_UploadTempCallBack( void * para )
{
	BIGIOT_Ctx_S *pstCtx = para;
	char szBuf[20] = {0};
	int iTemp = 0;
	int iRet = 0;
	int i = 0;

	if ( pstCtx == NULL )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "pstCtx is NULL");
		return 1;
	}

	for ( i = 0; i < BIGIOT_EVENT_NUM; i++ )
	{
		if ( pstCtx->astEvent[i].pcIfName != NULL &&
			 strcmp(pstCtx->astEvent[i].pcIfName, BIGIOT_IF_TEMP) == 0 )
		{
			break;
		}
	}

	if ( i >= BIGIOT_EVENT_NUM )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "unknown interface: %s.", pstCtx->astEvent[i].pcIfName);
		return 1;
	}

	iTemp = TEMP_GetTemperature();
	snprintf(szBuf, sizeof(szBuf), "%d", iTemp);

	iRet = Bigiot_SendMessage( pstCtx, pstCtx->astEvent[i].pcIfId, szBuf );
	if ( iRet )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "%s send failed", pstCtx->astEvent[i].pcIfName);
    	return 1;
	}
	BIGIOT_LOG(BIGIOT_INFO, "send %s data: %s", pstCtx->astEvent[i].pcIfName, szBuf);
	return 0;
}


static int Bigiot_JudgeUploadHumidity( void )
{
	static 	sntp_time_t LastTime = 0;
	sntp_time_t NowTime;

	NowTime = sntp_get_current_timestamp();
	if ( NowTime > ( LastTime + BIGIOT_TIME_HUMI_UPLOAD ) )
	{
		LastTime = NowTime;
		return 1;
	}
	return 0;
}

static int Bigiot_UploadHumidityCallBack( void * para )
{
	BIGIOT_Ctx_S *pstCtx = para;
	unsigned int uiRelayStatus = 0;
	int iRet = 0;
	int i = 0;

	if ( pstCtx == NULL )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "pstCtx is NULL");
		return 1;
	}

	for ( i = 0; i < BIGIOT_EVENT_NUM; i++ )
	{
		if ( pstCtx->astEvent[i].pcIfName != NULL &&
			 strcmp(pstCtx->astEvent[i].pcIfName, BIGIOT_IF_HUMI) == 0 )
		{
			break;
		}
	}

	if ( i >= BIGIOT_EVENT_NUM )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "unknown interface: %s.", pstCtx->astEvent[i].pcIfName);
		return 1;
	}

	iRet = Bigiot_SendMessage( pstCtx, pstCtx->astEvent[i].pcIfId, "50");
	if ( iRet )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "%s send failed", pstCtx->astEvent[i].pcIfName);
    	return 1;
	}
	//BIGIOT_LOG(BIGIOT_DEBUG, "Bigiot_UploadHumidityCallBack success");
	return 0;
}


static void Init( BIGIOT_Ctx_S *pstCtx, char* pcHostName, int iPort, char* pcDevId, char* pcApiKey )
{
	pstCtx->socket			= -1;
	pstCtx->pcHostName 		= pcHostName;
	pstCtx->port	   		= iPort;
	pstCtx->pcDeviceId		= pcDevId;
	pstCtx->pcApiKey		= pcApiKey;

	pstCtx->xKeepLiveHandle = 0;
	pstCtx->iAlived		= 0;

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


