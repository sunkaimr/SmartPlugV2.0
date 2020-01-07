/*
 * user_temp.c
 *
 *  Created on: 2018楠烇拷11閺堬拷17閺冿拷
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"

#define EXTIINT_NUM        5

COMM_ExtiInt g_astExtiIntBuf[EXTIINT_NUM];

UINT COMM_ExtiIntRegister(UINT uiNum, fn fInit, fn fDeInit, fn fHandle, CHAR *pcName)
{
    UINT8 i = 0;

    //先查找是否已经注册过，若已经注册过则更新
    for ( i = 0; i < EXTIINT_NUM; i++ )
    {
        if ( uiNum == g_astExtiIntBuf[i].uiGPIO )
        {
            if ( g_astExtiIntBuf[i].fDeInit != NULL )
            {
                g_astExtiIntBuf[i].fDeInit();
            }

            GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, uiNum);

            g_astExtiIntBuf[i].fInit = fInit;
            g_astExtiIntBuf[i].fDeInit = fDeInit;
            g_astExtiIntBuf[i].fHandle = fHandle;
            g_astExtiIntBuf[i].uiGPIO = uiNum;

            if ( pcName != NULL )
            {
                strncpy(g_astExtiIntBuf[i].szHandleName, pcName, EXTIINT_NAME_LEN-1);
                LOG_OUT(LOGOUT_INFO, "GPIO(0x%X) has Reregisted with handler: %s", uiNum, pcName);
            }
            else
            {
                LOG_OUT(LOGOUT_INFO, "GPIO(0x%X) has Reregisted", uiNum);
            }

            //执行初始化函数
            fInit();
            return OK;
        }
    }

    //若未注册过则找空余写入
    for ( i = 0; i < EXTIINT_NUM; i++ )
    {
        if ( NULL == g_astExtiIntBuf[i].fHandle )
        {
            GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, uiNum);

            g_astExtiIntBuf[i].fInit = fInit;
            g_astExtiIntBuf[i].fDeInit = fDeInit;
            g_astExtiIntBuf[i].fHandle = fHandle;
            g_astExtiIntBuf[i].uiGPIO = uiNum;

            if ( pcName != NULL )
            {
                strncpy(g_astExtiIntBuf[i].szHandleName, pcName, EXTIINT_NAME_LEN-1);
                LOG_OUT(LOGOUT_INFO, "GPIO(0x%X) has Registed with handler: %s", uiNum, pcName);
            }
            else
            {
                LOG_OUT(LOGOUT_INFO, "GPIO(0x%X) has Registed", uiNum);
            }

            //执行初始化函数
            fInit();
            return OK;
        }
    }

    LOG_OUT(LOGOUT_ERROR, "g_astExtiIntBuf upper limit(%d)", EXTIINT_NUM);
    return FAIL;
}

UINT COMM_ExtiIntUnregister(UINT uiNum)
{
    UINT8 i = 0;

    for ( i = 0; i < EXTIINT_NUM; i++ )
    {
        if ( uiNum == g_astExtiIntBuf[i].uiGPIO )
        {
            if ( g_astExtiIntBuf[i].fDeInit != NULL )
            {
                g_astExtiIntBuf[i].fDeInit();
            }

            GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, g_astExtiIntBuf[i].uiGPIO);

            LOG_OUT(LOGOUT_INFO, "GPIO(0x%X) has Unregisted", g_astExtiIntBuf[i].uiGPIO);

            g_astExtiIntBuf[i].fInit = NULL;
            g_astExtiIntBuf[i].fDeInit = NULL;
            g_astExtiIntBuf[i].fHandle = NULL;
            g_astExtiIntBuf[i].uiGPIO = 0;
            return OK;
        }
    }

    LOG_OUT(LOGOUT_ERROR, "gpio (0x%X) not registered", uiNum);
    return OK;
}

// 中断处理函数，这里边的处理要尽可能的简洁，不要打印日志避免占用过多时间
VOID COMM_ExtiIntHandle(VOID)
{
    UINT32 uiGPIOReg = 0;
    UINT8 i = 0;

    _xt_isr_mask(1 << ETS_GPIO_INUM);

    uiGPIOReg = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

    for ( i = 0; i < EXTIINT_NUM; i++ )
    {
        if ( NULL != g_astExtiIntBuf[i].fHandle && (g_astExtiIntBuf[i].uiGPIO & uiGPIOReg))
        {
            //LOG_OUT(LOGOUT_DEBUG, "GPIO(0x%X) Exti Interrupt Handle", uiGPIOReg);
            g_astExtiIntBuf[i].fHandle();
            break;
        }
    }

    if ( i >= EXTIINT_NUM )
    {
        LOG_OUT(LOGOUT_ERROR, "unknown gpio interrupt uiGPIOReg:0x%X", uiGPIOReg);
    }

    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS,  uiGPIOReg);
    _xt_isr_unmask(1 << ETS_GPIO_INUM);
}

VOID COMM_ExtiIntInit(VOID)
{
    UINT8 i = 0;

    for ( i = 0; i < EXTIINT_NUM; i++ )
    {
        if ( NULL == g_astExtiIntBuf[i].fHandle )
        {
            g_astExtiIntBuf[i].fInit = NULL;
            g_astExtiIntBuf[i].fDeInit = NULL;
            g_astExtiIntBuf[i].fHandle = NULL;
            g_astExtiIntBuf[i].uiGPIO = 0;
        }
    }

    gpio_intr_handler_register(COMM_ExtiIntHandle, NULL);

    _xt_isr_unmask(1 << ETS_GPIO_INUM);
}





