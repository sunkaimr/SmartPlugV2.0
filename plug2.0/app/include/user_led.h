/*
 * user_gpio.h
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */

#ifndef __LED_GPIO_H__
#define __LED_GPIO_H__

#include "user_type.h"


/* 接口说明

总共有四钟类型的插座，插座不同IO接口不同，编译不同的插座程序时请将对应的宏打开
若都不打开默认编译机智云固件调试用
    IS_PHILIPS        : 飞利浦的插座改装
    IS_JI_ZHI_YUN     : 机智云wifi模块(调试用)
    IS_CHANG_XIN      : 常新定时插座改装(带电量统计功能)
    IS_CHANG_XIN_V1   : 常新定时插座改装 V1版本
    IS_WELL           : 是否启用水井自动上水控制

IS_PHILIPS:
	wifi状态指示       	:IO_13   [0:on 1:off]
	继电器控制         	:IO_14   [1:on 0:off]
	继电器状态指示   	:无
	按键输入        		:IO_4    [按下为低电平]

IS_CHANG_XIN:
	wifi状态指示    	:IO_13     [1:on 0:off]
	继电器控制        	:IO_14     [1:on 0:off]
	继电器状态指示    	:IO_12     [1:on 0:off]
	按键输入        		:IO_16     [按下为低电平]

IS_CHANG_XIN_V1:
	wifi状态指示    	:IO_13     [1:on 0:off]
	继电器控制        	:IO_14     [1:on 0:off]
	继电器状态指示    	:IO_12     [1:on 0:off]
	按键输入        		:IO_4      [按下为低电平]

IS_JI_ZHI_YUN:
	wifi状态指示   	:IO_13     [1:on 0:off]
	继电器控制		:无
	继电器状态指示    	:IO_12     [1:on 0:off]
	继电器按键输入    	:IO_4      [按下为低电平]
*/

#define IS_PHILIPS      	0
#define IS_CHANG_XIN    	0
#define IS_CHANG_XIN_V1    	1
#define IS_WELL				0

#if IS_PHILIPS
    /* wifi状态指示 */
    #define LED_GPIO_WIFI_STATUS_MUX         PERIPHS_IO_MUX_MTCK_U
    #define LED_GPIO_WIFI_STATUS_NUM         13
    #define LED_GPIO_WIFI_STATUS_FUNC        FUNC_GPIO13

    #define GPIO_WIFI_STATUS_ON                (0x00)
    #define GPIO_WIFI_STATUS_OFF            (0x01)

    /* 继电器控制 */
    #define LED_GPIO_RELAY_MUX             PERIPHS_IO_MUX_MTMS_U
    #define LED_GPIO_RELAY_NUM             14
    #define LED_GPIO_RELAY_FUNC            FUNC_GPIO14

    #define GPIO_RELAY_ON                    (0x01)
    #define GPIO_RELAY_OFF                    (0x00)

    /* 继电器状态指示 */
    #define LED_GPIO_RELAY_STATUS_MUX         PERIPHS_IO_MUX_MTDI_U
    #define LED_GPIO_RELAY_STATUS_NUM         12
    #define LED_GPIO_RELAY_STATUS_FUNC        FUNC_GPIO12

    #define GPIO_RELAY_STATUS_ON            (0x01)
    #define GPIO_RELAY_STATUS_OFF            (0x00)

    /* 继电器按键输入 */
    #define LED_GPIO_KEY_MUX                 PERIPHS_IO_MUX_GPIO4_U
    #define LED_GPIO_KEY_NUM                 4
    #define LED_GPIO_KEY_FUNC                FUNC_GPIO4

#elif IS_CHANG_XIN

    /* wifi状态指示 */
    #define LED_GPIO_WIFI_STATUS_MUX         PERIPHS_IO_MUX_MTCK_U
    #define LED_GPIO_WIFI_STATUS_NUM         13
    #define LED_GPIO_WIFI_STATUS_FUNC        FUNC_GPIO13

    #define GPIO_WIFI_STATUS_ON              (0x01)
    #define GPIO_WIFI_STATUS_OFF             (0x00)


    #define LED_GPIO_RELAY_MUX               PERIPHS_IO_MUX_MTMS_U
    #define LED_GPIO_RELAY_NUM               14
    #define LED_GPIO_RELAY_FUNC              FUNC_GPIO14

    #define GPIO_RELAY_ON                    (0x01)
    #define GPIO_RELAY_OFF                   (0x00)

    /* 继电器状态指示 */
    #define LED_GPIO_RELAY_STATUS_MUX         PERIPHS_IO_MUX_MTDI_U
    #define LED_GPIO_RELAY_STATUS_NUM         12
    #define LED_GPIO_RELAY_STATUS_FUNC        FUNC_GPIO12

    #define GPIO_RELAY_STATUS_ON             (0x01)
    #define GPIO_RELAY_STATUS_OFF            (0x00)

    /* 继电器按键输入 */
    #define LED_GPIO_KEY_MUX                 PERIPHS_IO_MUX_GPIO4_U
    #define LED_GPIO_KEY_NUM                 4
    #define LED_GPIO_KEY_FUNC                FUNC_GPIO4
#elif IS_CHANG_XIN_V1

    /* wifi状态指示 */
    #define LED_GPIO_WIFI_STATUS_MUX         PERIPHS_IO_MUX_MTCK_U
    #define LED_GPIO_WIFI_STATUS_NUM         13
    #define LED_GPIO_WIFI_STATUS_FUNC        FUNC_GPIO13

    #define GPIO_WIFI_STATUS_ON              (0x01)
    #define GPIO_WIFI_STATUS_OFF             (0x00)


    #define LED_GPIO_RELAY_MUX               PERIPHS_IO_MUX_MTMS_U
    #define LED_GPIO_RELAY_NUM               14
    #define LED_GPIO_RELAY_FUNC              FUNC_GPIO14

    #define GPIO_RELAY_ON                    (0x01)
    #define GPIO_RELAY_OFF                   (0x00)

    /* 继电器状态指示 */
    #define LED_GPIO_RELAY_STATUS_MUX         PERIPHS_IO_MUX_MTDI_U
    #define LED_GPIO_RELAY_STATUS_NUM         12
    #define LED_GPIO_RELAY_STATUS_FUNC        FUNC_GPIO12

    #define GPIO_RELAY_STATUS_ON             (0x01)
    #define GPIO_RELAY_STATUS_OFF            (0x00)

    /* 继电器按键输入 */
    #define LED_GPIO_KEY_MUX                 PERIPHS_IO_MUX_GPIO4_U
    #define LED_GPIO_KEY_NUM                 4
    #define LED_GPIO_KEY_FUNC                FUNC_GPIO4

#else

    /* wifi状态指示 */
    #define LED_GPIO_WIFI_STATUS_MUX         PERIPHS_IO_MUX_MTCK_U
    #define LED_GPIO_WIFI_STATUS_NUM         13
    #define LED_GPIO_WIFI_STATUS_FUNC        FUNC_GPIO13

    #define GPIO_WIFI_STATUS_ON              (0x01)
    #define GPIO_WIFI_STATUS_OFF             (0x00)

    /* 继电器控制 */
    #define LED_GPIO_RELAY_MUX               PERIPHS_IO_MUX_MTDO_U
    #define LED_GPIO_RELAY_NUM               15
    #define LED_GPIO_RELAY_FUNC              FUNC_GPIO15

    #define GPIO_RELAY_ON                    (0x01)
    #define GPIO_RELAY_OFF                   (0x00)

    /* 继电器状态指示 */
    #define LED_GPIO_RELAY_STATUS_MUX         PERIPHS_IO_MUX_MTDI_U
    #define LED_GPIO_RELAY_STATUS_NUM         12
    #define LED_GPIO_RELAY_STATUS_FUNC        FUNC_GPIO12

    #define GPIO_RELAY_STATUS_ON              (0x01)
    #define GPIO_RELAY_STATUS_OFF             (0x00)

    /* 按键输入 */
    #define LED_GPIO_KEY_MUX                   PERIPHS_IO_MUX_GPIO4_U
    #define LED_GPIO_KEY_NUM                   4
    #define LED_GPIO_KEY_FUNC                  FUNC_GPIO4

#endif


typedef enum tagLED_WIFI_STATUS
{
    LED_WIFI_STATUS_OFF              = 0,     /* wifi连接失败，常灭 */
    LED_WIFI_STATUS_ON               = 1,     /* wifi已连接/AP模式下时间已同步，常亮 */
    LED_WIFI_STATUS_FIND_WIFI        = 2,     /* 寻找wifi热点,闪烁间隔2s */
    LED_WIFI_STATUS_SYNC_TIME        = 3,     /* AP模式下等待同步时间,闪烁间隔2s */
    LED_WIFI_STATUS_CONNECTTING      = 4,     /* 正在连接wifi热点,闪烁间隔1s*/
    LED_WIFI_STATUS_SET_AP           = 5,     /* 通过按键将wifi模式设置为AP模式，闪烁间隔0.2s */
    LED_WIFI_STATUS_SET_STA          = 6,     /* 通过按键将wifi模式设置为station模式，常亮  */

    LED_WIFI_STATUS_BUFF             = 0xFF
}LED_WIFI_STATUS_E;


VOID LED_GpioInit( VOID );
UINT32 LED_GetKeyStatus( VOID );

VOID LED_RelayOn( VOID );
VOID LED_RelayOff( VOID );


#endif /* __LED_GPIO_H__ */
