
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

#define MQTT_SENDSIZE 		256
#define MQTT_RECVSIZE 		256


static void Init( BIGIOT_Ctx_S *pstCtx, char* pcHostName, int iPort, char* pcDevId, char* pcApiKey);
static int Connect(BIGIOT_Ctx_S* pstCtx);
static void Disconnect(BIGIOT_Ctx_S* pstCtx);
static int Write( BIGIOT_Ctx_S* pstCtx, const unsigned char* buffer, unsigned int len, unsigned int timeout_s );
static int Read(BIGIOT_Ctx_S* pstCtx, unsigned char* buffer, unsigned int len, unsigned int timeout_s);

static char* BigiotParseString( const char* pcData, const char* pcKey, char*pcValue, unsigned int uiLen );
static int BigiotParseInt( const char* pcData, const char* pcKey );

static void BIGIOT_BigiotTask( void* para )
{
	int iRet = -1;
	BIGIOT_Ctx_S* pstCli = 0;
	char* pcDevId = 0;
	char* pcApiKey = 0;

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

	for (;;)
	{
		iRet = Bigiot_Login( pstCli );
		if ( iRet )
		{
			BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Login failed, iRet:%d", iRet);
			goto exit;
		}
		BIGIOT_LOG(BIGIOT_INFO, "Bigiot_Login success");

		iRet = Bigiot_KeepLive( pstCli );
		if ( iRet )
		{
			BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_KeepLive failed");
			goto exit;
		}
		BIGIOT_LOG(BIGIOT_INFO, "Bigiot_KeepLive success");

		for ( ;; )
		{
			if ( Bigiot_Cycle( pstCli ) )
			{
				break;
			}

			if ( !pstCli->iIsLived )
			{
				break;
			}
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

static void BigiotHeat( void* para )
{
	int iRet = 0;
	const char* pcMess = "{\"M\":\"beat\"}\n";
	BIGIOT_Ctx_S *pstCtx = para;
	int iFailedCnt = 0;

	for ( ;; )
	{
		iRet = pstCtx->Write( pstCtx, pcMess, strlen(pcMess), pstCtx->iTimeOut );
		if ( iRet != strlen(pcMess) )
		{
			iFailedCnt++;
			if ( iFailedCnt >= 2 )
			{
		    	BIGIOT_LOG(BIGIOT_ERROR, "BigiotHeat failed");
		    	pstCtx->iIsLived = 0;
			}
			continue;
		}

		BIGIOT_LOG(BIGIOT_DEBUG, "BigiotHeat success");
		pstCtx->iIsLived = 1;
		iFailedCnt = 0;

		vTaskDelay( pstCtx->iBeatInterval * 1000 / portTICK_RATE_MS );
	}

	return;
}

int Bigiot_KeepLive( BIGIOT_Ctx_S *pstCtx )
{
	if ( pdPASS != xTaskCreate( BigiotHeat,
								"BigiotHeat",
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
	if ( 0 != pcMethod )
	{
		if ( 0 == strcmp(pcMethod, BIGIOT_LOGINT_OK) )
		{
			return 0;
		}
		BIGIOT_LOG(BIGIOT_ERROR, "Bigiot_Login failed, unknown method:%s", pcMethod);
		return 5;
	}

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
		if ( 0 != pcMethod && 0 == strcmp(pcMethod, "say") )
		{
			pcContent = BigiotParseString(szMess, "C", szValue, sizeof(szValue));
			if ( 0 != pcContent && 0 == strcmp(pcContent, BIGIOT_ON) )
			{
				BIGIOT_LOG(BIGIOT_INFO, "Conent: %s", pcContent);
				PLUG_SetRelayByStatus( 1, 1 );
			}
			else if ( 0 != pcContent && 0 == strcmp(pcContent, BIGIOT_OFF) )
			{
				BIGIOT_LOG(BIGIOT_INFO, "Conent: %s", pcContent);
				PLUG_SetRelayByStatus( 0, 1 );
			}
			else if ( 0 != pcContent )
			{
				BIGIOT_LOG(BIGIOT_INFO, "recv content:%s", pcContent);
			}
			else
			{
				BIGIOT_LOG(BIGIOT_ERROR, "BigiotParseString failed, szMess:%s", szMess);
			}
		}
	}

	return 0;
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

static void Init( BIGIOT_Ctx_S *pstCtx, char* pcHostName, int iPort, char* pcDevId, char* pcApiKey)
{
	pstCtx->socket			= -1;
	pstCtx->pcHostName 		= pcHostName;
	pstCtx->port	   		= iPort;
	pstCtx->pcDeviceId		= pcDevId;
	pstCtx->pcApiKey		= pcApiKey;

	pstCtx->xKeepLiveHandle = 0;
	pstCtx->iBeatInterval   = 30;
	pstCtx->iIsLived		= 0;

	pstCtx->iTimeOut 		= BIGIOT_TIMEOUT;
	pstCtx->Read 			= Read;
	pstCtx->Write 			= Write;
	pstCtx->Connect 		= Connect;
	pstCtx->Disconnect 		= Disconnect;
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


