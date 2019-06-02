/*
 * user_gpio.c
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"


/* 接口说明

总共有四钟类型的插座，插座不同IO接口不同，编译不同的插座程序时请将对应的宏打开
若都不打开默认编译机智云固件调试用
	IS_PHILIPS		: 飞利浦的插座改装
	IS_CHANG_XIN	: 常新定时插座改装
	IS_JI_ZHI_YUN	: 机智云wifi模块(调试用)

IS_PHILIPS:
	wifi状态指示	:IO_13	 【0:on 1:off】
	继电器控制		:IO_14	 【1:on 0:off】
	继电器状态指示	:无
	按键输入		:IO_4 	 【按下为低电平】

IS_CHANG_XIN:
	wifi状态指示	:IO_13	 【1:on 0:off】
	继电器控制		:IO_14	 【1:on 0:off】
	继电器状态指示	:IO_12	 【1:on 0:off】
	按键输入		:IO_4    【按下为低电平】

IS_JI_ZHI_YUN:
	wifi状态指示	:IO_13	 【1:on 0:off】
	继电器控制		:无
	继电器状态指示	:IO_12	 【1:on 0:off】
	继电器按键输入	:IO_4    【按下为低电平】
*/

#define IS_PHILIPS     	0
#define IS_CHANG_XIN    0


#if IS_PHILIPS
	/* wifi状态指示 */
	#define LED_GPIO_WIFI_STATUS_MUX     	PERIPHS_IO_MUX_MTCK_U
	#define LED_GPIO_WIFI_STATUS_NUM     	13
	#define LED_GPIO_WIFI_STATUS_FUNC    	FUNC_GPIO13

	#define GPIO_WIFI_STATUS_ON				(0x00)
	#define GPIO_WIFI_STATUS_OFF			(0x01)

	/* 继电器控制 */
	#define LED_GPIO_RELAY_MUX     		PERIPHS_IO_MUX_MTMS_U
	#define LED_GPIO_RELAY_NUM     		14
	#define LED_GPIO_RELAY_FUNC    		FUNC_GPIO14

	#define GPIO_RELAY_ON					(0x01)
	#define GPIO_RELAY_OFF					(0x00)

	/* 继电器状态指示 */
	#define LED_GPIO_RELAY_STATUS_MUX     	PERIPHS_IO_MUX_MTDI_U
	#define LED_GPIO_RELAY_STATUS_NUM     	12
	#define LED_GPIO_RELAY_STATUS_FUNC    	FUNC_GPIO12

	#define GPIO_RELAY_STATUS_ON			(0x01)
	#define GPIO_RELAY_STATUS_OFF			(0x00)

	/* 继电器按键输入 */
	#define LED_GPIO_KEY_MUX     			PERIPHS_IO_MUX_GPIO4_U
	#define LED_GPIO_KEY_NUM     			4
	#define LED_GPIO_KEY_FUNC    			FUNC_GPIO4

#elif IS_CHANG_XIN

	/* wifi状态指示 */
	#define LED_GPIO_WIFI_STATUS_MUX     	PERIPHS_IO_MUX_MTCK_U
	#define LED_GPIO_WIFI_STATUS_NUM     	13
	#define LED_GPIO_WIFI_STATUS_FUNC    	FUNC_GPIO13

	#define GPIO_WIFI_STATUS_ON				(0x01)
	#define GPIO_WIFI_STATUS_OFF			(0x00)

	/* 继电器控制 */
	#define LED_GPIO_RELAY_MUX     			PERIPHS_IO_MUX_MTMS_U
	#define LED_GPIO_RELAY_NUM     			14
	#define LED_GPIO_RELAY_FUNC    			FUNC_GPIO14

	#define GPIO_RELAY_ON					(0x01)
	#define GPIO_RELAY_OFF					(0x00)

	/* 继电器状态指示 */
	#define LED_GPIO_RELAY_STATUS_MUX     	PERIPHS_IO_MUX_MTDI_U
	#define LED_GPIO_RELAY_STATUS_NUM     	12
	#define LED_GPIO_RELAY_STATUS_FUNC    	FUNC_GPIO12

	#define GPIO_RELAY_STATUS_ON			(0x01)
	#define GPIO_RELAY_STATUS_OFF			(0x00)

	/* 继电器按键输入 */
	#define LED_GPIO_KEY_MUX     			PERIPHS_IO_MUX_GPIO4_U
	#define LED_GPIO_KEY_NUM     			4
	#define LED_GPIO_KEY_FUNC    			FUNC_GPIO4

#else

	/* wifi状态指示 */
	#define LED_GPIO_WIFI_STATUS_MUX     	PERIPHS_IO_MUX_MTCK_U
	#define LED_GPIO_WIFI_STATUS_NUM     	13
	#define LED_GPIO_WIFI_STATUS_FUNC   	FUNC_GPIO13

	#define GPIO_WIFI_STATUS_ON				(0x01)
	#define GPIO_WIFI_STATUS_OFF			(0x00)

	/* 继电器控制 */
	#define LED_GPIO_RELAY_MUX     			PERIPHS_IO_MUX_MTDO_U
	#define LED_GPIO_RELAY_NUM     			15
	#define LED_GPIO_RELAY_FUNC    			FUNC_GPIO15

	#define GPIO_RELAY_ON					(0x01)
	#define GPIO_RELAY_OFF					(0x00)

	/* 继电器状态指示 */
	#define LED_GPIO_RELAY_STATUS_MUX     	PERIPHS_IO_MUX_MTDI_U
	#define LED_GPIO_RELAY_STATUS_NUM     	12
	#define LED_GPIO_RELAY_STATUS_FUNC    	FUNC_GPIO12

	#define GPIO_RELAY_STATUS_ON			(0x01)
	#define GPIO_RELAY_STATUS_OFF			(0x00)

	/* 按键输入 */
	#define LED_GPIO_KEY_MUX     			PERIPHS_IO_MUX_GPIO4_U
	#define LED_GPIO_KEY_NUM     			4
	#define LED_GPIO_KEY_FUNC    			FUNC_GPIO4

#endif



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
	PIN_FUNC_SELECT(LED_GPIO_KEY_MUX, LED_GPIO_KEY_FUNC);
	GPIO_AS_INPUT(GPIO_ID_PIN(LED_GPIO_KEY_NUM));
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
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_NUM), 		GPIO_RELAY_ON);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_STATUS_NUM), GPIO_RELAY_STATUS_ON);
}


VOID LED_RelayOff( VOID )
{
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_NUM), 		GPIO_RELAY_OFF);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_GPIO_RELAY_STATUS_NUM), GPIO_RELAY_STATUS_OFF);
}


UINT32 LED_GetKeyStatus( VOID )
{
	return GPIO_INPUT_GET(GPIO_ID_PIN(LED_GPIO_KEY_NUM));
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

