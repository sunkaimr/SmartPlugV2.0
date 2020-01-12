#ifndef __USER_WEBSOCKET_H__
#define __USER_WEBSOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "user_common.h"

typedef struct tagWebSocketResHeader
{
	UINT8		FIN;
	UINT8		Opcode;
	BOOL		bMask;
	UINT		DataLen;
	UINT8		Mask[4];
	UINT8*		Data;
}WS_RES_HEAD_S;

// 传入日志模块，将日志输出到控制台
extern HTTP_CTX *pstConsoleCtx;

UINT HTTP_GetConsole( HTTP_CTX *pstCtx );
UINT WEBSOCKET_SendData( HTTP_CTX *pstCtx, CHAR* pcData, UINT uiDataLen );


#ifdef __cplusplus
}
#endif

#endif

