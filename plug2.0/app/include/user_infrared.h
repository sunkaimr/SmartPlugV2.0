/*
 * user_infrared.h
 *
 *  Created on: 2018年11月17日
 *      Author: lenovo
 */

#ifndef __USER_INFRARE_H__
#define __USER_INFRARE_H__

#define INFRARED_MAX            10
#define INFRARED_NAME_MAX_LEN    32

#define INFRARED_ALL            (INFRARED_MAX + 1)

typedef struct tagINFRARED_VALUE                            /* 红外设置  */
{
    UINT                uiNum;                            /* 定时器编号 */
    CHAR                szName[INFRARED_NAME_MAX_LEN];    /* 名字         */
    BOOL                bEnable;                        /* 使能         */
    UINT                uiOnValue;                        /* 开启码值     */
    UINT                uiOffValue;                        /* 关闭码值     */
}INFRARED_VALUE_S;

typedef struct tagINFRARED_Set
{
    BOOL                bIsSetting;                    /* 是set还是get    */
    BOOL                bIsRefresh;                    /* 是否获取到新的值     */
    UINT                uiValue;                    /* 码值             */
}INFRARED_SET_S;

extern INFRARED_SET_S g_stinfrared_Set;

extern VOID INFRARED_InfraredInit( VOID );
extern VOID INFRARED_JudgeInfrared( VOID );

INFRARED_VALUE_S* INFRARED_GetInfraredData( UINT8 ucNum );
UINT32 INFRARED_GetInfraredDataSize();
UINT INFRARED_SaveInfraredData( INFRARED_VALUE_S* pstData );
UINT INFRARED_SetInfrared( UINT8 ucNum, UINT8 ucSwitch, UINT uiTimeOut_s );
VOID INFRARED_InfraredDataDeInit( VOID );


VOID INFRA_InfraredInit( VOID );

#endif /* __USER_INFRARE_H__ */
