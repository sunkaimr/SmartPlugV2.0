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


const CHAR szHttpCodeMap[][5] =
{
	"200",
	"201",
	"302",
	"400",
	"404",
	"500",

	""
};

const CHAR szHttpStatusMap[][20] =
{
	"OK",
	"Created",
	"Found",
	"BadRequest",
	"NotFound",
	"InternalServerError",

	""
};

const CHAR szHttpMethodStr[][10] =
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

const CHAR szHttpContentTypeStr[][25] =
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

const CHAR szHttpEncodingStr[][10] =
{
	"gzip",

	""
};

const CHAR szHttpCacheControlStr[][20] =
{
	"no-cache",
	"max-age=3600",
	"max-age=86400",
	"max-age=604800",
	"max-age=2592000",
	"max-age=31536000",

	""
};

const CHAR aGzipSuffix[HTTP_ENCODING_Buff+1][5] = {".gz",""};

UINT HTTP_RequestInit( HTTP_CTX *pstCtx );

INT32 HTTP_ParsingHttpHead( HTTP_CTX *pstCtx, CHAR * pcData, UINT32 uiLen )
{
	CHAR *pcCurPos = NULL;
	CHAR *pcTmpPos = NULL;
	CHAR *pcPos = NULL;
	INT32 iLoop = 0;
	int i = 0;

	if ( NULL == pcData || NULL == pstCtx)
	{
		LOG_OUT( LOGOUT_ERROR, "pcData:%p, pstCtx:%p", pcData, pstCtx);
		return FAIL;
	}

	//正在接收body体
	if ( pstCtx->stReq.eProcess == HTTP_PROCESS_GetBody ||
		 pstCtx->stReq.eProcess == HTTP_PROCESS_GotHeader )
	{
		//LOG_OUT(LOGOUT_DEBUG, "Getting Body...");
		if ( pstCtx->stReq.eProcess == HTTP_PROCESS_GotHeader )
		{
			pstCtx->stReq.eProcess = HTTP_PROCESS_GetBody;
		}

		pstCtx->stReq.uiRecvCurLen = uiLen;
		pstCtx->stReq.pcResqBody = pcData;
		pstCtx->stReq.uiRecvLen += uiLen;
		if ( pstCtx->stReq.uiRecvLen >= pstCtx->stReq.uiRecvTotalLen )
		{
			pstCtx->stReq.eProcess = HTTP_PROCESS_Finished;
			//LOG_OUT(LOGOUT_DEBUG, "HTTP_PROCESS_Finished.");
			return OK;
		}
		//LOG_OUT(LOGOUT_DEBUG, "HTTP_PROCESS_Finished.");
		return OK;
	}
	else if ( pstCtx->stReq.eProcess != HTTP_PROCESS_None )
	{
		HTTP_RequestInit( pstCtx );
		return FAIL;
	}

	pcData[uiLen] = '\r';
	pcData[uiLen+1] = '\n';
	pcData[uiLen+2] = 0;
	pcPos = pcCurPos = pcData;

	//LOG_OUT(LOGOUT_DEBUG, "\ndata:[%s]\n", pcData);
	HTTP_RequestInit( pstCtx );

	/* 逐行解析 */
	while ( pcData != NULL )
	{
		//LOG_OUT(LOGOUT_DEBUG, "pcData：[%s]", pcData);
		pcCurPos = strsep(&pcData, STRING_ENTER);
		if ( NULL == pcCurPos )
		{
			//LOG_OUT(LOGOUT_DEBUG, "continue");
			continue;
		}
		//LOG_OUT(LOGOUT_DEBUG, "pcCurPos：[%s]", pcCurPos);

		//header结束
		if ( strlen(pcCurPos) <= 1 && pstCtx->stReq.uiRecvTotalLen != 0 &&
		   ( pstCtx->stReq.eMethod == HTTP_METHOD_POST ||
			 pstCtx->stReq.eMethod == HTTP_METHOD_PUT ))
		{
			pcCurPos = pcCurPos + 2;
			pstCtx->stReq.pcResqBody = pcCurPos;
			pstCtx->stReq.uiRecvCurLen = uiLen - (pcCurPos - pcPos);
			pstCtx->stReq.uiRecvLen += pstCtx->stReq.uiRecvCurLen;

			if ( pstCtx->stReq.uiRecvCurLen == 0 )
			{
				//LOG_OUT(LOGOUT_DEBUG, "HTTP_PROCESS_GotHeader.");
				pstCtx->stReq.eProcess = HTTP_PROCESS_GotHeader;
				return OK;
			}

			if ( pstCtx->stReq.uiRecvLen >= pstCtx->stReq.uiRecvTotalLen )
			{
				//LOG_OUT(LOGOUT_DEBUG, "HTTP_PROCESS_Finished.");
				pstCtx->stReq.eProcess = HTTP_PROCESS_Finished;
				return OK;
			}
			pstCtx->stReq.eProcess = HTTP_PROCESS_GetBody;
			//LOG_OUT(LOGOUT_DEBUG, "HTTP_PROCESS_GetBody.");
			return OK;
		}

		/* GET / HTTP/1.1 : eMethod, URL*/
		if ( HTTP_METHOD_BUFF == pstCtx->stReq.eMethod )
		{
			//解析eMethod
			//LOG_OUT(LOGOUT_DEBUG, "eMethod：[%s]", pcCurPos);
			for ( iLoop = HTTP_METHOD_GET; iLoop < HTTP_METHOD_BUFF; iLoop++ )
			{
				if ( strstr( pcCurPos, szHttpMethodStr[iLoop]) )
				{
					pstCtx->stReq.eMethod = iLoop;
					//LOG_OUT(LOGOUT_DEBUG, "eMethod：[%d]", pstCtx->stReq.eMethod);
				}
			}

			/* 解析URL */
			if ( pstCtx->stReq.eMethod != HTTP_METHOD_BUFF && NULL != (pcTmpPos = strstr( pcCurPos, "/" )))
			{
				while( *pcTmpPos != ' ' )
				{
					pcTmpPos++;
				}
				pcTmpPos[0] = 0;
				//LOG_OUT(LOGOUT_DEBUG, "pcTmpPos:[%s]", strstr( pcCurPos, "/" ));
				strncpy( pstCtx->stReq.szURL, strstr( pcCurPos, "/" ), sizeof(pstCtx->stReq.szURL));
			}

			if ( pstCtx->stReq.eMethod == HTTP_METHOD_BUFF || pstCtx->stReq.szURL[0] == 0 )
			{
				//LOG_OUT( LOGOUT_ERROR, "HTTP_ParsingHttpHead get eMethod or URL failed.");
				HTTP_RequestInit( pstCtx );
				return FAIL;
			}
		}
		/* Host */
		else if ( NULL != (pcTmpPos = strstr( pcCurPos, "Host: " )))
		{
		    pcTmpPos = pcTmpPos + 6;
			strncpy( pstCtx->stReq.szHost, pcTmpPos, sizeof(pstCtx->stReq.szHost));
		}
		/* eUserAgent */
		else if ( strstr( pcCurPos, "User-Agent: " ))
		{
			//LOG_OUT(LOGOUT_DEBUG, "eUserAgent：[%s]", pcCurPos);
			for ( iLoop = HTTP_USERAGENT_WINDOWNS; iLoop < HTTP_USERAGENT_BUFF; iLoop++ )
			{
				if ( strstr( pcCurPos, szHttpUserAgentStringmap[iLoop]) )
				{
					pstCtx->stReq.eUserAgent = iLoop;
					break;
				}
			}
		}
		//curl命令等工具 发送的字段内容长度字段是大写字母开头"Content-Length"
		else if ( NULL != (pcTmpPos = strstr( pcCurPos, "Content-Length: " )))
		{
		    pcTmpPos = pcTmpPos + 16;
		    //LOG_OUT(LOGOUT_DEBUG, "Content-Length:[%s]", pcTmpPos);
		    pstCtx->stReq.uiRecvTotalLen = atoi(pcTmpPos);
		}
		//postman 发送的字段内容长度字段是小写字母"content-length"
		else if ( NULL != (pcTmpPos = strstr( pcCurPos, "content-length: " )))
		{
		    pcTmpPos = pcTmpPos + 16;
		    //LOG_OUT(LOGOUT_DEBUG, "Content-Length:[%s]", pcTmpPos);
		    pstCtx->stReq.uiRecvTotalLen = atoi(pcTmpPos);
		}
	}
	//LOG_OUT(LOGOUT_DEBUG, "HTTP_PROCESS_GotHeader.");
	pstCtx->stReq.eProcess = HTTP_PROCESS_GotHeader;
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
	}
}

UINT HTTP_RouterRegiste( UINT uiMethod, CHAR* pcUrl, VOID* pfFunc, CHAR* pcFunStr)
{
	UINT uiLoop = 0;

	if ( uiMethod >= HTTP_METHOD_BUFF )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_RouterRegiste uiMethod:%d", uiMethod);
		return FAIL;
	}

	if ( pcUrl == NULL ||  pfFunc == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_RouterRegiste pcUrl or pfFunc is NULL, pcUrl:%d,pfFunc:%d.", pcUrl, pfFunc);
		return FAIL;
	}

	//判断是否注册过，若注册过进行更新
	for ( uiLoop = 0; uiLoop < HTTP_ROUTER_MAP_MAX; uiLoop++ )
	{
		if ( stHttpRouterMap[uiLoop].eMethod == uiMethod &&
			 strcmp(stHttpRouterMap[uiLoop].szURL, pcUrl ) == 0 )
		{
			stHttpRouterMap[uiLoop].pfHttpHandler = pfFunc;
			return OK;
		}
	}

	for ( uiLoop = 0; uiLoop < HTTP_ROUTER_MAP_MAX; uiLoop++ )
	{
		if ( stHttpRouterMap[uiLoop].eMethod == HTTP_METHOD_BUFF )
		{
			stHttpRouterMap[uiLoop].eMethod = uiMethod;
			stHttpRouterMap[uiLoop].pfHttpHandler = pfFunc;
			strncpy(stHttpRouterMap[uiLoop].szURL, pcUrl, HTTP_URL_MAX_LEN);
			break;
		}
	}

	if ( uiLoop >= HTTP_ROUTER_MAP_MAX )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_RouterRegiste stHttpRouterMap is full, num:%d", HTTP_ROUTER_MAP_MAX);
		return FAIL;
	}

	return OK;
}


VOID HTTP_RouterInit( VOID )
{
	HTTP_RouterMapInit();

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/", 				HTTP_GetHome, 		"home");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/health", 		HTTP_GetHealth, 	"HTTP_GetHealth");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/info", 			HTTP_GetInfo, 		"info");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/timer/:timer",	HTTP_GetTimerData,	"HTTP_GetTimerData");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/delay/:delay",	HTTP_GetDelayData,	"HTTP_GetDelayData");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/infrared/:infrared",
															HTTP_GetInfraredData,"HTTP_GetInfraredData");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/infrared/:infrared/switch/:switch",
															HTTP_GetInfraredValue,"HTTP_GetInfraredValue");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/system",			HTTP_GetSystemData,	"HTTP_GetSystemData");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/cloudplatform",	HTTP_GetCloudPlatformData,	"HTTP_GetCloudPlatformData");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/temperature",	HTTP_GetTemperature,	"HTTP_GetTemperature");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/html/header",	HTTP_GetHtmlHeader,	"HTTP_GetHtmlHeader");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/html/header",	HTTP_PostHtmlHeader,"HTTP_PostHtmlHeader");
	HTTP_RouterRegiste(HTTP_METHOD_PUT,  "/html/:html",		HTTP_PutHtml,		"HTTP_PutHtml");

	HTTP_RouterRegiste(HTTP_METHOD_POST, "/timer",	   		HTTP_PostTimerData,	"HTTP_PostTimerData");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/delay",	   		HTTP_PostDelayData,	"HTTP_PostDelayData");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/infrared",		HTTP_PostInfraredData,	"HTTP_PostInfraredData");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/system",	   		HTTP_PostSystemData,	"HTTP_PostSystemData");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/cloudplatform",	HTTP_PostCloudPlatformData,	"HTTP_PostCloudPlatformData");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/webset",			HTTP_PostWebSet,		"HTTP_PostWebSet");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/webset",			HTTP_GetWebSet,			"HTTP_GetWebSet");

	HTTP_RouterRegiste(HTTP_METHOD_POST, "/control",	   	HTTP_PostDeviceControl,	"HTTP_PostDeviceControl");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/scanwifi",	   	HTTP_GetScanWifi,		"HTTP_GetScanWifi");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/relaystatus",	HTTP_GetRelayStatus,	"HTTP_GetRelayStatus");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/relaystatus",	HTTP_PostRelayStatus,	"HTTP_PostRelayStatus");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/date",	    	HTTP_GetDate,		"HTTP_GetDate");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/date",	    	HTTP_PostDate,		"HTTP_PostDate");

	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/meter",	    	HTTP_GetMeter,		"HTTP_GetDate");
	HTTP_RouterRegiste(HTTP_METHOD_POST, "/meter",	    	HTTP_PostMeter,		"HTTP_GetDate");

	HTTP_RouterRegiste(HTTP_METHOD_PUT,  "/upgrade",	    HTTP_PutUpgrade,	"HTTP_PutUpgrade");
	HTTP_RouterRegiste(HTTP_METHOD_GET,  "/upload",    		HTTP_GetUploadHtml,		"HTTP_GetUploadHtml");

	HTTP_FileListRegiste();
}

BOOL HTTP_RouterIsMatch( const CHAR* pcRouter, const CHAR* pcUrl)
{
	CHAR pcRBuf[HTTP_URL_MAX_LEN] = { 0 };
	CHAR pcUBuf[HTTP_URL_MAX_LEN] = { 0 };
	CHAR *pcRData, *pcUData = NULL;
	CHAR *pcRtmp, *pcUtmp = NULL;

	if (pcRouter == NULL || pcUrl == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pcRouter:%p, pcUrl:%p.", pcRouter, pcUrl);
		return FALSE;
	}

	if ( strcmp(pcRouter, pcUrl) == 0 )
	{
		return TRUE;
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
			return FALSE;
		}

		if ( NULL != strstr(pcRtmp, ":") )
		{
			continue;
		}

		if ( strcmp(pcRtmp, pcUtmp) != 0 )
		{
			return FALSE;
		}
	}

	if (pcRData == NULL && pcUData == NULL )
	{
		return TRUE;
	}

	return FALSE;
}

UINT HTTP_GetRouterPara( HTTP_CTX *pstCtx, const CHAR* pcKey, CHAR* pcValue )
{
	CHAR pcRBuf[HTTP_URL_MAX_LEN];
	CHAR pcUBuf[HTTP_URL_MAX_LEN];
	CHAR *pcRData, *pcUData = NULL;
	CHAR *pcRtmp, *pcUtmp = NULL;

	if ( pstCtx == NULL || pcKey == NULL || pcValue == NULL ||
		 pstCtx->stReq.pcRouter == NULL || pstCtx->stReq.szURL == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pstCtx:%p, pcKey:%p, pcValue:%p, pcRouter:%p, szURL:%p",
	    		pstCtx, pcKey, pcValue, pstCtx->stReq.pcRouter, pstCtx->stReq.szURL);
	    return FAIL;
	}

	strncpy(pcRBuf, pstCtx->stReq.pcRouter,	HTTP_URL_MAX_LEN);
	strncpy(pcUBuf, pstCtx->stReq.szURL, 	HTTP_URL_MAX_LEN);

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
				return OK;
			}
		}
	}
	return FAIL;
}

UINT HTTP_RequestInit( HTTP_CTX *pstCtx )
{
	if ( pstCtx == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstCtx:%p", pstCtx);
		return FAIL;
	}

	memset(&pstCtx->stReq, 0, sizeof(HTTP_REQ_S));
	pstCtx->stReq.eMethod  = HTTP_METHOD_BUFF;

	return OK;
}

UINT HTTP_ResponInit( HTTP_CTX *pstCtx )
{
	if ( pstCtx == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstCtx:%p", pstCtx);
		return FAIL;
	}

	FREE_MEM(pstCtx->stResp.pcResponBody);
	memset(&pstCtx->stResp, 0, sizeof(pstCtx->stResp));

	return OK;
}

UINT HTTP_RouterHandle( HTTP_CTX *pstCtx )
{
	UINT uiLoop = 0;

	if ( NULL == pstCtx )
	{
		LOG_OUT( LOGOUT_ERROR, "pstCtx:%p", pstCtx);
		return FAIL;
	}

	HTTP_ResponInit(pstCtx);

	if ( pstCtx->stReq.eProcess == HTTP_PROCESS_None ||
		 pstCtx->stReq.eProcess == HTTP_PROCESS_Invalid )
	{
		LOG_OUT(LOGOUT_ERROR, "eProcess:%d", pstCtx->stReq.eProcess);
		return FAIL;
	}

	if ( (pstCtx->stReq.eMethod == HTTP_METHOD_POST ||
		  pstCtx->stReq.eMethod == HTTP_METHOD_PUT ) &&
		  pstCtx->stReq.eProcess == HTTP_PROCESS_GotHeader )
	{
		//LOG_OUT( LOGOUT_DEBUG, "got header");
		return OK;
	}

	if ( pstCtx->stReq.szURL[0] == 0 )
	{
		LOG_OUT(LOGOUT_INFO, "szURL is NULL");
		return FAIL;
	}

	for ( uiLoop = 0; uiLoop < HTTP_ROUTER_MAP_MAX; uiLoop++ )
	{
		if ( stHttpRouterMap[uiLoop].eMethod == pstCtx->stReq.eMethod &&
			 HTTP_RouterIsMatch(stHttpRouterMap[uiLoop].szURL, pstCtx->stReq.szURL) )
		{
			if ( pstCtx->stReq.uiRecvCurLen == pstCtx->stReq.uiRecvLen )
			{
				LOG_OUT(LOGOUT_INFO, "[Request] Method:%s URL:%s",
						szHttpMethodStr[pstCtx->stReq.eMethod],
						pstCtx->stReq.szURL);
			}
			pstCtx->stReq.pcRouter = stHttpRouterMap[uiLoop].szURL;
			return stHttpRouterMap[uiLoop].pfHttpHandler(pstCtx);
		}
	}
	if ( uiLoop >= HTTP_ROUTER_MAP_MAX )
	{
		LOG_OUT(LOGOUT_INFO, "[Request] Method:%s URL:%s",
				szHttpMethodStr[pstCtx->stReq.eMethod],
				pstCtx->stReq.szURL);

		return HTTP_NotFound( pstCtx );
	}

	if ( HTTP_IS_SEND_FINISH( pstCtx ) )
	{
		HTTP_RequestInit( pstCtx );
	}

	return OK;
}

UINT HTTP_SetHeader( HTTP_CTX *pstCtx )
{
	if ( pstCtx->stResp.eHttpCode >= HTTP_CODE_Buff )
	{
		LOG_OUT(LOGOUT_ERROR, "uiHttpCode unknown, %d.", pstCtx->stResp.eHttpCode);
		return FAIL;
	}

	if ( pstCtx->stResp.eContentType >= HTTP_CONTENT_TYPE_Buff )
	{
		LOG_OUT(LOGOUT_ERROR, "ContentType unknown, %d.", pstCtx->stResp.eContentType);
		return FAIL;
	}

	if (pstCtx->stResp.eCacheControl >= HTTP_CACHE_CTL_TYPE_Buff )
	{
		LOG_OUT(LOGOUT_ERROR, "CacheControl unknown, %d.", pstCtx->stResp.eCacheControl);
		return FAIL;
	}

	pstCtx->stResp.uiPos += snprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
			"HTTP/1.1 %s %s \r\n",
			szHttpCodeMap[pstCtx->stResp.eHttpCode],
			szHttpStatusMap[pstCtx->stResp.eHttpCode] );

	pstCtx->stResp.uiPos += snprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
			"Accept-Ranges: bytes \r\n");

	pstCtx->stResp.uiPos += snprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
			"Content-Type: %s \r\n", szHttpContentTypeStr[pstCtx->stResp.eContentType]);

	pstCtx->stResp.uiPos += snprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
			"Cache-Control: %s \r\n", szHttpCacheControlStr[pstCtx->stResp.eCacheControl]);

	pstCtx->stResp.uiPos += snprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
			"Connection: Keep-Alive \r\n");

	pstCtx->stResp.uiPos += snprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
				"Content-Length:           \r\n\r\n");

	pstCtx->stResp.uiHeaderLen = pstCtx->stResp.uiPos;
	return OK;
}

UINT HTTP_SetCustomHeader( HTTP_CTX *pstCtx, CHAR* pcKey, CHAR *pcValues, ... )
{
	va_list Arg;

	if ( pstCtx == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstCtx is NULL.");
		return FAIL;
	}

	//先填充header再填充body，body一旦填充就不能再设置header了
	if ( pstCtx->stResp.uiBodyLen != 0 )
	{
		LOG_OUT(LOGOUT_ERROR, "response body has be set, Cannot set herder.");
		return FAIL;
	}

	pstCtx->stResp.uiPos -= 2;

	pstCtx->stResp.uiPos += snprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
			"%s: ", pcKey);

	va_start(Arg, pcValues);
	pstCtx->stResp.uiPos += vsnprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
			pcValues, Arg);
	va_end(Arg);

	pstCtx->stResp.uiPos += snprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
			"\r\n\r\n");

	pstCtx->stResp.uiHeaderLen = pstCtx->stResp.uiPos;

	return OK;
}


UINT HTTP_SetBodyLen( HTTP_CTX *pstCtx, UINT uiBodyLen )
{
	UINT uiLen = 0;
	CHAR* pcPos = NULL;

	if ( pstCtx == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstCtx is NULL.");
		return FAIL;
	}

	pcPos = strstr(pstCtx->stResp.pcResponBody, "Content-Length: ");
	if ( pcPos != NULL )
	{
		uiLen = sprintf( pcPos, "Content-Length: %d", uiBodyLen );
		*(pcPos + uiLen) = ' ';
	}

	return OK;
}

UINT HTTP_SendOnce( HTTP_CTX *pstCtx )
{
	UINT uiRet = 0;
	UINT uiLen = 0;
	CHAR* pcLength = NULL;

	if ( pstCtx == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstCtx is NULL.");
		return FAIL;
	}

	if( pstCtx->stResp.uiBodyLen != pstCtx->stResp.uiPos - pstCtx->stResp.uiHeaderLen )
	{
		pstCtx->stResp.uiBodyLen = pstCtx->stResp.uiPos - pstCtx->stResp.uiHeaderLen;
	}

	pcLength = strstr(pstCtx->stResp.pcResponBody, "Content-Length: ");
	if ( pcLength != NULL )
	{
		uiLen = sprintf( pcLength, "Content-Length: %d", pstCtx->stResp.uiBodyLen );
		*(pcLength + uiLen) = ' ';
	}

	pstCtx->stResp.uiSendCurLen		= pstCtx->stResp.uiPos;
	pstCtx->stResp.uiSendTotalLen	= pstCtx->stResp.uiSendCurLen;

	uiRet = WEB_WebSend(pstCtx);
	if ( uiRet != OK )
	{
		LOG_OUT(LOGOUT_ERROR, "send failed");
		return FAIL;
	}

#if 0
	LOG_OUT( LOGOUT_INFO, "eHttpCode:%d", 		pstCtx->stResp.eHttpCode);
	LOG_OUT( LOGOUT_INFO, "eContentType:%d", 	pstCtx->stResp.eContentType);
	LOG_OUT( LOGOUT_INFO, "eCacheControl:%d", 	pstCtx->stResp.eCacheControl);
	LOG_OUT( LOGOUT_INFO, "uiBodyLen;%d", 		pstCtx->stResp.uiBodyLen);
	LOG_OUT( LOGOUT_INFO, "uiHeaderLen:%d", 	pstCtx->stResp.uiHeaderLen);
	LOG_OUT( LOGOUT_INFO, "uiSendCurLen:%d", 	pstCtx->stResp.uiSendCurLen);
	LOG_OUT( LOGOUT_INFO, "uiPos:%d", 			pstCtx->stResp.uiPos);
	LOG_OUT( LOGOUT_INFO, "uiSendBufLen:%d", 	pstCtx->stResp.uiSendBufLen);
	LOG_OUT( LOGOUT_INFO, "uiSendTotalLen:%d", pstCtx->stResp.uiSendTotalLen);
	LOG_OUT( LOGOUT_INFO, "uiSentLen:%d", 		pstCtx->stResp.uiSentLen);
#endif
	return OK;
}



UINT HTTP_SetResponseBody( HTTP_CTX *pstCtx, CHAR* pcBody )
{
	UINT uiLen = 0;
	CHAR* pcLength = NULL;

	if ( pstCtx == NULL || pcBody == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstCtx:%p, pcBody:%p", pstCtx, pcBody);
		return FAIL;
	}

	pstCtx->stResp.uiBodyLen = snprintf(
			pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
			pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
			pcBody);

	pstCtx->stResp.uiPos += pstCtx->stResp.uiBodyLen;

	return OK;
}



