/*
 * user_plug.h
 *
 *  Created on: 2018年11月2日
 *      Author: lenovo
 */

#ifndef __USER_PLUG_H__
#define __USER_PLUG_H__



#define PLUG_NAME_MAX_LEN         32
#define PLUG_WIFI_SSID_LEN         32
#define PLUG_WIFI_PASSWD_LEN     64

#define PLUG_MQTT_PRODUCTKEY_LEN     20
#define PLUG_MQTT_DEVNAME_LEN         32
#define PLUG_MQTT_DEVSECRET_LEN     64

#define PLUG_BIGIOT_DEVID_LEN         64
#define PLUG_BIGIOT_APIKEY_LEN         20
#define PLUG_BIGIOT_IFID_LEN         20

#define PLUG_TIMER_MAX            10
#define PLUG_DELAY_MAX            10

#define PLUG_TIMER_ALL            (PLUG_TIMER_MAX + 1)
#define PLUG_DELAY_ALL            (PLUG_DELAY_MAX + 1)

#define PLUG_WEBSET_LEN         32

#define PLUG_NAME      "SmartPlug"



typedef struct tagPLUG_DATE
{
    INT32     iYear;                        /*  年    */
    INT8     iMonth;                        /*  月    */
    INT8     iDay;                        /*  日    */
    INT8    iWeek;                        /*  星期  */
    INT8     iHour;                        /*  时    */
    INT8     iMinute;                    /*  分    */
    INT8     iSecond;                    /*  秒    */

}PLUG_DATE_S;

typedef struct tagPLUG_TIME_POINT
{
    INT8     iDay;                        /*  第二天    */
    INT8     iHour;                        /*  时    */
    INT8     iMinute;                    /*  分    */

}PLUG_TIME_POINT_S;


typedef enum
{
    REPET_ONCE        = 0x00,        /* 执行一次 */
    REPET_MON        = 0x01,        /* 每周一 */
    REPET_TUE        = 0x02,        /* 每周二 */
    REPET_WED        = 0x04,        /* 每周三 */
    REPET_THU        = 0x08,        /* 每周四 */
    REPET_FRI        = 0x10,        /* 每周五 */
    REPET_SAT        = 0x20,        /* 每周六 */
    REPET_SUN        = 0x40,        /* 每周日 */
    REPET_ALL        = 0x7F,        /* 每天 */

    REPET_BUFF
}PLUG_REPETITION_E;


typedef struct tagPLUG_TIMER                            /*  定时模块    */
{
    UINT                uiNum;                            /* 定时器编号 */
    CHAR                szName[PLUG_NAME_MAX_LEN];        /* 名字         */
    BOOL                bEnable;                        /* 使能         */
    BOOL                bOnEnable;                        /* 开启时间使能    */
    BOOL                bOffEnable;                        /* 关闭时间使能    */
    PLUG_TIME_POINT_S     stOnTime;                        /* 开启时间        */
    PLUG_TIME_POINT_S     stOffTime;                        /* 关闭时间        */
    PLUG_REPETITION_E    eWeek;                            /* 重复设置        */
    BOOL                bCascode;                        /* 级联使能     */
    UINT                uiCascodeNum;                    /* 与那个延时级联 */

}PLUG_TIMER_S;



typedef struct tagPLUG_DELAY                            /*  延时模块    */
{
    UINT                uiNum;                            /* 延时编号 */
    CHAR                szName[PLUG_NAME_MAX_LEN];        /* 名字         */
    BOOL                bEnable;                        /* 使能         */
    BOOL                bOnEnable;                        /* 开启时间使能    */
    BOOL                bOffEnable;                        /* 关闭时间使能    */
    PLUG_TIME_POINT_S    stOnInterval;                    /* 开启时间间隔 */
    PLUG_TIME_POINT_S    stOffInterval;                    /* 关闭时间间隔 */
    UINT                uiCycleTimes;                    /* 循环次数     */
    UINT                uiTmpCycleTimes;                /* 存放临时循环次数     */
    BOOL                bCascode;                        /* 级联使能     */
    UINT                uiCascodeNum;                    /* 与那个延时级联 */
    UINT8                ucSwFlag;                        /* 当前需要计算开启时间还是关闭时间，2:off,1:on,0:都不 */
    PLUG_TIME_POINT_S stTimePoint;                        /* 开启还是关闭时间点 */

}PLUG_DELAY_S;

typedef enum
{
    PWUP_LAST        = 0x00,        /* 恢复下电前的状态 */
    PWUP_OFF        = 0x01,        /* 上电默认开启关闭 */
    PWUP_ON        = 0x02,        /* 上电默认开启开启 */

    PWUP_BUFF
}PLUG_RELAY_PWUP_E;


typedef struct tagPLUG_SYSSET                            /*  系统模块    */
{
    BOOL                 bRelayStatus;                                /*  继电器状态    */
    BOOL                 bSmartConfigFlag;                            /*  smart config是否配置    */
    UINT8                 ucWifiMode;                                    /* esp8266工作模式  1:station 2:ap */
    PLUG_RELAY_PWUP_E     eRelayPowerUp;                                /* 继电器上电状态 */
    CHAR                 szPlugName[PLUG_NAME_MAX_LEN+1];            /* hostname  */
    CHAR                szWifiSSID[PLUG_WIFI_SSID_LEN+1];            /* wifi名称 */
    CHAR                szWifiPasswd[PLUG_WIFI_PASSWD_LEN+1];        /* wifi密码 */

}PLUG_SYSSET_S;

typedef struct tagPLUG_WebSet                            /*  web的相关设置    */
{
    CHAR    szModelTab[PLUG_WEBSET_LEN+1];                /* tab标签 */
    CHAR    szMeterRefresh[PLUG_WEBSET_LEN+1];        /* meter刷新间隔 */

}PLUG_WEBSET_S;


typedef enum
{
    PLATFORM_NONE        = 0,        /* 不对接 */
    PLATFORM_ALIYUN        = 1,        /* 对接阿里云 */
    PLATFORM_BIGIOT        = 2,        /* 对接贝壳物联 */

    PLATFORM_BUFF
}PLATFORM_E;

typedef enum
{
    DEVTYPE_other = 0,            //0     默认设备
    DEVTYPE_TV,                    //1     电视
    DEVTYPE_lamp,                //2     灯
    DEVTYPE_air_conditioner,    //3     空调
    DEVTYPE_Air_purifier,         //4     空气净化器
    DEVTYPE_socket,                //5     插座
    DEVTYPE_switch,                //6     开关
    DEVTYPE_roomba,                //7     扫地机器人
    DEVTYPE_curtain,            //8     窗帘
    DEVTYPE_humidifier,            //9     加湿器
    DEVTYPE_fan,                //10    风扇
    DEVTYPE_milk_warmers,        //11    暖奶器
    DEVTYPE_soybean_milk,        //12    豆浆机
    DEVTYPE_electric_kettle,    //13    电热水壶
    DEVTYPE_water_dispenser,    //14    饮水机
    DEVTYPE_camera,                //15    摄像头
    DEVTYPE_router,                //16    路由器
    DEVTYPE_cooker,                //17    电饭煲
    DEVTYPE_water_heater,        //18    热水器
    DEVTYPE_oven,                //19    烤箱
    DEVTYPE_water_purifier,        //20    净水器
    DEVTYPE_refrigerator,        //21    冰箱
    DEVTYPE_settop_box,          //22    机顶盒
    DEVTYPE_sensor,                //23    传感器
    DEVTYPE_washing,            //24    洗衣机
    DEVTYPE_smart_bed,            //25    智能床
    DEVTYPE_Aromatherapy,       //26    香薰机
    DEVTYPE_window,                //27    窗
    DEVTYPE_smoke_lampblack,    //28    抽油烟机
    DEVTYPE_fingerprint_lock,    //29    指纹锁
    DEVTYPE_remote_control,        //30    万能遥控器
    DEVTYPE_dishwasher,            //31    洗碗机
    DEVTYPE_dehumidifier,        //32    除湿机
    DEVTYPE_clothes_dryer,        //33    干衣机
    DEVTYPE_wall_hanging_stove,    //34    壁挂炉
    DEVTYPE_microwave_oven,        //35    微波炉
    DEVTYPE_heater,                //36    取暖器
    DEVTYPE_mosquito_dispeller,    //37    驱蚊器
    DEVTYPE_treadmill,            //38    跑步机
    DEVTYPE_door_lock,            //39    智能门控(门锁)
    DEVTYPE_bracelet,            //40    智能手环
    DEVTYPE_clothes_horse,        //41    晾衣架

    DEVTYPE_BUFF
}DEVTYPE_E;

typedef struct tagPLUG_PLATFORM                                    /*  云平台    */
{
    PLATFORM_E     ucCloudPlatform;                                /* 对接的物联网平台  1:对接阿里云 2:对接贝壳物联 */

    CHAR        szMqttProductKey[PLUG_MQTT_PRODUCTKEY_LEN+1];    /* 对接阿里云 mqtt的product key */
    CHAR        szMqttDevName[PLUG_MQTT_DEVNAME_LEN+1];            /* 对接阿里云 mqtt的设备名称 */
    CHAR        szMqttDevSecret[PLUG_MQTT_DEVSECRET_LEN+1];        /* 对接阿里云 mqtt的Device Secret */

    DEVTYPE_E    eDevType;                                        /* 对接贝壳物联设备类型 */
    CHAR        szBigiotDevId[PLUG_BIGIOT_DEVID_LEN+1];            /* 对接贝壳物联设备id */
    CHAR        szBigiotApiKey[PLUG_BIGIOT_APIKEY_LEN+1];        /* 对接贝壳物联api key */

    CHAR        szSwitchId[PLUG_BIGIOT_IFID_LEN+1];                /* 对接贝壳物联开关状态接口ID */
    CHAR        szTempId[PLUG_BIGIOT_IFID_LEN+1];                /* 对接贝壳物联温度接口ID */
    CHAR        szHumidityId[PLUG_BIGIOT_IFID_LEN+1];            /* 对接贝壳物联湿度接口ID */

    CHAR        szVoltageId[PLUG_BIGIOT_IFID_LEN+1];            /* 对接贝壳物联电压接口ID */
    CHAR        szCurrentId[PLUG_BIGIOT_IFID_LEN+1];            /* 对接贝壳物联电流接口ID */
    CHAR        szPowerId[PLUG_BIGIOT_IFID_LEN+1];                /* 对接贝壳物联功率接口ID */
    CHAR        szElectricityId[PLUG_BIGIOT_IFID_LEN+1];        /* 对接贝壳物联电量接口ID */

}PLUG_PLATFORM_S;

typedef enum
{
    ACTION_REBOOT        = 0,        /* 重启 */
    ACTION_RESET        = 1,        /* 恢复出厂设置 */

    ACTION_BUFF
}PLUG_ACTION_E;


typedef struct tagPLUG_DevCtl                            /* 控制设备动作     */
{
    UINT8     ucAction;                                    /* 控制设备动作 */

}PLUG_DEVCTL_S;

typedef enum
{
    TIME_SYNC_NONE        = 0,        /* 未同步 */
    TIME_SYNC_NET        = 1,        /* 已同步络时间 */
    TIME_SYNC_MAN        = 2,        /* 已通过手工同步 */

    TIME_SYNC_BUFF
}PLUG_TIME_SYNC_E;


UINT8 PLUG_GetWifiMode( VOID );
VOID PLUG_SetWifiMode( UINT8 ucWifiMode );
CHAR* PLUG_GetWifiSsid( VOID );
UINT PLUG_GetWifiSsidLenth( VOID );
VOID PLUG_SetWifiSsid( CHAR* pcWifiSsid );
CHAR* PLUG_GetWifiPasswd( VOID );
VOID PLUG_SetWifiPasswd( CHAR* pcWifiPasswd );
UINT PLUG_GetWifiPasswdLenth( VOID );
UINT8 PLUG_GetRelayPowerUpStatus( VOID );
VOID PLUG_SetRelayPowerUpStatus( UINT8 ucStatus );

CHAR* PLUG_GetPlugName( VOID );
VOID PLUG_SetPlugName( CHAR* pcPlugName );
UINT PLUG_GetPlugNameLenth( VOID );

UINT8 PLUG_GetCloudPlatform( VOID );
VOID PLUG_SetCloudPlatform( UINT8 ucCloudPlatform );
CHAR* PLUG_GetMqttProductKey( VOID );
CHAR* PLUG_GetMqttDevName( VOID );
CHAR* PLUG_GetMqttDevSecret( VOID );
CHAR* PLUG_GetBigiotDevId( VOID );
CHAR* PLUG_GetBigiotApiKey( VOID );
CHAR* PLUG_GetBigiotSwitchId( VOID );
UINT PLUG_GetBigiotDeviceType( VOID );
CHAR* PLUG_GetBigiotTempId( VOID );
CHAR* PLUG_GetBigiotHumidityId( VOID );
CHAR* PLUG_GetBigiotVoltageId( VOID );
CHAR* PLUG_GetBigiotCurrentId( VOID );
CHAR* PLUG_GetBigiotPowerId( VOID );
CHAR* PLUG_GetBigiotElectricityId( VOID );
VOID PLUG_SetBigiotDevId( CHAR* pcDevId );
VOID PLUG_SetBigiotApiKey( CHAR* pcKey );
UINT PLUG_GetMqttDevSecretLenth( VOID );
VOID PLUG_SetMqttProductKey( CHAR* pcProductKey );
VOID PLUG_SetMqttDevName( CHAR* pcDevName );
VOID PLUG_SetMqttDevSecret( CHAR* DevSecret );

VOID PLUG_SetRelayReversal( UINT uiSaveFlag );
VOID PLUG_SetRelayOff( UINT uiSaveFlag );
VOID PLUG_SetRelayOn( UINT uiSaveFlag );
UINT8 PLUG_GetRelayStatus( VOID );
//BOOL PLUG_GetSmartConfig( VOID );
//VOID PLUG_SetSmartConfig( BOOL bStatus );


PLUG_TIMER_S* PLUG_GetTimerData( UINT8 ucNum );
PLUG_DELAY_S* PLUG_GetDelayData( UINT8 ucNum );
PLUG_SYSSET_S* PLUG_GetSystemSetData( VOID );
PLUG_PLATFORM_S* PLUG_GetPlatFormData( VOID );
UINT32 PLUG_GetTimerDataSize();
UINT32 PLUG_GetDelayDataSize();
UINT32 PLUG_GetSystemSetDataSize();

VOID PLUG_TimerDataDeInit( VOID );
VOID PLUG_DelayDataDeInit( VOID );
VOID PLUG_SystemSetDataDeInit( VOID );

UINT PLUG_MarshalJsonTimer( CHAR* pcBuf, UINT uiBufLen, UINT uiTimerNum );
UINT PLUG_MarshalJsonDelay( CHAR* pcBuf, UINT uiBufLen, UINT uiTimerNum);
UINT PLUG_MarshalJsonInfrared( CHAR* pcBuf, UINT uiBufLen, UINT uiNum );
UINT PLUG_MarshalJsonSystemSet( CHAR* pcBuf, UINT uiBufLen );
UINT PLUG_MarshalJsonHtmlData( CHAR* pcBuf, UINT uiBufLen );
UINT PLUG_MarshalJsonRelayStatus( CHAR* pcBuf, UINT uiBufLen );
UINT PLUG_MarshalJsonDate( CHAR* pcBuf, UINT uiBufLen );
UINT PLUG_MarshalJsonWebSet( CHAR* pcBuf, UINT uiBufLen );

INT32 PLUG_GetTimeFromInternet();
VOID PLUG_GetDate(PLUG_DATE_S * pstDate );
VOID PLUG_SetDate(PLUG_DATE_S * pstDate );

UINT PLUG_ParseDate( CHAR* pDateStr);
UINT PLUG_ParseTimerData( CHAR* pData );
UINT PLUG_ParseRelayStatus( CHAR* pDataStr);
UINT PLUG_ParseWebSetData( CHAR* pData );

VOID PLUG_StartJudgeTimeHanderTimer( VOID );


UINT PLUG_GetRunTime( VOID );



#endif /* __USER_PLUG_H__ */
