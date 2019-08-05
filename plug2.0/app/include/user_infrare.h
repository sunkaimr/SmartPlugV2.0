/*
 * user_infrare.h
 *
 *  Created on: 2018年11月17日
 *      Author: lenovo
 */

#ifndef __USER_INFRARE_H__
#define __USER_INFRARE_H__

#define INFRAED_MAX    		10
#define INFRAED_NAME_MAX_LEN	32

#define INFRAED_ALL    		(INFRAED_MAX + 1)

typedef struct tagINFRAED_VALUE							/* 红外设置  */
{
	UINT				uiNum;							/* 定时器编号 */
	CHAR    			szName[INFRAED_NAME_MAX_LEN];	/* 名字 		*/
	BOOL				bEnable;						/* 使能 		*/
	UINT				uiOnValue;						/* 开启码值 	*/
	UINT				uiOffValue;						/* 关闭码值 	*/
}INFRAED_VALUE_S;

typedef struct tagINFRAED_Set
{
	BOOL				bIsSetting;					/* 是set还是get	*/
	BOOL				bIsRefresh;					/* 是否获取到新的值 	*/
	UINT				uiValue;					/* 码值 			*/
}INFRAED_SET_S;

extern INFRAED_SET_S g_stINFRAED_Set;

extern VOID INFRAED_InfraedInit( VOID );
extern VOID INFRAED_JudgeInfraed( VOID );

INFRAED_VALUE_S* INFRAED_GetInfraedData( UINT8 ucNum );
UINT32 INFRAED_GetInfraedDataSize();
UINT INFRARED_SaveInfraredData( INFRAED_VALUE_S* pstData );
UINT INFRAED_SetInfraed( UINT8 ucNum, UINT8 ucSwitch, UINT uiTimeOut_s );
VOID INFRARED_InfraredDataDeInit( VOID );


VOID INFRA_InfrareInit( VOID );

#endif /* __USER_INFRARE_H__ */
