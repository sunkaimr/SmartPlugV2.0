/*
 * user_flash.h
 *
 *  Created on: 2018Äê11ÔÂ21ÈÕ
 *      Author: lenovo
 */

#ifndef __USER_FLASH_H__
#define __USER_FLASH_H__

#ifndef FLASH_SEC_SIZE
    #define FLASH_SEC_SIZE            4096
#endif

UINT FlASH_Write( UINT uiAddr, CHAR* pcBuf, UINT uiLen );
UINT FlASH_Read( UINT uiAddr, CHAR* pcBuf, UINT uiLen );
UINT FlASH_Erase( UINT uiAddr, UINT uiLen );

#endif /* __USER_FLASH_H__ */
