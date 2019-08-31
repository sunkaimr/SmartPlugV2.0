/*
 * user_mcu.c
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */
#include "esp_common.h"
#include "user_common.h"


extern uint8_t  Uart_RecvBuf[];
extern uint32_t Uart_RecvCnt;


UINT MCU_MarshalJsonGetRefresh( CHAR* pcBuf, UINT uiBufLen)
{
	UINT8 uiWaitCount = 0;
	UINT8 uiRetry = 0;
	CHAR* pcPos = NULL;

retry:

	uiWaitCount = 0;
	printf("GetMenu()\r\n");
	memset((char*)Uart_RecvBuf, 0, sizeof((char*)Uart_RecvBuf));
	for ( ;; )
	{
		if( 0 != strstr((char*)Uart_RecvBuf, "GetMenu() "))
		{
			pcPos = strstr((char*)Uart_RecvBuf, "\r\n");
			if ( pcPos != NULL )
			{
				pcPos[0] = 0;
			}

			snprintf(pcBuf, uiBufLen, "%s", Uart_RecvBuf+10);
			LOG_OUT(LOGOUT_INFO, "GetMenu");
			break;
		}
		/* 等待mcu回应后拼装 */
		else if ( uiWaitCount++ > 10 )
		{
			if ( uiRetry++ > 3 )
			{
				return 0;
			}

			goto retry;
		}
		vTaskDelay( 10 / portTICK_RATE_MS );
	}

	return strlen(pcBuf);
}


UINT MCU_GetRelayStatus( VOID )
{
	UINT8 uiWaitCount = 0;
	UINT8 uiRetry = 0;

retry:

	uiWaitCount = 0;
	printf("GetRelayStatus()\r\n");
	memset((char*)Uart_RecvBuf, 0, sizeof((char*)Uart_RecvBuf));
	for ( ;; )
	{
		if( 0 != strstr((char*)Uart_RecvBuf, "GetRelayStatus() "))
		{
			if ( 0 != strstr((char*)Uart_RecvBuf+17, "true" ))
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
			break;
		}
		/* 等待mcu回应后拼装 */
		else if ( uiWaitCount++ > 10 )
		{
			if ( uiRetry++ > 3 )
			{
				return 2;
			}
			goto retry;
		}
		vTaskDelay( 10 / portTICK_RATE_MS );
	}

	return 2;
}

UINT MCU_MarshalJsonRelayStatus( CHAR* pcBuf, UINT uiBufLen)
{
	UINT uiRelayStatus = 0;

	uiRelayStatus = MCU_GetRelayStatus();
	if ( uiRelayStatus == TRUE)
	{
		snprintf( pcBuf, uiBufLen, "{\"status\":\"on\"}");
	}
	else if ( uiRelayStatus == FALSE)
	{
		snprintf( pcBuf, uiBufLen, "{\"status\":\"off\"}");
	}
	else
	{
		return 0;
	}

	return strlen(pcBuf);
}

UINT MCU_ParseRelayStatus( CHAR* pDataStr )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonDate = NULL;
	CHAR *pcTmp = NULL;

	if ( pDataStr == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pDateStr is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pDataStr );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pDataStr);
	    goto error;
	}

	pJsonDate = cJSON_GetObjectItem(pJsonRoot, "status");
	if ( pJsonDate != NULL && pJsonDate->type == cJSON_String )
	{
		pcTmp = pJsonDate->valuestring;

		if ( strcmp(pcTmp, "on") == 0 )
		{
			printf("SetRelayOn()\r\n");
			LOG_OUT(LOGOUT_INFO, "SetRelayOn");
		}
		else if ( strcmp(pcTmp, "off") == 0 )
		{
			printf("SetRelayOff()\r\n");
			LOG_OUT(LOGOUT_INFO, "SetRelayOff");
		}
		else
		{
		    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse unknow status:%s.", pcTmp);
		    goto error;
		}
	}
	else
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse get \"status\" value failed, pDateStr:%s.", pDataStr);
	    goto error;
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}


UINT MCU_MarshalJsonDate( CHAR* pcBuf, UINT uiBufLen)
{
	UINT8 uiWaitCount = 0;
	UINT8 uiRetry = 0;
	CHAR* pcPos = NULL;

retry:

	uiWaitCount = 0;
	printf("GetDate()\r\n");
	memset((char*)Uart_RecvBuf, 0, sizeof((char*)Uart_RecvBuf));
	for ( ;; )
	{
		if( 0 != strstr((char*)Uart_RecvBuf, "GetDate() "))
		{
			pcPos = strstr((char*)Uart_RecvBuf, "\r\n");
			if ( pcPos != NULL )
			{
				pcPos[0] = 0;
			}

			snprintf( pcBuf, uiBufLen, "{\"Date\":\"%s\", \"SyncTime\":%s}",
						Uart_RecvBuf + 10,
						PLUG_GetTimeSyncFlag() == TIME_SYNC_NONE ? "false" : "true");
			break;
		}
		/* 等待mcu回应后拼装 */
		else if ( uiWaitCount++ > 10 )
		{
			if ( uiRetry++ > 3 )
			{
				return 0;
			}
			goto retry;
		}
		vTaskDelay( 10 / portTICK_RATE_MS );
	}

	return strlen(pcBuf);
}

UINT MCU_MarshalJsonTemperature( CHAR* pcBuf, UINT uiBufLen)
{
	UINT8 uiWaitCount = 0;
	UINT8 uiRetry = 0;
	CHAR* pcPos = NULL;

retry:

	uiWaitCount = 0;
	memset((char*)Uart_RecvBuf, 0, sizeof((char*)Uart_RecvBuf));
	printf("GetTemperature()\r\n");
	for ( ;; )
	{
		if( 0 != strstr((char*)Uart_RecvBuf, "GetTemperature() "))
		{
			pcPos = strstr((char*)Uart_RecvBuf, "\r\n");
			if ( pcPos != NULL )
			{
				pcPos[0] = 0;
			}

			snprintf(pcBuf, uiBufLen, "{\"Temperature\": %s}", Uart_RecvBuf+17);
			LOG_OUT(LOGOUT_INFO, "GetTemperature");
			break;
		}
		/* 等待mcu回应后拼装 */
		else if ( uiWaitCount++ > 10 )
		{
			if ( uiRetry++ > 3 )
			{
				return 0;
			}
			goto retry;
		}
		vTaskDelay( 10 / portTICK_RATE_MS );
	}

	return strlen(pcBuf);
}


UINT MCU_ParseKeyValue( CHAR* pDataStr )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonDate = NULL;
	CHAR *pcTmp = NULL;

	if ( pDataStr == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pDateStr is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pDataStr );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pDataStr);
	    goto error;
	}

	pJsonDate = cJSON_GetObjectItem(pJsonRoot, "key");
	if ( pJsonDate != NULL && pJsonDate->type == cJSON_String )
	{
		pcTmp = pJsonDate->valuestring;

		//正确的时间格式如："{"key":"esc"}
		if ( strcmp(pcTmp, "esc") == 0 )
		{
			printf("SetKeyEsc()\r\n");
		}
		else if ( strcmp(pcTmp, "up") == 0 )
		{
			printf("SetKeyUp()\r\n");
		}
		else if ( strcmp(pcTmp, "down") == 0 )
		{
			printf("SetKeyDown()\r\n");
		}
		else if ( strcmp(pcTmp, "enter") == 0 )
		{
			printf("SetKeyEnter()\r\n");
		}
		else
		{
		    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse unknow status:%s.", pcTmp);
		    goto error;
		}
	}
	else
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse get \"status\" value failed, pDateStr:%s.", pDataStr);
	    goto error;
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}


VOID MCU_SetRelayOn( VOID )
{
	printf("SetRelayOn()\r\n");
}

VOID MCU_SetRelayOff( VOID )
{
	printf("SetRelayOff()\r\n");
}






