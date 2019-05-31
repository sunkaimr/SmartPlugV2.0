/*
 * user_upgrade.h
 *
 *  Created on: 2018Äê12ÔÂ7ÈÕ
 *      Author: lenovo
 */

#ifndef __USER_UPGRADE_H__
#define __USER_UPGRADE_H__

#include "user_type.h"

#ifndef UPGRADE_FW_BIN1
	#define UPGRADE_FW_BIN1     0x00    /**< firmware, user1.bin */
#endif

#ifndef UPGRADE_FW_BIN2
	#define UPGRADE_FW_BIN2     0x01    /**< firmware, user2.bin */
#endif



UINT32 UPGRADE_GetUpgradeUserBinAddr( VOID );
VOID UPGRADE_StartUpgradeRebootTimer();
UINT32 UPGRADE_GetUser1BinAddr( VOID );
UINT32 UPGRADE_GetUser2BinAddr( VOID );
const CHAR* UPGRADE_GetFlashMap( VOID );

VOID UPGRADE_StartRebootTimer();
VOID UPGRADE_Reset();

#endif /* __USER_UPGRADE_H__ */
