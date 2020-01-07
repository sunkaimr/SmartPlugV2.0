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
    float     fCurrent;        //电压(单位A)
    float     fVoltage;        //电流(单位V)
    float    fPower;            //有功功率(单位W)
    float    fApparentPower;    //视在功率(单位W)
    float    fPowerFactor;    //功率因数
    float    fElectricity;    //电量(单位Wh)
    float    fRunTime;        //运行时间(小时h)

    float    fUnderVoltage;        //欠压保护电压
    float    fOverVoltage;        //过压保护电压
    float    fOverCurrent;        //过流保护电压
    float    fOverPower;            //过载保护功率
    float    fUnderPower;        //充电保护，低于干功率时关闭

    BOOL    bUnderVoltageEnable;    //是否开启欠压保护
    BOOL    bOverVoltageEnable;        //是否开启过压保护
    BOOL    bOverCurrentEnable;        //是否开启过流保护
    BOOL    bOverPowerEnable;        //是否开启过功率保护
    BOOL    bUnderPowerEnable;         //是否开启充电保护，手机等设备充满电后自动断电

}METER_MerterInfo;


VOID METER_Init();
UINT METER_MarshalJsonMeter( CHAR* pcBuf, UINT uiBufLen );
VOID METER_RestartHandle( VOID );

VOID METER_DeinitData( VOID );
UINT METER_ReadMeterDataFromFlash( VOID );

UINT METER_ParseMeterData( CHAR* pDataStr);

float METER_GetMeterVoltage( VOID );
float METER_GetMeterCurrent( VOID );
float METER_GetMeterPower( VOID );
float METER_GetMeterApparentPower( VOID );
float METER_GetMeterPowerFactor( VOID );
float METER_GetMeterElectricity( VOID );


#endif /* __USER_METER_H__ */
