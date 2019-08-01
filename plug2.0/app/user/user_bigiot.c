
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
#define BIGIOT_PORT    		8181
#define BIGIOT_TIMEOUT 		3

#define BIGIOT_DEVICEID 	"12348"
#define BIGIOT_APIKEY 		"100f7fdfc"

#define MQTT_SENDSIZE 	256
#define MQTT_RECVSIZE 	256

#define BIGIOT_NONE       4
#define BIGIOT_ERROR      3
#define BIGIOT_INFO       2
#define BIGIOT_DEBUG      1

#define BIGIOT_LOG(lev, arg...) LOG_OUT(lev, ##arg)

typedef struct tagBigiot
{
    int socket;
    char* pcHostName;
    int port;
    char* pcDeviceId;
    char* pcApiKey;
    int iIsConnected;
    int iTimeOut;

    xTaskHandle xKeepLiveHandle;
    int iIsLived;
    int iBeatInterval;

    int  (*Read)(struct tagBigiot*, unsigned char*, unsigned int, unsigned int);
    int  (*Write)(struct tagBigiot*, const unsigned char*, unsigned int, unsigned int);
    int (*Connect)(struct tagBigiot*);
    void (*Disconnect)(struct tagBigiot*);
    void (*BigiotDestroy)(struct tagBigiot**);

}BIGIOT_Ctx_S;


BIGIOT_Ctx_S* NewBigiot( void );
void DestroyBigiot( BIGIOT_Ctx_S **ppstCtx );
int BigiotLogin( BIGIOT_Ctx_S *pstCtx );
int BigiotLogout( BIGIOT_Ctx_S *pstCtx );
int BigiotCycle( BIGIOT_Ctx_S *pstCtx );
int BigiotKeepLive( BIGIOT_Ctx_S *pstCtx );

static void Init( BIGIOT_Ctx_S *pstCtx );
static int Connect(BIGIOT_Ctx_S* pstCtx);
static void Disconnect(BIGIOT_Ctx_S* pstCtx);
static int Write( BIGIOT_Ctx_S* pstCtx, const unsigned char* buffer, unsigned int len, unsigned int timeout_ms );
static int Read(BIGIOT_Ctx_S* pstCtx, unsigned char* buffer, unsigned int len, unsigned int timeout_ms);



static void BIGIOT_BigiotTask( void* para )
{
	int iRet = -1;
	unsigned int uiHeat = 0;
	BIGIOT_Ctx_S* pstCli = 0;

	pstCli = NewBigiot();
	if ( pstCli == 0 )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "NewBigiot failed");
		goto exit;
	}
	BIGIOT_LOG(BIGIOT_INFO, "Connect success");

reConnect:

	iRet = BigiotLogin( pstCli );
	if ( iRet )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "BigiotLogin failed");
		goto exit;
	}
	BIGIOT_LOG(BIGIOT_INFO, "BigiotLogin success");

	iRet = BigiotKeepLive( pstCli );
	if ( iRet )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "BigiotKeepLive failed");
		goto exit;
	}
	BIGIOT_LOG(BIGIOT_INFO, "BigiotKeepLive success");

	for ( ;; )
	{

		if ( BigiotCycle( pstCli ) )
		{
			goto reConnect;
		}

	}

exit:
	LOG_OUT(LOGOUT_INFO, "BIGIOT_BigiotTask stop");
	pstCli->BigiotDestroy( &pstCli );
	vTaskDelete(NULL);
	return;
}

void BIGIOT_StartBigiotTheard(void)
{
    xTaskCreate( BIGIOT_BigiotTask, "BIGIOT_BigiotTask", 1024, NULL, 5, NULL);
}

BIGIOT_Ctx_S* NewBigiot( void )
{
	int iRet = -1;
	BIGIOT_Ctx_S* pstCtx = NULL;

	pstCtx = malloc( sizeof(BIGIOT_Ctx_S) );
	if ( pstCtx == 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "NewBigiot failed");
    	return 0;
	}

	Init( pstCtx );

    iRet = pstCtx->Connect( pstCtx );
	if ( iRet )
	{
		BIGIOT_LOG(BIGIOT_ERROR, "NewBigiot failed");
		pstCtx->BigiotDestroy( &pstCtx );
	}

	return pstCtx;

exit:
	return 0;
}

void DestroyBigiot( BIGIOT_Ctx_S **ppstCtx )
{
	if ( ppstCtx != 0 && *ppstCtx != 0 )
	{
		(*ppstCtx)->Disconnect( *ppstCtx );
		vTaskDelete( (*ppstCtx)->xKeepLiveHandle );
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

	for ( ;; )
	{
		iRet = pstCtx->Write( pstCtx, pcMess, strlen(pcMess), pstCtx->iTimeOut );
		if ( iRet != strlen(pcMess) )
		{
	    	BIGIOT_LOG(BIGIOT_ERROR, "BigiotHeat failed");
	    	pstCtx->iIsLived = 0;
	    	goto exit;
		}

		BIGIOT_LOG(BIGIOT_DEBUG, "BigiotHeat success");
		pstCtx->iIsLived = 1;

		vTaskDelay( pstCtx->iBeatInterval * 1000 / portTICK_RATE_MS );
	}
exit:
	vTaskDelete(NULL);
	return;
}

int BigiotKeepLive( BIGIOT_Ctx_S *pstCtx )
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

int BigiotLogin( BIGIOT_Ctx_S *pstCtx )
{
	char szMess[100] = { 0 };
	int iLen = 0;
	int iRet = 0;

	iLen = snprintf(szMess, sizeof(szMess), "{\"M\":\"checkin\",\"ID\":\"%s\",\"K\":\"%s\"}\n",
					pstCtx->pcDeviceId, pstCtx->pcApiKey );

	iRet = pstCtx->Write( pstCtx, szMess, iLen, pstCtx->iTimeOut );
	if ( iRet != iLen )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "BigiotLogin failed");
    	return 1;
	}

	iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
	if ( iRet < 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "BigiotLogin failed");
    	return 1;
	}

	BIGIOT_LOG(BIGIOT_INFO, "recv:[%s]", szMess);

	return 0;
}


int BigiotCycle( BIGIOT_Ctx_S *pstCtx )
{
	char szMess[150] = { 0 };
	int iLen = 0;
	int iRet = 0;

	iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
	if ( iRet < 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "BigiotCycle failed");
    	return iRet;
	}

	if ( iRet > 0 )
	{
		BIGIOT_LOG(BIGIOT_INFO, "recv:[%s]", szMess);
	}

	return 0;
}


int BigiotLogout( BIGIOT_Ctx_S *pstCtx )
{
	char szMess[100] = { 0 };
	int iLen = 0;
	int iRet = 0;

	iLen = snprintf(szMess, sizeof(szMess), "{\"M\":\"checkout\",\"ID\":\"%s\",\"K\":\"%s\"}\n",
					pstCtx->pcDeviceId, pstCtx->pcApiKey);

	iRet = pstCtx->Write( pstCtx, szMess, iLen, pstCtx->iTimeOut );
	if ( iRet != iLen )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "BigiotLogout failed");
    	return 1;
	}

	iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
	if ( iRet < 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "BigiotLogout failed");
    	return 1;
	}

	BIGIOT_LOG(BIGIOT_INFO, "recv:[%s]", szMess);

	return 0;
}

static void Init( BIGIOT_Ctx_S *pstCtx )
{
	pstCtx->pcHostName 		= BIGIOT_HOSTNAME;
	pstCtx->port	   		= BIGIOT_PORT;
	pstCtx->pcDeviceId		= BIGIOT_DEVICEID;
	pstCtx->pcApiKey		= BIGIOT_APIKEY;

	pstCtx->xKeepLiveHandle = 0;
	pstCtx->iBeatInterval   = 30;
	pstCtx->iIsLived		= 0;

	pstCtx->iIsConnected 	= 0;
	pstCtx->iTimeOut 		= BIGIOT_TIMEOUT;
	pstCtx->Read 			= Read;
	pstCtx->Write 			= Write;
	pstCtx->Connect 		= Connect;
	pstCtx->Disconnect 		= Disconnect;
	pstCtx->BigiotDestroy 	= DestroyBigiot;
}

static int Connect(BIGIOT_Ctx_S* pstCtx)
{
    struct sockaddr_in sAddr;
    int iRet = -1;
    struct hostent* ipAddress;
	char szMess[100] = { 0 };

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
    	return 1;
    }

    iRet = connect(pstCtx->socket, (struct sockaddr*)&sAddr, sizeof(sAddr));
    if ( iRet < 0)
    {
    	BIGIOT_LOG(BIGIOT_ERROR, "connect failed, socket:%d, ret:%d", pstCtx->socket, iRet);
    	pstCtx->iIsConnected = 0;
    	close( pstCtx->socket );
    	return 1;
    }
    pstCtx->iIsConnected = 1;

	iRet = pstCtx->Read( pstCtx, szMess, sizeof(szMess), pstCtx->iTimeOut );
	if ( iRet < 0 )
	{
    	BIGIOT_LOG(BIGIOT_ERROR, "connect read failed");
    	return 1;
	}

	BIGIOT_LOG(BIGIOT_INFO, "recv:[%s]", szMess);

    return 0;
}

static void Disconnect(BIGIOT_Ctx_S* pstCtx)
{
	if ( pstCtx->iIsConnected )
	{
		close( pstCtx->socket );
		pstCtx->iIsConnected = 0;
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

	readysock = select( pstCtx->socket + 1, NULL, &fdset, NULL, &timeout );
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

    rc = select(pstCtx->socket + 1, &fdset, NULL, NULL, &timeout);
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


