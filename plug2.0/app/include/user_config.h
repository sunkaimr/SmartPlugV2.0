/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "user_type.h"

#define FLASH_SIZE_1K		(1024)
#define FLASH_SIZE_2K		(FLASH_SIZE_1K * 2)
#define FLASH_SIZE_4K		(FLASH_SIZE_1K * 4)
#define FLASH_SIZE_8K		(FLASH_SIZE_1K * 8)
#define FLASH_SIZE_10K		(FLASH_SIZE_1K * 10)
#define FLASH_SIZE_16K		(FLASH_SIZE_1K * 16)
#define FLASH_SIZE_20K		(FLASH_SIZE_1K * 20)
#define FLASH_SIZE_100K		(FLASH_SIZE_1K * 100)
#define FLASH_SIZE_1M		(FLASH_SIZE_1K * 1024)


#define FLASH_BASE_ADDR   	 0x200000
#define FLASH_TIMER_ADDR  	(FLASH_BASE_ADDR)
#define FLASH_DELAY_ADDR  	(FLASH_TIMER_ADDR + FLASH_SIZE_4K)
#define FLASH_SYSSET_ADDR  	(FLASH_DELAY_ADDR + FLASH_SIZE_4K)
#define FLASH_HTML_ADDR  	(FLASH_SYSSET_ADDR + FLASH_SIZE_4K)

#define FLASH_USER_ADDR     (FLASH_HTML_ADDR + FLASH_SIZE_4K)


typedef enum tagPLUG_MOUDLE
{
	PLUG_MOUDLE_NONE			= 0x00,		/* нч */
	PLUG_MOUDLE_TIMER			= 0x01, 	/* timer */
	PLUG_MOUDLE_DELAY			= 0x02, 	/* delay */
	PLUG_MOUDLE_SYSSET			= 0x04, 	/* system */
	PLUG_MOUDLE_HTML			= 0x08, 	/* html */

	PLUG_MOUDLE_BUFF         	= 0xFF
}PLUG_MOUDLE_E;



UINT CONFIG_ReadConfig( PLUG_MOUDLE_E uiMoudle );
UINT CONFIG_SaveConfig( PLUG_MOUDLE_E uiMoudle );




#endif

