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

INFRAED_SET_S g_stINFRAED_Set = { FALSE, FALSE, 0};
INFRAED_VALUE_S g_astINFRAED_Value[INFRAED_MAX];

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
                g_stINFRAED_Set.uiValue = infreadValue;
                g_stINFRAED_Set.bIsRefresh = TRUE;
                //LOG_OUT(LOGOUT_DEBUG, "infreadValue:%X", infreadValue);
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
	GPIO_ConfigTypeDef stGpioCfg;

	stGpioCfg.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
	stGpioCfg.GPIO_Mode 	= GPIO_Mode_Input;
	stGpioCfg.GPIO_Pullup 	= GPIO_PullUp_EN;
	stGpioCfg.GPIO_Pin 		= INFRA_GPIO_NUM;
	gpio_config( &stGpioCfg );

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, INFRA_GPIO_NUM);

	gpio_intr_handler_register(INFRA_InfrareHandle, NULL);

	_xt_isr_unmask(1 << ETS_GPIO_INUM);

	LOG_OUT(LOGOUT_INFO, "INFRA_InfrareInit success");
}

VOID INFRAED_JudgeInfraed( VOID )
{
	UINT uiLoopi = 0;
	INFRAED_VALUE_S *pstInfraed = NULL;

	/* 获取到新的红外键值 */
	if ( g_stINFRAED_Set.bIsRefresh && !g_stINFRAED_Set.bIsSetting )
	{
		g_stINFRAED_Set.bIsRefresh = FALSE;

		pstInfraed = g_astINFRAED_Value;
		/* 逐个比较键值看与设定的键值是否匹配 */
		for ( uiLoopi = 0 ; uiLoopi < INFRAED_MAX ; uiLoopi++, pstInfraed++ )
		{
			/* 如果on和off值一样则继电器交替动作 */
			if ( g_stINFRAED_Set.uiValue == pstInfraed->uiOnValue &&
				pstInfraed->uiOnValue == pstInfraed->uiOffValue)
			{
				LOG_OUT(LOGOUT_DEBUG, "Recv infread:%X", g_stINFRAED_Set.uiValue);
				PLUG_SetRelayReversal( TRUE );
				break;
			}
			/* on操作 */
			else if ( g_stINFRAED_Set.uiValue == pstInfraed->uiOnValue )
			{
				LOG_OUT(LOGOUT_DEBUG, "Recv infread:%X", g_stINFRAED_Set.uiValue);
				PLUG_SetRelayOn( TRUE );
				break;
			}
			/* off操作 */
			else if ( g_stINFRAED_Set.uiValue == pstInfraed->uiOffValue )
			{
				LOG_OUT(LOGOUT_DEBUG, "Recv infread:%X", g_stINFRAED_Set.uiValue);
				PLUG_SetRelayOff( TRUE );
				break;
			}
		}
	}
}

UINT INFRAED_SetInfraed( UINT8 ucNum, UINT8 ucSwitch, UINT uiTimeOut_s )
{
	INFRAED_VALUE_S* pstData = NULL;
	UINT uiCount = 0;
	UINT uiRet = 0;

	if ( ucNum > INFRAED_MAX )
	{
	    LOG_OUT(LOGOUT_ERROR, "Invalid infraed num: %d", ucNum);
		return FAIL;
	}
	g_stINFRAED_Set.bIsSetting = TRUE;


	while ( !g_stINFRAED_Set.bIsRefresh && uiCount < uiTimeOut_s*10 )
	{
		//LOG_OUT(LOGOUT_DEBUG, "waitting for set Infraed, %d times", uiCount);
		uiCount++;
		vTaskDelay( 100/portTICK_RATE_MS );
	}

	if ( g_stINFRAED_Set.bIsRefresh )
	{
		pstData = INFRAED_GetInfraedData( ucNum-1 );

		if ( ucSwitch )
		{
			pstData->uiOnValue = g_stINFRAED_Set.uiValue;
		}
		else
		{
			pstData->uiOffValue = g_stINFRAED_Set.uiValue;
		}

		g_stINFRAED_Set.bIsRefresh = FALSE;
		g_stINFRAED_Set.bIsSetting = FALSE;
		uiRet = INFRARED_SaveInfraredData(pstData);
		if ( uiRet != OK )
		{
			LOG_OUT(LOGOUT_ERROR, "SaveInfraredData failed");
			return FAIL;
		}

		return OK;
	}

	LOG_OUT(LOGOUT_ERROR, "Set infrared timeout %d S", uiTimeOut_s);

	g_stINFRAED_Set.bIsSetting = FALSE;
	return FAIL;
}

VOID INFRARED_InfraredDataDeInit( VOID )
{
	UINT uiLoopi = 0;
	INFRAED_VALUE_S *pstData = NULL;
	CHAR szName[PLUG_NAME_MAX_LEN] = "";

	for ( uiLoopi = 0; uiLoopi < INFRAED_MAX; uiLoopi++ )
	{
		pstData = &g_astINFRAED_Value[uiLoopi];

		pstData->uiNum		= uiLoopi+1;
		pstData->bEnable	= FALSE;
		pstData->uiOnValue	= 0;
		pstData->uiOffValue = 0;

		snprintf(szName, sizeof(szName), "infrared %d", uiLoopi+1);
		strcpy(pstData->szName, szName);
	}
}


INFRAED_VALUE_S* INFRAED_GetInfraedData( UINT8 ucNum )
{
	if ( ucNum >= INFRAED_MAX )
	{
		return &g_astINFRAED_Value[0];
	}
	return &g_astINFRAED_Value[ucNum];
}

UINT32 INFRAED_GetInfraedDataSize()
{
	return sizeof(g_astINFRAED_Value);
}

UINT INFRARED_SaveInfraredData( INFRAED_VALUE_S* pstData )
{
	UINT uiRet = OK;

	if ( NULL == pstData )
	{
	    LOG_OUT(LOGOUT_ERROR, "pstData = 0x%p", pstData);
		return FAIL;
	}

	uiRet = CONFIG_InfraedDataCheck( pstData );
	if ( uiRet != OK )
	{
	    LOG_OUT(LOGOUT_ERROR, "pstData check failed.");
		return FAIL;
	}

	memcpy(INFRAED_GetInfraedData(pstData->uiNum - 1), pstData, sizeof(INFRAED_VALUE_S));
	CONFIG_SaveConfig(PLUG_MOUDLE_INFRAED);
	return OK;
}



