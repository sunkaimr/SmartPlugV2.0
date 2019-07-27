/*
 * user_wifi.c
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */
#include "user_common.h"
#include "esp_common.h"


const CHAR szWifiModeString[][15] = {
	"none",
	"station",
	"AP",
	"station_AP",

	"buff"
};

const CHAR szWifiAuthModeString[][15] = {
	"open",
	"WEP",
	"WPA_PSK",
	"WPA2_PSK",
	"WPA_WPA2_PSK",
	"unknow"
};

const CHAR szWifiStatusString[][20] = {
	"idle",
	"connecting",
	"password is wrong",
	"can not find AP",
	"fail to connect AP",
	"got IP from AP",

	"unknow"
};


static UINT uiCurStatus = 0;
WIFI_SCAN_STATUS uiWifiScanStatus = WIFI_SCAN_STATUS_IDLE;
WIFI_SCAN_S *g_pstWifiScanHead = NULL;

VOID WIFI_ScanWifiSsidDone(void *arg, STATUS status)
{
	uint8 uiLength = 0;
	WIFI_SCAN_S *pstWifi = NULL;

	if (status == OK)
	{
		struct bss_info *bss_link = (struct bss_info *)arg;
		if ( g_pstWifiScanHead == NULL )
		{
			LOG_OUT(LOGOUT_ERROR, "WIFI_ScanWifiSsidDone pstWifiScanHead is NULL");
			return;
		}

		pstWifi = g_pstWifiScanHead;
		while ( bss_link != NULL && uiLength <= WIFI_SCAN_NUM )
		{
			strncpy(pstWifi->szSsid, bss_link->ssid, WIFI_SSID_LEN);
			sprintf(pstWifi->szMac, "%02X-%02X-%02d-%02X-%02X-%02X",
					(bss_link->bssid)[0], (bss_link->bssid)[1], (bss_link->bssid)[2],
					(bss_link->bssid)[3], (bss_link->bssid)[4], (bss_link->bssid)[5]);
			strcpy(pstWifi->szAuthMode, szWifiAuthModeString[bss_link->authmode]);
			pstWifi->ucChannel = bss_link->channel;
			pstWifi->iRssi = bss_link->rssi;

			uiLength++;
			pstWifi++;
			bss_link = bss_link->next.stqe_next;
		}
	}
	else
	{
		LOG_OUT(LOGOUT_ERROR, "wifi scan failed.");
	}

	uiWifiScanStatus = WIFI_SCAN_STATUS_FINISH;
}


UINT WIFI_ScanWifiSsid( VOID )
{
	BOOL bRet = FALSE;
	UINT8 ucTimeout = 60;

	//LOG_OUT(LOGOUT_INFO, "start time:%d", system_get_time()/1000/1000);

	//扫描可用wifi信号
	uiWifiScanStatus = WIFI_SCAN_STATUS_BUSY;
	bRet = wifi_station_scan(NULL, WIFI_ScanWifiSsidDone);
	if ( !bRet )
	{
		LOG_OUT(LOGOUT_ERROR, "wifi scan failed.");
		return FAIL;
	}

	//等待扫描结束
	while( uiWifiScanStatus != WIFI_SCAN_STATUS_FINISH )
	{
		vTaskDelay( 500/portTICK_RATE_MS );

		ucTimeout --;
		if ( ucTimeout == 0 )
		{
			LOG_OUT(LOGOUT_INFO, "wifi scan timeout.");
			break;
		}
	}

	//LOG_OUT(LOGOUT_INFO, "end time:%d", system_get_time()/1000/1000);
	return OK;
}


UINT WIFI_SetWifiModeStation( VOID )
{
	BOOL bRet = FALSE;
	UINT uiOpmode = NULL_MODE;
	UINT uiWifiConnSta = STATION_IDLE;
	struct station_config sta_conf;

	//检查是否为station模式，若不是设置为station模式
	uiOpmode = wifi_get_opmode_default();
	if ( uiOpmode != WIFI_MODE_STATION )
	{
		LOG_OUT(LOGOUT_DEBUG, "old opmode is: %s", szWifiModeString[uiOpmode]);

		uiOpmode = WIFI_MODE_STATION;
		bRet = wifi_set_opmode( uiOpmode );
		if ( !bRet )
		{
			LOG_OUT(LOGOUT_ERROR, "set station failed.");
			return RET_FAILED;
		}
		LOG_OUT(LOGOUT_DEBUG, "new opmode is: %s", szWifiModeString[uiOpmode]);
	}

	//设置为启动自动连接
	if ( !wifi_station_get_auto_connect() )
	{
		bRet = wifi_station_set_auto_connect( TRUE );
		if ( !bRet )
		{
			LOG_OUT(LOGOUT_ERROR, "wifi_station_set_auto_connect(TRUE) failed.");
			return RET_FAILED;
		}
		LOG_OUT(LOGOUT_DEBUG, "wifi_station_get_auto_connect is TRUE.");
	}

	//设置wifi断开自动重连
	if ( !wifi_station_get_reconnect_policy() )
	{
		bRet = wifi_station_set_reconnect_policy( TRUE );
		if ( !bRet )
		{
			LOG_OUT(LOGOUT_ERROR, "wifi_station_set_reconnect_policy(TRUE) failed.");
			return RET_FAILED;
		}
		LOG_OUT(LOGOUT_DEBUG, "wifi_station_set_reconnect_policy is TRUE.");
	}

	/* hostname 不会保存到Flash中每次重启需要重新设置 */
	bRet = wifi_station_set_hostname(PLUG_GetPlugName());
	if ( !bRet )
	{
		LOG_OUT(LOGOUT_ERROR, "wifi_station_set_hostname failed.");
		return RET_FAILED;
	}
	LOG_OUT(LOGOUT_INFO, "set hostname is %s", PLUG_GetPlugName());

	bRet = wifi_station_get_config_default( &sta_conf );
	if ( !bRet )
	{
		LOG_OUT(LOGOUT_ERROR, "wifi_station_get_config_default failed.");
		return RET_FAILED;
	}

	if ( !strncmp(sta_conf.ssid, PLUG_GetWifiSsid(), PLUG_GetWifiSsidLenth()) &&
		 !strncmp(sta_conf.password, PLUG_GetWifiPasswd(), PLUG_GetWifiPasswdLenth())
	)
	{
		LOG_OUT(LOGOUT_DEBUG, "wifi info %s not change.", szWifiModeString[uiOpmode]);
	}
	else
	{
		//先断开连接的其他wifi
		uiWifiConnSta = wifi_station_get_connect_status();
		if ( uiWifiConnSta != STATION_IDLE )
		{
			bRet = wifi_station_disconnect();
			if ( !bRet )
			{
				LOG_OUT(LOGOUT_ERROR, "disconnect old wifi failed.");
				return RET_FAILED;
			}
			LOG_OUT(LOGOUT_DEBUG, "disconnect old wifi successed.");
		}

		//设置要连接的wifi的SSID和密码
		memset((VOID*)&sta_conf, 0, sizeof(sta_conf) );
		memcpy(&sta_conf.ssid, PLUG_GetWifiSsid(), PLUG_GetWifiSsidLenth());
		memcpy(&sta_conf.password, PLUG_GetWifiPasswd(), PLUG_GetWifiPasswdLenth());
		bRet = wifi_station_set_config(&sta_conf);
		if ( !bRet )
		{
			LOG_OUT(LOGOUT_ERROR, "wifi_station_set_config failed.");
			return RET_FAILED;
		}
	}

	//连接wifi
	bRet = wifi_station_connect();
	if ( !bRet )
	{
		LOG_OUT(LOGOUT_ERROR, "wifi_station_connect failed.");
		return RET_FAILED;
	}

	//等待连接完成
	while ( uiWifiConnSta = wifi_station_get_connect_status(), uiWifiConnSta != STATION_GOT_IP )
	{
		LOG_OUT(LOGOUT_DEBUG, "uiWifiConnSta: %s", szWifiModeString[uiOpmode]);
		vTaskDelay( 1000/portTICK_RATE_MS );
	}

	LOG_OUT(LOGOUT_INFO, "set wifi mode %s successed", szWifiModeString[uiOpmode]);
}



UINT WIFI_SetWifiModeAP( VOID )
{
	BOOL bRet = FALSE;
	UINT uiOpmode = WIFI_MODE_NULL;
	struct softap_config config;
	UINT uiWifiConnSta = STATION_IDLE;

	//检查是否为ap模式，若不是设置为ap模式
	uiOpmode = wifi_get_opmode_default();
	if ( uiOpmode != WIFI_MODE_SOFTAP )
	{
		LOG_OUT(LOGOUT_DEBUG, "old opmode is: %s", szWifiModeString[uiOpmode]);

		//关闭启动自动连接wifi
		if ( wifi_station_get_auto_connect() )
		{
			bRet = wifi_station_set_auto_connect( FALSE );
			if ( !bRet )
			{
				LOG_OUT(LOGOUT_ERROR, "wifi_station_set_auto_connect(FALSE) failed.");
				return RET_FAILED;
			}
		}

		//取消wifi断开自动重连
		if ( wifi_station_get_reconnect_policy() )
		{
			bRet = wifi_station_set_reconnect_policy( FALSE );
			if ( !bRet )
			{
				LOG_OUT(LOGOUT_ERROR, "wifi_station_set_reconnect_policy(FALSE) failed.");
				return RET_FAILED;
			}
			LOG_OUT(LOGOUT_DEBUG, "wifi_station_set_reconnect_policy(FALSE) success");
		}

		//先断开连接的其他wifi
		uiWifiConnSta = wifi_station_get_connect_status();
		if ( uiWifiConnSta != STATION_IDLE )
		{
			LOG_OUT(LOGOUT_DEBUG, "wifi_station_disconnect...");
			bRet = wifi_station_disconnect();
			if ( !bRet )
			{
				LOG_OUT(LOGOUT_ERROR, "disconnect old wifi failed.");
			}
			LOG_OUT(LOGOUT_DEBUG, "wifi_station_disconnect success");
		}

		uiOpmode = WIFI_MODE_SOFTAP;
		bRet = wifi_set_opmode( uiOpmode );
		if ( !bRet )
		{
			LOG_OUT(LOGOUT_ERROR, "set ap mode failed.");
			return RET_FAILED;
		}
		LOG_OUT(LOGOUT_DEBUG, "new opmode is: %s", szWifiModeString[uiOpmode]);
	}

	bRet = wifi_softap_get_config_default( &config );
	if ( !bRet )
	{
		LOG_OUT(LOGOUT_ERROR, "wifi_softap_get_config_default failed.");
		return RET_FAILED;
	}

	LOG_OUT(LOGOUT_DEBUG, "ssid_len:[%d][%d]", config.ssid_len, PLUG_GetPlugNameLenth());
	LOG_OUT(LOGOUT_DEBUG, "authmode:[%d][%d]", config.authmode, AUTH_OPEN);
	LOG_OUT(LOGOUT_DEBUG, "max_connection:[%d][%d]", config.max_connection, 4);
	LOG_OUT(LOGOUT_DEBUG, "ssid:[%s][%s]", config.ssid, PLUG_GetPlugName());

	if ( config.authmode == AUTH_OPEN &&
		 config.max_connection == 4 &&
		 config.ssid_len == PLUG_GetPlugNameLenth() &&
		 strncmp(config.ssid, PLUG_GetPlugName(), PLUG_GetPlugNameLenth()) == 0)
	{
		LOG_OUT(LOGOUT_DEBUG, "wifi mode %s not change.", szWifiModeString[uiOpmode]);

		goto end;
	}
	config.ssid_len = PLUG_GetPlugNameLenth();
	strncpy( config.ssid, PLUG_GetPlugName(), config.ssid_len );
	config.authmode = AUTH_OPEN;
	config.max_connection = 4;
	bRet = wifi_softap_set_config( &config );
	if ( !bRet )
	{
		LOG_OUT(LOGOUT_ERROR, "wifi_softap_set_config failed.");
		return RET_FAILED;
	}
	LOG_OUT(LOGOUT_INFO, "set hostname is %s", PLUG_GetPlugName());
	LOG_OUT(LOGOUT_INFO, "set wifi mode %s successed", szWifiModeString[uiOpmode]);
end:

	return RET_SUCCESSED;
}

VOID WIFI_SmartConfigDone(sc_status status, void *pdata)
{
	sc_type *type;
	struct station_config *sta_conf;
	struct station_config conf;

    switch(status) {
        case SC_STATUS_WAIT:
            break;
        case SC_STATUS_FIND_CHANNEL:
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                //printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                //printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            sta_conf = pdata;
	        wifi_station_set_config(sta_conf);
	        wifi_station_disconnect();
	        wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};
                memcpy(phone_ip, (uint8*)pdata, 4);
                //printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            } else {
            	//SC_TYPE_AIRKISS - support airkiss v2.0
				//airkiss_start_discover();
			}
            smartconfig_stop();

            PLUG_SetSmartConfig(TRUE);

        	wifi_station_get_config_default( &conf );
    		PLUG_SetWifiSsid(conf.ssid);
    		PLUG_SetWifiPasswd(conf.password);
            break;
    }
}

UINT WIFI_SetWifiModeSmartConfig( VOID )
{
	BOOL bRet = FALSE;
	UINT uiOpmode = WIFI_MODE_NULL;
	struct station_config sta_conf;

	//检查是否为station模式，若不是设置为station模式
	uiOpmode = wifi_get_opmode_default();
	if ( uiOpmode != WIFI_MODE_STATION )
	{
		LOG_OUT(LOGOUT_DEBUG, "old opmode is: %s", szWifiModeString[uiOpmode]);

		uiOpmode = WIFI_MODE_STATION;
		bRet = wifi_set_opmode( uiOpmode );
		if ( !bRet )
		{
			LOG_OUT(LOGOUT_ERROR, "set station failed.");
			return RET_FAILED;
		}
		LOG_OUT(LOGOUT_DEBUG, "new opmode is: %s", szWifiModeString[uiOpmode]);
	}

	//设置为启动自动连接
	if ( !wifi_station_get_auto_connect() )
	{
		bRet = wifi_station_set_auto_connect( TRUE );
		if ( !bRet )
		{
			LOG_OUT(LOGOUT_ERROR, "wifi_station_set_auto_connect(TRUE) failed.");
			return RET_FAILED;
		}
		LOG_OUT(LOGOUT_DEBUG, "wifi_station_get_auto_connect is TRUE.");
	}

	//设置wifi断开自动重连
	if ( !wifi_station_get_reconnect_policy() )
	{
		bRet = wifi_station_set_reconnect_policy( TRUE );
		if ( !bRet )
		{
			LOG_OUT(LOGOUT_ERROR, "wifi_station_set_reconnect_policy(TRUE) failed.");
			return RET_FAILED;
		}
		LOG_OUT(LOGOUT_DEBUG, "wifi_station_set_reconnect_policy is TRUE.");
	}

	/* hostname 不会保存到Flash中每次重启需要重新设置 */
	bRet = wifi_station_set_hostname(PLUG_GetPlugName());
	if ( !bRet )
	{
		LOG_OUT(LOGOUT_ERROR, "wifi_station_set_hostname failed.");
		return RET_FAILED;
	}

	if ( PLUG_GetSmartConfig() == FALSE )
	{
		LOG_OUT(LOGOUT_INFO, "smartconfig verdion: %s", smartconfig_get_version());

		//每次smartconfig之前先要暂停
		smartconfig_stop();
		smartconfig_start(WIFI_SmartConfigDone);
	}

	//等待连接完成
	while ( wifi_station_get_connect_status() != STATION_GOT_IP )
	{
		LOG_OUT(LOGOUT_DEBUG, "uiWifiConnSta: %s", szWifiStatusString[wifi_station_get_connect_status()]);
		vTaskDelay( 1000/portTICK_RATE_MS );
	}

	LOG_OUT(LOGOUT_INFO, "set wifi mode %s success", szWifiModeString[WIFI_MODE_STATION]);
}


VOID WIFI_SetWifiModeTask( void *para )
{
	WIFI_MODE_E eWifiMode = 0;
	UINT uiRet = 0;
	PLUG_DATE_S pstDate = {2018, 1, 1, 0, 12, 0, 0 };

	LOG_OUT(LOGOUT_INFO, "WIFI_SetWifiModeTask started.");
	WIFI_StartWifiLinkStatusTheard();

	eWifiMode = PLUG_GetWifiMode();
	if ( eWifiMode == WIFI_MODE_SOFTAP )
	{
		WIFI_SetWifiModeAP();

		PLUG_SetDate(&pstDate);
		LOG_OUT(LOGOUT_INFO, "Socket_SetDate: 2018-01-01 12:00:00");

		WEB_StartWebServerTheard();
		LED_SetWifiStatus(LED_WIFI_STATUS_SYNC_TIME);
	}
	else if ( eWifiMode == WIFI_MODE_STATION )
	{
		if ( PLUG_GetSmartConfig() == FALSE)
		{
			WIFI_SetWifiModeSmartConfig();
		}
		else
		{
			WIFI_SetWifiModeStation();
		}

		uiRet = PLUG_GetTimeFromInternet();
		if ( uiRet != OK )
		{
			LOG_OUT(LOGOUT_ERROR, "Get time from internet failed.");
			PLUG_SetDate(&pstDate);
			LOG_OUT(LOGOUT_INFO, "Socket_SetDate: %d-%02d-%02d %02d:%02d:%02d",
									pstDate.iYear, pstDate.iMonth, pstDate.iDay,
									pstDate.iHour, pstDate.iMinute, pstDate.iSecond);
		}
		else
		{
			LOG_OUT(LOGOUT_INFO, "Get time from internet successed.");
		}

		MQTT_StartMqttTheard();

		for ( ; ; )
		{
#if 1
			//LOG_OUT(LOGOUT_DEBUG, "uiCurStatus:%d-%d", uiCurStatus, WEB_GetWebSvcStatus());
			if ( uiCurStatus == STATION_GOT_IP && WEB_GetWebSvcStatus() == FALSE )
			{
				LOG_OUT(LOGOUT_INFO, "WEB_StartWebServerTheard.");
				WEB_StartWebServerTheard();
			}

			if ( uiCurStatus != STATION_GOT_IP && WEB_GetWebSvcStatus() == TRUE )
			{
				LOG_OUT(LOGOUT_INFO, "WEB_StopWebServerTheard.");
				WEB_StopWebServerTheard();
			}
#endif
			vTaskDelay( 1000/portTICK_RATE_MS );
		}
	}

	LOG_OUT(LOGOUT_INFO, "WIFI_SetWifiModeTask stoppped.");
	vTaskDelete( NULL );
}


VOID WIFI_StartWifiModeTheard( void )
{
	xTaskCreate(WIFI_SetWifiModeTask, "WIFI_SetWifiModeTask", 512, NULL, 2, NULL);
}


VOID WIFI_WifiLinkStatusTask( void *para )
{
	UINT8 uiLastStatus = 1;
	struct ip_info stStationInfo;
	UINT8 uiWifiMode;

	LOG_OUT(LOGOUT_INFO, "WIFI_WifiLinkStatusTask started.");
	uiWifiMode = PLUG_GetWifiMode();
	if ( WIFI_MODE_STATION == uiWifiMode )
	{
		for ( ; ; )
		{
			uiCurStatus = wifi_station_get_connect_status();
			if ( uiLastStatus != uiCurStatus )
			{
				switch ( uiCurStatus )
				{
					case STATION_IDLE :
						LOG_OUT(LOGOUT_INFO, "Scan available wifi...");
						LED_SetWifiStatus(LED_WIFI_STATUS_FIND_WIFI);
						break;

					case STATION_CONNECTING :
						LOG_OUT(LOGOUT_INFO, "connect wifi...");
						LED_SetWifiStatus(LED_WIFI_STATUS_CONNECTTING);
						break;

					case STATION_WRONG_PASSWORD :
						LED_SetWifiStatus(LED_WIFI_STATUS_CONNECTTING);
						LOG_OUT(LOGOUT_INFO, "wifi password is worng.");
						break;

					case STATION_NO_AP_FOUND :
						LOG_OUT(LOGOUT_INFO, "No any wifi was found.");
						LED_SetWifiStatus(LED_WIFI_STATUS_FIND_WIFI);
						break;

					case STATION_CONNECT_FAIL:
						LOG_OUT(LOGOUT_INFO, "connect wifi failed.");
						LED_SetWifiStatus(LED_WIFI_STATUS_FIND_WIFI);
						break;

					case STATION_GOT_IP :
						LED_SetWifiStatus(LED_WIFI_STATUS_ON);

						wifi_get_ip_info( STATION_IF, &stStationInfo );
						LOG_OUT(LOGOUT_INFO, "connet wifi successed, IP:%d.%d.%d.%d.", stStationInfo.ip.addr&0xFF, (stStationInfo.ip.addr>>8)&0xFF,
							(stStationInfo.ip.addr>>16)&0xFF, (stStationInfo.ip.addr>>24)&0xFF);

						break;

					default : break;
				}

				uiLastStatus = uiCurStatus;
			}

			vTaskDelay( 1000/portTICK_RATE_MS );
		}
	}
	else
	{
		LED_SetWifiStatus(LED_WIFI_STATUS_SYNC_TIME);
	}

	LOG_OUT(LOGOUT_INFO, "WIFI_WifiLinkStatusTask stoppped.");

	vTaskDelete( NULL );
}


VOID WIFI_StartWifiLinkStatusTheard( void )
{
	xTaskCreate(WIFI_WifiLinkStatusTask, "WIFI_WifiLinkStatusTask", 256, NULL, 3, NULL);
}



WIFI_INFO_S WIFI_GetIpInfo()
{
	PLUG_SYSSET_S *pstSystemData = NULL;
	struct ip_info stStationInfo;
	WIFI_INFO_S stWifiInfo = {0, 0, 0};

	pstSystemData = PLUG_GetSystemSetData();
	if ( pstSystemData->ucWifiMode == WIFI_MODE_SOFTAP )
	{
		if ( TRUE != wifi_get_ip_info( SOFTAP_IF, &stStationInfo ))
		{
		    LOG_OUT(LOGOUT_ERROR, "WIFI_GetIpInfo, get device ip on ap mode failed.");
		    return stWifiInfo;
		}
	}
	else
	{
		if ( TRUE != wifi_get_ip_info( STATION_IF, &stStationInfo ))
		{
			LOG_OUT(LOGOUT_ERROR, "WIFI_GetIpInfo, get device ip on station mode failed.");
			return stWifiInfo;
		}
	}

	stWifiInfo.uiIp 	 = stStationInfo.ip.addr;
	stWifiInfo.uiGetWay  = stStationInfo.gw.addr;
	stWifiInfo.uiNetMask = stStationInfo.netmask.addr;

	return stWifiInfo;
}


BOOL WIFI_GetMacAddr(CHAR *pcMac, UINT uiLen)
{
	PLUG_SYSSET_S *pstSystemData = NULL;
	UINT8 ucMac[6] = {0};

	if ( pcMac == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "WIFI_GetMacAddr, pcMac is NULL.");
		goto err;
	}

	pstSystemData = PLUG_GetSystemSetData();
	if ( pstSystemData->ucWifiMode == WIFI_MODE_SOFTAP )
	{
		if ( TRUE != wifi_get_macaddr( SOFTAP_IF, ucMac))
		{
		    LOG_OUT(LOGOUT_ERROR, "WIFI_GetMacAddr, get device mac address on ap mode failed.");
		    goto err;
		}
	}
	else
	{
		if ( TRUE != wifi_get_macaddr( STATION_IF, ucMac))
		{
		    LOG_OUT(LOGOUT_ERROR, "WIFI_GetMacAddr, get device mac address on station mode failed.");
		    goto err;
		}
	}

	snprintf(pcMac, uiLen, "%02X-%02X-%02X-%02X-%02X-%02X", ucMac[0], ucMac[1], ucMac[2], ucMac[3], ucMac[4], ucMac[5]);
	return TRUE;

err:
	snprintf(pcMac, uiLen, "00-00-00-00-00-00");
	return FALSE;
}


UINT WIFI_WifiScanMarshalJson( CHAR* pcBuf, UINT uiBufLen )
{
	WIFI_SCAN_S *pstData = NULL;
	UINT uiLoopi = 0;
	UINT8 uiRet = 0;
	CHAR *pJsonStr = NULL;
	cJSON  *pJsonArry, *pJsonsub;

	//只有在stataion模式下才能扫描
	if ( PLUG_GetWifiMode() != WIFI_MODE_STATION )
	{
		return 0;
	}

	if( g_pstWifiScanHead == NULL )
	{
		g_pstWifiScanHead = ( WIFI_SCAN_S* )malloc( sizeof(WIFI_SCAN_S) * WIFI_SCAN_NUM );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "WIFI_WifiScanMarshalJson, malloc pcBuf failed.");
			return 0;
	    }
	    memset(g_pstWifiScanHead, 0, sizeof(WIFI_SCAN_S) * WIFI_SCAN_NUM);
	}

	uiRet = WIFI_ScanWifiSsid();
	if ( uiRet != OK )
	{
		FREE_MEM(pJsonStr);
		LOG_OUT(LOGOUT_ERROR, "WIFI_ScanWifiSsid failed.");
		return 0;
	}

	pstData = g_pstWifiScanHead;
	pJsonArry = cJSON_CreateArray();
	for ( uiLoopi = 0 ; uiLoopi < WIFI_SCAN_NUM; uiLoopi++, pstData++ )
	{
		if( pstData->szMac[0] == 0 )
		{
			break;
		}
		pJsonsub=cJSON_CreateObject();

		cJSON_AddStringToObject( pJsonsub,	"Ssid", 			pstData->szSsid);
		cJSON_AddStringToObject( pJsonsub,	"Mac", 				pstData->szMac);
		cJSON_AddStringToObject( pJsonsub,	"AuthMode",			pstData->szAuthMode);
		cJSON_AddNumberToObject( pJsonsub,	"Rssi",				pstData->iRssi);
		cJSON_AddNumberToObject( pJsonsub,	"Channel",			pstData->ucChannel);

		cJSON_AddItemToArray(pJsonArry, pJsonsub);
	}
	FREE_MEM(g_pstWifiScanHead);

    pJsonStr = cJSON_PrintUnformatted(pJsonArry);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJsonArry);
    FREE_MEM(pJsonStr);
	return strlen(pcBuf);
}


UINT WIFI_DeviceInfoMarshalJson( CHAR* pcBuf, UINT uiBufLen)
{
	cJSON  *pJson = NULL;
	CHAR *pJsonStr = NULL;
	CHAR szBuf[20];

	pJson = cJSON_CreateObject();

	snprintf(szBuf, sizeof(szBuf),  "%s %s", __DATE__, __TIME__);
	cJSON_AddStringToObject( pJson, "BuildDate", 	szBuf);

	cJSON_AddStringToObject( pJson, "SDKVersion", 	system_get_sdk_version());
	cJSON_AddStringToObject( pJson, "FlashMap", 	UPGRADE_GetFlashMap());

	snprintf(szBuf, sizeof(szBuf),  "user%d.bin", 	system_upgrade_userbin_check()+1);
	cJSON_AddStringToObject( pJson, "UserBin", 		szBuf);

	cJSON_AddNumberToObject( pJson, "RunTime", 		PLUG_GetRunTime());

    pJsonStr = cJSON_PrintUnformatted(pJson);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJson);
    FREE_MEM(pJsonStr);

	return strlen(pcBuf);
}
