/*
 * user_key.c
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */
#include "esp_common.h"
#include "user_common.h"


VOID KEY_GetKeyTimerHandle( VOID *pPara )
{
    UINT32 uiKeyStatus = 0;
    static UINT32 uiPressCount = 0;

    uiKeyStatus = LED_GetKeyStatus();
    if ( uiKeyStatus == 0 )
    {
        uiPressCount++;

        /* 中长按:按下3s-6s 进入station模式 */
        if ( uiPressCount > 150 && uiPressCount < 300 )
        {
        	LED_SetWifiStatus(LED_WIFI_STATUS_SET_STA);
        }
        /* 长按:>6s  进入AP模式  */
        else if ( uiPressCount >= 300 )
        {
        	LED_SetWifiStatus(LED_WIFI_STATUS_SET_AP);
        }
    }
    else
    {
        /* 短按:按下40ms-1s 开关继电器 */
        if ( uiPressCount > 2 && uiPressCount < 50 )
        {
            PLUG_SetRelayReversal(TRUE);
            //LOG_OUT(LOGOUT_INFO, "replay reversal!!!");
        }
        /* 中长按:按下3s-6s 进入station模式 */
        else if ( uiPressCount > 150 && uiPressCount < 300 )
        {
            PLUG_SetWifiMode(WIFI_MODE_STATION);
            PLUG_SetSmartConfig(FALSE);
            LOG_OUT(LOGOUT_INFO, "switch station mode, system restart!!!");
            UPGRADE_SetReboot();
        }
        /* 长按:>6s  进入AP模式  */
        else if ( uiPressCount >= 300 )
        {
            PLUG_SetWifiMode(WIFI_MODE_SOFTAP);
            LOG_OUT(LOGOUT_INFO, "switch ap mode, system restart!!!");
            UPGRADE_SetReboot();
        }
        uiPressCount = 0;
    }
}


VOID KEY_StartKeyHanderTimer( VOID )
{
    static xTimerHandle xKeyTimers = NULL;
    UINT32 uiKeyTimerId = 0;

    xKeyTimers = xTimerCreate("KEY_GetKeyTimerHandle", 20/portTICK_RATE_MS, TRUE, &uiKeyTimerId, KEY_GetKeyTimerHandle);
    if ( !xKeyTimers )
    {
        LOG_OUT(LOGOUT_ERROR, "xTimerCreate KEY_StartKeyHanderTimer failed.");
    }
    else
    {
        if(xTimerStart(xKeyTimers, 1) != pdPASS)
        {
            LOG_OUT(LOGOUT_ERROR, "xTimerCreate xKeyTimers start failed.");
        }
    }
}

VOID KEY_Restore( VOID )
{
    if ( LED_GetKeyStatus() == 0 )
    {
        LOG_OUT(LOGOUT_INFO, "power on and restore");
        UPGRADE_Reset();
    }
}


#if IS_WELL

static xTimerHandle xRelayOffTimers = NULL;
static xTimerHandle xWaterSensorTimers = NULL;

VOID KEY_SetRelayOff()
{
	LOG_OUT(LOGOUT_INFO, "water maybe full");
	PLUG_SetRelayOff(TRUE);
}

VOID KEY_CreateRelayOffTimer()
{
    UINT32 uiRelayOffTimerId = 0;

    xRelayOffTimers = xTimerCreate("KEY_SetRelayOff", 1000*60/portTICK_RATE_MS, FALSE, &uiRelayOffTimerId, KEY_SetRelayOff);
    if ( !xRelayOffTimers )
    {
        LOG_OUT(LOGOUT_ERROR, "xTimerCreate KEY_SetRelayOff failed.");
    }
}

VOID KEY_StartRelayOffTimer()
{
	// 延时关闭时间单位秒
	UINT uiTime = 60*8;

	if ( xRelayOffTimers == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "xRelayOffTimers is NULL.");
		return;
	}

	if(xTimerChangePeriod(xRelayOffTimers, 1000 * uiTime/portTICK_RATE_MS, 0) != pdPASS)
	{
		LOG_OUT(LOGOUT_ERROR, "xTimerChangePeriod xRelayOffTimers failed.");
		KEY_SetRelayOff();
		return;
	}

	if(xTimerStart(xRelayOffTimers, 0) != pdPASS)
	{
		KEY_SetRelayOff();
		LOG_OUT(LOGOUT_ERROR, "xTimerStart xRelayOffTimers start failed.");
	}

	LOG_OUT(LOGOUT_INFO, "Relay will be off after %ds", uiTime);
}

VOID KEY_StopRelayOffTimer()
{
	if ( xRelayOffTimers == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "xRelayOffTimers is NULL.");
		return;
	}

	if(xTimerStop(xRelayOffTimers, 0) != pdPASS)
	{
		LOG_OUT(LOGOUT_ERROR, "xTimerStop xRelayOffTimers start failed.");
	}
}

STATIC VOID KEY_WaterSensorErrorCallBack( VOID *Para)
{
    STATIC  UINT8 ucStatus = 0;

    ucStatus ^= 0x01;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_STATUS_NUM), ucStatus);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_WIFI_STATUS_NUM),  ucStatus);
}

os_timer_t stWaterSensorError;

VOID KEY_WaterSensorErrorTimerInit()
{
    os_timer_setfn(&stWaterSensorError, (os_timer_func_t *)KEY_WaterSensorErrorCallBack, NULL);
    os_timer_arm(&stWaterSensorError, 100, 1);
}

VOID KEY_WaterSensorTimerHandle( VOID *pPara )
{
    float fValue = 0;
    static UINT8 uiPressCount = 0;

    fValue = TEMP_GetTemperature();
    // 缺水时断开, fValue等于-1, 水满时关闭fValue等于66
    if ( fValue < 10 )
    {
        uiPressCount++;

        // 连续30s缺水
        if ( uiPressCount > 30 )
        {
        	//  没有在抽水状态
        	if ( !PLUG_GetRelayStatus() )
			{
        		LOG_OUT(LOGOUT_INFO, "detect water is shortage.");
        		PLUG_SetRelayOn(FALSE);
			}
        	// 在抽水状态。意味着已经开始抽水30s还没抽上来水，可能水井有问题
        	else
        	{
        		// 开关指示灯闪烁提示传感器异常
        		KEY_WaterSensorErrorTimerInit();
        		LOG_OUT(LOGOUT_ERROR, "water sensor abnormal.");

        		PLUG_SetRelayOff(TRUE);
        		xTimerDelete(xWaterSensorTimers, 0);
        	}
        	uiPressCount = 0;
        }
    }
    else
    {
    	uiPressCount = 0;
    }
}

VOID KEY_StartWaterSensorHanderTimer( VOID )
{
    UINT32 uiWaterSensorTimerId = 0;

    xWaterSensorTimers = xTimerCreate("KEY_WaterSensorTimerHandle", 1000/portTICK_RATE_MS, TRUE, &uiWaterSensorTimerId, KEY_WaterSensorTimerHandle);
    if ( !xWaterSensorTimers )
    {
        LOG_OUT(LOGOUT_ERROR, "xTimerCreate KEY_WaterSensorTimerHandle failed.");
    }
    else
    {
        if(xTimerStart(xWaterSensorTimers, 1) != pdPASS)
        {
            LOG_OUT(LOGOUT_ERROR, "xTimerCreate xWaterSensorTimers start failed.");
        }
    }

    KEY_CreateRelayOffTimer();
}

#endif

