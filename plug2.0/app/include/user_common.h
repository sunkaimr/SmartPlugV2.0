/*
 * user_common.h
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */

#ifndef __USER_COMMON_H__
#define __USER_COMMON_H__


#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>


#include "user_type.h"
#include "user_log.h"
#include "user_config.h"


#include "user_wifi.h"
#include "user_led.h"
#include "user_plug.h"
#include "user_http.h"
#include "user_web.h"
#include "user_httpHandle.h"
#include "user_temp.h"
#include "user_flash.h"
#include "user_upgrade.h"
#include "user_bigiot.h"
#include "user_infrared.h"
#include "user_meter.h"
#include "user_websocket.h"
#include "user_mqtt.h"

#ifndef GIT_COMMIT_SHA1
    #define GIT_COMMIT_SHA1 "NULL"
#endif

#ifndef SOFTWARE_VERSION
    #define SOFTWARE_VERSION "5.0.1"
#endif


#if IS_PHILIPS
    	#define HARDWARE    "philips"
#elif IS_CHANG_XIN
	#if IS_WELL
		#define HARDWARE    "changxin meter and well"
	#else
		#define HARDWARE    "changxin meter"
	#endif
#elif IS_CHANG_XIN_V1
	#if IS_WELL
		#define HARDWARE    "changxin well"
	#else
		#define	HARDWARE    "changxin"
	#endif
#else
	#define HARDWARE        "jizhiyun"
#endif

#define EXTIINT_NAME_LEN    (32)

typedef VOID(*fn)(VOID);


typedef struct tagExtiInt
{
    UINT            uiGPIO;            //GPIO num
    fn              fInit;             //中断初始化函数
    fn              fDeInit;           //中断去初始化函数
    fn              fHandle;           //中断处理函数
    CHAR            szHandleName[EXTIINT_NAME_LEN]; //中断处理函数名称

}COMM_ExtiInt;

VOID COMM_ExtiIntInit(VOID);
UINT COMM_ExtiIntRegister(UINT uiNum, fn fInit, fn fDeInit, fn fHandle, CHAR *pcName);
UINT COMM_ExtiIntUnregister(UINT uiNum);

#define __ALIYUN__


#endif /* APP_INCLUDE_USER_COMMON_H_ */
