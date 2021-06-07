/*
 * user_gpio.h
 *
 *  Created on: 2018��10��21��
 *      Author: lenovo
 */

#ifndef __LED_GPIO_H__
#define __LED_GPIO_H__

#include "user_type.h"


/* �ӿ�˵��

�ܹ����������͵Ĳ�����������ͬIO�ӿڲ�ͬ�����벻ͬ�Ĳ�������ʱ�뽫��Ӧ�ĺ��
��������Ĭ�ϱ�������ƹ̼�������
    IS_PHILIPS        : �����ֵĲ�����װ
    IS_JI_ZHI_YUN     : ������wifiģ��(������)
    IS_CHANG_XIN      : ���¶�ʱ������װ(������ͳ�ƹ���)
    IS_CHANG_XIN_V1   : ���¶�ʱ������װ V1�汾
    IS_WELL           : �Ƿ�����ˮ���Զ���ˮ����

IS_PHILIPS:
	wifi״ָ̬ʾ       	:IO_13   [0:on 1:off]
	�̵�������         	:IO_14   [1:on 0:off]
	�̵���״ָ̬ʾ   	:��
	��������        		:IO_4    [����Ϊ�͵�ƽ]

IS_CHANG_XIN:
	wifi״ָ̬ʾ    	:IO_13     [1:on 0:off]
	�̵�������        	:IO_14     [1:on 0:off]
	�̵���״ָ̬ʾ    	:IO_12     [1:on 0:off]
	��������        		:IO_16     [����Ϊ�͵�ƽ]

IS_CHANG_XIN_V1:
	wifi״ָ̬ʾ    	:IO_13     [1:on 0:off]
	�̵�������        	:IO_14     [1:on 0:off]
	�̵���״ָ̬ʾ    	:IO_12     [1:on 0:off]
	��������        		:IO_4      [����Ϊ�͵�ƽ]

IS_JI_ZHI_YUN:
	wifi״ָ̬ʾ   	:IO_13     [1:on 0:off]
	�̵�������		:��
	�̵���״ָ̬ʾ    	:IO_12     [1:on 0:off]
	�̵�����������    	:IO_4      [����Ϊ�͵�ƽ]
*/

#define IS_PHILIPS      	0
#define IS_CHANG_XIN    	1
#define IS_CHANG_XIN_V1    	0
#define IS_WELL				0

#if IS_PHILIPS
    /* wifi״ָ̬ʾ */
    #define LED_GPIO_WIFI_STATUS_MUX         PERIPHS_IO_MUX_MTCK_U
    #define LED_GPIO_WIFI_STATUS_NUM         13
    #define LED_GPIO_WIFI_STATUS_FUNC        FUNC_GPIO13

    #define GPIO_WIFI_STATUS_ON                (0x00)
    #define GPIO_WIFI_STATUS_OFF            (0x01)

    /* �̵������� */
    #define LED_GPIO_RELAY_MUX             PERIPHS_IO_MUX_MTMS_U
    #define LED_GPIO_RELAY_NUM             14
    #define LED_GPIO_RELAY_FUNC            FUNC_GPIO14

    #define GPIO_RELAY_ON                    (0x01)
    #define GPIO_RELAY_OFF                    (0x00)

    /* �̵���״ָ̬ʾ */
    #define LED_GPIO_RELAY_STATUS_MUX         PERIPHS_IO_MUX_MTDI_U
    #define LED_GPIO_RELAY_STATUS_NUM         12
    #define LED_GPIO_RELAY_STATUS_FUNC        FUNC_GPIO12

    #define GPIO_RELAY_STATUS_ON            (0x01)
    #define GPIO_RELAY_STATUS_OFF            (0x00)

    /* �̵����������� */
    #define LED_GPIO_KEY_MUX                 PERIPHS_IO_MUX_GPIO4_U
    #define LED_GPIO_KEY_NUM                 4
    #define LED_GPIO_KEY_FUNC                FUNC_GPIO4

#elif IS_CHANG_XIN

    /* wifi״ָ̬ʾ */
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

    /* �̵���״ָ̬ʾ */
    #define LED_GPIO_RELAY_STATUS_MUX         PERIPHS_IO_MUX_MTDI_U
    #define LED_GPIO_RELAY_STATUS_NUM         12
    #define LED_GPIO_RELAY_STATUS_FUNC        FUNC_GPIO12

    #define GPIO_RELAY_STATUS_ON             (0x01)
    #define GPIO_RELAY_STATUS_OFF            (0x00)

    /* �̵����������� */
    #define LED_GPIO_KEY_MUX                 PERIPHS_IO_MUX_GPIO4_U
    #define LED_GPIO_KEY_NUM                 4
    #define LED_GPIO_KEY_FUNC                FUNC_GPIO4
#elif IS_CHANG_XIN_V1

    /* wifi״ָ̬ʾ */
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

    /* �̵���״ָ̬ʾ */
    #define LED_GPIO_RELAY_STATUS_MUX         PERIPHS_IO_MUX_MTDI_U
    #define LED_GPIO_RELAY_STATUS_NUM         12
    #define LED_GPIO_RELAY_STATUS_FUNC        FUNC_GPIO12

    #define GPIO_RELAY_STATUS_ON             (0x01)
    #define GPIO_RELAY_STATUS_OFF            (0x00)

    /* �̵����������� */
    #define LED_GPIO_KEY_MUX                 PERIPHS_IO_MUX_GPIO4_U
    #define LED_GPIO_KEY_NUM                 4
    #define LED_GPIO_KEY_FUNC                FUNC_GPIO4

#else

    /* wifi״ָ̬ʾ */
    #define LED_GPIO_WIFI_STATUS_MUX         PERIPHS_IO_MUX_MTCK_U
    #define LED_GPIO_WIFI_STATUS_NUM         13
    #define LED_GPIO_WIFI_STATUS_FUNC        FUNC_GPIO13

    #define GPIO_WIFI_STATUS_ON              (0x01)
    #define GPIO_WIFI_STATUS_OFF             (0x00)

    /* �̵������� */
    #define LED_GPIO_RELAY_MUX               PERIPHS_IO_MUX_MTDO_U
    #define LED_GPIO_RELAY_NUM               15
    #define LED_GPIO_RELAY_FUNC              FUNC_GPIO15

    #define GPIO_RELAY_ON                    (0x01)
    #define GPIO_RELAY_OFF                   (0x00)

    /* �̵���״ָ̬ʾ */
    #define LED_GPIO_RELAY_STATUS_MUX         PERIPHS_IO_MUX_MTDI_U
    #define LED_GPIO_RELAY_STATUS_NUM         12
    #define LED_GPIO_RELAY_STATUS_FUNC        FUNC_GPIO12

    #define GPIO_RELAY_STATUS_ON              (0x01)
    #define GPIO_RELAY_STATUS_OFF             (0x00)

    /* �������� */
    #define LED_GPIO_KEY_MUX                   PERIPHS_IO_MUX_GPIO4_U
    #define LED_GPIO_KEY_NUM                   4
    #define LED_GPIO_KEY_FUNC                  FUNC_GPIO4

#endif


typedef enum tagLED_WIFI_STATUS
{
    LED_WIFI_STATUS_OFF              = 0,     /* wifi����ʧ�ܣ����� */
    LED_WIFI_STATUS_ON               = 1,     /* wifi������/APģʽ��ʱ����ͬ�������� */
    LED_WIFI_STATUS_FIND_WIFI        = 2,     /* Ѱ��wifi�ȵ�,��˸���2s */
    LED_WIFI_STATUS_SYNC_TIME        = 3,     /* APģʽ�µȴ�ͬ��ʱ��,��˸���2s */
    LED_WIFI_STATUS_CONNECTTING      = 4,     /* ��������wifi�ȵ�,��˸���1s*/
    LED_WIFI_STATUS_SET_AP           = 5,     /* ͨ��������wifiģʽ����ΪAPģʽ����˸���0.2s */
    LED_WIFI_STATUS_SET_STA          = 6,     /* ͨ��������wifiģʽ����Ϊstationģʽ������  */

    LED_WIFI_STATUS_BUFF             = 0xFF
}LED_WIFI_STATUS_E;


VOID LED_GpioInit( VOID );
UINT32 LED_GetKeyStatus( VOID );

VOID LED_RelayOn( VOID );
VOID LED_RelayOff( VOID );


#endif /* __LED_GPIO_H__ */
