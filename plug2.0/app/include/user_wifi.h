/*
 * user_wifi.h
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */

#ifndef __USER_WIFI_H__
#define __USER_WIFI_H__

#define WIFI_SCAN_NUM        20

#ifndef WIFI_SSID_LEN
    #define WIFI_SSID_LEN    32
#endif

#ifndef WIFI_MAC_LEN
    #define WIFI_MAC_LEN     20
#endif

#define WIFI_AUTHMODE_LEN     15

typedef enum {
    WIFI_SCAN_STATUS_IDLE = 0,
    WIFI_SCAN_STATUS_BUSY,
    WIFI_SCAN_STATUS_FINISH

} WIFI_SCAN_STATUS;

typedef enum {
    WIFI_MODE_NULL = 0,      /**< null mode */
    WIFI_MODE_STATION,       /**< WiFi station mode */
    WIFI_MODE_SOFTAP,        /**< WiFi soft-AP mode */
    WIFI_MODE_STATIONAP,     /**< WiFi station + soft-AP mode */

    WIFI_MODE_BUFF
} WIFI_MODE_E;

typedef struct tagWifiScan
{
    CHAR                 szSsid[WIFI_SSID_LEN];    //wifi名称
    CHAR                 szMac[WIFI_MAC_LEN];    //mac地址
    INT8                iRssi;                    //信号强度单位是dbm，负值越接近0信号越好
    UINT8                ucChannel;                //信道
    CHAR                 szAuthMode[WIFI_AUTHMODE_LEN];    //加密类型

}WIFI_SCAN_S;

typedef struct tagWifiInfo
{
    UINT32                 uiIp;
    UINT32                uiGetWay;
    UINT32                uiNetMask;

}WIFI_INFO_S;

UINT WIFI_SetWifiModeStation( VOID );
UINT WIFI_SetWifiModeAP( VOID );

VOID WIFI_StartWifiLinkStatusTheard( VOID );
void WIFI_StartWifiModeTheard( void );

UINT WIFI_ScanWifiSsid( VOID );
WIFI_INFO_S WIFI_GetIpInfo();
CHAR* WIFI_GetMacAddr( CHAR *pcMac, UINT uiLen );


WIFI_SCAN_S *g_pstWifiScanHead;

#endif /* __USER_WIFI_H__ */
