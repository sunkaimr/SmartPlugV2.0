/*
 * user_http.c
 *
 *  Created on: 2018年11月11日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"
#include "lwip/netdb.h"

const CHAR* HttpStatus_Ok                     = "OK";
const CHAR* HttpStatus_Created                = "Created";
const CHAR* HttpStatus_Found                  = "Found";
const CHAR* HttpStatus_BadRequest             = "BadRequest";
const CHAR* HttpStatus_NotFound               = "NotFound";
const CHAR* HttpStatus_InternalServerError    = "InternalServerError";

const CHAR* STRING_SPACE = " ";
const CHAR* STRING_ENTER = "\n";

HTTP_ROUTER_MAP_S stHttpRouterMap[HTTP_ROUTER_MAP_MAX];


const CHAR szHttpCodeMap[][5] =
{
	"999",
	"101",
    "200",
    "201",
	"204",
	"206",
    "302",
    "400",
    "404",
    "500",

    ""
};

const CHAR szHttpStatusMap[][20] =
{
	"None",
	"Switching Protocols",
    "OK",
    "Created",
	"No Content",
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

    //header已经解析完成，正在接收body体
    if ( pstCtx->stReq.eProcess == RES_Process_GetBody ||
         pstCtx->stReq.eProcess == RES_Process_GotHeader )
    {
        //LOG_OUT(LOGOUT_DEBUG, "Getting Body...");
        pstCtx->stReq.uiRecvCurLen = uiLen;
        pstCtx->stReq.pcResqBody = pcData;
        pstCtx->stReq.uiRecvLen += uiLen;

        if ( pstCtx->stReq.eProtocol == HTTP_websocket )
        {
        	//LOG_OUT(LOGOUT_DEBUG, "Getting Body...");
        	return OK;
        }

        if ( pstCtx->stReq.eProcess == RES_Process_GotHeader )
        {
            pstCtx->stReq.eProcess = RES_Process_GetBody;
        }

        if ( pstCtx->stReq.uiRecvLen >= pstCtx->stReq.uiRecvTotalLen )
        {
            pstCtx->stReq.eProcess = RES_Process_Finished;
            //LOG_OUT(LOGOUT_DEBUG, "RES_Process_Finished.");
            return OK;
        }
        //LOG_OUT(LOGOUT_DEBUG, "RES_Process_Finished.");
        return OK;
    }
    else if ( pstCtx->stReq.eProcess != RES_Process_None )
    {
    	LOG_OUT(LOGOUT_ERROR, "eProcess = %d.", pstCtx->stReq.eProcess);
        HTTP_RequestInit( pstCtx );
        return FAIL;
    }

    pcData[uiLen] = '\r';
    pcData[uiLen+1] = '\n';
    pcData[uiLen+2] = 0;
    pcPos = pcCurPos = pcData;

    //LOG_OUT(LOGOUT_DEBUG, "\n%s\n", pcData);
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
                //LOG_OUT(LOGOUT_DEBUG, "RES_Process_GotHeader.");
                pstCtx->stReq.eProcess = RES_Process_GotHeader;
                return OK;
            }

            if ( pstCtx->stReq.uiRecvLen >= pstCtx->stReq.uiRecvTotalLen )
            {
                //LOG_OUT(LOGOUT_DEBUG, "RES_Process_Finished.");
                pstCtx->stReq.eProcess = RES_Process_Finished;
                return OK;
            }
            pstCtx->stReq.eProcess = RES_Process_GetBody;
            //LOG_OUT(LOGOUT_DEBUG, "RES_Process_GetBody.");
            return OK;
        }

        /* GET / HTTP/1.1 : eMethod, URL*/
        if ( HTTP_METHOD_BUFF == pstCtx->stReq.eMethod )
        {
            //解析eMethod
            //LOG_OUT(LOGOUT_INFO, "eMethod:[%s]", pcCurPos);
        	for ( ; isspace(*pcCurPos); pcCurPos++){}
        	pcTmpPos = strstr( pcCurPos, " ");
        	if ( pcTmpPos == 0 )
        	{
                LOG_OUT( LOGOUT_ERROR, "Parsing eMethod failed, [%s]", pcCurPos);
                return FAIL;
        	}
        	*pcTmpPos = 0;
        	strupr(pcCurPos);

            for ( iLoop = HTTP_METHOD_GET; iLoop < HTTP_METHOD_BUFF; iLoop++ )
            {
                if ( strstr( pcCurPos, szHttpMethodStr[iLoop]) )
                {
                    pstCtx->stReq.eMethod = iLoop;
                    //LOG_OUT(LOGOUT_DEBUG, "eMethod：[%d]", pstCtx->stReq.eMethod);
                    pcCurPos = pcTmpPos + 1;
                    break;
                }
            }

            /* 解析URL */
            if ( pstCtx->stReq.eMethod != HTTP_METHOD_BUFF && NULL != (pcTmpPos = strstr( pcCurPos, "/" )))
            {
                for ( ; !isspace(*pcTmpPos); pcTmpPos++){}
                *pcTmpPos = 0;
                //LOG_OUT(LOGOUT_DEBUG, "pcTmpPos:[%s]", strstr( pcCurPos, "/" ));
                strncpy( pstCtx->stReq.szURL, strstr( pcCurPos, "/" ), sizeof(pstCtx->stReq.szURL));
            }

            if ( pstCtx->stReq.eMethod == HTTP_METHOD_BUFF || pstCtx->stReq.szURL[0] == 0 )
            {
                LOG_OUT( LOGOUT_ERROR, "get eMethod or URL failed.");
                HTTP_RequestInit( pstCtx );
                return FAIL;
            }
        }
        //curl命令等工具 发送的字段内容长度字段是大写字母开头"Content-Length", postman 发送的字段内容长度字段是小写字母"content-length"
        else if ( NULL != (pcTmpPos = strstr( pcCurPos, "ontent-Length: " )))
        {
            pcTmpPos += 15;
            //LOG_OUT(LOGOUT_DEBUG, "Content-Length:[%s]", pcTmpPos);
            pstCtx->stReq.uiRecvTotalLen = atoi(pcTmpPos);
        }
        else if ( NULL != (pcTmpPos = strstr( pcCurPos, ":" )))
        {
            // 跳过空白字符
        	for ( ; isspace(*pcCurPos); pcCurPos++){}
            *pcTmpPos = 0;
            pcTmpPos++;
            // 跳过空白字符
            for ( ; isspace(*pcTmpPos); pcTmpPos++ ){}
            HTTP_SetReqHeader( &pstCtx->stReq.stHeader[0], pcCurPos, pcTmpPos );
            for ( ; !isspace(*pcTmpPos); pcTmpPos++ ){}
            *pcTmpPos = 0;
        }
    }
    //LOG_OUT(LOGOUT_DEBUG, "RES_Process_GotHeader.");

    pstCtx->stReq.eProcess = RES_Process_GotHeader;
    return OK;
}

UINT HTTP_SetReqHeader( HTTP_HEADER *pstHeader, CHAR* pcKey, CHAR*pcValue )
{
    UINT i = 0;

    if ( pstHeader == NULL ||  pcKey == NULL || pcValue == NULL)
    {
        LOG_OUT(LOGOUT_ERROR, "pstHeader, pcKey or pcValue is NULL, pstCtx:%p pcKey:%p pcValue:%p",
        		pstHeader, pcKey, pcValue);
        return FAIL;
    }

    for ( i = 0; i < HTTP_HRADER_NUM_MAX; i++ )
    {
        if ( pstHeader[i].pcKey == NULL )
        {
        	pstHeader[i].pcKey = pcKey;
        	pstHeader[i].pcValue = pcValue;

            //LOG_OUT(LOGOUT_DEBUG, "pcKey:%s, pcValue:%s", pcKey, pcValue);
            return OK;
        }
    }

    LOG_OUT(LOGOUT_ERROR, "stHeader num is full %d, pcKey:%s pcValue:%s",HTTP_HRADER_NUM_MAX, pcKey, pcValue);
    return FAIL;
}

CHAR* HTTP_GetReqHeader( HTTP_HEADER *pstHeader, const CHAR* pcKey )
{
    UINT i = 0;

    if ( pstHeader == NULL ||  pcKey == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "pstHeader, pcKey is NULL, pstCtx:%p pcKey:%p",pstHeader, pcKey);
        return NULL;
    }

    for ( i = 0; i < HTTP_HRADER_NUM_MAX; i++ )
    {
        if ( pstHeader[i].pcKey == NULL )
		{
        	break;
		}

        if ( strcmp(pstHeader[i].pcKey, pcKey) == 0 )
        {
            return pstHeader[i].pcValue;
        }
    }

    return NULL;
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
        LOG_OUT(LOGOUT_ERROR, "HTTP_RouterRegiste pcUrl or pfFunc is NULL, pcUrl:%d,pfFunc:%d", pcUrl, pfFunc);
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

    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/",                 HTTP_GetHome,       "HTTP_GetHome");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/health",           HTTP_GetHealth,     "HTTP_GetHealth");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/info",             HTTP_GetInfo,       "HTTP_GetInfo");

    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/timer/:timer",     HTTP_GetTimerData,  "HTTP_GetTimerData");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/delay/:delay",     HTTP_GetDelayData,  "HTTP_GetDelayData");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/infrared/:infrared",
                                                              HTTP_GetInfraredData,"HTTP_GetInfraredData");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/infrared/:infrared/switch/:switch",
                                                              HTTP_GetInfraredValue,"HTTP_GetInfraredValue");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/system",           HTTP_GetSystemData,   "HTTP_GetSystemData");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/cloudplatform",    HTTP_GetCloudPlatformData, "HTTP_GetCloudPlatformData");

    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/temperature",      HTTP_GetTemperature, "HTTP_GetTemperature");

    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/html/header",      HTTP_GetHtmlHeader,  "HTTP_GetHtmlHeader");
    HTTP_RouterRegiste(HTTP_METHOD_POST, "/html/header",      HTTP_PostHtmlHeader, "HTTP_PostHtmlHeader");
    HTTP_RouterRegiste(HTTP_METHOD_PUT,  "/html/:html",       HTTP_PutHtml,        "HTTP_PutHtml");

    HTTP_RouterRegiste(HTTP_METHOD_POST, "/timer",            HTTP_PostTimerData,  "HTTP_PostTimerData");
    HTTP_RouterRegiste(HTTP_METHOD_POST, "/delay",            HTTP_PostDelayData,  "HTTP_PostDelayData");
    HTTP_RouterRegiste(HTTP_METHOD_POST, "/infrared",         HTTP_PostInfraredData, "HTTP_PostInfraredData");
    HTTP_RouterRegiste(HTTP_METHOD_POST, "/system",           HTTP_PostSystemData, "HTTP_PostSystemData");
    HTTP_RouterRegiste(HTTP_METHOD_POST, "/cloudplatform",    HTTP_PostCloudPlatformData, "HTTP_PostCloudPlatformData");
    HTTP_RouterRegiste(HTTP_METHOD_POST, "/webset",           HTTP_PostWebSet,      "HTTP_PostWebSet");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/webset",           HTTP_GetWebSet,       "HTTP_GetWebSet");

    HTTP_RouterRegiste(HTTP_METHOD_POST, "/control",          HTTP_PostDeviceControl, "HTTP_PostDeviceControl");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/scanwifi",         HTTP_GetScanWifi,     "HTTP_GetScanWifi");

    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/relaystatus",      HTTP_GetRelayStatus,  "HTTP_GetRelayStatus");
    HTTP_RouterRegiste(HTTP_METHOD_POST, "/relaystatus",      HTTP_PostRelayStatus, "HTTP_PostRelayStatus");

    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/date",             HTTP_GetDate,         "HTTP_GetDate");
    HTTP_RouterRegiste(HTTP_METHOD_POST, "/date",             HTTP_PostDate,        "HTTP_PostDate");

    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/meter",            HTTP_GetMeter,        "HTTP_GetMeter");
    HTTP_RouterRegiste(HTTP_METHOD_POST, "/meter",            HTTP_PostMeter,       "HTTP_PostMeter");

    HTTP_RouterRegiste(HTTP_METHOD_PUT,  "/upgrade",          HTTP_PutUpgrade,      "HTTP_PutUpgrade");
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/upload",           HTTP_GetUploadHtml,   "HTTP_GetUploadHtml");

    // websocket
    HTTP_RouterRegiste(HTTP_METHOD_GET,  "/console",          HTTP_GetConsole,     "HTTP_GetConsole");

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
        LOG_OUT(LOGOUT_ERROR, "pcRouter:%p, pcUrl:%p", pcRouter, pcUrl);
        return FALSE;
    }

    if ( strcmp(pcRouter, pcUrl) == 0 )
    {
        return TRUE;
    }

    strncpy(pcRBuf, pcRouter,    HTTP_URL_MAX_LEN);
    strncpy(pcUBuf, pcUrl,       HTTP_URL_MAX_LEN);

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

        // URL中包含通配符*的判断逻辑
        if ( strstr(pcRtmp, "*") )
        {
            while ( *pcRtmp )
            {
            	// 通配符往后的无需再判断
            	if ( *pcRtmp == '*' )
            	{
            		return TRUE;
            	}

            	if ( *pcRtmp != *pcUtmp )
            	{
            		return FALSE;
            	}

            	pcRtmp++;
    			pcUtmp++;
            }
        }
        else
        {
        	// 没有通配符需要绝对匹配
			if ( strcmp(pcRtmp, pcUtmp) != 0 )
			{
				return FALSE;
			}
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

    strncpy(pcRBuf, pstCtx->stReq.pcRouter,    HTTP_URL_MAX_LEN);
    strncpy(pcUBuf, pstCtx->stReq.szURL,     HTTP_URL_MAX_LEN);

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
    pstCtx->stResp.eHttpCode = HTTP_CODE_Buff;

    return OK;
}

UINT HTTP_RouterHandle( HTTP_CTX *pstCtx )
{
	UINT uiRet = 0;
    UINT uiLoop = 0;
    CHAR *pcValue = NULL;
    UINT i = 0;

    if ( NULL == pstCtx )
    {
        LOG_OUT( LOGOUT_ERROR, "pstCtx:%p", pstCtx);
        uiRet = FAIL;
        goto end;
    }

    if ( pstCtx->stReq.eProcess == RES_Process_None ||
         pstCtx->stReq.eProcess == RES_Process_Invalid )
    {
        LOG_OUT(LOGOUT_ERROR, "eProcess:%d", pstCtx->stReq.eProcess);
        uiRet = FAIL;
        goto end;
    }

    if ( (pstCtx->stReq.eMethod == HTTP_METHOD_POST ||
          pstCtx->stReq.eMethod == HTTP_METHOD_PUT ) &&
          pstCtx->stReq.eProcess == RES_Process_GotHeader )
    {
        uiRet = OK;
        goto end;
    }

    if ( pstCtx->stReq.szURL[0] == 0 )
    {
        LOG_OUT(LOGOUT_INFO, "szURL is NULL");
        uiRet = FAIL;
        goto end;
    }

    if ( pstCtx->stReq.pfHandler != NULL )
    {
    	//LOG_OUT( LOGOUT_DEBUG, "handle...");
    	uiRet = pstCtx->stReq.pfHandler(pstCtx);
        goto end;
    }

    // websocket 协议的特殊处理
	CHAR* pcHeader = NULL;
    pcHeader = HTTP_GetReqHeader(&pstCtx->stReq.stHeader[0], "Upgrade");
    if ( pcHeader != NULL && strcmp(pcHeader, "websocket") == 0 )
    {
    	//3600s无数据交互则断开连接
    	pstCtx->uiTimeOut = 3600;
    	pstCtx->stReq.eProtocol = HTTP_websocket;
    }
    else
    {
    	pstCtx->stReq.eProtocol = HTTP_1_1;
    }

    HTTP_ResponInit(pstCtx);

    pcValue = HTTP_GetReqHeader(&pstCtx->stReq.stHeader[0], "Host");

    if ( pcValue != NULL &&
    	 sscanf(pcValue,"%d.%d.%d.%d",&i,&i,&i,&i) != 4 &&
		 strstr(pstCtx->stReq.szURL, "favicon") == 0 &&
    	 strstr(pstCtx->stReq.szURL, "system") == 0 &&
		 strstr(pstCtx->stReq.szURL, "upload") == 0 &&
		 strstr(pstCtx->stReq.szURL, "html/") == 0 )
    {
    	//HTTP_GetredirectHtml(pstCtx);
    	LOG_OUT(LOGOUT_INFO, "[Request] Method:%s URL:%s -> /redirect.html",
    			szHttpMethodStr[pstCtx->stReq.eMethod],
				pstCtx->stReq.szURL);

    	strcpy(pstCtx->stReq.szURL, "/redirect.html");
    	HTTP_GetRedirectHtml(pstCtx);
    	goto end;
    }

    for ( uiLoop = 0; uiLoop < HTTP_ROUTER_MAP_MAX; uiLoop++ )
    {
        if ( stHttpRouterMap[uiLoop].eMethod == pstCtx->stReq.eMethod &&
             HTTP_RouterIsMatch(stHttpRouterMap[uiLoop].szURL, pstCtx->stReq.szURL) )
        {
            if ( pstCtx->stReq.uiRecvCurLen == pstCtx->stReq.uiRecvLen )
            {
                LOG_OUT(LOGOUT_INFO, "[Request] Method:%s URL:%s", szHttpMethodStr[pstCtx->stReq.eMethod], pstCtx->stReq.szURL);
            }
            pstCtx->stReq.pcRouter = stHttpRouterMap[uiLoop].szURL;
            pstCtx->stReq.pfHandler = stHttpRouterMap[uiLoop].pfHttpHandler;

            uiRet = stHttpRouterMap[uiLoop].pfHttpHandler(pstCtx);
            goto end;
        }
    }

    if ( uiLoop >= HTTP_ROUTER_MAP_MAX )
    {
        LOG_OUT(LOGOUT_INFO, "[Request] Method:%s URL:%s", szHttpMethodStr[pstCtx->stReq.eMethod], pstCtx->stReq.szURL);

        uiRet = HTTP_NotFound( pstCtx );
        goto end;
    }

end:
    if ( HTTP_IS_SEND_FINISH( pstCtx ) )
    {
    	pstCtx->uiTimeOut = WEB_CONTINUE_TMOUT;
        LOG_OUT(LOGOUT_INFO, "[Response] Method:%s URL:%s Code:%s",
                szHttpMethodStr[pstCtx->stReq.eMethod],
                pstCtx->stReq.szURL,
                szHttpCodeMap[pstCtx->stResp.eHttpCode]);
        HTTP_RequestInit( pstCtx );
        HTTP_ResponInit( pstCtx );
    }

    return uiRet;
}

UINT HTTP_SetRespHeader( HTTP_CTX *pstCtx )
{
    if ( pstCtx->stResp.eHttpCode < HTTP_CODE_Buff )
    {
        pstCtx->stResp.uiPos += snprintf(
                pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
                pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
                "HTTP/1.1 %s %s \r\n",
                szHttpCodeMap[pstCtx->stResp.eHttpCode],
                szHttpStatusMap[pstCtx->stResp.eHttpCode] );
    }

    // websocket协议
    if ( pstCtx->stReq.eProtocol == HTTP_websocket )
    {
		pstCtx->stResp.uiPos += snprintf(
				pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
				pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
				"Upgrade: websocket \r\n");

		pstCtx->stResp.uiPos += snprintf(
				pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
				pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
				"Connection: Upgrade \r\n\r\n");
    }
    // http协议
    else
    {
		if ( pstCtx->stResp.eContentType < HTTP_CONTENT_TYPE_Buff )
		{
			pstCtx->stResp.uiPos += snprintf(
					pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
					pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
					"Content-Type: %s \r\n", szHttpContentTypeStr[pstCtx->stResp.eContentType]);
		}

		if (pstCtx->stResp.eCacheControl < HTTP_CACHE_CTL_TYPE_Buff )
		{
			pstCtx->stResp.uiPos += snprintf(
					pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
					pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
					"Cache-Control: %s \r\n", szHttpCacheControlStr[pstCtx->stResp.eCacheControl]);
		}

		pstCtx->stResp.uiPos += snprintf(
				pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
				pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
				"Accept-Ranges: bytes \r\n");

		pstCtx->stResp.uiPos += snprintf(
				pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
				pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
				"Connection: Keep-Alive \r\n");

		pstCtx->stResp.uiPos += snprintf(
				pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
				pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
					"Content-Length:           \r\n\r\n");
    }
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

    pstCtx->stResp.uiSendCurLen   = pstCtx->stResp.uiPos;
    pstCtx->stResp.uiSendTotalLen = pstCtx->stResp.uiSendCurLen;

    //LOG_OUT(LOGOUT_DEBUG, "[%s]", pstCtx->stResp.pcResponBody);

    uiRet = WEB_WebSend(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "send failed");
        return FAIL;
    }

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

HTTP_CLIENT_S* HTTP_NewClient(CHAR* pcMethod, CHAR* pcEndpoint, CHAR* pcUrl, CHAR* pcBody, UINT uiLen)
{
    struct sockaddr_in sAddr;
    struct hostent* ipAddress;
    HTTP_CLIENT_S *pstCli = NULL;
    CHAR* pcPort = NULL;
    INT iRet = -1;
    UINT i = 0;

	if (pcMethod == NULL || pcEndpoint == NULL|| pcUrl == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pcMethod:%p, pcEndpoint:%p, pcUrl:%p",
				pcMethod, pcEndpoint, pcUrl);
		return NULL;
	}

    for ( i = HTTP_METHOD_GET; i < HTTP_METHOD_BUFF; i++ )
    {
        if ( strcmp( pcMethod, szHttpMethodStr[i]) == 0)
        {
            break;
        }
    }
    if ( i >= HTTP_METHOD_BUFF )
    {
		LOG_OUT(LOGOUT_ERROR, " unsupport method: [%s]", pcMethod);
		return NULL;
    }

    sAddr.sin_port = htons(80);
    pcPort = strstr( pcEndpoint, ":");
    if ( pcPort != NULL )
    {
    	sAddr.sin_port = htons(atoi(pcPort));
    	*pcPort = '0';
    }

	pstCli = malloc(sizeof(HTTP_CLIENT_S));
	if ( pstCli == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "malloc pstCli failed");
		return NULL;
	}

	memset(pstCli, 0, sizeof(HTTP_CLIENT_S));

	pstCli->uiTimeOut = 5;
	pstCli->uiCost = 0;
	pstCli->stReq.uiHeadBufLen = 1024;
	pstCli->stReq.uiHeadPos = 0;
	pstCli->stReq.pcBodyBuf = pcBody;
	pstCli->stReq.uiBodyBufLen = uiLen ;

	pstCli->stReson.pcHeadBuf = NULL;
	pstCli->stReson.pcBodyBuf = NULL;
	pstCli->stReson.uiHeadBufLen = 1024;
	pstCli->stReson.uiBodyBufLen = 1024;

	pstCli->stReq.pcHeadBuf = malloc(pstCli->stReq.uiHeadBufLen);
	if ( pstCli->stReq.pcHeadBuf == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "malloc pcBuf failed");
		goto end;
	}

    ipAddress = gethostbyname( pcEndpoint );
    if ( ipAddress == NULL ) {
    	LOG_OUT(LOGOUT_ERROR, "gethostbyname failed, name:%s", pcEndpoint);
        return NULL;
    }

    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = ((struct in_addr*)(ipAddress->h_addr))->s_addr;

    pstCli->iSocket = socket(AF_INET, SOCK_STREAM, 0);
    if ( pstCli->iSocket < 0 )
    {
        BIGIOT_LOG(BIGIOT_ERROR, "socket failed, socket:%d", pstCli->iSocket);
        return NULL;
    }

    iRet = connect(pstCli->iSocket, (struct sockaddr*)&sAddr, sizeof(sAddr));
    if ( iRet < 0)
    {
        BIGIOT_LOG(BIGIOT_ERROR, "connect failed, socket:%d, ret:%d", pstCli->iSocket, iRet);
        close( pstCli->iSocket );
        return NULL;
    }

    if (strlen(pcUrl) == 0)
    {
    	pcUrl = "/";
    }

    pstCli->stReq.uiHeadPos += snprintf(pstCli->stReq.pcHeadBuf + pstCli->stReq.uiHeadPos,
    		pstCli->stReq.uiHeadBufLen - pstCli->stReq.uiHeadPos,
    			"%s ", pcMethod);
    strncpy(pstCli->stReq.pcHeadBuf + pstCli->stReq.uiHeadPos, pcUrl, strlen(pcUrl));
    pstCli->stReq.uiHeadPos += strlen(pcUrl);

    pstCli->stReq.uiHeadPos += snprintf(pstCli->stReq.pcHeadBuf + pstCli->stReq.uiHeadPos,
    		pstCli->stReq.uiHeadBufLen - pstCli->stReq.uiHeadPos,
			" HTTP/1.1\r\n"
			"User-Agent: SmartPlug\r\n"
			"Accept: */*\r\n"
			"Content-Length: %d \r\n"
			"Host: %s\r\n\r\n", uiLen, pcEndpoint);
    return pstCli;

end:
	close( pstCli->iSocket );
	FREE_MEM(pstCli->stReq.pcHeadBuf);
	FREE_MEM(pstCli);
	return NULL;
}


VOID HTTP_DestoryClient(HTTP_CLIENT_S* pstCli)
{
	if ( pstCli == NULL )
	{
		return;
	}

	close( pstCli->iSocket );
	FREE_MEM(pstCli->stReq.pcHeadBuf);
	FREE_MEM(pstCli->stReson.pcBodyBuf);
	FREE_MEM(pstCli->stReson.pcHeadBuf);
	FREE_MEM(pstCli);
}


VOID HTTP_ClientSetHeader(HTTP_CLIENT_S *pstCli, CHAR* pcKey, CHAR* pcValue)
{
	if (pstCli == NULL || pcKey == NULL|| pcValue == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstCli:%p, pcKey:%p, pcValue:%p",
				pstCli, pcKey, pcValue);
		return;
	}

	pstCli->stReq.uiHeadPos -= 2;
	pstCli->stReq.uiHeadPos += snprintf(pstCli->stReq.pcHeadBuf + pstCli->stReq.uiHeadPos,
			pstCli->stReq.uiHeadBufLen - pstCli->stReq.uiHeadPos,
			"%s: %s\r\n\r\n", pcKey, pcValue);
}

UINT HTTP_ClientDoRequest(HTTP_CLIENT_S *pstCli)
{
    struct timeval stTimeOut = {1, 0};
    fd_set stFdWrite;
    INT iRet = -1;
    UINT8 ucRetry = 0;
    CHAR* pcBuf = NULL;
    UINT uiLen = 0;

    for (;;)
    {
		switch(pstCli->stReq.eProcess)
		{
		case CLI_ReqProcess_None:
			pcBuf = pstCli->stReq.pcHeadBuf;
			uiLen = pstCli->stReq.uiHeadPos;
			pstCli->stReq.eProcess = CLI_ReqProcess_SentHeader;
			break;
		case CLI_ReqProcess_SentHeader:
			pcBuf = pstCli->stReq.pcBodyBuf;
			uiLen = pstCli->stReq.uiBodyBufLen;
			pstCli->stReq.eProcess = CLI_ReqProcess_SentBody;
			break;
		case CLI_ReqProcess_SentBody:
			pstCli->stReq.eProcess = CLI_ReqProcess_Finish;
			return OK;
		}

		if ( uiLen == 0 )
		{
			//LOG_OUT(LOGOUT_INFO, "uiLen == 0");
			continue;
		}

		if ( pcBuf == NULL )
		{
			LOG_OUT(LOGOUT_ERROR, "pcBuf is NULL");
			goto end;
		}

retry:
		FD_ZERO( &stFdWrite );
		FD_SET( pstCli->iSocket, &stFdWrite );

		iRet = select( pstCli->iSocket + 1, NULL, &stFdWrite, NULL, &stTimeOut );
		if ( iRet < 0 )
		{
			LOG_OUT(LOGOUT_ERROR, "select error, errno:%d, iRet:%d", errno, iRet);
			goto end;
		}
		//等待接收超时
		else if ( 0 == iRet )
		{
			pstCli->uiCost++;
			if ( pstCli->uiCost > pstCli->uiTimeOut )
			{
				LOG_OUT(LOGOUT_ERROR, "send timeout, errno:%d", errno);
				goto end;
			}
			goto retry;
		}

		if ( !FD_ISSET(pstCli->iSocket, &stFdWrite ))
		{
			ucRetry ++;
			if ( ucRetry >= 10 )
			{
				LOG_OUT(LOGOUT_ERROR, "FdWrite error, errno:%d", errno);
				goto end;
			}
			LOG_OUT(LOGOUT_DEBUG, "FdWrite error, errno", errno);
			goto retry;
		}

		pstCli->uiCost = 0;

		//puts(pcBuf);

		iRet = send(pstCli->iSocket, pcBuf, uiLen, 0);
		if (iRet <= 0)
		{
			LOG_OUT(LOGOUT_ERROR, "send failed, errno:%d", errno);
			goto end;
		}
		else if ( iRet != uiLen )
		{
			LOG_OUT(LOGOUT_ERROR, "should send %d, but send %d actually", uiLen, iRet);
			goto end;
		}
    }

end:
	pstCli->uiCost = 0;
	return FAIL;
}


INT32 HTTP_ClientParseResponseHead( HTTP_CLIENT_S *pstCli, CHAR * pcData, UINT32 uiLen )
{
    CHAR *pcCurPos = NULL;
    CHAR *pcTmpPos = NULL;
    CHAR *pcPos = NULL;
    INT32 iLoop = 0;
    int i = 0;

    if ( NULL == pstCli || NULL == pcData)
    {
        LOG_OUT( LOGOUT_ERROR, "pstCli:%p, pcData:%p", pstCli, pcData);
        return FAIL;
    }

    //header已经解析完成，正在接收body体
    if ( pstCli->stReson.eProcess == RES_Process_GetBody ||
    	 pstCli->stReson.eProcess == RES_Process_GotHeader )
    {
        //LOG_OUT(LOGOUT_DEBUG, "Getting Body...");
        pstCli->stReson.uiRecvCurLen = uiLen;
        pstCli->stReson.pcBody = pcData;
        pstCli->stReson.uiRecvLen += uiLen;

        if ( pstCli->stReson.eProcess == RES_Process_GotHeader )
        {
        	pstCli->stReson.eProcess = RES_Process_GetBody;
        }

        if ( pstCli->stReson.uiRecvLen >= pstCli->stReson.uiRecvTotalLen )
        {
        	pstCli->stReson.eProcess = RES_Process_Finished;
            //LOG_OUT(LOGOUT_DEBUG, "RES_Process_Finished.");
            return OK;
        }
        //LOG_OUT(LOGOUT_DEBUG, "RES_Process...");
        return OK;
    }
    else if ( pstCli->stReson.eProcess != RES_Process_None )
    {
    	LOG_OUT(LOGOUT_ERROR, "eProcess = %d.", pstCli->stReson.eProcess);
        return FAIL;
    }

    pcData[uiLen] = '\r';
    pcData[uiLen+1] = '\n';
    pcData[uiLen+2] = 0;
    pcPos = pcCurPos = pcData;

    /* 逐行解析 */
    while ( pcData != NULL )
    {
        //LOG_OUT(LOGOUT_DEBUG, "pcData:[%s]", pcData);
        pcCurPos = strsep(&pcData, STRING_ENTER);
        if ( NULL == pcCurPos )
        {
            LOG_OUT(LOGOUT_DEBUG, "continue");
            continue;
        }
        //LOG_OUT(LOGOUT_DEBUG, "pcCurPos:[%s]", pcCurPos);

        //header结束
        if ( strlen(pcCurPos) <= 1 && pstCli->stReson.uiRecvTotalLen != 0 )
        {
            pcCurPos = pcCurPos + 2;
            pstCli->stReson.pcBody = pcCurPos;
            pstCli->stReson.uiRecvCurLen = uiLen - (pcCurPos - pcPos);
            pstCli->stReson.uiRecvLen += pstCli->stReson.uiRecvCurLen;

            if ( pstCli->stReson.uiRecvCurLen == 0 )
            {
                //LOG_OUT(LOGOUT_DEBUG, "RES_Process_GotHeader.");
            	pstCli->stReson.eProcess = RES_Process_GotHeader;
                return OK;
            }

            if ( pstCli->stReson.uiRecvLen >= pstCli->stReson.uiRecvTotalLen )
            {
                //LOG_OUT(LOGOUT_DEBUG, "RES_Process_Finished.");
            	pstCli->stReson.eProcess = RES_Process_Finished;
                return OK;
            }
            pstCli->stReson.eProcess = RES_Process_GetBody;
            //LOG_OUT(LOGOUT_DEBUG, "RES_Process_GetBody.");
            return OK;
        }

        /* HTTP/1.1 200 OK*/
        if ( HTTP_CODE_None == pstCli->stReson.eHttpCode )
        {
            //解析eHttpCode
        	//LOG_OUT( LOGOUT_ERROR, "[%s]", pcCurPos);
            for ( iLoop = HTTP_CODE_Ok; iLoop < HTTP_CODE_Buff; iLoop++ )
            {
                if ( strstr( pcCurPos, szHttpCodeMap[iLoop]) )
                {
                	pstCli->stReson.eHttpCode = iLoop;
                    pcCurPos = pcTmpPos + 1;
                    break;
                }
            }

           	if ( iLoop >= HTTP_CODE_Buff )
			{
				LOG_OUT( LOGOUT_ERROR, "Parsing eHttpCode failed, [%s]", pcCurPos);
				return FAIL;
			}
        }
        //curl命令等工具 发送的字段内容长度字段是大写字母开头"Content-Length", postman 发送的字段内容长度字段是小写字母"content-length"
        else if ( NULL != (pcTmpPos = strstr( pcCurPos, "ontent-Length: " )))
        {
            pcTmpPos += 15;
            pstCli->stReson.uiRecvTotalLen = atoi(pcTmpPos);
            pstCli->stReson.uiContentLength = pstCli->stReson.uiRecvTotalLen;
        }
        else if ( NULL != (pcTmpPos = strstr( pcCurPos, ":" )))
        {
            // 跳过空白字符
        	for ( ; isspace(*pcCurPos); pcCurPos++){}
            *pcTmpPos = 0;
            pcTmpPos++;
            // 跳过空白字符
            for ( ; isspace(*pcTmpPos); pcTmpPos++ ){}
            HTTP_SetReqHeader( &pstCli->stReson.stHeader[0], pcCurPos, pcTmpPos );
            //LOG_OUT(LOGOUT_DEBUG, "pcKey:%s, pcValue:%s", pcCurPos, pcTmpPos);
            for ( ; !isspace(*pcTmpPos); pcTmpPos++ ){}
            *pcTmpPos = 0;
        }
    }

    pstCli->stReson.eProcess = RES_Process_GotHeader;
    return OK;
}

UINT HTTP_ClientDoResponse(HTTP_CLIENT_S *pstCli, HttpClientHandle pfHandle, VOID* pPara)
{
    struct timeval stTimeOut = {1, 0};
    fd_set stFdRead;
    INT iRet = -1;
    UINT8 ucRetry = 0;
    UINT uiRecvBufLen = 0;
    CHAR* pcRecvBuf = NULL;

    pstCli->stReson.pcHeadBuf = ( CHAR* )malloc( pstCli->stReson.uiHeadBufLen + 4);
    if ( NULL == pstCli->stReson.pcHeadBuf )
    {
        LOG_OUT(LOGOUT_ERROR, "malloc pstCli->stReson.pcBuf failed.");
        goto end;
    }

    pstCli->stReson.pcBodyBuf = ( CHAR* )malloc( pstCli->stReson.uiBodyBufLen + 4);
    if ( NULL == pstCli->stReson.pcBodyBuf )
    {
        LOG_OUT(LOGOUT_ERROR, "malloc pstCli->stReson.pcBodyBuf failed.");
        goto end;
    }

    for(;;)
    {
retry:
		FD_ZERO( &stFdRead );
		FD_SET( pstCli->iSocket, &stFdRead );
		iRet = select( pstCli->iSocket + 1, &stFdRead, NULL, NULL, &stTimeOut );
		if ( iRet < 0 )
		{
			LOG_OUT(LOGOUT_ERROR, "select error, errno:%d, iRet:%d", errno, iRet);
			goto end;
		}
		//等待接收超时
		else if ( 0 == iRet )
		{
			pstCli->uiCost++;
			if ( pstCli->uiCost > pstCli->uiTimeOut )
			{
				LOG_OUT(LOGOUT_ERROR, "recv timeout, errno:%d", errno);
				pstCli->uiCost = 0;
				return -2;
			}
			LOG_OUT(LOGOUT_DEBUG, "select timeout, count: %d", pstCli->uiCost);
			goto retry;
		}
		if ( !FD_ISSET(pstCli->iSocket, &stFdRead ))
		{
			ucRetry ++;
			if ( ucRetry >= 10 )
			{
				LOG_OUT(LOGOUT_ERROR, "stFdRead error, errno:%d", errno);
				goto end;
			}
			LOG_OUT(LOGOUT_DEBUG, "stFdRead error, errno", errno);
			goto retry;
		}
		pstCli->uiCost = 0;

		if ( pstCli->stReson.eProcess == CLI_ResponProcess_None )
		{
			pcRecvBuf = pstCli->stReson.pcHeadBuf;
			uiRecvBufLen = pstCli->stReson.uiHeadBufLen;
		}
		else
		{
			pcRecvBuf = pstCli->stReson.pcBodyBuf;
			uiRecvBufLen = pstCli->stReson.uiBodyBufLen;
		}

		iRet = recv( pstCli->iSocket, pcRecvBuf, uiRecvBufLen, 0 );
		if ( iRet <= 0 )
		{
			LOG_OUT(LOGOUT_DEBUG, "recv failed, client closed");
			goto end;
		}
		pcRecvBuf[iRet] = 0;
		// LOG_OUT(LOGOUT_INFO,"pcRecvBuf:[%s]", pcRecvBuf);

		iRet = HTTP_ClientParseResponseHead( pstCli, pcRecvBuf, iRet );
		if ( iRet != OK )
		{
			LOG_OUT(LOGOUT_ERROR, "Parsing http header failed");
			goto end;
		}
		//LOG_OUT(LOGOUT_DEBUG,"[%d][%d]", pstCli->stReson.uiRecvLen, pstCli->stReson.uiRecvTotalLen);
		//LOG_OUT(LOGOUT_INFO, "get response process: %d", pstCli->stReson.uiRecvLen*100 / pstCli->stReson.uiRecvTotalLen);

		//uint32 start = system_get_time();
		if ( NULL != pfHandle)
		{
	        iRet = pfHandle( pPara );
	        if ( iRet != OK )
	        {
	            LOG_OUT(LOGOUT_ERROR, "pfHandle failed, iRet:%d", iRet);
	            goto end;
	        }
		}

        //LOG_OUT(LOGOUT_ERROR, "recv:[%d], used:%d us", pstCli->stReson.uiRecvCurLen, system_get_time() - start);
        //MQTT_WriteSoftWarePara_S *pstWriteSoftWare = pPara;
        //pstWriteSoftWare->pstMqttCtx->iDownloadProcess = pstCli->stReson.uiRecvLen*100 / pstCli->stReson.uiRecvTotalLen;

        //LOG_OUT(LOGOUT_DEBUG, "uiSentLen:%d, uiSendTotalLen:%d", pstCtx->stResp.uiSentLen, pstCtx->stResp.uiSendTotalLen);
        if ( pstCli->stReson.eProcess == CLI_ResponProcess_Finished )
        {
        	return OK;
        }
    }
end:
	pstCli->uiCost = 0;
	return -1;
}
