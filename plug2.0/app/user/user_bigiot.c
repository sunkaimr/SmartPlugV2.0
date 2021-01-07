
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

#define BIGIOT_HOSTNAME        "www.bigiot.net"
#define BIGIOT_PORT            ((int)8181)
#define BIGIOT_TIMEOUT         3

#define BIGIOT_CONNECT_OK    "WELCOME TO BIGIOT"
#define BIGIOT_LOGINT_OK    "checkinok"
#define BIGIOT_ON            "play"
#define BIGIOT_OFF            "stop"
#define BIGIOT_SAY            "say"
#define BIGIOT_LOGIN        "login"
#define BIGIOT_LOGOUT        "logout"
#define BIGIOT_CHECKED       "checked"

// 两次发送间隔不得小于5s
#define BIGIOT_MIN_INTERVAL  5

//每40s发送一次，如果两次没有应答（即在80s内没有向服务器发送有效数据），服务将自动与客户端断开连接。
#define BIGIOT_MAX_INTERVAL  40


static void Init( BIGIOT_Ctx_S *pstCtx, char* pcHostName, int iPort, char* pcDevId, char* pcApiKey );
static int Connect(BIGIOT_Ctx_S* pstCtx);
static void Disconnect(BIGIOT_Ctx_S* pstCtx);
static int Write( BIGIOT_Ctx_S* pstCtx, const unsigned char* buffer, unsigned int len, unsigned int timeout_s );
static int Read(BIGIOT_Ctx_S* pstCtx, unsigned char* buffer, unsigned int len, unsigned int timeout_s);
static int Bigiot_EventRegister( BIGIOT_Ctx_S *pstCtx, BIGIOT_Event_S* pstEvent );
static void Bigiot_EventCallBack( BIGIOT_Ctx_S *pstCtx );
static int Bigiot_SendIfMsg( BIGIOT_Ctx_S *pstCtx, char* pcIfId, char* pcValue );
static int Bigiot_SendMultipleIfMsg( BIGIOT_Ctx_S *pstCtx, char* pcIf );
static int Bigiot_StartEventTask( BIGIOT_Ctx_S *pstCtx );

static char* BigiotParseString( const char* pcData, const char* pcKey, char*pcValue, unsigned int uiLen );
static int BigiotParseInt( const char* pcData, const char* pcKey );

static int Bigiot_DeviceIDRegister( BIGIOT_Ctx_S *pstCtx );
static int Bigiot_HeartBeat( void * para );
static int Bigiot_HealthCheck( void * para );
static char*  Bigiot_GenerateRelayStatus( void * para );
static char*  Bigiot_GenerateTemp( void * para );
static char*  Bigiot_GenerateHumidity( void * para );
static char*  Bigiot_GenerateVoltage( void * para );
static char*  Bigiot_GenerateCurrent( void * para );
static char*  Bigiot_GeneratePower( void * para );
static char*  Bigiot_GenerateElectricity( void * para );
static int Bigiot_UploadAllIfData( BIGIOT_Ctx_S *pstCtx );
static int WaitReadLock(BIGIOT_Ctx_S* pstCtx, unsigned int timeout_ms);
static void ReleaseReadLock(BIGIOT_Ctx_S* pstCtx);

BIGIOT_Ctx_S* pstCli = NULL;

static void BIGIOT_BigiotTask( void* para )
{
    int iRet = -1;
    char* pcDevId = 0;
    char* pcApiKey = 0;
    unsigned char iCount = 0;
    static unsigned char iFailCount = 0;

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
    iFailCount = 0;
    //等待wifi连接就绪
    while ( STATION_GOT_IP != wifi_station_get_connect_status() )
    {
        BIGIOT_LOG(BIGIOT_DEBUG, "wait for network");
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
        	iCount++;
        	// 40s一次健康检查, Bigiot_Cycle需要消耗3s+延时1s
        	if ( iCount%10 == 0 )
        	{
        		iCount = 0;
        		iRet = Bigiot_HealthCheck(pstCli);
        		if ( iRet !=0 )
        		{
        			iFailCount++;
        			// 健康检查连续3次失败重新登陆
        			if ( iFailCount >= 3 )
        			{
        				pstCli->iAlived = 3;
        			}
        		}
        	}

        	if ( pstCli->iAlived == 3 )
        	{
                BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_HealthCheck failed, will reconnect");
                goto exit;
        	}

            Bigiot_Cycle( pstCli );

            vTaskDelay( 1000/portTICK_RATE_MS );
        }
    }

exit:
    vTaskDelay( 3000/portTICK_RATE_MS );
    BIGIOT_LOG(BIGIOT_INFO, "BIGIOT_BigiotTask stop");
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
        vTaskDelay( 1000 / portTICK_RATE_MS );
    }
}

static int Bigiot_StartEventTask( BIGIOT_Ctx_S *pstCtx )
{
    if ( pdPASS != xTaskCreate( Bigiot_EventHanderTask,
                                "Bigiot_EventHanderTask",
                                512,
                                (void*)pstCtx,
                                6,
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

    pstCli->iAlived = 1;

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

    // 先加锁
    WaitReadLock(pstCtx, 0);

    iRet = pstCtx->Write( pstCtx, szMess, iLen, pstCtx->iTimeOut );
    if ( iRet != iLen )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Login failed");
        return 2;
    }

    //登陆成功会返回checkinok和设备名称等字段
    iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
    // 释放锁
    ReleaseReadLock(pstCtx);

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

        pstCli->iAlived = 2;

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

    WaitReadLock(pstCtx, 0);

    //判断是否有数据发送过来
    iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );

    ReleaseReadLock(pstCtx);

    if ( iRet < 0 )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "Read failed");
        return 1;
    }

    if ( iRet > 0 )
    {
        BIGIOT_LOG(BIGIOT_DEBUG, "szMess:%s", szMess);

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
                Bigiot_UploadAllIfData( pstCtx );
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

            pstCtx->astEvent[i].pcIfId        = pstEvent->pcIfId;
            pstCtx->astEvent[i].cb             = pstEvent->cb;
            pstCtx->astEvent[i].cbPara         = &pstCtx->astEvent[i];
            pstCtx->astEvent[i].pstCtx      = pstCtx;

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
    sntp_time_t NowTime;
    static sntp_time_t lLastTime = 0;
    char *pcBuf = NULL;
    UINT uiPos = 0;
    char szMsg[200] = {};

    NowTime = sntp_get_current_timestamp();

    if ( (NowTime - lLastTime) > BIGIOT_MAX_INTERVAL )
    {
        Bigiot_HeartBeat( pstCtx );
        lLastTime = NowTime;
    }
    else if ( (NowTime - lLastTime) > BIGIOT_MIN_INTERVAL )
    {
        Bigiot_UploadAllIfData( pstCtx );
        lLastTime = NowTime;
    }
}

static int Bigiot_DeviceIDRegister( BIGIOT_Ctx_S *pstCtx )
{
    BIGIOT_Event_S stEvent;
    int iRet = 0;
    UINT uiDevType = 0;

    uiDevType = PLUG_GetBigiotDeviceType();

    //开关状态事件注册
    stEvent.pcIfId = PLUG_GetBigiotSwitchId();
    stEvent.cb = Bigiot_GenerateRelayStatus;
    strncpy(stEvent.szCbName, "Bigiot_GenerateRelayStatus", BIGIOT_CBNAME_NUM);
    if ( strlen(stEvent.pcIfId) != 0 )
    {
        iRet = Bigiot_EventRegister( pstCli, &stEvent );
        if ( iRet )
        {
            BIGIOT_LOG(BIGIOT_ERROR, "Registe %s failed", stEvent.szCbName);
            return 1;
        }
    }

    stEvent.pcIfId = PLUG_GetBigiotHumidityId();
    stEvent.cb = Bigiot_GenerateHumidity;
    strncpy(stEvent.szCbName, "Bigiot_GenerateHumidity", BIGIOT_CBNAME_NUM);
    if ( strlen(stEvent.pcIfId) != 0 )
    {
        iRet = Bigiot_EventRegister( pstCli, &stEvent );
        if ( iRet )
        {
            BIGIOT_LOG(BIGIOT_ERROR, "Register %s failed", stEvent.szCbName);
            return 1;
        }
    }

    stEvent.pcIfId = PLUG_GetBigiotTempId();
    stEvent.cb = Bigiot_GenerateTemp;
    strncpy(stEvent.szCbName, "Bigiot_GenerateTemp", BIGIOT_CBNAME_NUM);
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
    stEvent.cb = Bigiot_GenerateVoltage;
    strncpy(stEvent.szCbName, "Bigiot_GenerateVoltage", BIGIOT_CBNAME_NUM);
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
    stEvent.cb = Bigiot_GenerateCurrent;
    strncpy(stEvent.szCbName, "Bigiot_GenerateCurrent", BIGIOT_CBNAME_NUM);
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
    stEvent.cb = Bigiot_GeneratePower;
    strncpy(stEvent.szCbName, "Bigiot_GeneratePower", BIGIOT_CBNAME_NUM);
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
    stEvent.cb = Bigiot_GenerateElectricity;
    strncpy(stEvent.szCbName, "Bigiot_GenerateElectricity", BIGIOT_CBNAME_NUM);
    if ( strlen(stEvent.pcIfId) != 0 )
    {
        iRet = Bigiot_EventRegister( pstCli, &stEvent );
        if ( iRet )
        {
            BIGIOT_LOG(BIGIOT_ERROR, "Register %s failed", stEvent.szCbName);
            return 1;
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
    int iValue = 999999;    //999999表示解析数字出错

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

static int Bigiot_SendIfMsg( BIGIOT_Ctx_S *pstCtx, char* pcIfId, char* pcValue )
{
    char szMsg[100] = { 0 };
    int iLen = 0;
    int iRet = 0;

    iLen = snprintf(szMsg, sizeof(szMsg), "{\"M\":\"update\",\"ID\":\"%s\",\"V\":{\"%s\":\"%s\"}}\n",
                    pstCtx->pcDeviceId, pcIfId, pcValue);

    BIGIOT_LOG(BIGIOT_DEBUG, "msg: %s", szMsg);
    iRet = pstCtx->Write( pstCtx, szMsg, iLen, pstCtx->iTimeOut );
    if ( iRet != iLen )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_SendIfMsg failed, msg:%s", szMsg);
        return 1;
    }

    return 0;
}

static int Bigiot_SendMultipleIfMsg( BIGIOT_Ctx_S *pstCtx, char* pcIf )
{
    char szMsg[300] = { 0 };
    int iLen = 0;
    int iRet = 0;

    iLen = snprintf(szMsg, sizeof(szMsg), "{\"M\":\"update\",\"ID\":\"%s\",\"V\":{%s}}\n",
                    pstCtx->pcDeviceId, pcIf);

    BIGIOT_LOG(BIGIOT_DEBUG, "msg: %s", szMsg);
    iRet = pstCtx->Write( pstCtx, szMsg, iLen, pstCtx->iTimeOut );
    if ( iRet != iLen )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "send multiple interface msg failed, msg:%s", szMsg);
        return 1;
    }

    return 0;
}

static int Bigiot_HeartBeat( void * para )
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
            pstCtx->iAlived = 3;
            iFailedCnt = 0;
            return 2;
        }
    }

    BIGIOT_LOG(BIGIOT_DEBUG, "send heartbeat");
    pstCtx->iAlived = 2;
    iFailedCnt = 0;

    return 0;
}

static int Bigiot_HealthCheck( void * para )
{
    BIGIOT_Ctx_S *pstCtx = para;
    const char* pcMsg = "{\"M\":\"status\"}\n";
    int iRet = 0;
    char szMess[150] = { 0 };
    char szValue[100] = { 0 };
    char* pcStatus = 0;

    if ( pstCtx == NULL )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "pstCtx is NULL");
        return 1;
    }

    WaitReadLock(pstCtx, 0);

    iRet = pstCtx->Write( pstCtx, pcMsg, strlen(pcMsg), pstCtx->iTimeOut );
    if ( iRet != strlen(pcMsg) )
    {
    	ReleaseReadLock(pstCtx);

		BIGIOT_LOG(BIGIOT_ERROR, "send failed");
		return 2;
    }
    BIGIOT_LOG(BIGIOT_DEBUG, "send HealthCheck");

    iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );

    ReleaseReadLock(pstCtx);

    if ( iRet < 0 )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "Read failed");
        return iRet;
    }

    if ( iRet > 0 )
    {
    	//BIGIOT_LOG(BIGIOT_DEBUG, "szMess: %s", szMess);

    	pcStatus = BigiotParseString(szMess, "M", szValue, sizeof(szValue));
        if ( 0 != pcStatus && 0 == strcmp(pcStatus, BIGIOT_CHECKED) )
        {
        	BIGIOT_LOG(BIGIOT_DEBUG, "HealthCheck ok");
        	return 0;
        }
        else
        {
        	BIGIOT_LOG(BIGIOT_ERROR, "HealthCheck failed, szMess: %s", szMess);
        	return 3;
        }
    }

    BIGIOT_LOG(BIGIOT_ERROR, "HealthCheck failed, read timeout");
    return 4;
}

static char* Bigiot_GenerateRelayStatus( void * para )
{
    BIGIOT_Event_S *pstEvn = para;
    static char szBuf[20];

    if ( pstEvn == NULL || pstEvn->pcIfId == NULL )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "pstEvn or pstEvn.pcIfId is NULL");
        return NULL;
    }

    snprintf(szBuf, sizeof(szBuf), "\"%s\":\"%s\"", pstEvn->pcIfId, (PLUG_GetRelayStatus() ? "1" : "0" ));
    return szBuf;
}

static char* Bigiot_GenerateTemp( void * para )
{
    BIGIOT_Event_S *pstEvn = para;
    static char szBuf[20] = {0};

    if ( pstEvn == NULL || pstEvn->pcIfId == NULL )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "pstEvn or pstEvn.pcIfId is NULL");
        return NULL;
    }

    snprintf(szBuf, sizeof(szBuf), "\"%s\":\"%2.1f\"", pstEvn->pcIfId, TEMP_GetTemperature());
    return szBuf;
}

static char* Bigiot_GenerateHumidity( void * para )
{
    BIGIOT_Event_S *pstEvn = para;
    static char szBuf[20] = {0};

    if ( pstEvn == NULL || pstEvn->pcIfId == NULL )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "pstEvn or pstEvn.pcIfId is NULL");
        return NULL;
    }

    snprintf(szBuf, sizeof(szBuf), "\"%s\":\"%2.1f\"", pstEvn->pcIfId, 50.0);
    return szBuf;
}

static char* Bigiot_GenerateVoltage( void * para )
{
    BIGIOT_Event_S *pstEvn = para;
    static char szBuf[20] = {0};

    if ( pstEvn == NULL || pstEvn->pcIfId == NULL )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "pstEvn or pstEvn.pcIfId is NULL");
        return NULL;
    }

    snprintf(szBuf, sizeof(szBuf), "\"%s\":\"%3.1f\"", pstEvn->pcIfId, METER_GetMeterVoltage());
    return szBuf;
}

static char* Bigiot_GenerateCurrent( void * para )
{
    BIGIOT_Event_S *pstEvn = para;
    static char szBuf[20] = {0};

    if ( pstEvn == NULL || pstEvn->pcIfId == NULL )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "pstEvn or pstEvn.pcIfId is NULL");
        return NULL;
    }

    snprintf(szBuf, sizeof(szBuf), "\"%s\":\"%3.1f\"", pstEvn->pcIfId, METER_GetMeterCurrent());
    return szBuf;
}

static char* Bigiot_GeneratePower( void * para )
{
    BIGIOT_Event_S *pstEvn = para;
    static char szBuf[20] = {0};

    if ( pstEvn == NULL || pstEvn->pcIfId == NULL )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "pstEvn or pstEvn.pcIfId is NULL");
        return NULL;
    }

    snprintf(szBuf, sizeof(szBuf), "\"%s\":\"%3.1f\"", pstEvn->pcIfId, METER_GetMeterPower());
    return szBuf;
}

static char* Bigiot_GenerateElectricity( void * para )
{
    BIGIOT_Event_S *pstEvn = para;
    static char szBuf[20] = {0};

    if ( pstEvn == NULL || pstEvn->pcIfId == NULL )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "pstEvn or pstEvn.pcIfId is NULL");
        return NULL;
    }

    snprintf(szBuf, sizeof(szBuf), "\"%s\":\"%3.1f\"", pstEvn->pcIfId, METER_GetMeterElectricity());
    return szBuf;
}

static int Bigiot_UploadAllIfData( BIGIOT_Ctx_S *pstCtx )
{
    int iRet = -1;
    int i = 0;
    char *pcBuf = NULL;
    UINT uiPos = 0;
    char szMsg[200] = {};

    for ( i = 0; i < BIGIOT_EVENT_NUM; i++ )
    {
        if ( pstCtx->astEvent[i].cb != NULL )
        {
            pcBuf = pstCtx->astEvent[i].cb( pstCtx->astEvent[i].cbPara );
            if ( pcBuf != NULL )
            {
                uiPos += snprintf( szMsg+uiPos, sizeof(szMsg)-uiPos, "%s,", pcBuf);
            }
        }
    }

    if ( uiPos != 0 )
    {
        //去掉最后结尾的","
        szMsg[uiPos-1] = '\0';
        iRet = Bigiot_SendMultipleIfMsg( pstCtx, szMsg);
        if ( iRet )
        {
            BIGIOT_LOG(BIGIOT_ERROR, "SendMultipleIfMsg failed, msg: %s", szMsg);
            return 1;
        }
        return 0;
    }

    return 1;
}

static void Init( BIGIOT_Ctx_S *pstCtx, char* pcHostName, int iPort, char* pcDevId, char* pcApiKey )
{
    pstCtx->socket            = -1;
    pstCtx->pcHostName         = pcHostName;
    pstCtx->port               = iPort;
    pstCtx->pcDeviceId        = pcDevId;
    pstCtx->pcApiKey        = pcApiKey;

    pstCtx->xEventHandle     = 0;
    pstCtx->iAlived            = 0;
    pstCtx->ucReadLock       = FALSE;

    pstCtx->iTimeOut         = BIGIOT_TIMEOUT;
    pstCtx->Read             = Read;
    pstCtx->Write             = Write;
    pstCtx->Connect         = Connect;
    pstCtx->Disconnect         = Disconnect;

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



static int WaitReadLock(BIGIOT_Ctx_S* pstCtx, unsigned int timeout_ms)
{
	unsigned int count = 0;

	while( pstCtx->ucReadLock )
	{
		vTaskDelay( 1 / portTICK_RATE_MS );
		count++;

		if ( timeout_ms > 0 && count > timeout_ms )
		{
			return -1;
		}
	}
	pstCtx->ucReadLock = TRUE;

	return 0;
}

static void ReleaseReadLock(BIGIOT_Ctx_S* pstCtx)
{
	pstCtx->ucReadLock = FALSE;
}

