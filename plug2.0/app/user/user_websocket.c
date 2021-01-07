#include "user_common.h"
#include "sha1.h"
#include "base64.h"

const char* pcMagicStr 		= "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char* pcWebSocketKey 	= "Sec-WebSocket-Key";

UINT WEBSOCKET_Handshake( HTTP_CTX *pstCtx );
UINT WEBSOCKET_ParseData( HTTP_CTX *pstCtx, WS_RES_HEAD_S *pstWCHeader );
UINT WEBSOCKET_CmdHandle( CHAR* pcCmd, UINT uiCmdLen, CHAR* pcBuf, UINT uiBufLen );

UINT HTTP_GetConsole( HTTP_CTX *pstCtx )
{
	UINT uiRet = 0;
	UINT uiLen = 0;
    WS_RES_HEAD_S stWCHeader;
    CHAR* pcBuf = NULL;

    if ( pstCtx->stResp.eProcess == RESP_Process_None )
    {
    	uiRet = WEBSOCKET_Handshake(pstCtx);
    	if ( uiRet != OK )
    	{
    		LOG_OUT( LOGOUT_ERROR, "websocket hand shake failed");
    		return FAIL;
    	}
    }
    else
    {
        uiRet = WEBSOCKET_ParseData(pstCtx, &stWCHeader);
        if ( uiRet != OK )
        {
        	LOG_OUT( LOGOUT_ERROR, "websocket parse data failed");
        	return FAIL;
        }

        LOG_OUT( LOGOUT_DEBUG, "websocket recv data: %s", stWCHeader.Data);

        if ( stWCHeader.DataLen == 0 )
        {
        	return OK;
        }

        pcBuf = malloc( HTTP_BUF_4K );
        if ( pcBuf == NULL )
        {
        	LOG_OUT( LOGOUT_ERROR, "malloc failed");
        	return FAIL;
        }

        uiLen = WEBSOCKET_CmdHandle(stWCHeader.Data, stWCHeader.DataLen, pcBuf, HTTP_BUF_4K);

        uiRet = WEBSOCKET_SendData(pstCtx, pcBuf, uiLen);
        if ( uiRet != OK )
        {
        	LOG_OUT( LOGOUT_ERROR, "websocket send data failed");
        	FREE_MEM(pcBuf);
        	return FAIL;
        }

        LOG_OUT( LOGOUT_DEBUG, "websocket send data: %s", pcBuf);
        FREE_MEM(pcBuf);
    }

    return OK;
}


UINT WEBSOCKET_Handshake( HTTP_CTX *pstCtx )
{
    UINT i, uiRet = 0;
    CHAR* pcKey = NULL;
    CHAR szBuf[100] = {};
    CHAR* pcSha1 = NULL;
    CHAR* pcServerKey = NULL;

    if ( pstCtx == NULL )
    {
    	LOG_OUT( LOGOUT_ERROR, "pstCtx is NULL" );
    	return FAIL;
    }

    HTTP_Malloc(pstCtx, HTTP_BUF_512);

	pstCtx->stResp.eHttpCode = HTTP_CODE_SwitchingProtocols;

	pcKey = HTTP_GetReqHeader(pstCtx, pcWebSocketKey);
	if ( pcKey == NULL )
	{
    	LOG_OUT( LOGOUT_ERROR, "header %s not found", pcWebSocketKey);
    	return FAIL;
	}

	sprintf( szBuf, "%s%s", pcKey, pcMagicStr );
	pcSha1 = sha1_hash(szBuf);

	memset(szBuf, 0, sizeof(szBuf));
	for( i = 0; i < strlen(pcSha1); i += 2 )
	{
		szBuf[i/2] = htoi(pcSha1, i, 2);
	}

	pcServerKey = base64Encode(szBuf, strlen(szBuf));
	FREE_MEM(pcSha1);

	uiRet = HTTP_SetRespHeader( pstCtx );
	if ( uiRet != OK )
	{
		LOG_OUT( LOGOUT_ERROR, "set header failed");
		return FAIL;
	}

	HTTP_SetCustomHeader( pstCtx, "Sec-WebSocket-Accept", pcServerKey);
	FREE_MEM(pcServerKey);

	pstCtx->stResp.uiSendTotalLen = 0xFFFFFFFE;
	pstCtx->stResp.uiSendCurLen   = pstCtx->stResp.uiPos;

    uiRet = WEB_WebSend(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "send failed");
        return FAIL;
    }

	pstCtx->stResp.eProcess = RESP_Process_SentHeader;

	return OK;
}

UINT WEBSOCKET_ParseData( HTTP_CTX *pstCtx, WS_RES_HEAD_S *pstWCHeader )
{
	UINT i, uiRet = 0;
	CHAR* pcBuf = NULL;

	if ( pstCtx == NULL || pstCtx->stReq.pcResqBody == NULL || pstWCHeader == NULL )
	{
		LOG_OUT( LOGOUT_INFO, "pstCtx:%p, pcResqBody:%p, pstWCHeader:%p",
				 pstCtx, pstCtx->stReq.pcResqBody, pstWCHeader );
		return FAIL;
	}
	pcBuf = pstCtx->stReq.pcResqBody;

	pstWCHeader->FIN     = pcBuf[0] >> 7;
	pstWCHeader->Opcode  = pcBuf[0] & 0x0F;
	pstWCHeader->bMask   = pcBuf[1] >> 7;
	pstWCHeader->DataLen = pcBuf[1] & 0x7F;

	if ( pstWCHeader->DataLen < 126 )
	{
		memcpy(pstWCHeader->Mask, &pcBuf[2], 4);
		pstWCHeader->Data = &pcBuf[6];
	}
	else if ( pstWCHeader->DataLen == 126 )
	{
		pstWCHeader->DataLen = pcBuf[2] << 8 | pcBuf[3];
		memcpy(pstWCHeader->Mask, &pcBuf[4], 4);
		pstWCHeader->Data = &pcBuf[8];
	}
	else if ( pstWCHeader->DataLen == 127 )
	{
		pstWCHeader->DataLen = pcBuf[6] << 24 | pcBuf[7] << 16 | pcBuf[8] << 8 | pcBuf[9];
		memcpy(pstWCHeader->Mask, &pcBuf[10], 4);
		pstWCHeader->Data = &pcBuf[14];
	}
	else
	{
		LOG_OUT( LOGOUT_INFO, "unknown DataLen:%d", pstWCHeader->DataLen );
		return FAIL;
	}

	//	Opcode表示帧的类型: 0x0 表示附加数据帧, 0x1 表示文本数据帧, 0x2 表示二进制数据帧, 0x3-7 暂时无定义，为以后的非控制帧保留
	//	0x8 表示连接关闭, 0x9 表示ping, 0xA 表示pong, 0xB-F 暂时无定义，为以后的控制帧保留
	if ( pstWCHeader->Opcode == 0x08 )
	{
		LOG_OUT( LOGOUT_INFO, "websocker client closed" );
		*(pstWCHeader->Data) = 0;
		pstWCHeader->DataLen = 0;
		pstCtx->stResp.eProcess = RESP_Process_Finished;
	}

	for ( i = 0; i < pstWCHeader->DataLen; i++ )
	{
		pstWCHeader->Data[i] = pstWCHeader->Data[i]^pstWCHeader->Mask[i % 4];
	}
	pstWCHeader->Data[i] = 0;

#if 0
	LOG_OUT( LOGOUT_INFO, "pstWCHeader->FIN:%d",      pstWCHeader->FIN);
	LOG_OUT( LOGOUT_INFO, "pstWCHeader->Opcode:0x%X", pstWCHeader->Opcode);
	LOG_OUT( LOGOUT_INFO, "pstWCHeader->bMask:0x%X",  pstWCHeader->bMask);
	LOG_OUT( LOGOUT_INFO, "pstWCHeader->DataLen:%d",  pstWCHeader->DataLen);
	LOG_OUT( LOGOUT_INFO, "pstWCHeader->Data:[%s]",   pstWCHeader->Data);
#endif
	return OK;
}

UINT WEBSOCKET_SendData( HTTP_CTX *pstCtx, CHAR* pcData, UINT uiDataLen )
{
	UINT uiRet = 0;
	UINT8* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	WS_RES_HEAD_S stWCHeader;

	if ( pstCtx == NULL || pcData == NULL )
	{
		LOG_OUT( LOGOUT_INFO, "pstCtx:%p, pcData:%p", pstCtx, pcData );
		return FAIL;
	}

	HTTP_Malloc(pstCtx, uiDataLen+20);

	stWCHeader.FIN     = 1 << 7;
	stWCHeader.Opcode  = 0x1;
	stWCHeader.bMask   = 0;
	stWCHeader.DataLen = uiDataLen;

	if ( pstCtx->stResp.eProcess == RESP_Process_Finished )
	{
		stWCHeader.Opcode = 0x8;
	}

	pcBuf = pstCtx->stResp.pcResponBody;
	pcBuf[0] = stWCHeader.FIN | stWCHeader.Opcode;

	if ( stWCHeader.DataLen < 126 )
	{
		pcBuf[1] = stWCHeader.DataLen;
		stWCHeader.Data = &pcBuf[2];
		uiHeaderLen = 2;
	}
	else if ( stWCHeader.DataLen < 0xFFFF )
	{
		pcBuf[1] = 126;
		pcBuf[2] = stWCHeader.DataLen >> 8 & 0xFF;
		pcBuf[3] = stWCHeader.DataLen & 0xFF;
		stWCHeader.Data = &pcBuf[4];
		uiHeaderLen = 4;
	}
	else
	{
		pcBuf[1] = 127;
		memset(&pcBuf[2], 0, 8);
		pcBuf[6] = (stWCHeader.DataLen >> 24 & 0xFF);
		pcBuf[7] = (stWCHeader.DataLen >> 26 & 0xFF);
		pcBuf[8] = (stWCHeader.DataLen >> 8 & 0xFF);
		pcBuf[9] = stWCHeader.DataLen & 0xFF;
		stWCHeader.Data = &pcBuf[10];
		uiHeaderLen = 10;
	}

	if ( (uiHeaderLen + stWCHeader.DataLen) > pstCtx->stResp.uiSendBufLen )
	{
		LOG_OUT( LOGOUT_INFO, "websocket send buf too short, uiSendBufLen:%d, DataLen:%d", pstCtx->stResp.uiSendBufLen, stWCHeader.DataLen );
		return FAIL;
	}

	pstCtx->stResp.uiSendCurLen = uiHeaderLen + stWCHeader.DataLen;
	memcpy( stWCHeader.Data, pcData, stWCHeader.DataLen);
	stWCHeader.Data[stWCHeader.DataLen] = 0;

	//LOG_OUT( LOGOUT_DEBUG, "[%d][%s]", pstCtx->stResp.uiSendCurLen, stWCHeader.Data );

    uiRet = WEB_WebSend(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "send failed");
        pstCtx->stResp.eProcess = RESP_Process_Finished;
        return FAIL;
    }

    return OK;
}


UINT WEBSOCKET_CmdHandle( CHAR* pcCmd, UINT uiCmdLen, CHAR* pcBuf, UINT uiBufLen )
{
	UINT uiPos = 0;

	if ( pcCmd == NULL || pcBuf == NULL )
	{
		LOG_OUT( LOGOUT_ERROR, "pcCmd:%p, pcBuf:%p", pcCmd, pcBuf );
		return 0;
	}

	if ( 0 == strcmp(pcCmd, "help") )
	{
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"Supported Commands:\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get timer            : print all timer data.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get delay            : print all delay data.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get infrared         : print all infrared data.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get platform         : print cloud platform data.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get sys              : print system config.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get date             : print now time.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get device info      : print device info.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get html header      : print html info in falsh.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get relay            : print relay status.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get heap             : print free heap\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"reboot               : reboot\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"set log {level}      : set log level, {level} could be: none, debug, info, error.\r\n");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"get log              : get log");
	    uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"\r\n");
	    return uiPos;
	}
	else if ( 0 == strcmp(pcCmd, "get device info") )
	{
		return WIFI_DeviceInfoMarshalJson(pcBuf, uiBufLen);
	}
	else if ( 0 == strcmp(pcCmd, "get sys") )
	{
		return PLUG_MarshalJsonSystemSet(pcBuf, uiBufLen);
	}
	else if ( 0 == strcmp(pcCmd, "get timer") )
	{
		return PLUG_MarshalJsonTimer(pcBuf, uiBufLen, PLUG_TIMER_ALL);
	}
	else if ( 0 == strcmp(pcCmd, "get delay") )
	{
		return PLUG_MarshalJsonDelay(pcBuf, uiBufLen, PLUG_DELAY_ALL);
	}
	else if ( 0 == strcmp(pcCmd, "get infrared") )
	{
		return PLUG_MarshalJsonInfrared(pcBuf, uiBufLen, INFRARED_ALL);
	}
	else if ( 0 == strcmp(pcCmd, "get platform") )
	{
		return PLUG_MarshalJsonCloudPlatformSet(pcBuf, uiBufLen);
	}
	else if ( 0 == strcmp(pcCmd, "get html header") )
	{
		return PLUG_MarshalJsonHtmlData(pcBuf, uiBufLen);
	}
	else if ( 0 == strcmp(pcCmd, "get wifi") )
	{
		//WIFI_WifiScanMarshalJson(pcBuf, uiBufLen);
	}
	else if ( 0 == strcmp(pcCmd, "get relay status") )
	{
		return PLUG_MarshalJsonRelayStatus(pcBuf, uiBufLen);
	}
	else if ( 0 == strcmp(pcCmd, "get date") )
	{
		return PLUG_MarshalJsonDate(pcBuf, uiBufLen);
	}
	else if ( 0 == strcmp(pcCmd, "get heap") )
	{
		return snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"Free heap Size:%d", system_get_free_heap_size());
	}
	else if ( 0 == strcmp(pcCmd, "reboot") )
	{
		UPGRADE_StartRebootTimer();
		return snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"reboot success");
	}
	else if ( 0 != strstr(pcCmd, "set log ") )
	{
		if ( strstr(pcCmd,"debug" ) != 0 ){
			LOG_SetLogLevel(LOGOUT_DEBUG);
			return snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"set log debug");
		}
		else if (strstr(pcCmd, "info" ) != 0){
			LOG_SetLogLevel(LOGOUT_INFO);
			return snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"set log info");
		}
		else if (strstr(pcCmd, "error" ) != 0){
			LOG_SetLogLevel(LOGOUT_ERROR);
			return snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"set log error");
		}
		else if (strstr(pcCmd, "none" ) != 0){
			LOG_SetLogLevel(LOGOUT_NONE);
			return snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"set log none");
		}
		else{
			return snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"unknown \'%s\', you can input \'set log none, debug, info or error\'.", pcCmd+8);
		}
	}
	else if ( 0 == strcmp(pcCmd, "get log") )
	{
		return LOG_PrintHistoryLog(pcBuf+uiPos, uiBufLen-uiPos);
	}
	else
	{
		return snprintf( pcBuf+uiPos, uiBufLen-uiPos,	"unknown cmd \'%s\', input \'help\' to list supported cmd.", pcCmd );
	}
}



