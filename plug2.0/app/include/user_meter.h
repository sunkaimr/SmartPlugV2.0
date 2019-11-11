/*
 * user_meter.h
 *
 *  Created on: 2019年11月17日
 *      Author: lenovo
 */

#ifndef __USER_METER_H__
#define __USER_METER_H__


typedef struct tagMeter
{
	float 	fCurrent;		//电压(单位A)
	float 	fVoltage;		//电流(单位V)
	float	fPower;			//有功功率(单位W)
	float	fApparentPower;	//视在功率
	float	fPowerFactor;	//功率因数
	float	fElectricity;	//电量(单位W)
	float	fRunTime;		//运行时间

}METER_MerterInfo;


VOID METER_Init();
UINT METER_MarshalJsonMeter( CHAR* pcBuf, UINT uiBufLen );
VOID METER_RestartHandle( VOID );

VOID METER_DeinitData( VOID );
UINT METER_GetMeterData( VOID );

UINT METER_ParseMeterData( CHAR* pDataStr);

#endif /* __USER_METER_H__ */
