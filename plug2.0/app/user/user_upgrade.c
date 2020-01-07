/*
 * user_upgrade.c
 *
 *  Created on: 2018年12月7日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"

/*
在makefile 23行左右修改对应的字段
-----------------------------------------------------------------------------------------
BOOT            none    表示不执行 boot.bin文件，即没有boot.bin文件
                new        表示执行 boot.bin文件，每次执行将要执行boot.bin文件
APP                1        表示生成 user1.bin文件    注意这个仅当上面的BOOT=new 才有效
                2        表示生成 user2.bin文件    注意这个仅当上面的BOOT=new 才有效
SPI_SPEED        40        表示烧录时候的频率选择，这里一般为40，对应烧录工具默认值即可
SPI_MODE        QIO        这个主要看8266模块制作商，比如现在常见的是安信可的8266-12F，就是QIO。
SPI_SIZE_MAP    1        flash=256，user1地址0x1000,user2地址0x41000        制作对应不同的falsh大小固件
                2        flash=1024，user1地址0x1000,user2地址0x81000    制作对应不同的falsh大小固件
                3        flash=2048，user1地址0x1000,user2地址0x81000    制作对应不同的falsh大小固件
                4        flash=4096，user1地址0x1000,user2地址0x81000    制作对应不同的falsh大小固件
                5        flash=2048，user1地址0x1000,user2地址0x101000    制作对应不同的falsh大小固件
                6        flash=4096，user1地址0x1000,user2地址0x101000    制作对应不同的falsh大小固件
                8        flash=8192，user1地址0x1000,user2地址0x101000    制作对应不同的falsh大小固件
                9        flash=16384，user1地址0x1000,user2地址0x101000    制作对应不同的falsh大小固件
                其它数值    flash=256，user1地址0x1000,user2地址0x41000        制作对应不同的falsh大小固件
-----------------------------------------------------------------------------------------
*/

const CHAR szFlashMap[][5] = {"none", "0.4M", "1M", "2M", "4M", "2M", "4M", "8M", "16M", "0.4M" };

const UINT32 User1BinFlashSizeMap[] =
{
        0x1000,     //Flash:4Mbits. 256KBytes+256KBytes，  user1地址0x1000,user2地址0x41000
        0x1000,     //Flash:2Mbits. 256KBytes，  user1地址0x1000,user2地址0x1000
        0x1000,     //Flash:8Mbits. 512KBytes+512KBytes，  user1地址0x1000,user2地址0x81000
        0x1000,     //Flash:16Mbits.512KBytes+512KBytes，  user1地址0x1000,user2地址0x81000
        0x1000,     //Flash:32Mbits.512KBytes+512KBytes，  user1地址0x1000,user2地址0x81000
        0x1000,        //Flash:16Mbits.1024KBytes+1024KBytes，  user1地址0x1000,user2地址0x101000
        0x1000,     //Flash:32Mbits.1024KBytes+1024KBytes，  user1地址0x1000,user2地址0x101000
        0x1000,     //Flash:32Mbits don't support now
        0x1000,     //Flash:64Mbits.1024KBytes+1024KBytes，  user1地址0x1000,user2地址0x101000
        0x1000      //Flash:128Mbits.1024KBytes+1024KBytes，  user1地址0x1000,user2地址0x101000
};


const UINT32 User2BinFlashSizeMap[] =
{
        0x41000,     //Flash:4Mbits. 256KBytes+256KBytes，  user1地址0x1000,user2地址0x41000
        0x01000,     //Flash:2Mbits. 256KBytes，  user1地址0x1000,user2地址0x1000
        0x81000,     //Flash:8Mbits. 512KBytes+512KBytes，  user1地址0x1000,user2地址0x81000
        0x81000,     //Flash:16Mbits.512KBytes+512KBytes，  user1地址0x1000,user2地址0x81000
        0x81000,     //Flash:32Mbits.512KBytes+512KBytes，  user1地址0x1000,user2地址0x81000
        0x101000,    //Flash:16Mbits.1024KBytes+1024KBytes，  user1地址0x1000,user2地址0x101000
        0x101000,     //Flash:32Mbits.1024KBytes+1024KBytes，  user1地址0x1000,user2地址0x101000
        0x41000,     //Flash:32Mbits don't support now
        0x101000,     //Flash:64Mbits.1024KBytes+1024KBytes，  user1地址0x1000,user2地址0x101000
        0x101000      //Flash:128Mbits.1024KBytes+1024KBytes，  user1地址0x1000,user2地址0x101000
};



UINT32 UPGRADE_GetUpgradeUserBinAddr( VOID )
{
    UINT8 ucUserBin = 0;
    UINT8 ucFlashSize = 0;

    ucUserBin = system_upgrade_userbin_check();
    ucFlashSize = system_get_flash_size_map();

    if ( ucUserBin == UPGRADE_FW_BIN1 )
    {
        //当前运行的是user1.bin则升级user2.bin的数据
        return User2BinFlashSizeMap[ucFlashSize];
    }
    else if ( ucUserBin == UPGRADE_FW_BIN2 )
    {
        //当前运行的是user2.bin则升级user1.bin的数据
        return User1BinFlashSizeMap[ucFlashSize];
    }
    else
    {
        LOG_OUT(LOGOUT_ERROR, "not support upgrade.");
        return 0;
    }
}

UINT32 UPGRADE_GetUser1BinAddr( VOID )
{
    UINT8 ucFlashSize = 0;

    ucFlashSize = system_get_flash_size_map();
    return User1BinFlashSizeMap[ucFlashSize];
}

UINT32 UPGRADE_GetUser2BinAddr( VOID )
{
    UINT8 ucFlashSize = 0;

    ucFlashSize = system_get_flash_size_map();
    return User2BinFlashSizeMap[ucFlashSize];
}

const CHAR* UPGRADE_GetFlashMap( VOID )
{
    UINT8 ucFlashSize = 0;
    ucFlashSize = system_get_flash_size_map();
    return szFlashMap[ucFlashSize];
}

VOID UPGRADE_SetUpgradeReboot()
{
    UINT32 uiAddr = 0;
    UINT8 UserBinNum = 0;

    uiAddr = UPGRADE_GetUpgradeUserBinAddr();

    LOG_OUT(LOGOUT_INFO, "system will upgrade reboot with 0x%X", uiAddr);
    system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
    METER_RestartHandle();
    system_upgrade_reboot();
}

VOID UPGRADE_StartUpgradeRebootTimer()
{
    static xTimerHandle xUpgradeTimers = NULL;
    UINT32 uiUpgradeTimerId = 0;

    xUpgradeTimers = xTimerCreate("UPGRADE_StartUpgradeRebootTimer", 500/portTICK_RATE_MS, FALSE, &uiUpgradeTimerId, UPGRADE_SetUpgradeReboot);
    if ( !xUpgradeTimers )
    {
        LOG_OUT(LOGOUT_ERROR, "xTimerCreate UPGRADE_SetUpgradeReboot failed.");
    }
    else
    {
        if(xTimerStart(xUpgradeTimers, 0) != pdPASS)
        {
            LOG_OUT(LOGOUT_ERROR, "xTimerCreate xUpgradeTimers start failed.");
        }
    }
}

VOID UPGRADE_SetReboot()
{
    METER_RestartHandle();
    system_restart();
}

VOID UPGRADE_StartRebootTimer()
{
    static xTimerHandle xRebootTimers = NULL;
    UINT32 uiRebootTimerId = 0;

    xRebootTimers = xTimerCreate("UPGRADE_StartRebootTimer", 500/portTICK_RATE_MS, FALSE, &uiRebootTimerId, UPGRADE_SetReboot);
    if ( !xRebootTimers )
    {
        LOG_OUT(LOGOUT_ERROR, "xTimerCreate UPGRADE_SetReboot failed.");
    }
    else
    {
        if(xTimerStart(xRebootTimers, 0) != pdPASS)
        {
            LOG_OUT(LOGOUT_ERROR, "xTimerCreate xRebootTimers start failed.");
        }
    }
}

VOID UPGRADE_Reset()
{
    LOG_OUT(LOGOUT_INFO, "system will reset and reboot");
    PLUG_TimerDataDeInit();
    PLUG_DelayDataDeInit();
    INFRARED_InfraredDataDeInit();
    PLUG_SystemSetDataDeInit();
    //HTTP_FileListInit();
    PLUG_PlatformDeInit();
    METER_DeinitData();
    CONFIG_SaveConfig(PLUG_MOUDLE_BUFF);

    UPGRADE_StartRebootTimer();
}



