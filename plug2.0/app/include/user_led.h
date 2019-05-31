/*
 * user_gpio.h
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */

#ifndef __LED_GPIO_H__
#define __LED_GPIO_H__

#include "user_type.h"


typedef enum tagLED_WIFI_STATUS
{
	LED_WIFI_STATUS_OFF				= 0,	/* wifi连接失败，常灭 */
	LED_WIFI_STATUS_ON				= 1, 	/* wifi已连接/AP模式下时间已同步，常亮 */
	LED_WIFI_STATUS_FIND_WIFI		= 2, 	/* 寻找wifi热点,闪烁间隔2s */
	LED_WIFI_STATUS_SYNC_TIME		= 3, 	/* AP模式下等待同步时间,闪烁间隔2s */
	LED_WIFI_STATUS_CONNECTTING		= 4, 	/* 正在连接wifi热点,闪烁间隔1s*/
	LED_WIFI_STATUS_SET_AP			= 5,    /* 通过按键将wifi模式设置为AP模式，闪烁间隔0.2s */
	LED_WIFI_STATUS_SET_STA			= 6,    /* 通过按键将wifi模式设置为station模式，常亮  */


	LED_WIFI_STATUS_BUFF          	= 0xFF
}LED_WIFI_STATUS_E;


VOID LED_GpioInit( VOID );
UINT32 LED_GetKeyStatus( VOID );

VOID LED_RelayOn( VOID );
VOID LED_RelayOff( VOID );


#endif /* __LED_GPIO_H__ */
