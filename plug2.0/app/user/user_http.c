/*
 * user_http.c
 *
 *  Created on: 2018年11月11日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"



const CHAR* HttpStatus_Ok 					= "OK";
const CHAR* HttpStatus_Created				= "Created";
const CHAR* HttpStatus_Found			 	= "Found";
const CHAR* HttpStatus_BadRequest 			= "BadRequest";
const CHAR* HttpStatus_NotFound 			= "NotFound";
const CHAR* HttpStatus_InternalServerError 	= "InternalServerError";

const CHAR* STRING_SPACE = " ";
const CHAR* STRING_ENTER= "\n";

HTTP_ROUTER_MAP_S stHttpRouterMap[HTTP_ROUTER_MAP_MAX];

const CHAR szHttpMethodStringmap[][10] =
{
	"GET",
	"POST",
	"HEAD",
	"PUT",
	"DELETE",
	""
};

const CHAR szHttpUserAgentStringmap[][10] =
{
	"Windows",
	"Android",
	"curl",
	"Wget",
	""
};

const CHAR szHttpContentTypeStr[][30] =
{
	"text/html, charset=utf-8",
	"application/x-javascript",
	"text/css",
	"application/json",
	"image/x-icon",
	"image/png",
	"image/gif",

	"application/octet-stream",
	""
};

const CHAR szHttpCacheControlStr[][30] =
{
	"no-cache",
	"max-age=3600",
	"max-age=86400",
	"max-age=604800",
	"max-age=2592000",
	"max-age=31536000",

	""
};


INT32 HTTP_ParsingHttpHead( CHAR * pcData, UINT32 uiLen,  HTTP_REQUEST_HEAD_S *pstHttpHead )
{
	CHAR *pcCurPos = NULL;
	CHAR *pcTmpPos = NULL;
	CHAR *pcPos = NULL;
	INT32 iLoop = 0;
	int i = 0;

	if ( NULL == pcData || NULL == pstHttpHead )
	{
		LOG_OUT( LOGOUT_ERROR, "HTTP_ParsingHttpHead: pcData or  pstHttpHead is NULL.");
		return FAIL;
	}

	if ( pstHttpHead->eProcess == HTTP_PROCESS_GetBody )
	{
		//LOG_OUT(LOGOUT_DEBUG, "HTTP_ParsingHttpHead HTTP_PROCESS_GetBody.");

		pstHttpHead->uiRecvCurLenth = uiLen;
		pstHttpHead->pcResqBody = pcData;

		pstHttpHead->uiRecvPresentLenth += uiLen;
		if ( pstHttpHead->uiRecvPresentLenth >= pstHttpHead->uiContentLenth )
		{
			pstHttpHead->eProcess = HTTP_PROCESS_Finished;

			//LOG_OUT(LOGOUT_DEBUG, "HTTP_ParsingHttpHead HTTP_PROCESS_Finished.");
			return OK;
		}
		return OK;
	}
	else if ( pstHttpHead->eProcess != HTTP_PROCESS_None )
	{
		pstHttpHead->uiRecvCurLenth = 0;
		pstHttpHead->uiRecvPresentLenth = 0;
		pstHttpHead->pcResqBody = NULL;
		pstHttpHead->eProcess = HTTP_PROCESS_Invalid;
		pstHttpHead->eMethod = HTTP_METHOD_BUFF;

		//LOG_OUT(LOGOUT_DEBUG, "HTTP_ParsingHttpHead HTTP_PROCESS_None.");
		return FAIL;
	}

	pcData[uiLen] = '\r';
	pcData[uiLen+1] = '\n';
	pcData[uiLen+2] = 0;
	pcPos = pcCurPos = pcData;
	//LOG_OUT(LOGOUT_DEBUG, "\ndata:[%s]\n", pcData);

	pstHttpHead->eMethod = HTTP_METHOD_BUFF;
	pstHttpHead->uiRecvCurLenth = 0;
	pstHttpHead->uiRecvPresentLenth = 0;
	pstHttpHead->pcResqBody = NULL;
	pstHttpHead->szURL[0] = 0;

	/* 逐行解析 */
	while ( pcData != NULL )
	{
		pcCurPos = strsep(&pcData, STRING_ENTER);
		if ( NULL == pcCurPos )
		{
			continue;
		}
		//LOG_OUT(LOGOUT_DEBUG, "pcCurPos：[%s]", pcCurPos);

		//header结束
		if ( strlen(pcCurPos) <= 1 && pstHttpHead->uiContentLenth != 0 &&
			 ( pstHttpHead->eMethod == HTTP_METHOD_POST || pstHttpHead->eMethod == HTTP_METHOD_PUT ))
		{
			pcCurPos = pcCurPos + 2;
			pstHttpHead->pcResqBody = pcCurPos;
			pstHttpHead->uiRecvCurLenth = uiLen - (pcCurPos - pcPos);

			pstHttpHead->uiRecvPresentLenth += pstHttpHead->uiRecvCurLenth;
			if ( pstHttpHead->uiRecvPresentLenth >= pstHttpHead->uiContentLenth )
			{
				//LOG_OUT(LOGOUT_DEBUG, "HTTP_ParsingHttpHead HTTP_PROCESS_Finished.");
				pstHttpHead->eProcess = HTTP_PROCESS_Finished;
				return OK;
			}
			pstHttpHead->eProcess = HTTP_PROCESS_GetBody;
			//LOG_OUT(LOGOUT_DEBUG, "HTTP_ParsingHttpHead HTTP_PROCESS_GetBody.");
			return OK;
		}

		/* GET / HTTP/1.1 : eMethod, URL*/
		if ( HTTP_METHOD_BUFF == pstHttpHead->eMethod )
		{
			//解析eMethod
			//LOG_OUT(LOGOUT_DEBUG, "eMethod：[%s]", pcCurPos);
			for ( iLoop = HTTP_METHOD_GET; iLoop < HTTP_METHOD_BUFF; iLoop++ )
			{
				if ( strstr( pcCurPos, szHttpMethodStringmap[iLoop]) )
				{
					pstHttpHead->eMethod = iLoop;
				}
			}

			/* 解析URL */
			if ( pstHttpHead->eMethod != HTTP_METHOD_BUFF && NULL != (pcTmpPos = strstr( pcCurPos, "/" )))
			{
				while( *pcTmpPos != ' ' )
				{
					pcTmpPos++;
				}
				pcTmpPos[0] = 0;
				//LOG_OUT(LOGOUT_DEBUG, "pcCurPos:[%s]", pcCurPos);
				strncpy( pstHttpHead->szURL, strstr( pcCurPos, "/" ), sizeof(pstHttpHead->szURL));
				//LOG_OUT(LOGOUT_DEBUG, "szURL:[%s]", pstHttpHead->szURL);
			}

			if ( pstHttpHead->eMethod == HTTP_METHOD_BUFF || pstHttpHead->szURL[0] == 0 )
			{
				LOG_OUT( LOGOUT_ERROR, "HTTP_ParsingHttpHead get eMethod or URL failed.");

				pstHttpHead->uiRecvCurLenth = 0;
				pstHttpHead->uiRecvPresentLenth = 0;
				pstHttpHead->pcResqBody = NULL;
				pstHttpHead->szURL[0] = 0;
				pstHttpHead->eProcess = HTTP_PROCESS_Invalid;

				return FAIL;
			}
		}
		/* Host */
		else if ( NULL != (pcTmpPos = strstr( pcCurPos, "Host: " )))
		{
		    pcTmpPos = pcTmpPos + 6;
			strncpy( pstHttpHead->szHost, pcTmpPos, sizeof(pstHttpHead->szHost));
		}
		/* eUserAgent */
		else if ( strstr( pcCurPos, "User-Agent: " ))
		{
			//LOG_OUT(LOGOUT_DEBUG, "eUserAgent：[%s]", pcCurPos);
			for ( iLoop = HTTP_USERAGENT_WINDOWNS; iLoop < HTTP_USERAGENT_BUFF; iLoop++ )
			{
				if ( strstr( pcCurPos, szHttpUserAgentStringmap[iLoop]) )
				{
					pstHttpHead->eUserAgent = iLoop;
					break;
				}
			}
		}
		//curl命令等工具 发送的字段内容长度字段是大写字母开头"Content-Length"
		else if ( NULL != (pcTmpPos = strstr( pcCurPos, "Content-Length: " )))
		{
		    pcTmpPos = pcTmpPos + 16;
		    //LOG_OUT(LOGOUT_DEBUG, "Content-Length:[%s]", pcTmpPos);
			pstHttpHead->uiContentLenth = atoi(pcTmpPos);
			//LOG_OUT(LOGOUT_DEBUG, "Content-Length:[%d]", pstHttpHead->uiContentLenth);
		}
		//postman 发送的字段内容长度字段是小写字母"content-length"
		else if ( NULL != (pcTmpPos = strstr( pcCurPos, "content-length: " )))
		{
		    pcTmpPos = pcTmpPos + 16;
		    //LOG_OUT(LOGOUT_DEBUG, "Content-Length:[%s]", pcTmpPos);
			pstHttpHead->uiContentLenth = atoi(pcTmpPos);
			//LOG_OUT(LOGOUT_DEBUG, "Content-Length:[%d]", pstHttpHead->uiContentLenth);
		}
	}

	pstHttpHead->uiRecvCurLenth = 0;
	pstHttpHead->uiRecvPresentLenth = 0;
	pstHttpHead->pcResqBody = NULL;
	pstHttpHead->eProcess = HTTP_PROCESS_GotHeader;
	//LOG_OUT( LOGOUT_DEBUG, "HTTP_ParsingHttpHead HTTP_PROCESS_GotHeader.");
	return OK;
}


VOID HTTP_RouterMapInit( VOID )
{
	UINT uiLoop = 0;

	for ( uiLoop = 0; uiLoop < HTTP_ROUTER_MAP_MAX; uiLoop++ )
	{
		stHttpRouterMap[uiLoop].eMethod = HTTP_METHOD_BUFF;
		stHttpRouterMap[uiLoop].pfHttpHandler = NULL;
		memset(stHttpRouterMap[uiLoop].szURL, 0, HTTP_URL_MAX_LEN);
		memset(stHttpRouterMap[uiLoop].szHttpHandlerStr, 0, HTTP_HANDLE_MAX_LEN);
	}
}

VOID HTTP_RouterRegiste( UINT uiMethod, CHAR* pcUrl, VOID* pfFunc, CHAR* pcFunStr)
{
	UINT uiLoop = 0;

	if ( uiMethod >= HTTP_METHOD_BUFF )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_RouterRegiste uiMethod:%d", uiMethod);
		return;
	}

	if ( pcUrl == NULL ||  pfFunc == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_RouterRegiste pcUrl or pfFunc is NULL, pcUrl:%d,pfFunc:%d.", pcUrl, pfFunc);
		return;
	}

	//判断是否注册过，若注册过进行更新
	for ( uiLoop = 0; uiLoop < HTTP_ROUTER_MAP_MAX; uiLoop++ )
	{
		if ( stHttpRouterMap[uiLoop].eMethod == uiMethod &&
			 strcmp(stHttpRouterMap[uiLoop].szURL, pcUrl ) == 0 )
		{
			stHttpRouterMap[uiLoop].pfHttpHandler = pfFunc;
			strncpy(stHttpRouterMap[uiLoop].szHttpHandlerStr, pcFunStr, HTTP_HANDLE_MAX_LEN);
			return;
		}
	}

	for ( uiLoop = 0; uiLoop < HTTP_ROUTER_MAP_MAX; uiLoop++ )
	{
		if ( stHttpRouterMap[uiLoop].eMethod == HTTP_METHOD_BUFF )
		{
			stHttpRouterMap[uiLoop].eMethod = uiMethod;
			stHttpRouterMap[uiLoop].pfHttpHandler = pfFunc;
			strncpy(stHttpRouterMap[uiLoop].szURL, pcUrl, HTTP_URL_MAX_LEN);
			strncpy(stHttpRouterMap[uiLoop].szHttpHandlerStr, pcFunStr, HTTP_HANDLE_MAX_LEN);
			break;
		}
	}

	if ( uiLoop >= HTTP_ROUTER_MAP_MAX )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_RouterRegiste stHttpRouterMap is full.");
	}
}


VOID HTTP_RouterInit( VOID )
{
	HTTP_RouterMapInit();

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/", 				HTTP_GetHome, 		"home");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/health", 		HTTP_GetHealth, 	"HTTP_GetHealth");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/info", 			HTTP_GetInfo, 		"info");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/timer/:timer",	HTTP_GetTimerData,	"HTTP_GetTimerData");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/delay/:delay",	HTTP_GetDelayData,	"HTTP_GetDelayData");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/system",			HTTP_GetSystemData,	"HTTP_GetSystemData");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/html/header",	HTTP_GetHtmlHeader,	"HTTP_GetHtmlHeader");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/html/header",	HTTP_PostHtmlHeader,"HTTP_PostHtmlHeader");
	HTTP_RouterRegiste(HTTP_METHOD_PUT,  "/html/:html",		HTTP_PutHtml,		"HTTP_PutHtml");

	HTTP_RouterRegiste(HTTP_METHOD_POST, "/timer",	   		HTTP_PostTimerData,	"HTTP_PostTimerData");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/delay",	   		HTTP_PostDelayData,	"HTTP_PostDelayData");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/system",	   		HTTP_PostSystemData,"HTTP_PostSystemData");

	HTTP_RouterRegiste(HTTP_METHOD_POST, "/control",	   	HTTP_PostDeviceControl,	"HTTP_PostDeviceControl");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/scanwifi",	   	HTTP_GetScanWifi,		"HTTP_GetScanWifi");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/relaystatus",	HTTP_GetRelayStatus,	"HTTP_GetRelayStatus");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/relaystatus",	HTTP_PostRelayStatus,	"HTTP_PostRelayStatus");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/date",	    	HTTP_GetDate,		"HTTP_GetDate");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/date",	    	HTTP_PostDate,		"HTTP_PostDate");

	HTTP_RouterRegiste(HTTP_METHOD_PUT,  "/upgrade",	    HTTP_PutUpgrade,	"HTTP_PutUpgrade");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/upload",    		HTTP_GetUpload,		"HTTP_GetUpload");
	//HTTP_RouterRegiste(HTTP_METHOD_GET, "/upgrade",	    HTTP_GetUpgrade,"HTTP_GetUpgrade");

	HTTP_HtmlUrlRegiste();

	//测试
	//HTTP_RouterRegiste(HTTP_METHOD_GET, "/test", 			HTTP_GetTest, 	"HTTP_GetTest");
	//HTTP_RouterRegiste(HTTP_METHOD_PUT, "/test",			HTTP_PutTest,	"HTTP_PutTest");
	//HTTP_RouterRegiste(HTTP_METHOD_PUT, "/image",			HTTP_PutImage,	"HTTP_PutImage");
	//HTTP_RouterRegiste(HTTP_METHOD_GET, "/image",			HTTP_GetImage,	"HTTP_GetImage");
}

BOOL HTTP_RouterIsMatch( const CHAR* pcRouter, const CHAR* pcUrl)
{
	CHAR *pcRBuf, *pcUBuf = NULL;
	CHAR *pcRData, *pcUData = NULL;
	CHAR *pcRtmp, *pcUtmp = NULL;

	if (pcRouter == NULL || pcUrl == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pcRouter:%p, pcUrl:%p.", pcRouter, pcUrl);
		return FALSE;
	}

	pcRBuf = malloc(HTTP_URL_MAX_LEN);
	if ( NULL == pcRBuf )
	{
	    LOG_OUT(LOGOUT_ERROR, "malloc pcRBuf is NULL.");
	    goto error;
	}

	pcUBuf = malloc(HTTP_URL_MAX_LEN);
	if ( NULL == pcUBuf )
	{
	    LOG_OUT(LOGOUT_ERROR, "malloc pcUBuf is NULL.");
	    goto error;
	}

	if ( strcmp(pcRouter, pcUrl) == 0 )
	{
		goto succ;
	}

	strncpy(pcRBuf, pcRouter,	HTTP_URL_MAX_LEN);
	strncpy(pcUBuf, pcUrl, 		HTTP_URL_MAX_LEN);

	pcRData = pcRBuf;
	pcUData = pcUBuf;

	while( pcRData != NULL && pcUData != NULL)
	{
		pcRtmp = strsep(&pcRData, "/");
		pcUtmp = strsep(&pcUData, "/");

		if (pcRtmp == NULL || pcUtmp == NULL )
		{
			goto error;
		}

		if ( NULL != strstr(pcRtmp, ":") )
		{
			continue;
		}
		if ( strcmp(pcRtmp, pcUtmp) != 0 )
		{
			goto error;
		}
	}

	if (pcRData == NULL && pcUData == NULL )
	{
		goto succ;
	}
	else
	{
		goto error;
	}

error:
	FREE_MEM(pcRBuf);
	FREE_MEM(pcUBuf);
	return FALSE;

succ:
	FREE_MEM(pcRBuf);
	FREE_MEM(pcUBuf);
	return TRUE;
}

UINT HTTP_GetRouterPara( HTTP_REQUEST_HEAD_S *pstHeader, const CHAR* pcKey, CHAR* pcValue )
{
	CHAR *pcRBuf, *pcUBuf = NULL;
	CHAR *pcRData, *pcUData = NULL;
	CHAR *pcRtmp, *pcUtmp = NULL;

	if ( pstHeader == NULL || pcKey == NULL || pcValue == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pstHeader:%p, pcKey:%p, pcValue:%p", pstHeader, pcKey, pcValue);
	    return FAIL;
	}

	pcRBuf = malloc(HTTP_URL_MAX_LEN);
	if ( NULL == pcRBuf )
	{
	    LOG_OUT(LOGOUT_ERROR, "malloc pcRBuf is NULL.");
	    goto error;
	}

	pcUBuf = malloc(HTTP_URL_MAX_LEN);
	if ( NULL == pcUBuf )
	{
	    LOG_OUT(LOGOUT_ERROR, "malloc pcUBuf is NULL.");
	    goto error;
	}

	strncpy(pcRBuf, pstHeader->pcRouter,	HTTP_URL_MAX_LEN);
	strncpy(pcUBuf, pstHeader->szURL, 		HTTP_URL_MAX_LEN);

	pcRData = pcRBuf;
	pcUData = pcUBuf;

	while( pcRData != NULL && pcUData != NULL)
	{
		pcRtmp = strsep(&pcRData, "/");
		pcUtmp = strsep(&pcUData, "/");

		if ( NULL != strstr(pcRtmp, ":") )
		{
			if ( strcmp(pcRtmp+1, pcKey) == 0 )
			{
				strcpy(pcValue, pcUtmp);
				goto succ;
			}
		}
	}

error:
	FREE_MEM(pcRBuf);
	FREE_MEM(pcUBuf);
	return FAIL;

succ:
	FREE_MEM(pcRBuf);
	FREE_MEM(pcUBuf);
	return OK;
}

VOID HTTP_RouterHandle( HTTP_REQUEST_HEAD_S *pstHeader )
{
	UINT uiLoop = 0;

	FREE_MEM( pstHeader->pcResponBody );

	if ( pstHeader->eProcess == HTTP_PROCESS_Invalid )
	{
		LOG_OUT(LOGOUT_INFO, "HTTP_RouterHandle BadRequest.");
		HTTP_BadRequest( pstHeader );
		return;
	}

	if ( pstHeader->szURL[0] == 0 )
	{
		LOG_OUT(LOGOUT_INFO, "HTTP_RouterHandle szURL is NULL.");
		return;
	}

	for ( uiLoop = 0; uiLoop < HTTP_ROUTER_MAP_MAX; uiLoop++ )
	{
		if ( stHttpRouterMap[uiLoop].eMethod == pstHeader->eMethod &&
			 HTTP_RouterIsMatch(stHttpRouterMap[uiLoop].szURL, pstHeader->szURL) )
		{
			if ( pstHeader->uiRecvCurLenth == pstHeader->uiRecvPresentLenth )
			{
				LOG_OUT(LOGOUT_INFO, "router: %s.", stHttpRouterMap[uiLoop].szHttpHandlerStr);
			}

			pstHeader->pcRouter = stHttpRouterMap[uiLoop].szURL;
			stHttpRouterMap[uiLoop].pfHttpHandler(pstHeader);
			return;
		}
	}

	if ( uiLoop >= HTTP_ROUTER_MAP_MAX )
	{
		LOG_OUT(LOGOUT_INFO, "Not match any router. Method:%s,URL:%s.",
				szHttpMethodStringmap[pstHeader->eMethod],  pstHeader->szURL);
		HTTP_NotFound( pstHeader );
	}

	return;
}

UINT HTTP_SetHeaderCode( CHAR* pcBuf, UINT uiBufLen, HTTP_RESPONSE_HEAD_S* pstResponse )
{
	UINT uiPos = 0;
	const CHAR* pcHttpStatus = NULL;

	if ( pcBuf == NULL || pstResponse == NULL)
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_SetHeaderCode pcBuf:%p, pstResponse:%p.", pcBuf, pstResponse);
		return 0;
	}

	if ( pstResponse->eHttpCode >= HTTP_CODE_Buff )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_SetHeaderCode ContentType error uiContentType:%d.", pstResponse->eHttpCode);
		return 0;
	}

	switch ( pstResponse->eHttpCode )
	{
		case HTTP_CODE_Ok:
			pcHttpStatus = HttpStatus_Ok;
			break;
		case HTTP_CODE_Created:
			pcHttpStatus = HttpStatus_Created;
			break;
		case HTTP_CODE_Found:
			pcHttpStatus = HttpStatus_Found;
			break;
		case HTTP_CODE_BadRequest:
			pcHttpStatus = HttpStatus_BadRequest;
			break;
		case HTTP_CODE_NotFound:
			pcHttpStatus = HttpStatus_NotFound;
			break;
		case HTTP_CODE_InternalServerError:
			pcHttpStatus = HttpStatus_InternalServerError;
			break;
		default:
			LOG_OUT(LOGOUT_ERROR, "HTTP_SetHeaderCode uiHttpCode unknown, uiHttpCode:%d.", pstResponse->eHttpCode);
			return 0;
	}

	uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos, "HTTP/1.1 %d %s \r\n", pstResponse->eHttpCode, pcHttpStatus );
	uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos, "Accept-Ranges: bytes \r\n");
	uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos, "Content-Type: %s \r\n", szHttpContentTypeStr[pstResponse->eContentType]);
	uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos, "Cache-Control: %s \r\n", szHttpCacheControlStr[pstResponse->eCacheControl]);
	uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos, "Connection: Keep-Alive \r\n");

	if ( pstResponse->eHttpCode == HTTP_CODE_Found )
	{
		WIFI_INFO_S stWifiInfo = WIFI_GetIpInfo();
		uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos, "Location: http://%d.%d.%d.%d/index.html \r\n", stWifiInfo.uiIp&0xFF, (stWifiInfo.uiIp>>8)&0xFF,
				(stWifiInfo.uiIp>>16)&0xFF,(stWifiInfo.uiIp>>24)&0xFF);
	}

	uiPos += snprintf( pcBuf+uiPos, uiBufLen-uiPos, "Content-Length:           \r\n\r\n");//长度稍后待body体确定后填写，保持10个以上空格避免数字过大被截断


	return uiPos;
}

VOID HTTP_SetHeaderContentLength( CHAR* pcBuf, UINT uiBodyLen )
{
	UINT uiLen = 0;
	CHAR* pcLength = NULL;

	if ( pcBuf == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_SetHeaderContentLength pcBuf is NULL.");
		return;
	}

	pcLength = strstr(pcBuf, "Content-Length: ");
	if ( pcLength != NULL )
	{
		uiLen = sprintf( pcLength, "Content-Length: %d", uiBodyLen );
		*(pcLength + uiLen) = ' ';
	}

	return;
}

VOID HTTP_SetDefaultSendHeader( HTTP_REQUEST_HEAD_S *pstHeader, CHAR* pcBody, UINT uiSendLen )
{
	UINT uiLen = 0;
	CHAR* pcLength = NULL;

	if ( pstHeader == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_SetRequestHeader pstHeader is NULL.");
		return;
	}

	pstHeader->uiSentLength = 0;
    pstHeader->pcResponBody = pcBody;
    pstHeader->uiSendCurLength = uiSendLen;
    pstHeader->uiSendTotalLength = pstHeader->uiSendCurLength;
    pstHeader->bIsCouldSend = TRUE;

	return;
}

INT HTTP_SetResponseBody( CHAR* pcBuf, UINT uiBodyLen, CHAR* pcBody )
{
	UINT uiLen = 0;
	CHAR* pcLength = NULL;

	if ( pcBuf == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_SetResponseBody pcBuf is NULL.");
		return 0;
	}

	return snprintf(pcBuf, uiBodyLen, "%s", pcBody);
}

