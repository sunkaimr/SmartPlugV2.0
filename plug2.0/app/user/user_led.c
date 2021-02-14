/*
 * user_gpio.c
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"

STATIC  os_timer_t stLedLinkTimer;



VOID LED_GpioInit( VOID )
{
    /* wifi状态指示 */
    PIN_FUNC_SELECT(LED_GPIO_WIFI_STATUS_MUX, LED_GPIO_WIFI_STATUS_FUNC);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_WIFI_STATUS_NUM), GPIO_WIFI_STATUS_OFF);

    /* 继电器控制 */
    PIN_FUNC_SELECT(LED_GPIO_RELAY_MUX, LED_GPIO_RELAY_FUNC);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_NUM), GPIO_RELAY_OFF);

    /* 继电器状态指示 */
    PIN_FUNC_SELECT(LED_GPIO_RELAY_STATUS_MUX, LED_GPIO_RELAY_STATUS_FUNC);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_STATUS_NUM), GPIO_RELAY_STATUS_OFF);

    /* 按键输入 */
#if IS_CHANG_XIN
    gpio16_input_conf();
#else
    PIN_FUNC_SELECT(LED_GPIO_KEY_MUX, LED_GPIO_KEY_FUNC);
    GPIO_AS_INPUT(GPIO_ID_PIN(LED_GPIO_KEY_NUM));
#endif
}


VOID LED_WifiStatusOn( VOID )
{
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_WIFI_STATUS_NUM), GPIO_WIFI_STATUS_ON);
}

VOID LED_WifiStatusOff( VOID )
{
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_WIFI_STATUS_NUM), GPIO_WIFI_STATUS_OFF);
}


VOID LED_RelayOn( VOID )
{
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_NUM),         GPIO_RELAY_ON);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_STATUS_NUM),  GPIO_RELAY_STATUS_ON);
}


VOID LED_RelayOff( VOID )
{
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_NUM),         GPIO_RELAY_OFF);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_STATUS_NUM),  GPIO_RELAY_STATUS_OFF);
}


UINT32 LED_GetKeyStatus( VOID )
{
#if IS_CHANG_XIN
    return gpio16_input_get();
#else
    return GPIO_INPUT_GET(GPIO_ID_PIN(LED_GPIO_KEY_NUM));
#endif
}

STATIC VOID USER_LedLinkTimerCallbak( VOID *Para)
{
    STATIC  UINT8 ucLedLinkStatus = 0;

    ucLedLinkStatus ^= 0x01;

    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_WIFI_STATUS_NUM), ucLedLinkStatus);
}

STATIC VOID LED_LedLinkTimerInit(UINT32 uiTime )
{
    os_timer_disarm(&stLedLinkTimer);
    os_timer_setfn(&stLedLinkTimer, (os_timer_func_t *)USER_LedLinkTimerCallbak, NULL);
    os_timer_arm(&stLedLinkTimer, uiTime, 1);
}

VOID LED_SetWifiStatus( LED_WIFI_STATUS_E eStatus )
{
    STATIC UINT uiLastStatus;

    if ( uiLastStatus != eStatus)
    {
        os_timer_disarm(&stLedLinkTimer);

        switch ( eStatus )
        {
            /* wifi连接失败，常灭 */
            case LED_WIFI_STATUS_OFF :
                LED_WifiStatusOff();
                break;

            /* wifi已连接，常亮 */
            case LED_WIFI_STATUS_ON :
            /* 通过按键将wifi模式设置为station模式，常亮  */
            case LED_WIFI_STATUS_SET_STA:
                LED_WifiStatusOn();
                break;

            /* 寻找wifi热点,闪烁间隔2s */
            case LED_WIFI_STATUS_FIND_WIFI :
                LED_LedLinkTimerInit(1000);
                break;

            /* AP模式下等待同步时间,闪烁间隔2s */
            case LED_WIFI_STATUS_SYNC_TIME :
                LED_LedLinkTimerInit(1000);
                break;

            /* 正在连接wifi热点,闪烁间隔1s*/
            case LED_WIFI_STATUS_CONNECTTING :
                LED_LedLinkTimerInit(500);
                break;

            /* 通过按键将wifi模式设置为AP模式，闪烁间隔0.2s */
            case LED_WIFI_STATUS_SET_AP :
                LED_LedLinkTimerInit(100);
                break;

            default:
                break;
        }

        uiLastStatus = eStatus;
    }
}

