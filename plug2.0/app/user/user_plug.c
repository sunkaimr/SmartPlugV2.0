/*
 * user_plug.c
 *
 *  Created on: 2018年11月2日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"



static VOID PLUG_JudgeTimer( VOID );
static VOID PLUG_TimerCascade( PLUG_TIMER_S *pstTimer );
static VOID PLUG_JudgeDelay( VOID );
VOID PLUG_GetNextDelayTime( PLUG_DELAY_S *pstDelay );
VOID PLUG_StartDelayTime( PLUG_DELAY_S *pstDelay );

PLUG_TIMER_S 	g_astPLUG_Timer[PLUG_TIMER_MAX];
PLUG_DELAY_S 	g_astPLUG_Delay[PLUG_DELAY_MAX];
PLUG_SYSSET_S 	g_stPLUG_SystemSet;
PLUG_PLATFORM_S g_stPLUG_PlatForm;

/* 保存时间同步状态 0:未同步，1:已网同步络时间 2:已通过手工同步  */
UINT g_PLUG_TimeSyncFlag = TIME_SYNC_NONE;
UINT32 g_uiRunTime = 0;

UINT PLUG_GetRunTime( VOID )
{
	return g_uiRunTime;
}

PLUG_TIMER_S* PLUG_GetTimerData( UINT8 ucNum )
{
	if ( ucNum >= PLUG_TIMER_MAX )
	{
		return &g_astPLUG_Timer[0];
	}
	return &g_astPLUG_Timer[ucNum];
}

UINT PLUG_SetTimerData( PLUG_TIMER_S* pstData )
{
	UINT uiRet = OK;

	if ( NULL == pstData )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_SetTimerData, pstData = 0x%p.", pstData);
		return FAIL;
	}

	uiRet = CONFIG_TimerDataCheck( pstData );
	if ( uiRet != OK )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_SetTimerData, pstData check failed.");
		return FAIL;
	}

	memcpy(PLUG_GetTimerData(pstData->uiNum - 1), pstData, sizeof(PLUG_TIMER_S));
	CONFIG_SaveConfig(PLUG_MOUDLE_TIMER);
	return OK;
}

PLUG_DELAY_S* PLUG_GetDelayData( UINT8 ucNum )
{
	if ( ucNum >= PLUG_TIMER_MAX )
	{
		return &g_astPLUG_Delay[0];
	}
	return &g_astPLUG_Delay[ucNum];
}

UINT PLUG_SetDelayData( PLUG_DELAY_S* pstData )
{
	UINT uiRet = OK;
	PLUG_DELAY_S *pstDelay = NULL;

	if ( NULL == pstData )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_SetDelayData, pstData:0x%p.", pstData);
		return FAIL;
	}

	uiRet = CONFIG_DelayDataCheck( pstData );
	if ( uiRet != OK )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_SetDelayData, pstData check failed.");
		return FAIL;
	}

	pstDelay = PLUG_GetDelayData(pstData->uiNum - 1);
	memcpy(pstDelay, pstData, sizeof(PLUG_DELAY_S));
	CONFIG_SaveConfig(PLUG_MOUDLE_DELAY);

	PLUG_StartDelayTime( pstDelay );

	return OK;
}

PLUG_SYSSET_S* PLUG_GetSystemSetData( VOID )
{
	return &g_stPLUG_SystemSet;
}

UINT PLUG_SetSystemSetData( PLUG_SYSSET_S* pstData )
{
	UINT uiRet = OK;

	if ( NULL == pstData )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_SetDelayData, pstData:0x%p.", pstData);
		return FAIL;
	}

	uiRet = CONFIG_SysSetDataCheck( pstData );
	if ( uiRet != OK )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_SetSystemSetData, pstData check failed.");
		return FAIL;
	}
	memcpy(PLUG_GetSystemSetData(), pstData, sizeof(PLUG_SYSSET_S));
	CONFIG_SaveConfig(PLUG_MOUDLE_SYSSET);

	return OK;
}

PLUG_PLATFORM_S* PLUG_GetPlatFormData( VOID )
{
	return &g_stPLUG_PlatForm;
}

UINT32 PLUG_GetPlatFormDataSize()
{
	return sizeof(g_stPLUG_PlatForm);
}

UINT PLUG_SetPlatFormData( PLUG_PLATFORM_S* pstData )
{
	UINT uiRet = OK;

	if ( NULL == pstData )
	{
	    LOG_OUT(LOGOUT_ERROR, "pstData:0x%p.", pstData);
		return FAIL;
	}

	uiRet = CONFIG_PlatFormDataCheck( pstData );
	if ( uiRet != OK )
	{
	    LOG_OUT(LOGOUT_ERROR, "CONFIG_PlatFormDataCheck check failed.");
		return FAIL;
	}
	memcpy(PLUG_GetPlatFormData(), pstData, sizeof(PLUG_PLATFORM_S));
	CONFIG_SaveConfig(PLUG_MOUDLE_PLATFORM);

	return OK;
}

UINT32 PLUG_GetTimerDataSize()
{
	return sizeof(g_astPLUG_Timer);
}

UINT32 PLUG_GetDelayDataSize()
{
	return sizeof(g_astPLUG_Delay);
}

UINT32 PLUG_GetSystemSetDataSize()
{
	return sizeof(g_stPLUG_SystemSet);
}

VOID PLUG_TimerDataDeInit( VOID )
{
	UINT uiLoopi = 0;
	PLUG_TIMER_S *pstTimer = NULL;
	PLUG_TIME_POINT_S stData;
	CHAR szName[PLUG_NAME_MAX_LEN] = "";

	stData.iHour	= 0;
	stData.iMinute	= 0;

	for ( uiLoopi = 0; uiLoopi < PLUG_TIMER_MAX; uiLoopi++ )
	{
	    pstTimer = &g_astPLUG_Timer[uiLoopi];

		memset(pstTimer, 0, sizeof(PLUG_TIMER_S));

		memcpy(&pstTimer->stOnTime, 	&stData, sizeof(PLUG_TIME_POINT_S));
		memcpy(&pstTimer->stOffTime, 	&stData, sizeof(PLUG_TIME_POINT_S));

		snprintf(szName, sizeof(szName), "timer %d", uiLoopi+1);
		strcpy(pstTimer->szName, szName);

		pstTimer->uiNum 		= uiLoopi + 1;
		pstTimer->bOnEnable 	= TRUE;
		pstTimer->bOffEnable 	= TRUE;
		pstTimer->bEnable		= FALSE;
		pstTimer->eWeek			= REPET_ONCE;
		pstTimer->bCascode		= FALSE;
		pstTimer->uiCascodeNum	= 1;
	}
}

VOID PLUG_DelayDataDeInit( VOID )
{
	UINT uiLoopi = 0;
	PLUG_DELAY_S *pstDelay = NULL;
	CHAR szName[PLUG_NAME_MAX_LEN] = "";

	for ( uiLoopi = 0; uiLoopi < PLUG_DELAY_MAX; uiLoopi++ )
	{
	    pstDelay = &g_astPLUG_Delay[uiLoopi];

	    pstDelay->uiNum					= uiLoopi+1;
		pstDelay->bEnable				= FALSE;
		pstDelay->uiCycleTimes 			= 1;
		pstDelay->uiTmpCycleTimes		= 0;
		pstDelay->bOnEnable				= TRUE;
		pstDelay->stOnInterval.iHour 	= 0;
		pstDelay->stOnInterval.iMinute	= 1;
		pstDelay->bOffEnable			= TRUE;
		pstDelay->stOffInterval.iHour 	= 0;
		pstDelay->stOffInterval.iMinute	= 1;
		pstDelay->bCascode				= FALSE;
		pstDelay->uiCascodeNum			= (uiLoopi + 2) % (PLUG_DELAY_MAX+1);
		pstDelay->ucSwFlag              = 0;

		snprintf(szName, sizeof(szName), "delay %d", uiLoopi+1);
		strcpy(pstDelay->szName, szName);
	}
}

VOID PLUG_SystemSetDataDeInit( VOID )
{
	g_stPLUG_SystemSet.bRelayStatus = 0;
	g_stPLUG_SystemSet.ucWifiMode = WIFI_MODE_STATION;
	g_stPLUG_SystemSet.bSmartConfigFlag = FALSE;

	strncpy(g_stPLUG_SystemSet.szPlugName, PLUG_NAME, PLUG_NAME_MAX_LEN);
	memset(g_stPLUG_SystemSet.szWifiSSID, 0, PLUG_WIFI_SSID_LEN);
	memset(g_stPLUG_SystemSet.szWifiPasswd, 0, PLUG_WIFI_PASSWD_LEN);
}


VOID PLUG_PlatformDeInit( VOID )
{
	g_stPLUG_PlatForm.ucCloudPlatform = PLATFORM_NONE;

	g_stPLUG_PlatForm.eDevType		  = DEVTYPE_OTHER;

	memset(g_stPLUG_PlatForm.szMqttProductKey, 0, PLUG_MQTT_PRODUCTKEY_LEN);
	memset(g_stPLUG_PlatForm.szMqttDevName, 0, PLUG_MQTT_DEVNAME_LEN);
	memset(g_stPLUG_PlatForm.szMqttDevSecret, 0, PLUG_MQTT_DEVSECRET_LEN);

	memset(g_stPLUG_PlatForm.szBigiotDevId, 0, PLUG_BIGIOT_DEVID_LEN);
	memset(g_stPLUG_PlatForm.szBigiotApiKey, 0, PLUG_BIGIOT_APIKEY_LEN);
	memset(g_stPLUG_PlatForm.szSwitchId, 0, PLUG_BIGIOT_IFID_LEN);
	memset(g_stPLUG_PlatForm.szTempId, 0, PLUG_BIGIOT_IFID_LEN);
	memset(g_stPLUG_PlatForm.szHumidityId, 0, PLUG_BIGIOT_IFID_LEN);
}

UINT8 PLUG_GetWifiMode( VOID )
{
	return g_stPLUG_SystemSet.ucWifiMode;
}

VOID PLUG_SetWifiMode( UINT8 ucWifiMode )
{
	if (ucWifiMode != WIFI_MODE_STATION && ucWifiMode != WIFI_MODE_SOFTAP )
	{
	    LOG_OUT(LOGOUT_ERROR, "ucWifiMode error, ucWifiMode:%d", ucWifiMode);
	    return;
	}
	g_stPLUG_SystemSet.ucWifiMode = ucWifiMode;
	CONFIG_SaveConfig(PLUG_MOUDLE_SYSSET);
}

CHAR* PLUG_GetWifiSsid( VOID )
{
	return g_stPLUG_SystemSet.szWifiSSID;
}

UINT PLUG_GetWifiSsidLenth( VOID )
{
	UINT uiLen = strlen(g_stPLUG_SystemSet.szWifiSSID);;
	return  uiLen > PLUG_WIFI_SSID_LEN ? PLUG_WIFI_SSID_LEN : uiLen;
}

VOID PLUG_SetWifiSsid( CHAR* pcWifiSsid )
{
	UINT uiLen = 0;

	if ( pcWifiSsid == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcWifiSsid is NULL.");
	    return;
	}

	uiLen =  strlen(pcWifiSsid);
	uiLen =  uiLen > PLUG_WIFI_SSID_LEN ? PLUG_WIFI_SSID_LEN : uiLen;
	memcpy(g_stPLUG_SystemSet.szWifiSSID, pcWifiSsid, uiLen);
	g_stPLUG_SystemSet.szWifiSSID[uiLen] = 0;
	CONFIG_SaveConfig(PLUG_MOUDLE_SYSSET);
}

CHAR* PLUG_GetWifiPasswd( VOID )
{
	return g_stPLUG_SystemSet.szWifiPasswd;
}

UINT PLUG_GetWifiPasswdLenth( VOID )
{
	UINT uiLen = strlen(g_stPLUG_SystemSet.szWifiPasswd);;
	return  uiLen > PLUG_WIFI_PASSWD_LEN ? PLUG_WIFI_PASSWD_LEN : uiLen;
}

UINT8 PLUG_GetCloudPlatform( VOID )
{
	return g_stPLUG_PlatForm.ucCloudPlatform;
}

VOID PLUG_SetCloudPlatform( UINT8 ucCloudPlatform )
{
	if ( ucCloudPlatform >= PLATFORM_BUFF )
	{
	    LOG_OUT(LOGOUT_ERROR, "ucCloudPlatform error, ucCloudPlatform:%d", ucCloudPlatform);
	    return;
	}
	g_stPLUG_PlatForm.ucCloudPlatform = ucCloudPlatform;
	CONFIG_SaveConfig(PLUG_MOUDLE_PLATFORM);
}

CHAR* PLUG_GetMqttProductKey( VOID )
{
	return g_stPLUG_PlatForm.szMqttProductKey;
}

CHAR* PLUG_GetMqttDevName( VOID )
{
	return g_stPLUG_PlatForm.szMqttDevName;
}

CHAR* PLUG_GetMqttDevSecret( VOID )
{
	return g_stPLUG_PlatForm.szMqttDevSecret;
}

CHAR* PLUG_GetBigiotDevId( VOID )
{
	return g_stPLUG_PlatForm.szBigiotDevId;
}

CHAR* PLUG_GetBigiotApiKey( VOID )
{
	return g_stPLUG_PlatForm.szBigiotApiKey;
}

CHAR* PLUG_GetBigiotSwitchId( VOID )
{
	return g_stPLUG_PlatForm.szSwitchId;
}

CHAR* PLUG_GetBigiotTempId( VOID )
{
	return g_stPLUG_PlatForm.szTempId;
}

CHAR* PLUG_GetBigiotHumidityId( VOID )
{
	return g_stPLUG_PlatForm.szHumidityId;
}

VOID PLUG_SetBigiotDevId( CHAR* pcDevId )
{
	UINT uiLen = 0;

	if ( pcDevId == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcDevId is NULL");
	    return;
	}

	uiLen =  strlen(pcDevId);
	uiLen =  uiLen > PLUG_BIGIOT_DEVID_LEN ? PLUG_BIGIOT_DEVID_LEN : uiLen;
	memcpy( g_stPLUG_PlatForm.szBigiotDevId, pcDevId, uiLen );
	g_stPLUG_PlatForm.szBigiotDevId[uiLen] = 0;
	CONFIG_SaveConfig(PLUG_MOUDLE_PLATFORM);
}

VOID PLUG_SetBigiotApiKey( CHAR* pcKey )
{
	UINT uiLen = 0;

	if ( pcKey == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcKey is NULL");
	    return;
	}

	uiLen =  strlen(pcKey);
	uiLen =  uiLen > PLUG_BIGIOT_APIKEY_LEN ? PLUG_BIGIOT_APIKEY_LEN : uiLen;

	memcpy( g_stPLUG_PlatForm.szBigiotApiKey, pcKey, uiLen );
	g_stPLUG_PlatForm.szBigiotApiKey[uiLen] = 0;

	CONFIG_SaveConfig(PLUG_MOUDLE_PLATFORM);
}

UINT PLUG_GetMqttDevSecretLenth( VOID )
{
	UINT uiLen = strlen( g_stPLUG_PlatForm.szMqttDevSecret );
	return  uiLen > PLUG_WIFI_PASSWD_LEN ? PLUG_WIFI_PASSWD_LEN : uiLen;
}

VOID PLUG_SetMqttProductKey( CHAR* pcProductKey )
{
	UINT uiLen = 0;

	if ( pcProductKey == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcProductKey is NULL");
	    return;
	}

	uiLen =  strlen(pcProductKey);
	uiLen =  uiLen > PLUG_MQTT_PRODUCTKEY_LEN ? PLUG_MQTT_PRODUCTKEY_LEN : uiLen;
	memcpy( g_stPLUG_PlatForm.szMqttProductKey, pcProductKey, uiLen );
	g_stPLUG_PlatForm.szMqttProductKey[uiLen] = 0;
	CONFIG_SaveConfig(PLUG_MOUDLE_PLATFORM);
}

VOID PLUG_SetMqttDevName( CHAR* pcDevName )
{
	UINT uiLen = 0;

	if ( pcDevName == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcDevName is NULL");
	    return;
	}

	uiLen =  strlen(pcDevName);
	uiLen =  uiLen > PLUG_MQTT_DEVNAME_LEN ? PLUG_MQTT_DEVNAME_LEN : uiLen;
	memcpy( g_stPLUG_PlatForm.szMqttDevName, pcDevName, uiLen );
	g_stPLUG_PlatForm.szMqttDevName[uiLen] = 0;
	CONFIG_SaveConfig(PLUG_MOUDLE_PLATFORM);
}

VOID PLUG_SetMqttDevSecret( CHAR* DevSecret )
{
	UINT uiLen = 0;

	if ( DevSecret == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "DevSecret is NULL");
	    return;
	}

	uiLen =  strlen(DevSecret);
	uiLen =  uiLen > PLUG_MQTT_DEVSECRET_LEN ? PLUG_MQTT_DEVSECRET_LEN : uiLen;
	memcpy( g_stPLUG_PlatForm.szMqttDevSecret, DevSecret, uiLen );
	g_stPLUG_PlatForm.szMqttDevSecret[uiLen] = 0;
	CONFIG_SaveConfig(PLUG_MOUDLE_PLATFORM);
}

VOID PLUG_SetWifiPasswd( CHAR* pcWifiPasswd )
{
	UINT uiLen = 0;

	if ( pcWifiPasswd == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcWifiSsid is NULL");
	    return;
	}

	uiLen =  strlen(pcWifiPasswd);
	uiLen =  uiLen > PLUG_WIFI_PASSWD_LEN ? PLUG_WIFI_PASSWD_LEN : uiLen;
	memcpy(g_stPLUG_SystemSet.szWifiPasswd, pcWifiPasswd, uiLen);
	g_stPLUG_SystemSet.szWifiPasswd[uiLen] = 0;
	CONFIG_SaveConfig(PLUG_MOUDLE_SYSSET);
}

CHAR* PLUG_GetPlugName( VOID )
{
	return g_stPLUG_SystemSet.szPlugName;
}

UINT PLUG_GetPlugNameLenth( VOID )
{
	UINT uiLen = strlen(g_stPLUG_SystemSet.szPlugName);;
	return  uiLen > PLUG_NAME_MAX_LEN ? PLUG_NAME_MAX_LEN : uiLen;
}

VOID PLUG_SetPlugName( CHAR* pcPlugName )
{
	UINT uiLen = 0;

	if ( pcPlugName == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pcPlugName is NULL");
	    return;
	}

	uiLen =  strlen(pcPlugName);
	uiLen =  uiLen > PLUG_NAME_MAX_LEN ? PLUG_NAME_MAX_LEN : uiLen;
	memcpy(g_stPLUG_SystemSet.szPlugName, pcPlugName, uiLen);
	g_stPLUG_SystemSet.szPlugName[uiLen] = 0;
	CONFIG_SaveConfig(PLUG_MOUDLE_SYSSET);
}

UINT8 PLUG_GetRelayStatus( VOID )
{
	g_stPLUG_SystemSet.bRelayStatus = MCU_GetRelayStatus();
	return g_stPLUG_SystemSet.bRelayStatus;
}


VOID PLUG_SetRelayOn( UINT uiSaveFlag )
{
	g_stPLUG_SystemSet.bRelayStatus = TRUE;
	LED_RelayOn();
	MCU_SetRelayOn();
	if ( uiSaveFlag )
	{
	    CONFIG_SaveConfig(PLUG_MOUDLE_SYSSET);
		LOG_OUT(LOGOUT_INFO, "RelayOn, saved");
	}
	else
	{
		LOG_OUT(LOGOUT_INFO, "RelayOn");
	}
}

VOID PLUG_SetRelayOff( UINT uiSaveFlag )
{
	g_stPLUG_SystemSet.bRelayStatus = FALSE;
	LED_RelayOff();
	MCU_SetRelayOff();
	if ( uiSaveFlag )
	{
	    CONFIG_SaveConfig(PLUG_MOUDLE_SYSSET);
		LOG_OUT(LOGOUT_INFO, "RelayOff, saved");
	}
	else
	{
		LOG_OUT(LOGOUT_INFO, "RelayOff");
	}
}

VOID PLUG_SetRelayByStatus( UINT8 ucStatus, BOOL bSaveFlag )
{
	if ( ucStatus )
	{
		PLUG_SetRelayOn( bSaveFlag );
	}
	else
	{
		PLUG_SetRelayOff( bSaveFlag );
	}
}

VOID PLUG_SetRelayReversal( UINT uiSaveFlag )
{
	if ( g_stPLUG_SystemSet.bRelayStatus == TRUE )
	{
		LED_RelayOff();
		MCU_SetRelayOff();
		g_stPLUG_SystemSet.bRelayStatus = FALSE;
	}
	else
	{
		LED_RelayOn();
		MCU_SetRelayOn();
		g_stPLUG_SystemSet.bRelayStatus = TRUE;
	}

	if ( uiSaveFlag )
	{
		CONFIG_SaveConfig(PLUG_MOUDLE_SYSSET);
		LOG_OUT(LOGOUT_INFO, "%s, saved", g_stPLUG_SystemSet.bRelayStatus ? "RelayOn":"RelayOff");
	}
	else
	{
		LOG_OUT(LOGOUT_INFO, "%s", g_stPLUG_SystemSet.bRelayStatus ? "RelayOn":"RelayOff");
	}
}

UINT8 PLUG_GetSmartConfig( VOID )
{
	return (UINT8)g_stPLUG_SystemSet.bSmartConfigFlag;
}

VOID PLUG_SetSmartConfig( BOOL bStatus )
{
	g_stPLUG_SystemSet.bSmartConfigFlag = bStatus ? TRUE : FALSE;
	CONFIG_SaveConfig(PLUG_MOUDLE_SYSSET);
}

VOID PLUG_SetDelayTurnOff( UINT8 ucNum )
{
	UINT uiLoopi = 0;
	PLUG_DELAY_S *pstDelay = NULL;

	if ( ucNum >= PLUG_DELAY_MAX )
	{
		pstDelay = g_astPLUG_Delay;
		for ( uiLoopi = 0; uiLoopi < PLUG_DELAY_MAX; uiLoopi++, pstDelay++ )
		{
			pstDelay->bEnable = FALSE;
		}
	}
	else
	{
		 g_astPLUG_Delay[ucNum].bEnable = FALSE;
	}
}


UINT PLUG_MarshalJsonTimer( CHAR* pcBuf, UINT uiBufLen, UINT uiTimerNum )
{
	PLUG_TIMER_S *pstData = NULL;
	UINT uiLoopi = 0;
	CHAR *pJsonStr = NULL;
	cJSON  *pJsonArry, *pJsonsub, *pJsonInt;
	CHAR szTimerPoint[10];

	pstData = g_astPLUG_Timer;
	pJsonArry = cJSON_CreateArray();
	for ( uiLoopi = 0 ; uiLoopi < PLUG_TIMER_MAX; uiLoopi++, pstData++ )
	{
		if (uiTimerNum != pstData->uiNum && uiTimerNum != PLUG_TIMER_ALL)
		{
			continue;
		}

		pJsonsub=cJSON_CreateObject();
		cJSON_AddNumberToObject( pJsonsub, 	"Num", 				pstData->uiNum);
		cJSON_AddStringToObject( pJsonsub,	"Name", 			pstData->szName);
		cJSON_AddBoolToObject( pJsonsub, 	"Enable", 			pstData->bEnable);
		cJSON_AddBoolToObject( pJsonsub, 	"OnEnable", 		pstData->bOnEnable);
		cJSON_AddBoolToObject( pJsonsub, 	"OffEnable", 		pstData->bOffEnable);
		cJSON_AddBoolToObject( pJsonsub, 	"Cascode", 			pstData->bCascode);
		cJSON_AddNumberToObject( pJsonsub, 	"Week", 			pstData->eWeek);
		cJSON_AddNumberToObject( pJsonsub, 	"CascodeNum", 		pstData->uiCascodeNum);

		snprintf(szTimerPoint, sizeof(szTimerPoint), "%02d:%02d", pstData->stOnTime.iHour, pstData->stOnTime.iMinute );
		cJSON_AddStringToObject( pJsonsub,	"OnTime", 			szTimerPoint);

		snprintf(szTimerPoint, sizeof(szTimerPoint), "%02d:%02d", pstData->stOffTime.iHour, pstData->stOffTime.iMinute );
		cJSON_AddStringToObject( pJsonsub,	"OffTime", 			szTimerPoint);

		cJSON_AddItemToArray(pJsonArry, pJsonsub);

	}

    pJsonStr = cJSON_PrintUnformatted(pJsonArry);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJsonArry);
    FREE_MEM(pJsonStr);

	return strlen(pcBuf);
}

UINT PLUG_MarshalJsonDelay( CHAR* pcBuf, UINT uiBufLen, UINT uiTimerNum)
{
	PLUG_DELAY_S *pstData = NULL;
	UINT uiLoopi = 0;
	CHAR *pJsonStr = NULL;
	cJSON  *pJsonArry, *pJsonsub, *pJsonInt;
	CHAR szTimerPoint[10];

	pstData = g_astPLUG_Delay;
	pJsonArry = cJSON_CreateArray();
	for ( uiLoopi = 0 ; uiLoopi < PLUG_DELAY_MAX; uiLoopi++, pstData++ )
	{
		if (uiTimerNum != pstData->uiNum && uiTimerNum != PLUG_DELAY_ALL)
		{
			continue;
		}

		pJsonsub=cJSON_CreateObject();

		cJSON_AddNumberToObject( pJsonsub, 	"Num", 				pstData->uiNum);
		cJSON_AddStringToObject( pJsonsub,	"Name", 			pstData->szName);
		cJSON_AddBoolToObject( pJsonsub, 	"Enable", 			pstData->bEnable);
		cJSON_AddBoolToObject( pJsonsub, 	"OnEnable", 		pstData->bOnEnable);
		cJSON_AddBoolToObject( pJsonsub, 	"OffEnable", 		pstData->bOffEnable);
		cJSON_AddNumberToObject( pJsonsub, 	"CycleTimes", 		pstData->uiCycleTimes);
		cJSON_AddNumberToObject( pJsonsub, 	"TmpCycleTimes", 	pstData->uiTmpCycleTimes);
		cJSON_AddNumberToObject( pJsonsub, 	"SwFlag", 			pstData->ucSwFlag);
		cJSON_AddBoolToObject( pJsonsub, 	"Cascode", 			pstData->bCascode);
		cJSON_AddNumberToObject( pJsonsub, 	"CascodeNum", 		pstData->uiCascodeNum);

		snprintf(szTimerPoint, sizeof(szTimerPoint), "%02d:%02d", pstData->stOnInterval.iHour, pstData->stOnInterval.iMinute );
		cJSON_AddStringToObject( pJsonsub,	"OnInterval", 		szTimerPoint);

		snprintf(szTimerPoint, sizeof(szTimerPoint), "%02d:%02d", pstData->stOffInterval.iHour, pstData->stOffInterval.iMinute );
		cJSON_AddStringToObject( pJsonsub,	"OffInterval", 		szTimerPoint);

		snprintf(szTimerPoint, sizeof(szTimerPoint), "%02d:%02d", pstData->stTimePoint.iHour, pstData->stTimePoint.iMinute );
		cJSON_AddStringToObject( pJsonsub,	"TimePoint", 		szTimerPoint);

		cJSON_AddItemToArray(pJsonArry, pJsonsub);
	}

    pJsonStr = cJSON_PrintUnformatted(pJsonArry);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJsonArry);
    FREE_MEM(pJsonStr);

	return strlen(pcBuf);
}


UINT PLUG_MarshalJsonInfrared( CHAR* pcBuf, UINT uiBufLen, UINT uiNum )
{
	INFRARED_VALUE_S *pstData = NULL;
	UINT uiLoopi = 0;
	CHAR *pJsonStr = NULL;
	cJSON  *pJsonArry, *pJsonsub, *pJsonInt;
	CHAR szBuf[10];

	pstData = INFRARED_GetInfraredData(0);
	pJsonArry = cJSON_CreateArray();
	for ( uiLoopi = 0 ; uiLoopi < INFRARED_MAX; uiLoopi++, pstData++ )
	{
		if (uiNum != pstData->uiNum && uiNum != INFRARED_ALL)
		{
			continue;
		}

		pJsonsub=cJSON_CreateObject();

		cJSON_AddNumberToObject( pJsonsub, 	"Num", 				pstData->uiNum);
		cJSON_AddStringToObject( pJsonsub,	"Name", 			pstData->szName);
		cJSON_AddBoolToObject( pJsonsub, 	"Enable", 			pstData->bEnable);

		snprintf(szBuf, sizeof(szBuf), "%X", pstData->uiOnValue);
		cJSON_AddStringToObject( pJsonsub, 	"OnValue", 			szBuf);

		snprintf(szBuf, sizeof(szBuf), "%X", pstData->uiOffValue);
		cJSON_AddStringToObject( pJsonsub, 	"OffValue", 		szBuf);

		cJSON_AddItemToArray(pJsonArry, pJsonsub);
	}

    pJsonStr = cJSON_PrintUnformatted(pJsonArry);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJsonArry);
    FREE_MEM(pJsonStr);

	return strlen(pcBuf);
}


UINT PLUG_MarshalJsonSystemSet( CHAR* pcBuf, UINT uiBufLen )
{
	cJSON  *pJson = NULL;
	CHAR *pJsonStr = NULL;
	WIFI_INFO_S stWifiInfo = {0, 0, 0};
	CHAR szBuf[20];

	pJson = cJSON_CreateObject();

	cJSON_AddBoolToObject( pJson, 	"RelayStatus", 		g_stPLUG_SystemSet.bRelayStatus);
	cJSON_AddBoolToObject( pJson, 	"SmartConfigFlag", 	g_stPLUG_SystemSet.bSmartConfigFlag);
	cJSON_AddNumberToObject( pJson, "WifiMode", 		g_stPLUG_SystemSet.ucWifiMode);
	cJSON_AddStringToObject( pJson, "PlugName", 		g_stPLUG_SystemSet.szPlugName);
	cJSON_AddStringToObject( pJson, "WifiSSID", 		g_stPLUG_SystemSet.szWifiSSID);
	cJSON_AddStringToObject( pJson, "WifiPasswd", 		g_stPLUG_SystemSet.szWifiPasswd);

	stWifiInfo = WIFI_GetIpInfo();
	snprintf(szBuf, sizeof(szBuf), "%d.%d.%d.%d", stWifiInfo.uiIp&0xFF, (stWifiInfo.uiIp>>8)&0xFF,
			(stWifiInfo.uiIp>>16)&0xFF,(stWifiInfo.uiIp>>24)&0xFF);
	cJSON_AddStringToObject( pJson, "IP", szBuf);

	snprintf(szBuf, sizeof(szBuf), "%d.%d.%d.%d", stWifiInfo.uiGetWay&0xFF, (stWifiInfo.uiGetWay>>8)&0xFF,
			(stWifiInfo.uiGetWay>>16)&0xFF,(stWifiInfo.uiGetWay>>24)&0xFF);
	cJSON_AddStringToObject( pJson, "GetWay", szBuf);

	snprintf(szBuf, sizeof(szBuf), "%d.%d.%d.%d", stWifiInfo.uiNetMask&0xFF, (stWifiInfo.uiNetMask>>8)&0xFF,
			(stWifiInfo.uiNetMask>>16)&0xFF,(stWifiInfo.uiNetMask>>24)&0xFF);
	cJSON_AddStringToObject( pJson, "NetMask", szBuf);

	WIFI_GetMacAddr(szBuf, sizeof(szBuf));
	cJSON_AddStringToObject( pJson, "Mac", szBuf);

    pJsonStr = cJSON_PrintUnformatted(pJson);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJson);
    FREE_MEM(pJsonStr);

	return strlen(pcBuf);
}


UINT PLUG_MarshalJsonCloudPlatformSet( CHAR* pcBuf, UINT uiBufLen )
{
	cJSON  *pJson = NULL;
	CHAR *pJsonStr = NULL;
	WIFI_INFO_S stWifiInfo = {0, 0, 0};
	CHAR szBuf[20];

	pJson = cJSON_CreateObject();

	cJSON_AddNumberToObject( pJson, "CloudPlatform", 	g_stPLUG_PlatForm.ucCloudPlatform);

	cJSON_AddStringToObject( pJson, "MqttProductKey", 	g_stPLUG_PlatForm.szMqttProductKey);
	cJSON_AddStringToObject( pJson, "MqttDevName", 		g_stPLUG_PlatForm.szMqttDevName);
	cJSON_AddStringToObject( pJson, "MqttDevSecret", 	g_stPLUG_PlatForm.szMqttDevSecret);

	cJSON_AddNumberToObject( pJson, "DevType", 			g_stPLUG_PlatForm.eDevType);
	cJSON_AddStringToObject( pJson, "BigiotDevId", 		g_stPLUG_PlatForm.szBigiotDevId);
	cJSON_AddStringToObject( pJson, "BigiotApiKey", 	g_stPLUG_PlatForm.szBigiotApiKey);

	cJSON_AddStringToObject( pJson, "SwitchId", 		g_stPLUG_PlatForm.szSwitchId);
	cJSON_AddStringToObject( pJson, "TempId",	 		g_stPLUG_PlatForm.szTempId);
	cJSON_AddStringToObject( pJson, "HumidityId", 		g_stPLUG_PlatForm.szHumidityId);

	if ( Bigiot_GetBigioDeviceName() != NULL )
	{
		cJSON_AddStringToObject( pJson, "BigiotDevName", 	Bigiot_GetBigioDeviceName());
	}
	else
	{
		cJSON_AddStringToObject( pJson, "BigiotDevName", 	"");
	}

	if ( g_stPLUG_PlatForm.ucCloudPlatform == PLATFORM_BIGIOT )
	{
		if ( Bigiot_GetBigioStatus() )
		{
			snprintf(szBuf, sizeof(szBuf), "connected");
		}
		else
		{
			snprintf(szBuf, sizeof(szBuf), "disconnect");
		}

	}
	else if ( g_stPLUG_PlatForm.ucCloudPlatform == PLATFORM_ALIYUN )
	{
		if ( MQTT_GetConnectStatus() )
		{
			snprintf(szBuf, sizeof(szBuf), "connected");
		}
		else
		{
			snprintf(szBuf, sizeof(szBuf), "disconnect");
		}
	}
	else
	{
		snprintf(szBuf, sizeof(szBuf), "unknown");
	}

	cJSON_AddStringToObject( pJson,   "ConnectSta", szBuf);

    pJsonStr = cJSON_PrintUnformatted(pJson);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJson);
    FREE_MEM(pJsonStr);

	return strlen(pcBuf);
}


UINT PLUG_MarshalJsonHtmlData( CHAR* pcBuf, UINT uiBufLen )
{
	UINT uiLoopi = 0;
	HTTP_FILE_LIST_S* pstHtmlData = NULL;
	cJSON  *pJsonArry, *pJsonsub;

	pJsonArry = cJSON_CreateArray();
	pstHtmlData = HTTP_GetFileList(NULL);
	for ( uiLoopi = 0 ; uiLoopi < HTTP_FILE_NUM_MAX; uiLoopi++, pstHtmlData++ )
	{
		if ( pstHtmlData->szName[0] == 0 )
		{
			continue;
		}
		pJsonsub=cJSON_CreateObject();

		cJSON_AddStringToObject( pJsonsub,	"Name", 	pstHtmlData->szName);
		cJSON_AddBoolToObject( pJsonsub,	"IsUpload", pstHtmlData->bIsUpload);
		cJSON_AddNumberToObject( pJsonsub, 	"Addr", 	pstHtmlData->uiAddr);
		cJSON_AddNumberToObject( pJsonsub, 	"Length", 	pstHtmlData->uiLength);
		cJSON_AddStringToObject( pJsonsub, 	"Type", 	szHttpContentTypeStr[pstHtmlData->eType]);

		cJSON_AddItemToArray(pJsonArry, pJsonsub);
	}

    strncpy(pcBuf, cJSON_PrintUnformatted(pJsonArry), uiBufLen);
    cJSON_Delete(pJsonArry);

	return strlen(pcBuf);
}


UINT PLUG_ParseHtmlData( CHAR* pData )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonArr = NULL;
	cJSON *pJsonIteam = NULL;
	INT iCount = 0;
	UINT uiLoop = 0;
	HTTP_FILE_LIST_S *pstHtml, *pstHtmlPrev;
	CHAR *pcTmp = NULL;
	UINT uiRet = 0;
	UINT uiIndex = 0;
	const CHAR aSuffix[HTTP_CONTENT_TYPE_Buff][10] = {".html", ".js", ".css", ".json", ".ico", ".png", ".gif",};

	if ( pData == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_ParseHtmlData, pData is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pData );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pData);
	    goto error;
	}

	iCount = cJSON_GetArraySize( pJsonRoot );
	if ( iCount <= 0)
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_GetArraySize failed, iCount:%d.", iCount);
	    goto error;
	}

	if ( iCount > HTTP_FILE_NUM_MAX )
	{
		LOG_OUT(LOGOUT_ERROR, "PLUG_ParseHtmlData ArraySize:%d big than %d.", iCount, HTTP_FILE_NUM_MAX);
		goto error;
	}

	HTTP_FileListInit();
	pstHtml = HTTP_GetFileList(NULL);
	for ( uiLoop = 0; uiLoop < iCount; uiLoop++ )
	{
		pJsonArr = cJSON_GetArrayItem(pJsonRoot, uiLoop);

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Name");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			strncpy(pstHtml->szName, pJsonIteam->valuestring, HTTP_FILE_NAME_MAX_LEN);
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Length");
		if (pJsonIteam && pJsonIteam->type == cJSON_Number)
		{
			pstHtml->uiLength = pJsonIteam->valueint;
		}

		for (uiIndex = 0; uiIndex < HTTP_CONTENT_TYPE_Buff-1; uiIndex++ )
		{
			if ( strstr(pstHtml->szName, aSuffix[uiIndex]) != NULL )
			{
				pstHtml->eType = uiIndex;
				break;
			}
		}
		if (uiIndex == HTTP_CONTENT_TYPE_Buff-1 )
		{
			pstHtml->eType = HTTP_CONTENT_TYPE_Stream;
		}

		if ( uiLoop == 0 )
		{
			pstHtml->uiAddr = FLASH_USER_ADDR;
		}
		else
		{
			pstHtml->uiAddr = pstHtmlPrev->uiAddr + pstHtmlPrev->uiLength;
		}

		pstHtml->bIsUpload = FALSE;

		pstHtmlPrev = pstHtml;
		pstHtml++;
	}

	uiRet = HTTP_SaveFileListToFlash();
	if ( uiRet != OK )
	{
		LOG_OUT(LOGOUT_ERROR, "PLUG_ParseTimerData, HTTP_SaveFileListToFlash failed.");
		goto error;
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}



UINT8 PLUG_GetTimeSyncFlag( VOID )
{
	return g_PLUG_TimeSyncFlag;
}

VOID PLUG_SetTimeSyncFlag( PLUG_TIME_SYNC_E ucFlag )
{
	if ( ucFlag >= TIME_SYNC_BUFF )
	{
		LOG_OUT(LOGOUT_ERROR, "SetTimeSyncFlag failed. ucFlag:%d", ucFlag);
		return;
	}

	g_PLUG_TimeSyncFlag = ucFlag;
}

static VOID SyncTimeTask( VOID* para )
{
	INT iRetry = 0;
	UINT uiRet = 0;

	LOG_OUT(LOGOUT_INFO, "SyncTimeTask start");

	sntp_stop();
	sntp_setservername(0, "ntp1.aliyun.com");
	sntp_setservername(1, "0.cn.pool.ntp.org");
	sntp_setservername(2, "time.windows.com");
	sntp_init();

	for( iRetry = 1; iRetry < 100; iRetry++ )
	{
		uiRet = sntp_get_current_timestamp();
		if ( uiRet )
		{
			PLUG_SetTimeSyncFlag(TIME_SYNC_NET);
			LOG_OUT(LOGOUT_INFO, "Get time from internet successed.");
			break;
		}
		LOG_OUT(LOGOUT_DEBUG, "Get time from retry %d times", iRetry);
		vTaskDelay( 3000/portTICK_RATE_MS );
	}

	LOG_OUT(LOGOUT_INFO, "SyncTimeTask stop");

	sntp_stop();
	vTaskDelete(NULL);
}

INT32 PLUG_GetTimeFromInternet()
{
	if ( pdPASS != xTaskCreate( SyncTimeTask,
								"SyncTimeTask",
								configMINIMAL_STACK_SIZE * 2,
								0,
								uxTaskPriorityGet(NULL),
								0))
	{
		return FAIL;
	}

	return OK;
}



VOID PLUG_GetDate(PLUG_DATE_S * pstDate )
{
	UINT32 time = 0;
	sntp_tm stSntpDate;
	sntp_time_t tim_p;

	time = sntp_get_current_timestamp();
	tim_p = time;
	sntp_localtime_r(&tim_p, &stSntpDate);

	pstDate->iSecond	= stSntpDate.tm_sec;
	pstDate->iMinute 	= stSntpDate.tm_min;
	pstDate->iHour 		= stSntpDate.tm_hour;
	pstDate->iWeek		= stSntpDate.tm_wday;
	pstDate->iDay 		= stSntpDate.tm_mday;
	pstDate->iMonth 	= 1 + stSntpDate.tm_mon;
	pstDate->iYear 		= 1900+stSntpDate.tm_year;
	if ( pstDate->iWeek == 0 )
	{
	    pstDate->iWeek = 7;
	}
	//LOG_OUT(LOGOUT_DEBUG, "%d-%d-%d  %d:%d:%d", pstDate->iYear, pstDate->iMonth, pstDate->iDay,
	//		pstDate->iHour, pstDate->iMinute, pstDate->iSecond);
}

VOID PLUG_SetDate(PLUG_DATE_S *pstDate )
{
	sntp_time_t LocalTime = 0;
	UINT32 DayCount = 0;
	INT iCurYear = 1970;
	INT iCurMonth = 1;

	//                             1   2   3   4   5   6   7   8   9   10  11  12 月
	static UINT8 ucDayTab[] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if ( pstDate->iYear < 1970 )
	{
		pstDate->iYear = 2017;
		pstDate->iMonth = 1;
		pstDate->iDay = 1;
		pstDate->iHour = 0;
		pstDate->iMinute = 0;
		pstDate->iSecond = 0;
	}

	while( iCurYear < pstDate->iYear )
	{
		if( (iCurYear % 4 == 0 && iCurYear % 100 != 0 ) || (iCurYear % 400 == 0 ))
		{
			DayCount += 366;
		}
		else
		{
			DayCount += 365;
		}

		iCurYear ++;
	}

	if( (iCurYear % 4 == 0 && iCurYear % 100 !=0 ) || (iCurYear % 400 == 0 ))
	{
		ucDayTab[2] = 29;
	}
	else
	{
		ucDayTab[2] = 28;
	}

	while( iCurMonth < pstDate->iMonth )
	{
		DayCount += ucDayTab[iCurMonth];
		iCurMonth++;
	}

	DayCount += pstDate->iDay - 1;

	LocalTime = DayCount * 24 * 60 * 60;

	LocalTime += pstDate->iHour * 3600 + pstDate->iMinute * 60 + pstDate->iSecond;

	LocalTime -= 8 * 3600;

	sntp_set_system_time( LocalTime );
}

UINT PLUG_ParseRelayStatus( CHAR* pDataStr )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonDate = NULL;
	CHAR *pcTmp = NULL;

	if ( pDataStr == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_ParseDate, pDateStr is NULL.");
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

		//正确的时间格式如："{"status":"on"}
		if ( strcmp(pcTmp, "on") == 0 )
		{
			printf("SetRelayOn()\r\n");
			LOG_OUT(LOGOUT_INFO, "HTTP_PostRelayStatus, RelayStatus:on");
		}
		else if ( strcmp(pcTmp, "off") == 0 )
		{
			printf("SetRelayOff()\r\n");
			LOG_OUT(LOGOUT_INFO, "HTTP_PostRelayStatus, RelayStatus:off");
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


UINT PLUG_ParseDate( CHAR* pDateStr)
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonDate = NULL;
	CHAR *pcTmp = NULL;
	PLUG_DATE_S stDate;

	if ( pDateStr == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pDateStr is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pDateStr );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pDateStr);
	    goto error;
	}

	pJsonDate = cJSON_GetObjectItem(pJsonRoot, "Date");
	if ( pJsonDate != NULL && pJsonDate->type == cJSON_String )
	{
		pcTmp = pJsonDate->valuestring;

		//正确的时间格式如："2018-12-01 00:00:00"
		if ( strlen(pcTmp) != 19 )
		{
			LOG_OUT(LOGOUT_ERROR, "PLUG_ParseDate Invalid time format, date:%s.", pcTmp);
			return FAIL;
		}

		stDate.iYear = (pcTmp[0]-'0')*1000 + (pcTmp[1]-'0')*100 + (pcTmp[2]-'0')*10 + (pcTmp[3]-'0');
		pcTmp += 5;

		stDate.iMonth = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
		pcTmp += 3;

		stDate.iDay = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
		pcTmp += 3;

		stDate.iHour = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
		pcTmp += 3;

		stDate.iMinute = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
		pcTmp += 3;

		stDate.iSecond = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');

		printf("SetDate() %d-%02d-%02d %02d:%02d:%02d",
				stDate.iYear, stDate.iMonth, stDate.iDay,
				stDate.iHour, stDate.iMinute, stDate.iSecond);
		PLUG_SetTimeSyncFlag(TIME_SYNC_MAN);
		LED_SetWifiStatus(LED_WIFI_STATUS_ON);

		LOG_OUT( LOGOUT_INFO, "PLUG_ParseDate:%d-%02d-%02d %02d:%02d:%02d",
							stDate.iYear, stDate.iMonth, stDate.iDay,
							stDate.iHour, stDate.iMinute, stDate.iSecond);
	}
	else
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse get \"date\" value failed, pDateStr:%s.", pDateStr);
	    goto error;
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}

UINT PLUG_ParseTimerData( CHAR* pData )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonArr = NULL;
	cJSON *pJsonIteam = NULL;
	INT iCount = 0;
	UINT uiLoop = 0;
	PLUG_TIMER_S stTimer;
	CHAR *pcTmp = NULL;
	UINT uiRet = 0;

	if ( pData == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_ParseTimerData, pData is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pData );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pData);
	    goto error;
	}

	iCount = cJSON_GetArraySize( pJsonRoot );
	if ( iCount <= 0)
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_GetArraySize failed, iCount:%d.", iCount);
	    goto error;
	}

	for ( uiLoop = 0; uiLoop < iCount; uiLoop++ )
	{
		pJsonArr = cJSON_GetArrayItem(pJsonRoot, uiLoop);

		stTimer.uiNum = PLUG_TIMER_ALL;
		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Num");
		if (pJsonIteam && pJsonIteam->type == cJSON_Number)
		{
			stTimer.uiNum = pJsonIteam->valueint;
			memcpy(&stTimer, PLUG_GetTimerData(stTimer.uiNum-1), sizeof(stTimer));
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Name");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			strncpy(stTimer.szName, pJsonIteam->valuestring, PLUG_NAME_MAX_LEN);
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Enable");
		if (pJsonIteam && pJsonIteam->type == cJSON_True)
		{
			stTimer.bEnable = 1;
		}
		else if (pJsonIteam && pJsonIteam->type == cJSON_False)
		{
			stTimer.bEnable = 0;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OnEnable");
		if (pJsonIteam && pJsonIteam->type == cJSON_True)
		{
			stTimer.bOnEnable = 1;
		}
		else if (pJsonIteam && pJsonIteam->type == cJSON_False)
		{
			stTimer.bOnEnable = 0;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OffEnable");
		if (pJsonIteam && pJsonIteam->type == cJSON_True)
		{
			stTimer.bOffEnable = 1;
		}
		else if (pJsonIteam && pJsonIteam->type == cJSON_False)
		{
			stTimer.bOffEnable = 0;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Cascode");
		if (pJsonIteam && pJsonIteam->type == cJSON_True)
		{
			stTimer.bCascode = 1;
		}
		else if (pJsonIteam && pJsonIteam->type == cJSON_False)
		{
			stTimer.bCascode = 0;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Week");
		if (pJsonIteam && pJsonIteam->type == cJSON_Number)
		{
			stTimer.eWeek = pJsonIteam->valueint;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "CascodeNum");
		if (pJsonIteam && pJsonIteam->type == cJSON_Number)
		{
			stTimer.uiCascodeNum = pJsonIteam->valueint;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OnTime");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			pcTmp = pJsonIteam->valuestring;
			//正确的格式如："00:00"
			if ( strlen(pcTmp) != 5 )
			{
				LOG_OUT(LOGOUT_ERROR, "PLUG_ParseTimerData, Invalid OnTime format, %s.", pcTmp);
				return FAIL;
			}
			stTimer.stOnTime.iHour = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
			pcTmp += 3;
			stTimer.stOnTime.iMinute = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OffTime");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			pcTmp = pJsonIteam->valuestring;
			//正确的格式如："00:00"
			if ( strlen(pcTmp) != 5 )
			{
				LOG_OUT(LOGOUT_ERROR, "PLUG_ParseTimerData, Invalid OffTime format, %s.", pcTmp);
				return FAIL;
			}
			stTimer.stOffTime.iHour = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
			pcTmp += 3;
			stTimer.stOffTime.iMinute = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
		}

		uiRet = PLUG_SetTimerData( &stTimer);
		if ( uiRet != OK )
		{
			LOG_OUT(LOGOUT_ERROR, "PLUG_ParseTimerData, PLUG_SetTimerData failed.");
			goto error;
		}
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}



UINT PLUG_ParseDelayData( CHAR* pData )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonArr = NULL;
	cJSON *pJsonIteam = NULL;
	INT iCount = 0;
	UINT uiLoop = 0;
	PLUG_DELAY_S stDelay;
	CHAR *pcTmp = NULL;
	UINT uiRet = 0;

	if ( pData == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_ParseDelayData, pData is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pData );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pData);
	    goto error;
	}

	iCount = cJSON_GetArraySize( pJsonRoot );
	if ( iCount <= 0)
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_GetArraySize failed, iCount:%d.", iCount);
	    goto error;
	}

	for ( uiLoop = 0; uiLoop < iCount; uiLoop++ )
	{
		pJsonArr = cJSON_GetArrayItem(pJsonRoot, uiLoop);

		stDelay.uiNum = PLUG_DELAY_ALL;
		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Num");
		if (pJsonIteam && pJsonIteam->type == cJSON_Number)
		{
			stDelay.uiNum = pJsonIteam->valueint;
			memcpy(&stDelay, PLUG_GetDelayData(stDelay.uiNum-1), sizeof(stDelay));
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Name");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			strncpy(stDelay.szName, pJsonIteam->valuestring, PLUG_NAME_MAX_LEN);
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Enable");
		if (pJsonIteam && pJsonIteam->type == cJSON_True)
		{
			stDelay.bEnable = 1;
		}
		else if (pJsonIteam && pJsonIteam->type == cJSON_False)
		{
			stDelay.bEnable = 0;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OnEnable");
		if (pJsonIteam && pJsonIteam->type == cJSON_True)
		{
			stDelay.bOnEnable = 1;
		}
		else if (pJsonIteam && pJsonIteam->type == cJSON_False)
		{
			stDelay.bOnEnable = 0;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OffEnable");
		if (pJsonIteam && pJsonIteam->type == cJSON_True)
		{
			stDelay.bOffEnable = 1;
		}
		else if (pJsonIteam && pJsonIteam->type == cJSON_False)
		{
			stDelay.bOffEnable = 0;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Cascode");
		if (pJsonIteam && pJsonIteam->type == cJSON_True)
		{
			stDelay.bCascode = 1;
		}
		else if (pJsonIteam && pJsonIteam->type == cJSON_False)
		{
			stDelay.bCascode = 0;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "CascodeNum");
		if (pJsonIteam && pJsonIteam->type == cJSON_Number)
		{
			stDelay.uiCascodeNum = pJsonIteam->valueint;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "CycleTimes");
		if (pJsonIteam && pJsonIteam->type == cJSON_Number)
		{
			stDelay.uiCycleTimes = pJsonIteam->valueint;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OnInterval");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			pcTmp = pJsonIteam->valuestring;
			//正确的格式如："00:00"
			if ( strlen(pcTmp) != 5 )
			{
				LOG_OUT(LOGOUT_ERROR, "PLUG_ParseTimerData, Invalid OnTime format, %s.", pcTmp);
				return FAIL;
			}
			stDelay.stOnInterval.iHour = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
			pcTmp += 3;
			stDelay.stOnInterval.iMinute = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OffInterval");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			pcTmp = pJsonIteam->valuestring;
			//正确的格式如："00:00"
			if ( strlen(pcTmp) != 5 )
			{
				LOG_OUT(LOGOUT_ERROR, "PLUG_ParseDelayData, Invalid OffTime format, %s.", pcTmp);
				return FAIL;
			}
			stDelay.stOffInterval.iHour = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
			pcTmp += 3;
			stDelay.stOffInterval.iMinute = (pcTmp[0]-'0')*10 + (pcTmp[1]-'0');
		}

		uiRet = PLUG_SetDelayData( &stDelay);
		if ( uiRet != OK )
		{
			LOG_OUT(LOGOUT_ERROR, "PLUG_ParseDelayData, PLUG_SetTimerData failed.");
			goto error;
		}
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}


UINT PLUG_ParseInfraredData( CHAR* pData )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonIteam = NULL;
	cJSON *pJsonArr = NULL;
	UINT uiRet = OK;
	INFRARED_VALUE_S stData;
	INT iCount = 0;
	UINT uiLoop = 0;

	if ( pData == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pData is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pData );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pData);
	    uiRet = FAIL;
	    goto exit;
	}

	iCount = cJSON_GetArraySize( pJsonRoot );
	if ( iCount <= 0)
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_GetArraySize failed, iCount:%d.", iCount);
	    uiRet = FAIL;
	    goto exit;
	}

	for ( uiLoop = 0; uiLoop < iCount; uiLoop++ )
	{
		pJsonArr = cJSON_GetArrayItem(pJsonRoot, uiLoop);

		stData.uiNum = INFRARED_ALL;
		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Num");
		if (pJsonIteam && pJsonIteam->type == cJSON_Number)
		{
			stData.uiNum = pJsonIteam->valueint;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Enable");
		if (pJsonIteam && pJsonIteam->type == cJSON_True)
		{
			stData.bEnable = 1;
		}
		else if (pJsonIteam && pJsonIteam->type == cJSON_False)
		{
			stData.bEnable = 0;
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "Name");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			strncpy(stData.szName, pJsonIteam->valuestring, INFRARED_NAME_MAX_LEN);
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OnValue");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			sscanf(pJsonIteam->valuestring, "%X", &stData.uiOnValue);
		}

		pJsonIteam = cJSON_GetObjectItem(pJsonArr, "OffValue");
		if (pJsonIteam && pJsonIteam->type == cJSON_String)
		{
			sscanf(pJsonIteam->valuestring, "%X", &stData.uiOffValue);
		}

		uiRet = INFRARED_SaveInfraredData( &stData );
		if ( uiRet != OK )
		{
			LOG_OUT(LOGOUT_ERROR, "INFRARED_SaveInfraredData failed");
			uiRet = FAIL;
			goto exit;
		}
	}

exit:
	cJSON_Delete(pJsonRoot);
	return uiRet;
}


UINT PLUG_ParseSystemData( CHAR* pData )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonIteam = NULL;
	INT iCount = 0;
	UINT uiLoop = 0;
	PLUG_SYSSET_S stSys;
	CHAR *pcTmp = NULL;
	UINT uiRet = 0;

	if ( pData == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_ParseSystemData, pData is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pData );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pData);
	    goto error;
	}

	memcpy(&stSys, PLUG_GetSystemSetData(), sizeof(PLUG_SYSSET_S));

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "RelayStatus");
	if (pJsonIteam && pJsonIteam->type == cJSON_True)
	{
		stSys.bRelayStatus = 1;
	}
	else if (pJsonIteam && pJsonIteam->type == cJSON_False)
	{
		stSys.bRelayStatus = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "SmartConfigFlag");
	if (pJsonIteam && pJsonIteam->type == cJSON_True)
	{
		stSys.bSmartConfigFlag = 1;
	}
	else if (pJsonIteam && pJsonIteam->type == cJSON_False)
	{
		stSys.bSmartConfigFlag = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "WifiMode");
	if (pJsonIteam && pJsonIteam->type == cJSON_Number)
	{
		stSys.ucWifiMode = pJsonIteam->valueint;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "PlugName");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy(stSys.szPlugName, pJsonIteam->valuestring, PLUG_NAME_MAX_LEN);
		stSys.szPlugName[PLUG_NAME_MAX_LEN] = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "WifiSSID");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy(stSys.szWifiSSID, pJsonIteam->valuestring, PLUG_WIFI_SSID_LEN);
		stSys.szWifiSSID[PLUG_WIFI_SSID_LEN] = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "WifiPasswd");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy(stSys.szWifiPasswd, pJsonIteam->valuestring, PLUG_WIFI_PASSWD_LEN);
		stSys.szWifiPasswd[PLUG_WIFI_PASSWD_LEN] = 0;
	}

	uiRet = PLUG_SetSystemSetData( &stSys);
	if ( uiRet != OK )
	{
		LOG_OUT(LOGOUT_ERROR, "PLUG_SetSystemSetData failed");
		goto error;
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}


UINT PLUG_ParseCloudPlatformData( CHAR* pData )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonIteam = NULL;
	PLUG_PLATFORM_S stPlatform;
	CHAR *pcTmp = NULL;
	UINT uiRet = 0;

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

	memcpy( &stPlatform, PLUG_GetPlatFormData(), sizeof(stPlatform));

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "CloudPlatform");
	if (pJsonIteam && pJsonIteam->type == cJSON_Number)
	{
		stPlatform.ucCloudPlatform = pJsonIteam->valueint;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "DevType");
	if (pJsonIteam && pJsonIteam->type == cJSON_Number)
	{
		stPlatform.eDevType = pJsonIteam->valueint;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "MqttProductKey");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy( stPlatform.szMqttProductKey, pJsonIteam->valuestring, PLUG_MQTT_PRODUCTKEY_LEN);
		stPlatform.szMqttProductKey[PLUG_MQTT_PRODUCTKEY_LEN] = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "MqttDevName");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy( stPlatform.szMqttDevName, pJsonIteam->valuestring, PLUG_MQTT_DEVNAME_LEN);
		stPlatform.szMqttDevName[PLUG_MQTT_DEVNAME_LEN] = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "MqttDevSecret");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy(stPlatform.szMqttDevSecret, pJsonIteam->valuestring, PLUG_MQTT_DEVSECRET_LEN);
		stPlatform.szMqttDevSecret[PLUG_MQTT_DEVSECRET_LEN] = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "BigiotDevId");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy(stPlatform.szBigiotDevId, pJsonIteam->valuestring, PLUG_BIGIOT_DEVID_LEN);
		stPlatform.szBigiotDevId[PLUG_BIGIOT_DEVID_LEN] = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "BigiotApiKey");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy(stPlatform.szBigiotApiKey, pJsonIteam->valuestring, PLUG_BIGIOT_APIKEY_LEN);
		stPlatform.szBigiotApiKey[PLUG_BIGIOT_APIKEY_LEN] = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "SwitchId");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy(stPlatform.szSwitchId, pJsonIteam->valuestring, PLUG_BIGIOT_IFID_LEN);
		stPlatform.szSwitchId[PLUG_BIGIOT_IFID_LEN] = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "TempId");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy(stPlatform.szTempId, pJsonIteam->valuestring, PLUG_BIGIOT_IFID_LEN);
		stPlatform.szTempId[PLUG_BIGIOT_IFID_LEN] = 0;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "HumidityId");
	if (pJsonIteam && pJsonIteam->type == cJSON_String)
	{
		strncpy(stPlatform.szHumidityId, pJsonIteam->valuestring, PLUG_BIGIOT_IFID_LEN);
		stPlatform.szHumidityId[PLUG_BIGIOT_IFID_LEN] = 0;
	}

	uiRet = PLUG_SetPlatFormData( &stPlatform);
	if ( uiRet != OK )
	{
		LOG_OUT(LOGOUT_ERROR, "PLUG_SetPlatFormData failed");
		goto error;
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}


UINT PLUG_ParseDeviceControlData( CHAR* pData )
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJsonIteam = NULL;
	PLUG_DEVCTL_S stSys;

	if ( pData == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "PLUG_ParseDeviceControlData, pData is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pData );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pData);
	    goto error;
	}

	pJsonIteam = cJSON_GetObjectItem(pJsonRoot, "Action");
	if (pJsonIteam && pJsonIteam->type == cJSON_Number)
	{
		stSys.ucAction = pJsonIteam->valueint;
	}

	//重启
	if ( stSys.ucAction == ACTION_REBOOT )
	{
		LOG_OUT(LOGOUT_INFO, "system will reboot");
		UPGRADE_StartRebootTimer();
	}
	//恢复出厂设置
	else if ( stSys.ucAction == ACTION_RESET )
	{
		LOG_OUT(LOGOUT_INFO, "system will reset");
		UPGRADE_Reset();
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}

UINT PLUG_MarshalJsonRelayStatus( CHAR* pcBuf, UINT uiBufLen )
{
	return snprintf( pcBuf, uiBufLen, "{\"status\":\"%s\"}", PLUG_GetRelayStatus() ? "on" : "off");
}


UINT PLUG_MarshalJsonDate( CHAR* pcBuf, UINT uiBufLen )
{
	PLUG_DATE_S stDate;
    PLUG_GetDate(&stDate);

    return snprintf( pcBuf, uiBufLen,
			"{\"Date\":\"%02d-%02d-%02d %02d:%02d:%02d\", \"SyncTime\":%s}",
			stDate.iYear, stDate.iMonth, stDate.iDay,
			stDate.iHour, stDate.iMinute, stDate.iSecond,
			PLUG_GetTimeSyncFlag() == TIME_SYNC_NONE ? "false" : "true");
}

static VOID PLUG_JudgeTimer( VOID )
{
	UINT8 ucLoopi = 0;
	UINT8 ucWeek = 0;
	PLUG_DATE_S stDate = {0, 0, 0, 0, 0, 0, 0};
	PLUG_TIMER_S *pstTimer = NULL;

	static BOOL bIsJudged = FALSE;
	static PLUG_DATE_S stDateLast = {0, 0, 0, 0, 0, 0, 0};

	pstTimer = PLUG_GetTimerData(PLUG_TIMER_MAX);
	PLUG_GetDate( &stDate );
	ucWeek = stDate.iWeek;

	if ( stDate.iMinute != stDateLast.iMinute)
	{
	    bIsJudged = FALSE;

		stDateLast = stDate;
	}

	for ( ucLoopi = 0 ; ucLoopi < PLUG_TIMER_MAX; ucLoopi++, pstTimer++ )
	{
	    if ( FALSE == pstTimer->bEnable )
	    {
			continue;
		}

    	/* 判断开启时间是否到了 */
		if ( TRUE == pstTimer->bOnEnable &&
			 pstTimer->stOnTime.iHour == stDate.iHour &&
			 pstTimer->stOnTime.iMinute == stDate.iMinute &&
			 FALSE == bIsJudged)
		{
			/* 判断重复 */
			if ( pstTimer->eWeek & (1 << (ucWeek-1)) )
			{
				PLUG_SetRelayOn( TRUE );
				bIsJudged = TRUE;
			}
			/* 只执行一次，执行完毕就关闭 */
			else if ( REPET_ONCE == pstTimer->eWeek )
			{
				PLUG_SetRelayOn( TRUE );
				pstTimer->bOnEnable = FALSE;
				bIsJudged = TRUE;

				/* 要将核心数据写入Flash */
				CONFIG_SaveConfig(PLUG_MOUDLE_TIMER);
			}
		}

    	/* 判断关闭时间是否到了 */
		if ( TRUE == pstTimer->bOffEnable &&
			 pstTimer->stOffTime.iHour == stDate.iHour &&
			 pstTimer->stOffTime.iMinute == stDate.iMinute &&
			 FALSE == bIsJudged)
		{
			if ( pstTimer->eWeek & (1 << (ucWeek-1)) )
			{
				PLUG_SetRelayOff( TRUE );
				bIsJudged = TRUE;

				/* 判断有无与delay级联 */
				PLUG_TimerCascade(pstTimer);
			}
			else if ( REPET_ONCE == pstTimer->eWeek )
			{
				PLUG_SetRelayOff( TRUE );
				pstTimer->bOffEnable = FALSE;
				bIsJudged = TRUE;

				/* 要将核心数据写入Flash */
				CONFIG_SaveConfig(PLUG_MOUDLE_TIMER);

				/* 判断有无与delay级联 */
				PLUG_TimerCascade(pstTimer);
			}
		}
	}
}


static VOID PLUG_TimerCascade( PLUG_TIMER_S *pstTimer )
{
	PLUG_DELAY_S *pstDelay = NULL;
	UINT uiLoopi = 0;
	UINT uiMember = 0;

	if ( NULL == pstTimer )
	{
		LOG_OUT(LOGOUT_ERROR, "PLUG_TimerCascade pstTimer = 0x%p.", pstTimer);
	    return;
	}

	if ( FALSE == pstTimer->bCascode )
	{
		return;
	}
	//pstTimer->bCascode = FALSE;
	uiMember = pstTimer->uiCascodeNum;

	if ( PLUG_DELAY_MAX < uiMember )
	{
		LOG_OUT(LOGOUT_ERROR, "PLUG_TimerCascade uiMember:%u.", uiMember);
		return;
	}

	pstDelay = PLUG_GetDelayData(PLUG_DELAY_MAX);
	for ( uiLoopi = 0 ; uiLoopi < PLUG_DELAY_MAX; uiLoopi++, pstDelay++ )
	{
		pstDelay->bEnable = FALSE;
	}

	pstDelay = PLUG_GetDelayData(uiMember-1);

	pstDelay->bEnable  = TRUE;
	PLUG_StartDelayTime(pstDelay);
}


static VOID PLUG_JudgeDelay( VOID )
{
	UINT8 ucLoopi = 0;
	PLUG_DELAY_S *pstDelay = NULL;
	PLUG_DATE_S stDate;
	UINT32 uiMember = 0;

	PLUG_GetDate( &stDate );

	pstDelay = PLUG_GetDelayData(PLUG_DELAY_MAX);

	for ( ucLoopi = 0 ; ucLoopi < PLUG_DELAY_MAX; ucLoopi++, pstDelay++ )
	{
		if ( FALSE == pstDelay->bEnable )
		{
			continue;
		}

		if ( pstDelay->stTimePoint.iMinute == stDate.iMinute &&
			pstDelay->stTimePoint.iHour == stDate.iHour )
		{
			/* 延时结束 */
			if ( 0 >= pstDelay->uiTmpCycleTimes )
			{
			    pstDelay->bEnable = FALSE;
				pstDelay->uiTmpCycleTimes = pstDelay->uiCycleTimes;

				if ( TRUE == pstDelay->bCascode )
				{
					uiMember = pstDelay->uiCascodeNum;
					pstDelay = PLUG_GetDelayData(uiMember-1);
					pstDelay->bEnable = TRUE;
					PLUG_StartDelayTime(pstDelay);
				}
				return;
			}

			/* 1:开启   2:关闭 */
			if ( 1 == pstDelay->ucSwFlag )
			{
				PLUG_SetRelayOn( FALSE );
			}
			else if ( 2 == pstDelay->ucSwFlag  )
			{
				PLUG_SetRelayOff( FALSE );
			}

			PLUG_GetNextDelayTime( pstDelay );
		}
		break;
	}
}


VOID PLUG_GetNextDelayTime( PLUG_DELAY_S *pstDelay )
{
	PLUG_DATE_S stDate;

	if ( NULL == pstDelay )
	{
		LOG_OUT(LOGOUT_ERROR, "PLUG_GetNextDelayTime pstDelay = 0x%p.", pstDelay);
	    return;
	}

	if ( FALSE == pstDelay->bEnable )
	{
		return;
	}

	/* 只使能了开的延时，就间隔固定的时间执行on动作，不执行off动作 */
	if ( TRUE == pstDelay->bOnEnable && FALSE == pstDelay->bOffEnable )
	{
		/* 0:不动作   1:下次开启   2:下次关闭 */
		pstDelay->ucSwFlag = 1;
	}
	/* 只使能了关的延时，就间隔固定的时间执行off动作，不执行on动作 */
	else if ( FALSE == pstDelay->bOnEnable && TRUE == pstDelay->bOffEnable )
	{
		/* 0:不动作   1:下次开启   2:下次关闭 */
		pstDelay->ucSwFlag = 2;
	}
	/* 开，关都是能了，就轮换执行on和off */
	else if ( TRUE == pstDelay->bOnEnable && TRUE == pstDelay->bOffEnable )
	{
		if ( 2 == pstDelay->ucSwFlag )
		{
			pstDelay->uiTmpCycleTimes --;
		}
		pstDelay->ucSwFlag = (1 == pstDelay->ucSwFlag) ? (2) : (1);
	}
	/* 开，关都禁了，就不动作 */
	else if ( FALSE == pstDelay->bOnEnable && FALSE == pstDelay->bOffEnable )
	{
		/* 0:不动作   1:下次开启   2:下次关闭 */
		pstDelay->ucSwFlag = 0;
		return;
	}

	PLUG_GetDate( &stDate );
	/* 1:下次开启 */
	if ( 1 == pstDelay->ucSwFlag )
	{
		pstDelay->stTimePoint.iMinute = stDate.iMinute + pstDelay->stOffInterval.iMinute;
		pstDelay->stTimePoint.iHour	= stDate.iHour + pstDelay->stOffInterval.iHour;
	}
	/* 2:下次关闭 */
	else if ( 2 == pstDelay->ucSwFlag )
	{
		pstDelay->stTimePoint.iMinute = stDate.iMinute + pstDelay->stOnInterval.iMinute;
		pstDelay->stTimePoint.iHour	= stDate.iHour + pstDelay->stOnInterval.iHour;
	}

	if ( pstDelay->uiTmpCycleTimes == 0 )
	{
		pstDelay->ucSwFlag = 0;
	}

	/* 防止相加结果越界 */
	if ( 59 < pstDelay->stTimePoint.iMinute )
	{
	    pstDelay->stTimePoint.iMinute -= 59;
		pstDelay->stTimePoint.iHour ++;
	}

	pstDelay->stTimePoint.iHour = pstDelay->stTimePoint.iHour % 24;
	return;
}


VOID PLUG_StartDelayTime( PLUG_DELAY_S *pstDelay )
{
	PLUG_DATE_S stDate;
	UINT uiLoopi = 0;
	PLUG_DELAY_S *pstDelayTmp = NULL;

	if ( NULL == pstDelay )
	{
		LOG_OUT(LOGOUT_ERROR, "PLUG_StartDelayTime pstDelay = 0x%p.", pstDelay);
	    return;
	}

	if ( FALSE == pstDelay->bEnable )
	{
		return;
	}

	/* 关闭其他延时任务 */
	pstDelayTmp = PLUG_GetDelayData(PLUG_DELAY_MAX);
	for ( uiLoopi = 0; uiLoopi < PLUG_DELAY_MAX; uiLoopi++, pstDelayTmp++ )
	{
		pstDelayTmp->bEnable = FALSE;
	}

	pstDelay->bEnable = TRUE;
	pstDelay->uiTmpCycleTimes = pstDelay->uiCycleTimes;

	PLUG_GetDate( &stDate );

	if (stDate.iSecond > 55 )
	{
		pstDelay->stTimePoint.iMinute = stDate.iMinute + 1;
	}
	else
	{
		pstDelay->stTimePoint.iMinute = stDate.iMinute;
	}

	pstDelay->stTimePoint.iHour	= stDate.iHour;

	/* 防止相加结果越界 */
	if ( 59 < pstDelay->stTimePoint.iMinute )
	{
	    pstDelay->stTimePoint.iMinute -= 59;
		pstDelay->stTimePoint.iHour ++;
	}

	if ( pstDelay->stTimePoint.iHour >= 24 )
	{
		pstDelay->stTimePoint.iHour = pstDelay->stTimePoint.iHour % 24;
	}
	pstDelay->ucSwFlag = 1;

	return;
}

VOID PLUG_TimerHandle( VOID *pPara )
{
	static UINT8 uiCount = 0;

	if ( uiCount++ >= 10 )
	{
		uiCount = 0;

		g_uiRunTime++;
		if ( PLUG_GetTimeSyncFlag() != TIME_SYNC_NONE  )
		{
			PLUG_JudgeTimer();
			PLUG_JudgeDelay();
		}
	}

	INFRARED_JudgeInfrared();
}

VOID PLUG_StartJudgeTimeHanderTimer( VOID )
{
	static xTimerHandle xJudgeTimeTimers = NULL;
	UINT32 uiJudgeTimeTimerId = 0;

	xJudgeTimeTimers = xTimerCreate("PLUG_TimerHandle", 100/portTICK_RATE_MS, TRUE, &uiJudgeTimeTimerId, PLUG_TimerHandle);
	if ( !xJudgeTimeTimers )
	{
		LOG_OUT(LOGOUT_ERROR, "xTimerCreate PLUG_TimerHandle failed.");
	}
	else
	{
		if(xTimerStart(xJudgeTimeTimers, 1) != pdPASS)
	    {
			LOG_OUT(LOGOUT_ERROR, "xTimerCreate xKeyTimers start failed.");
		}
	}
}







