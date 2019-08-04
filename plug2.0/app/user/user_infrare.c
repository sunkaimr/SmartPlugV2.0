/*
 * user_infrare.c
 *
 *  Created on: 2018年11月17日
 *      Author: lenovo
 */

#include "esp_common.h"
#include "user_common.h"

#define INFRA_GPIO_NUM			GPIO_Pin_14

UINT32 uiInfreadValue = 0;


VOID INFRA_InfrareHandle( VOID* Para )
{
	static UINT32 infreadValue = 0;	/* 解码的红外值 */
	static UINT32 uiCurTime = 0;
	static UINT32 uiLastTime = 0;
	static UINT8  Step = 0;
	static UINT uiPluse = 0;
	static uint8_t  BitCount = 0;   	/* BitCount等于32时解码完成 */

	//GPIO_INPUT_GET( GPIO_ID_PIN(INFRA_GPIO_NUM) );
	_xt_isr_mask(1 << ETS_GPIO_INUM);

	uiCurTime = system_get_time();

	/* 计算2次下降沿脉宽时间 */
	if ( uiCurTime > uiLastTime )
	{
		uiPluse = uiCurTime - uiLastTime;
	}
	else
	{
		uiPluse = 0xFFFFFFFF + uiCurTime - uiLastTime;
	}

	uiLastTime = uiCurTime;

    switch ( Step )
    {
        case 0 :
            /* 引导码高电平9ms，低电平4.5ms  判断是否引导成功 */
            if ( 13000 < uiPluse && 14000 > uiPluse )
            {
                Step = 1;
                BitCount = 0;
            }
            else
            {
                Step = 0;
            }
            break;
        case 1 :
			/* 开始解码 */
			infreadValue <<= 1;
            BitCount ++;
            /* 数据1 用“高电平0.56ms＋低电平1.69ms=2.25ms”表示 */
            if ( 2000 < uiPluse && 2500 > uiPluse )
            {
            	/* 此位为1 */
                infreadValue |= 0x01;
            }
            /* 发射数据时0 用“0.56ms 高电平＋0.565ms 低电平=1.125ms”表示 */
            else if ( 1000 < uiPluse && 1300 > uiPluse )
            {
            	/* 此位为0 */
            }
            else
            {
            	/* 不能被识别，重置初始条件 */
                BitCount = 0;
                Step = 0;
            }

			/* 解码完成 */
            if ( 32 <= BitCount )
            {
                Step = 0;
                uiInfreadValue = infreadValue;
                LOG_OUT(LOGOUT_INFO, "uiInfreadValue:%x", uiInfreadValue);
            }
            break;
        default:
            break;
    }

    GPIO_REG_WRITE( GPIO_STATUS_W1TC_ADDRESS, INFRA_GPIO_NUM );
    _xt_isr_unmask(1 << ETS_GPIO_INUM);

}


void INFRA_InfrareInit(void)
{
	GPIO_ConfigTypeDef gpio_in_cfg;

	gpio_in_cfg.GPIO_IntrType 	= GPIO_PIN_INTR_NEGEDGE;
	gpio_in_cfg.GPIO_Mode 		= GPIO_Mode_Input;
	gpio_in_cfg.GPIO_Pullup 	= GPIO_PullUp_EN;
	gpio_in_cfg.GPIO_Pin 		= INFRA_GPIO_NUM;
	gpio_config(&gpio_in_cfg);

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, INFRA_GPIO_NUM);

	gpio_intr_handler_register(INFRA_InfrareHandle, NULL);

	_xt_isr_unmask(1 << ETS_GPIO_INUM);

	LOG_OUT(LOGOUT_INFO, "INFRA_InfrareInit success");
}







