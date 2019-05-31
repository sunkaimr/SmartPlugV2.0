/*
 * user_http.c
 *
 *  Created on: 2018年11月11日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"


const char HTML_UploadHtml[] = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n <meta http-equiv=\"content-type\" content=\"text/html;charset=gb2312\">\r\n <title>升级</title>\r\n</head>\r\n<body>\r\n	<table border=\"0\" width=\"70%\" align=\"center\" cellpadding=\"6\" id=\"tab\" cellspacing=\"0\" >\r\n		<tr><th colspan=\"4\">固件升级</th></tr>\r\n		<tr><td colspan=\"4\"><hr/></td></tr>\r\n		<tr align=\"left\">\r\n			<th width=\"40%\">文件</th>\r\n			<th width=\"15%\">大小</th>\r\n			<th width=\"20%\">状态</th>\r\n			<th width=\"25%\"></th>\r\n		</tr>\r\n <tr align=\"left\">\r\n <td><input type=\"file\" id=\"binFile\" accept=\".bin\" onchange=\"return fileChg(this);\"></td>\r\n <td>----</td>\r\n <td>----</td>\r\n <td><input type=\"button\" onclick=\"upgread()\" value=\"升级\"/></td>\r\n </tr>\r\n		<tr><td colspan=\"4\"><hr/></td></tr>\r\n <tr><td colspan=\"4\">&nbsp;</td></tr>\r\n		<tr><th colspan=\"4\">网页升级</th></tr>\r\n <tr><td colspan=\"4\"><hr/></td></tr>\r\n <tr><td colspan=\"4\"><hr/></td></tr>\r\n		<tr>\r\n			<td colspan=\"3\"></td>\r\n			<td>\r\n <input type=\"button\" onclick=\"addFile()\" value=\"添加\"/>\r\n <input type=\"button\" onclick=\"uploadFile()\" value=\"上传\"/>\r\n <input type=\"button\" onclick=\"reboot()\" value=\"重启\"/>\r\n </td>\r\n		</tr>\r\n	</table>\r\n <script type=\"text/javascript\">\r\n window.onload = function() {\r\n			addFile();\r\n }\r\n	 function addFile() {\r\n			var t = document.getElementById('tab');\r\n			var r = t.insertRow(t.rows.length-2);\r\n			r.insertCell(0).innerHTML=\"<input type=\\\"file\\\" onchange=\\\"return fileChg(this);\\\">\";\r\n			r.insertCell(1).innerHTML=\"----\";\r\n			r.insertCell(2).innerHTML=\"----\";\r\n			r.insertCell(3).innerHTML=\"<a href=\\\"javascript:void(0);\\\" onclick=\\\"this.parentNode.parentNode.parentNode.removeChild(this.parentNode.parentNode)\\\">删除</a>\";\r\n }\r\n		function fileChg(obj) {\r\n			var fz=obj.files[0].size;\r\n			if( fz > 1024*1024 ){\r\n				fz=(fz/1024/1024).toFixed(1) + \"MB\";\r\n			}else if(fz > 1024){\r\n				fz=(fz/1024).toFixed(1) + \"KB\";\r\n			}else{\r\n				fz=fz+\"B\";\r\n			}\r\n			var sta = obj.parentNode.parentNode.cells;\r\n sta[1].innerHTML = fz;\r\n sta[2].innerHTML = \"等待上传\";\r\n }\r\n\r\n		function uploadFile() {\r\n			var files = new Array();\r\n			var tableObj = document.getElementById(\"tab\");\r\n			for (var i = 8; i < tableObj.rows.length-2; i++) {\r\n				file = tableObj.rows[i].cells[0].getElementsByTagName(\"input\")[0];\r\n				if ( file.files[0] == null ){\r\n					continue;\r\n				}\r\n				files.push(file.files[0]);\r\n tableObj.rows[i].cells[2].innerHTML = \"等待上传\";\r\n			}\r\n			if (files.length == 0){\r\n			 alert(\"请选择文件！\");\r\n			 return;\r\n			}\r\n			if( sendHead(files)){\r\n sendFile(files, 0);\r\n }\r\n\r\n }\r\n function sendHead(fileObj) {\r\n			var dataArr=[];\r\n			for ( var i in fileObj ){\r\n				var data = {};\r\n				data.Name = fileObj[i].name;\r\n				data.Length = parseInt(fileObj[i].size);\r\n				dataArr.push(data);\r\n			}\r\n xhr = new XMLHttpRequest();\r\n xhr.open(\"post\", \"/html/header\", false);\r\n xhr.send(JSON.stringify(dataArr));\r\n return true;\r\n }\r\n function sendFile(fileObj, index) {\r\n if ( index >= fileObj.length){\r\n alert(\"上传完成\");\r\n return;\r\n }\r\n var t = document.getElementById('tab');\r\n xhr = new XMLHttpRequest();\r\n url = \"/html/\"+fileObj[index].name\r\n xhr.open(\"put\", url, true);\r\n xhr.upload.onprogress = function progressFunction(evt) {\r\n if (evt.lengthComputable) {\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = Math.round(evt.loaded / evt.total * 100) + \"%\";\r\n }\r\n };\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = \"%0\";\r\n xhr.onreadystatechange = function () {\r\n if ( xhr.readyState == 2 ){\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = \"正在校验\";\r\n }else if (xhr.readyState == 4) {\r\n if( xhr.status == 201){\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = \"上传成功\";\r\n index=index+1;\r\n sendFile(fileObj, index);\r\n }else{\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = \"上传失败\";\r\n }\r\n }\r\n }\r\n xhr.send(fileObj[index]);\r\n }\r\n function reboot(){\r\n xhr = new XMLHttpRequest();\r\n xhr.open(\"post\", \"/control\", true);\r\n xhr.onreadystatechange = function () {\r\n if (xhr.readyState == 4) {\r\n if( xhr.status == 200){\r\n alert(\"设备正在重启\");\r\n }else{\r\n alert(\"设备重启失败\");\r\n }\r\n }\r\n }\r\n xhr.send(\"{\\\"Action\\\":0}\");\r\n }\r\n function upgread(){\r\n var file = document.getElementById(\"binFile\").files[0];\r\n if(file == null){\r\n alert(\"请选择固件\");\r\n return;\r\n }\r\n var t = document.getElementById('tab');\r\n xhr = new XMLHttpRequest();\r\n xhr.upload.onprogress = function progressFunction(evt) {\r\n if (evt.lengthComputable) {\r\n t.rows[3].cells[2].innerHTML= Math.round(evt.loaded / evt.total * 100) + \"%\";\r\n }\r\n };\r\n xhr.open(\"put\", \"/upgrade\", true);\r\n t.rows[3].cells[2].innerHTML = \"0%\";\r\n xhr.onreadystatechange = function () {\r\n if ( xhr.readyState == 2 ){\r\n t.rows[3].cells[2].innerHTML = \"正在校验\";\r\n }else if (xhr.readyState == 4) {\r\n if( xhr.status == 201){\r\n t.rows[3].cells[2].innerHTML = \"上传成功\";\r\n alert(\"升级成功，设备正在重启\");\r\n }else{\r\n t.rows[3].cells[2].innerHTML = \"上传失败\";\r\n alert(\"升级是失败\");\r\n }\r\n }\r\n }\r\n xhr.send(file);\r\n }\r\n </script>\r\n</body>\r\n</html>\r\n";


HTTP_HTMLDATA_S astHttpHtmlData[HTTP_HTML_DATE_MAX];


VOID HTTP_HtmlDataInit( VOID )
{
	UINT8 i = 0;

	for ( i = 0; i < HTTP_HTML_DATE_MAX; i ++)
	{
		astHttpHtmlData[i].uiLength 	= 0;
		astHttpHtmlData[i].uiAddr		= 0;
		astHttpHtmlData[i].bIsUpload	= FALSE;
		astHttpHtmlData[i].eType 		= HTTP_CONTENT_TYPE_Stream;
		memset( astHttpHtmlData[i].szName, 0, HTTP_HTML_NAME_MAX_LEN);
	}
}


HTTP_HTMLDATA_S* HTTP_GetHtmlData( CHAR* pcName )
{
	UINT8 i = 0;

	if ( pcName == NULL )
	{
		return &astHttpHtmlData[0];
	}

	for ( i = 0; i < HTTP_HTML_DATE_MAX; i ++)
	{
		if ( strcmp( pcName, astHttpHtmlData[i].szName) == 0)
		{
			return &astHttpHtmlData[i];
		}
	}
	return NULL;
}

UINT HTTP_SaveHtmlData( VOID )
{
	UINT uiRet = 0;

	uiRet = CONFIG_SaveConfig(PLUG_MOUDLE_HTML);
	if ( uiRet != OK )
	{
		LOG_OUT( LOGOUT_ERROR, "HTTP_SaveHtmlData failed.");
		return FAIL;
	}
	return OK;
}

UINT32 HTTP_GetHtmlDataLength()
{
	return sizeof(astHttpHtmlData);
}

VOID HTTP_HtmlUrlRegiste( VOID )
{
	UINT8 i = 0;
	CHAR szUrl[HTTP_HTML_NAME_MAX_LEN]={};

	for ( i = 0; i < HTTP_HTML_DATE_MAX; i ++)
	{
		if ( astHttpHtmlData[i].bIsUpload == TRUE && strlen(astHttpHtmlData[i].szName) != 0 )
		{
			snprintf(szUrl, HTTP_HTML_NAME_MAX_LEN, "/%s", astHttpHtmlData[i].szName);
			HTTP_RouterRegiste(HTTP_METHOD_GET, szUrl, HTTP_GetHtml, "HTTP_GetHtml");
			LOG_OUT( LOGOUT_INFO, "HTTP_HtmlUrlRegiste: URL:%s.", szUrl);
		}
	}
}

VOID HTTP_NotFound( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_NotFound, HTTP_CONTENT_TYPE_Html, HTTP_CACHE_CTL_TYPE_No};

	pcBuf = ( CHAR* )malloc( HTTP_BUF_1K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_NotFound, malloc pcBuf failed.");
		return;
    }

    uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_1K, &stResponHead );
    uiBodyLen = snprintf( pcBuf+uiHeaderLen, HTTP_BUF_1K-uiHeaderLen, "<html><head><title>404 Not Found</title></head><center><h1>404 Not Found</h1></center><hr><center>SmartPlug</center></body></html>");
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);

    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
    return;
}


VOID HTTP_BadRequest( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_BadRequest, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};

	pcBuf = ( CHAR* )malloc( HTTP_BUF_1K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_BadRequest, malloc pcBuf failed.");
		return;
    }

    uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_1K, &stResponHead );
    uiBodyLen = snprintf( pcBuf+uiHeaderLen, HTTP_BUF_1K-uiHeaderLen, "{\"result\":\"failed\", \"msg\":\"bad request\"}");
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);

    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
    return;
}

VOID HTTP_InternalServerError( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_InternalServerError, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};

	pcBuf = ( CHAR* )malloc( HTTP_BUF_1K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_InternalServerError, malloc pcBuf failed.");
		return;
    }

    uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_1K, &stResponHead );
    uiBodyLen = snprintf( pcBuf+uiHeaderLen, HTTP_BUF_1K-uiHeaderLen, "{\"result\":\"failed\", \"msg\":\"internal server error\"}");
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);

    return;
}

VOID HTTP_GetHome( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Found, HTTP_CONTENT_TYPE_Html, HTTP_CACHE_CTL_TYPE_No};

	pcBuf = ( CHAR* )malloc( HTTP_BUF_1K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetHome, malloc pcBuf failed.");
		return;
    }

    uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_1K, &stResponHead );
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);

    return;
}


VOID HTTP_GetHealth( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};

	pcBuf = ( CHAR* )malloc( HTTP_BUF_1K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetHealth, malloc pcBuf failed.");
		return;
    }

    uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_1K, &stResponHead );
    uiBodyLen = snprintf( pcBuf+uiHeaderLen, HTTP_BUF_1K-uiHeaderLen,
    		"{\"health\":true}" );
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);

    return;
}

VOID HTTP_GetInfo( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};

	pcBuf = ( CHAR* )malloc( HTTP_BUF_1K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetInfo, malloc pcBuf failed.");
		return;
    }
    uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_1K, &stResponHead );
	uiBodyLen = HTTP_DeviceInfoMarshalJson( pcBuf+uiHeaderLen, HTTP_BUF_1K-uiHeaderLen );
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);

    return;
}

VOID HTTP_PutTest( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Created, HTTP_CONTENT_TYPE_Html, HTTP_CACHE_CTL_TYPE_No};

	LOG_OUT(LOGOUT_DEBUG, "HTTP_PutTest process :%d.", pstHeader->uiRecvPresentLenth*100/pstHeader->uiContentLenth);
	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PutUpgrade, malloc pcBuf failed.");
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
	   	HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);

	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
	}
    return;
}


VOID HTTP_GetTest( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 4096;
	UINT uiFileLength = 1024*1024*1;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Stream, HTTP_CACHE_CTL_TYPE_No};

	pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;
	pstHeader->uiSentLength = 0;
	while ( 1 )
	{
		while( pstHeader->bIsCouldSend == TRUE )
		{
			vTaskDelay(10/portTICK_RATE_MS);
		}

		if ( pstHeader->uiSentLength >= pstHeader->uiSendTotalLength)
		{
			break;
		}

		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_GetTest, malloc pcBuf failed.");
			return;
	    }

	    if ( pstHeader->uiSentLength == 0 )
	    {
	    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
			HTTP_SetHeaderContentLength(pcBuf, uiFileLength);
			pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;
	   }
	    else
	    {
	    	uiHeaderLen = 0;
	    }

	    if ( pstHeader->uiSendTotalLength - pstHeader->uiSentLength > 4096 )
	    {
	    	uiBodyLen = uiFileLength > 4096 - uiHeaderLen ? 4096 - uiHeaderLen : uiFileLength;
	    }
	    else
	    {
	    	uiBodyLen = pstHeader->uiSendTotalLength - pstHeader->uiSentLength;
	    }

	    pstHeader->uiSendCurLength = uiHeaderLen + uiBodyLen;
	    memset(pcBuf+uiHeaderLen, '0', uiBodyLen);
	    pstHeader->pcResponBody = pcBuf;
	    pstHeader->bIsCouldSend = TRUE;
	}

    return;
}

#if 0
VOID HTTP_PutImage( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	UINT uiRet = 0;
	STATIC UINT uiPos = 0;
	HTTP_HTMLDATA_S* pstHtmlData = NULL;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Created, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};

	pstHtmlData = HTTP_GetHtmlData(HTTP_HTML_2);

	LOG_OUT(LOGOUT_DEBUG, "HTTP_PutImage process :%d.", pstHeader->uiRecvPresentLenth*100/pstHeader->uiContentLenth);

	uiRet = FlASH_Write(pstHtmlData->uiAddr + uiPos,
						pstHeader->pcResqBody,
						pstHeader->uiRecvCurLenth);
	if ( uiRet != OK  )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_PutImage, FlASH_Write failed.");
		return;
	}
	uiPos = pstHeader->uiRecvPresentLenth;

	if (  pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
    	uiPos = 0;

		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PutImage, malloc pcBuf failed.");
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
	    uiBodyLen = snprintf( pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen, HttpStatus_Created);
		HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);

		pstHeader->uiSentLength = 0;
		pstHeader->uiSendCurLength = uiHeaderLen + uiBodyLen;
	    pstHeader->pcResponBody = pcBuf;
	    pstHeader->uiSendTotalLength = pstHeader->uiSendCurLength;
	    pstHeader->bIsCouldSend = TRUE;

	    pstHtmlData->uiLength = pstHeader->uiContentLenth;
	    CONFIG_SaveConfig(PLUG_MOUDLE_HTML);
	}

    return;
}

VOID HTTP_GetImage( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 1;
	UINT uiBodyLen = 0;
	UINT uiFileLength = 0;
	UINT uiRet = 0;
	STATIC UINT uiPos = 0;
	HTTP_HTMLDATA_S* pstHtmlData = NULL;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Stream, HTTP_CACHE_CTL_TYPE_No};

	pstHtmlData = HTTP_GetHtmlData(HTTP_HTML_2);
	uiFileLength =  pstHtmlData->uiLength;
	pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;

	pstHeader->uiSentLength = 0;
	while ( 1 )
	{
		while( pstHeader->bIsCouldSend == TRUE )
		{
			vTaskDelay(10/portTICK_RATE_MS);
		}

		if ( pstHeader->uiSentLength >= pstHeader->uiSendTotalLength)
		{
			uiPos = 0;
			break;
		}

		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_GetImage, malloc pcBuf failed.");
			return;
	    }

	    if ( pstHeader->uiSentLength == 0 )
	    {
	    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
			HTTP_SetHeaderContentLength(pcBuf, uiFileLength);
			pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;
	    }
	    else
	    {
	    	uiHeaderLen = 0;
	    }

	    if ( pstHeader->uiSendTotalLength - pstHeader->uiSentLength > 4096 )
	    {
	    	uiBodyLen = uiFileLength > (4096 - uiHeaderLen) ? (4096 - uiHeaderLen) : uiFileLength;
	    }
	    else
	    {
	    	uiBodyLen = pstHeader->uiSendTotalLength - pstHeader->uiSentLength;
	    }

	    pstHeader->uiSendCurLength = uiHeaderLen + uiBodyLen;

	    uiRet = FlASH_Read(pstHtmlData->uiAddr + uiPos,
							pcBuf+uiHeaderLen,
							uiBodyLen);

		uiPos += uiBodyLen;
	    pstHeader->pcResponBody = pcBuf;
	    pstHeader->bIsCouldSend = TRUE;
		if ( uiRet != OK  )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_GetImage, FlASH_Read failed.");
		}
	}
	return;
}


#endif

VOID HTTP_GetTimerData( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	CHAR szTimer[HTTP_URL_MAX_LEN];
	UINT uiTimerNum = 0;

	pcBuf = ( CHAR* )malloc( HTTP_BUF_2K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetTimerData, malloc pcBuf failed.");
		return;
    }

    if ( OK != HTTP_GetRouterPara(pstHeader, "timer", szTimer))
    {
    	LOG_OUT(LOGOUT_ERROR, "HTTP_GetRouterPara, get timer failed.");
    }
    else
    {
        if ( strcmp(szTimer, "all") == 0 )
        {
        	uiTimerNum = PLUG_TIMER_ALL;
        }
        else
        {
        	uiTimerNum = atoi(szTimer);
        }
    }

    if ( uiTimerNum == 0 || uiTimerNum > PLUG_TIMER_ALL )
    {
		stResponHead.eHttpCode = HTTP_CODE_BadRequest;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
    }

	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_2K, &stResponHead );
    uiBodyLen = PLUG_MarshalJsonTimer( pcBuf+uiHeaderLen, HTTP_BUF_2K-uiHeaderLen, uiTimerNum);
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
    return;
}

VOID HTTP_GetDelayData( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	CHAR szDelay[HTTP_URL_MAX_LEN];
	UINT uiDelayNum = 0;

	pcBuf = ( CHAR* )malloc( HTTP_BUF_3K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetDelayData, malloc pcBuf failed.");
		return;
    }

    if ( OK != HTTP_GetRouterPara(pstHeader, "delay", szDelay))
    {
    	LOG_OUT(LOGOUT_ERROR, "HTTP_GetRouterPara, get delay failed.");
    }
    else
    {
        if ( strcmp(szDelay, "all") == 0 )
        {
        	uiDelayNum = PLUG_DELAY_ALL;
        }
        else
        {
        	uiDelayNum = atoi(szDelay);
        }
    }

    if ( uiDelayNum == 0 || uiDelayNum > PLUG_DELAY_ALL )
    {
		stResponHead.eHttpCode = HTTP_CODE_BadRequest;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
    }

	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_3K, &stResponHead );
    uiBodyLen = PLUG_MarshalJsonDelay( pcBuf+uiHeaderLen, HTTP_BUF_3K-uiHeaderLen, uiDelayNum );
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
    return;
}

VOID HTTP_GetSystemData( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};

	pcBuf = ( CHAR* )malloc( HTTP_BUF_1K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetSystemData, malloc pcBuf failed.");
		return;
    }

	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_1K, &stResponHead );
    uiBodyLen = PLUG_MarshalJsonSystemSet( pcBuf+uiHeaderLen, HTTP_BUF_1K-uiHeaderLen );
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
    return;
}

VOID HTTP_GetHtmlHeader( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};

	pcBuf = ( CHAR* )malloc( HTTP_BUF_2K );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetHtmlHeader, malloc pcBuf failed.");
		return;
    }

	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_2K, &stResponHead );
    uiBodyLen = PLUG_MarshalJsonHtmlData( pcBuf+uiHeaderLen, HTTP_BUF_2K-uiHeaderLen );
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
    return;
}

VOID HTTP_PostHtmlHeader( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	UINT uiRet = 0;

	if (pstHeader->uiRecvCurLenth < pstHeader->uiContentLenth)
	{
		return;
	}

	pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_PostHtmlHeader, malloc pcBuf failed.");
		return;
    }

	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );

	uiRet = PLUG_ParseHtmlData(pstHeader->pcResqBody);
	if ( uiRet != OK )
	{
		stResponHead.eHttpCode = HTTP_CODE_BadRequest;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
		pstHeader->eProcess = HTTP_PROCESS_Finished;
	}

	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PostHtmlHeader, malloc pcBuf failed.");
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
    	uiBodyLen = PLUG_MarshalJsonHtmlData( pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen );
	   	HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
	    return;
	}
}

VOID HTTP_PutHtml( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	UINT uiRet = 0;
	STATIC UINT uiAddr = 0;
	STATIC UINT uiPos = 0;
	STATIC HTTP_HTMLDATA_S* pstHtmlData = NULL;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Created, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	STATIC CHAR szHtmlName[HTTP_HTML_NAME_MAX_LEN];

	if ( uiAddr == 0 )
	{
	    if ( OK != HTTP_GetRouterPara(pstHeader, "html", szHtmlName))
	    {
	    	LOG_OUT(LOGOUT_ERROR, "HTTP_PutHtml, get router para html failed.");
	    }
	    else
	    {
			pstHtmlData = HTTP_GetHtmlData(szHtmlName);
			if ( pstHtmlData == NULL )
			{
				LOG_OUT(LOGOUT_ERROR, "HTTP_PutHtml, get html data failed.");

				stResponHead.eHttpCode = HTTP_CODE_BadRequest;
				stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
				stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
				pstHeader->eProcess = HTTP_PROCESS_Finished;
			}

			uiAddr = pstHtmlData->uiAddr;
			if ( pstHeader->uiContentLenth != pstHtmlData->uiLength )
			{
				LOG_OUT(LOGOUT_ERROR, "HTTP_PutHtml, %s length not equal header:%d != flash:%d.",
						szHtmlName, pstHeader->uiContentLenth, pstHtmlData->uiLength);

				stResponHead.eHttpCode = HTTP_CODE_BadRequest;
				stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
				stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
				pstHeader->eProcess = HTTP_PROCESS_Finished;
			}
	    }
	}

	uiRet = FlASH_Write(uiAddr + uiPos,
						pstHeader->pcResqBody,
						pstHeader->uiRecvCurLenth);
	if ( uiRet != OK  )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_PutHtml: %s, FlASH_Write failed.", szHtmlName);

		stResponHead.eHttpCode = HTTP_CODE_InternalServerError;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Html;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
		pstHeader->eProcess = HTTP_PROCESS_Finished;
	}
	else
	{
		if (pstHeader->uiContentLenth > 0)
		{
			LOG_OUT(LOGOUT_INFO, "Put %s process :%d.", szHtmlName,
					pstHeader->uiRecvPresentLenth*100 / pstHeader->uiContentLenth);
		}
	}
	uiPos = pstHeader->uiRecvPresentLenth;

	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		uiPos = 0;
		uiAddr = 0;

	    if ( stResponHead.eHttpCode == HTTP_CODE_Created )
	    {
	    	pstHtmlData->bIsUpload = TRUE;
	    	uiRet = HTTP_SaveHtmlData();
	    	if ( uiRet != OK )
	    	{
	    		LOG_OUT(LOGOUT_ERROR, "HTTP_PutHtml: %s, HTTP_SaveHtmlData failed.", szHtmlName);
	    		stResponHead.eHttpCode = HTTP_CODE_InternalServerError;
	    	}
	    	sprintf(szHtmlName, "/%s", pstHtmlData->szName);
	    	HTTP_RouterRegiste(HTTP_METHOD_GET, szHtmlName, HTTP_GetHtml, "HTTP_GetHtml");
	    }

		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
    		LOG_OUT(LOGOUT_ERROR, "HTTP_PutHtml: %s, malloc buf failed.", szHtmlName);
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
    	uiBodyLen = HTTP_SetResponseBody(pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen, "{\"result\":\"success\", \"msg\":\"success\"}");
		HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
	}

    return;
}

VOID HTTP_GetHtml( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	CHAR* pcHtmlName = NULL;
	UINT uiHeaderLen = 1;
	UINT uiBodyLen = 0;
	UINT uiFileLength = 0;
	UINT uiRet = 0;
	STATIC UINT uiPos = 0, uiAddr = 0;
	HTTP_HTMLDATA_S* pstHtmlData = NULL;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Html, HTTP_CACHE_CTL_TYPE_MaxAge_1y};

	pcHtmlName = pstHeader->szURL;
	pcHtmlName++;
	pstHtmlData = HTTP_GetHtmlData(pcHtmlName);
	if ( pstHtmlData != NULL )
	{
		uiFileLength =  pstHtmlData->uiLength;
		uiAddr = pstHtmlData->uiAddr;
		pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;
		stResponHead.eContentType = pstHtmlData->eType;

		LOG_OUT(LOGOUT_INFO, "HTTP_GetHtml, name:%s addr:%d length:%d.",
				pstHtmlData->szName, pstHtmlData->uiAddr, pstHtmlData->uiLength);
	}
	else
	{
		LOG_OUT(LOGOUT_DEBUG, "HTTP_GetHtml, cannot find %s.", pcHtmlName);

		uiFileLength =  0;
		uiAddr = 0;
		pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;
	}

	pstHeader->uiSentLength = 0;
	while ( 1 )
	{
		//等待上一次发送完成
		while( pstHeader->bIsCouldSend == TRUE )
		{
			vTaskDelay(10/portTICK_RATE_MS);
		}

		//发送结束时退出循环
		if ( pstHeader->uiSentLength >= pstHeader->uiSendTotalLength)
		{
			uiPos = 0;
			break;
		}

		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_GetHtml, %s malloc pcBuf failed.", pcHtmlName);
			return;
	    }

	    //首次发送时发送http头部
		if ( pstHeader->uiSentLength == 0 )
		{
			uiPos = 0;
			uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
			HTTP_SetHeaderContentLength(pcBuf, uiFileLength);
			pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;
		}
	    else
	    {
	    	uiHeaderLen = 0;
	    }

		//http响应的body体一次最多4096字节
	    if ( pstHeader->uiSendTotalLength - pstHeader->uiSentLength > 4096 )
	    {
	    	uiBodyLen = uiFileLength > (4096 - uiHeaderLen) ? (4096 - uiHeaderLen) : uiFileLength;
	    }
	    else
	    {
	    	uiBodyLen = pstHeader->uiSendTotalLength - pstHeader->uiSentLength;
	    }

	    pstHeader->uiSendCurLength = uiHeaderLen + uiBodyLen;

	    uiRet = FlASH_Read(uiAddr + uiPos,
							pcBuf + uiHeaderLen,
							uiBodyLen);

		uiPos += uiBodyLen;
	    pstHeader->pcResponBody = pcBuf;
	    pstHeader->bIsCouldSend = TRUE;
		if ( uiRet != OK  )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_GetHtml, %s FlASH_Read failed.", pcHtmlName);
		}
	}
	return;
}


VOID HTTP_GetDate( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	PLUG_DATE_S stDate;

	pcBuf = ( CHAR* )malloc( HTTP_BUF_512 );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetDate, malloc pcBuf failed.");
		return;
    }

    PLUG_GetDate(&stDate);
	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_512, &stResponHead );

	uiBodyLen = snprintf( pcBuf+uiHeaderLen, HTTP_BUF_512-uiHeaderLen,
			"{\"Date\":\"%02d-%02d-%02d %02d:%02d:%02d\", \"SyncTime\":%s}",
			stDate.iYear, stDate.iMonth, stDate.iDay,
			stDate.iHour, stDate.iMinute, stDate.iSecond,
			PLUG_GetTimeSyncFlag() == TIME_SYNC_NONE ? "false" : "true");

    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
    return;
}

VOID HTTP_PostTimerData( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	PLUG_DATE_S stDate;
	UINT uiRet = 0;

	if (pstHeader->uiRecvCurLenth < pstHeader->uiContentLenth)
	{
		return;
	}

	uiRet = PLUG_ParseTimerData(pstHeader->pcResqBody);
	if ( uiRet != OK )
	{
		stResponHead.eHttpCode = HTTP_CODE_BadRequest;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
		pstHeader->eProcess = HTTP_PROCESS_Finished;
	}

	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PostTimerData, malloc pcBuf failed.");
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
    	uiBodyLen = HTTP_SetResponseBody(pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen, "{\"result\":\"success\", \"msg\":\"\"}");
	   	HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
	    return;
	}
}

VOID HTTP_PostDelayData( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	PLUG_DATE_S stDate;
	UINT uiRet = 0;

	if (pstHeader->uiRecvCurLenth < pstHeader->uiContentLenth)
	{
		return;
	}


	uiRet = PLUG_ParseDelayData(pstHeader->pcResqBody);
	if ( uiRet != OK )
	{
		stResponHead.eHttpCode = HTTP_CODE_BadRequest;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
		pstHeader->eProcess = HTTP_PROCESS_Finished;
	}

	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PostDelayData, malloc pcBuf failed.");
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
    	uiBodyLen = HTTP_SetResponseBody(pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen, "{\"result\":\"success\", \"msg\":\"\"}");
	   	HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
	    return;
	}
}

VOID HTTP_PostSystemData( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	PLUG_DATE_S stDate;
	UINT uiRet = 0;

	if (pstHeader->uiRecvCurLenth < pstHeader->uiContentLenth)
	{
		return;
	}

	uiRet = PLUG_ParseSystemData(pstHeader->pcResqBody);
	if ( uiRet != OK )
	{
		stResponHead.eHttpCode = HTTP_CODE_BadRequest;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
		pstHeader->eProcess = HTTP_PROCESS_Finished;
	}

	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PostSystemData, malloc pcBuf failed.");
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
    	uiBodyLen = HTTP_SetResponseBody(pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen, "{\"result\":\"success\", \"msg\":\"\"}");
	   	HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
	    return;
	}
}

VOID HTTP_PostDeviceControl( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	PLUG_DATE_S stDate;
	UINT uiRet = 0;

	if (pstHeader->uiRecvCurLenth < pstHeader->uiContentLenth)
	{
		return;
	}

	uiRet = PLUG_ParseDeviceControlData(pstHeader->pcResqBody);
	if ( uiRet != OK )
	{
		stResponHead.eHttpCode = HTTP_CODE_BadRequest;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
		pstHeader->eProcess = HTTP_PROCESS_Finished;
	}

	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PostDeviceControl, malloc pcBuf failed.");
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
    	uiBodyLen = HTTP_SetResponseBody(pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen, "{\"result\":\"success\", \"msg\":\"\"}");
	   	HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
	    return;
	}
}

VOID HTTP_GetScanWifi( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};

	pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetRelayStatus, malloc pcBuf failed.");
		return;
    }

	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
	uiBodyLen = HTTP_WifiScanMarshalJson( pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen );
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
    return;
}

VOID HTTP_PostDate( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	UINT uiRet = 0;
	PLUG_DATE_S stDate;

	if (pstHeader->uiRecvCurLenth < pstHeader->uiContentLenth)
	{
		return;
	}

	uiRet = PLUG_ParseDate(pstHeader->pcResqBody);
	if ( uiRet != OK )
	{
		stResponHead.eHttpCode = HTTP_CODE_BadRequest;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
		pstHeader->eProcess = HTTP_PROCESS_Finished;
	}

	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PostDate, malloc pcBuf failed.");
			return;
	    }
	    PLUG_GetDate(&stDate);

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
    	uiBodyLen = snprintf( pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen,
    			"{\"Date\":\"%02d-%02d-%02d %02d:%02d:%02d\", \"SyncTime\":%s}",
    			stDate.iYear, stDate.iMonth, stDate.iDay,
    			stDate.iHour, stDate.iMinute, stDate.iSecond,
    			PLUG_GetTimeSyncFlag() == TIME_SYNC_NONE ? "false" : "true");
	   	HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
	    return;
	}
}


VOID HTTP_PutUpgrade( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	UINT uiRet = 0;
	STATIC UINT uiPos = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Created, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	STATIC UINT32 uiAddr = 0;
	HTTP_HTMLDATA_S* pstHtmlData = NULL;

	//取要升级的user.bin的地址
	if ( uiAddr == 0 )
	{
		uiAddr = UPGRADE_GetUpgradeUserBinAddr();
		if ( uiAddr != 0 )
		{
			LOG_OUT(LOGOUT_INFO, "new bin uiAddr:0x%X, length:%d", uiAddr, pstHeader->uiContentLenth);
		}
	}

	//将要升级的bin文件写入对应flash地址中
	if ( uiAddr != 0 )
	{
		uiRet = FlASH_Write(uiAddr + uiPos,
							pstHeader->pcResqBody,
							pstHeader->uiRecvCurLenth);
		//写失败直接返回500
		if ( uiRet != OK  )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PutUpgrade, FlASH_Write failed.");

			stResponHead.eHttpCode = HTTP_CODE_InternalServerError;
			stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
			stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
			pstHeader->eProcess = HTTP_PROCESS_Finished;
		}
		else
		{
			if (pstHeader->uiContentLenth > 0)
			{
				//输出user.bin下载进度
				LOG_OUT(LOGOUT_INFO, "upgrade process :%d.",
						pstHeader->uiRecvPresentLenth*100 / pstHeader->uiContentLenth);
			}
		}
	}
	//user.bin的地址无效时返回500
	else
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_PutUpgrade, Get user bin addr failed.");

		stResponHead.eHttpCode = HTTP_CODE_InternalServerError;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
		pstHeader->eProcess = HTTP_PROCESS_Finished;
	}

	//更新偏移地址
	uiPos = pstHeader->uiRecvPresentLenth;

	//bin数据接收完成
	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		uiPos = 0;
		uiAddr = 0;

		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PutUpgrade, malloc pcBuf failed.");
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
    	//uiBodyLen = HTTP_SetResponseBody(pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen, "{\"result\":\"success\", \"msg\":\"\"}");
    	uiBodyLen = snprintf( pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen, "{\"result\":\"success\", \"msg\":\"\"}");
    	HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);

	    if (stResponHead.eHttpCode == HTTP_CODE_Created )
	    {
		    LOG_OUT(LOGOUT_INFO, "HTTP_PutUpgrade new bin download successed.");
		    UPGRADE_StartUpgradeRebootTimer();
	    }
	    else
	    {
	    	LOG_OUT(LOGOUT_ERROR, "HTTP_PutUpgrade new bin download failed.");
	    }
	}
    return;
}


#if 0
VOID HTTP_GetUpgrade( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 1;
	UINT uiBodyLen = 0;
	UINT uiFileLength = 0;
	UINT uiRet = 0;
	STATIC UINT uiPos = 0;
	HTTP_HTMLDATA_S* pstHtmlData = NULL;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Stream, HTTP_CACHE_CTL_TYPE_MaxAge_1y};

	pstHtmlData = HTTP_GetHtmlData(HTTP_HTML_1);
	uiFileLength =  pstHtmlData->uiLength;
	pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;

	pstHeader->uiSentLength = 0;
	while ( 1 )
	{
		while( pstHeader->bIsCouldSend == TRUE )
		{
			vTaskDelay(10/portTICK_RATE_MS);
		}

		if ( pstHeader->uiSentLength >= pstHeader->uiSendTotalLength)
		{
			uiPos = 0;
			break;
		}

		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_GetUpgrade, malloc pcBuf failed.");
			return;
	    }

	    if ( pstHeader->uiSentLength == 0 )
	    {
	    	uiPos = 0;
	    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
			HTTP_SetHeaderContentLength(pcBuf, uiFileLength);
			pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;
	   }
	    else
	    {
	    	uiHeaderLen = 0;
	    }

	    if ( pstHeader->uiSendTotalLength - pstHeader->uiSentLength > 4096 )
	    {
	    	uiBodyLen = uiFileLength > (4096 - uiHeaderLen) ? (4096 - uiHeaderLen) : uiFileLength;
	    }
	    else
	    {
	    	uiBodyLen = pstHeader->uiSendTotalLength - pstHeader->uiSentLength;
	    }

	    pstHeader->uiSendCurLength = uiHeaderLen + uiBodyLen;

	    uiRet = FlASH_Read(pstHtmlData->uiAddr + uiPos,
							pcBuf+uiHeaderLen,
							uiBodyLen);

		uiPos += uiBodyLen;
	    pstHeader->pcResponBody = pcBuf;
	    pstHeader->bIsCouldSend = TRUE;
		if ( uiRet != OK  )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_GetUpgrade, FlASH_Read failed.");
		}
	}
	return;
}
#endif

VOID HTTP_GetRelayStatus( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	PLUG_DATE_S stDate;

	pcBuf = ( CHAR* )malloc( HTTP_BUF_512 );
    if ( NULL == pcBuf )
	{
		LOG_OUT(LOGOUT_ERROR, "HTTP_GetRelayStatus, malloc pcBuf failed.");
		return;
    }

    PLUG_GetDate(&stDate);
	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, HTTP_BUF_512, &stResponHead );

	uiBodyLen = snprintf( pcBuf+uiHeaderLen, HTTP_BUF_512-uiHeaderLen,
			"{\"status\":\"%s\"}", PLUG_GetRelayStatus() ? "on" : "off");
    HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
    return;
}

VOID HTTP_PostRelayStatus( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 0;
	UINT uiBodyLen = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Json, HTTP_CACHE_CTL_TYPE_No};
	UINT uiRet = 0;

	if (pstHeader->uiRecvCurLenth < pstHeader->uiContentLenth)
	{
		return;
	}

	uiRet = PLUG_ParseRelayStatus(pstHeader->pcResqBody);
	if ( uiRet != OK )
	{
		stResponHead.eHttpCode = HTTP_CODE_BadRequest;
		stResponHead.eContentType = HTTP_CONTENT_TYPE_Json;
		stResponHead.eCacheControl = HTTP_CACHE_CTL_TYPE_No;
		pstHeader->eProcess = HTTP_PROCESS_Finished;
	}

	if ( pstHeader->eProcess == HTTP_PROCESS_Finished )
	{
		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_PostRelayStatus, malloc pcBuf failed.");
			return;
	    }

    	uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
    	uiBodyLen = snprintf( pcBuf+uiHeaderLen, USER_SENDBUF_SIZE-uiHeaderLen,
    			"{\"status\":\"%s\"}", PLUG_GetRelayStatus() ? "on" : "off");
	   	HTTP_SetHeaderContentLength(pcBuf, uiBodyLen);
	    HTTP_SetDefaultSendHeader(pstHeader, pcBuf, uiBodyLen + uiHeaderLen);
	    return;
	}
}

UINT HTTP_WifiScanMarshalJson( CHAR* pcBuf, UINT uiBufLen)
{
	WIFI_SCAN_S *pstData = NULL;
	UINT uiLoopi = 0;
	UINT8 uiRet = 0;
	CHAR *pJsonStr = NULL;
	cJSON  *pJsonArry, *pJsonsub;

	if( g_pstWifiScanHead == NULL )
	{
		g_pstWifiScanHead = ( WIFI_SCAN_S* )malloc( sizeof(WIFI_SCAN_S) * WIFI_SCAN_NUM );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_WifiScanMarshalJson, malloc pcBuf failed.");
			return 0;
	    }
	    memset(g_pstWifiScanHead, 0, sizeof(WIFI_SCAN_S) * WIFI_SCAN_NUM);
	}

	uiRet = WIFI_ScanWifiSsid();
	if ( uiRet != OK )
	{
		FREE_MEM(pJsonStr);
		LOG_OUT(LOGOUT_ERROR, "WIFI_ScanWifiSsid failed.");
		return 0;
	}

	pstData = g_pstWifiScanHead;
	pJsonArry = cJSON_CreateArray();
	for ( uiLoopi = 0 ; uiLoopi < WIFI_SCAN_NUM; uiLoopi++, pstData++ )
	{
		if( pstData->szMac[0] == 0 )
		{
			break;
		}
		pJsonsub=cJSON_CreateObject();

		cJSON_AddStringToObject( pJsonsub,	"Ssid", 			pstData->szSsid);
		cJSON_AddStringToObject( pJsonsub,	"Mac", 				pstData->szMac);
		cJSON_AddStringToObject( pJsonsub,	"AuthMode",			pstData->szAuthMode);
		cJSON_AddNumberToObject( pJsonsub,	"Rssi",				pstData->iRssi);
		cJSON_AddNumberToObject( pJsonsub,	"Channel",			pstData->ucChannel);

		cJSON_AddItemToArray(pJsonArry, pJsonsub);
	}
	FREE_MEM(g_pstWifiScanHead);

    pJsonStr = cJSON_PrintUnformatted(pJsonArry);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJsonArry);
    FREE_MEM(pJsonStr);
	return strlen(pcBuf);
}


UINT HTTP_DeviceInfoMarshalJson( CHAR* pcBuf, UINT uiBufLen)
{
	cJSON  *pJson = NULL;
	CHAR *pJsonStr = NULL;
	CHAR szBuf[20];

	pJson = cJSON_CreateObject();

	snprintf(szBuf, sizeof(szBuf), "%s %s", __DATE__, __TIME__);
	cJSON_AddStringToObject( pJson, "BuildDate", 	szBuf);

	cJSON_AddStringToObject( pJson, "SDKVersion", 	system_get_sdk_version());
	cJSON_AddStringToObject( pJson, "FlashMap", 	UPGRADE_GetFlashMap());

	snprintf(szBuf, sizeof(szBuf), "user%d.bin", 	system_upgrade_userbin_check()+1);
	cJSON_AddStringToObject( pJson, "UserBin", 		szBuf);

	cJSON_AddNumberToObject( pJson, "RunTime", 		PLUG_GetRunTime());

    pJsonStr = cJSON_PrintUnformatted(pJson);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJson);
    FREE_MEM(pJsonStr);

	return strlen(pcBuf);
}


VOID HTTP_GetUpload( HTTP_REQUEST_HEAD_S *pstHeader )
{
	CHAR* pcBuf = NULL;
	UINT uiHeaderLen = 1;
	UINT uiBodyLen = 0;
	UINT uiFileLength = 0;
	STATIC UINT uiPos = 0;
	HTTP_RESPONSE_HEAD_S stResponHead = {HTTP_CODE_Ok, HTTP_CONTENT_TYPE_Html, HTTP_CACHE_CTL_TYPE_MaxAge_1y};

	uiFileLength =  uiFileLength = strlen(HTML_UploadHtml);
	pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;
	pstHeader->uiSentLength = 0;
	while ( 1 )
	{
		//等待上一次发送完成
		while( pstHeader->bIsCouldSend == TRUE )
		{
			vTaskDelay(10/portTICK_RATE_MS);
		}

		//发送结束时推迟循环
		if ( pstHeader->uiSentLength >= pstHeader->uiSendTotalLength)
		{
			uiPos = 0;
			break;
		}

		pcBuf = ( CHAR* )malloc( USER_SENDBUF_SIZE + 5 );
	    if ( NULL == pcBuf )
		{
			LOG_OUT(LOGOUT_ERROR, "HTTP_GetUpload, malloc pcBuf failed.");
			return;
	    }

	    //首次发送时发送http头部
		if ( pstHeader->uiSentLength == 0 )
		{
			uiPos = 0;
			uiHeaderLen = HTTP_SetHeaderCode( pcBuf, USER_SENDBUF_SIZE, &stResponHead );
			HTTP_SetHeaderContentLength(pcBuf, uiFileLength);
			pstHeader->uiSendTotalLength = uiFileLength + uiHeaderLen;
		}
	    else
	    {
	    	uiHeaderLen = 0;
	    }

		//http响应的body体一次最多4096字节
	    if ( pstHeader->uiSendTotalLength - pstHeader->uiSentLength > 4096 )
	    {
	    	uiBodyLen = uiFileLength > (4096 - uiHeaderLen) ? (4096 - uiHeaderLen) : uiFileLength;
	    }
	    else
	    {
	    	uiBodyLen = pstHeader->uiSendTotalLength - pstHeader->uiSentLength;
	    }

	    pstHeader->uiSendCurLength = uiHeaderLen + uiBodyLen;

	    memcpy(pcBuf + uiHeaderLen, HTML_UploadHtml+uiPos, uiBodyLen);

		uiPos += uiBodyLen;
	    pstHeader->pcResponBody = pcBuf;
	    pstHeader->bIsCouldSend = TRUE;
	}
	return;
}
