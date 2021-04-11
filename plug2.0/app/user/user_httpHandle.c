/*
 * user_http.c
 *
 *  Created on: 2018年11月11日
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"


#define HTTP_HEADER_LEN        (500)     //http 响应头长度可根据实际情况调整


//const CHAR HTML_UploadHtml[] ICACHE_RODATA_ATTR = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n <meta http-equiv=\"content-type\" content=\"text/html;charset=gb2312\">\r\n <title>升级</title>\r\n</head>\r\n<body>\r\n <table border=\"0\" width=\"70%\" align=\"center\" cellpadding=\"6\" id=\"tab\" cellspacing=\"0\" >\r\n <tr><th colspan=\"4\">固件升级</th></tr>\r\n <tr><td colspan=\"4\"><hr/></td></tr>\r\n <tr align=\"left\">\r\n <th width=\"40%\">文件</th>\r\n <th width=\"15%\">大小</th>\r\n <th width=\"20%\">状态</th>\r\n <th width=\"25%\"></th>\r\n </tr>\r\n <tr align=\"left\">\r\n <td><input type=\"file\" id=\"binFile\" accept=\".bin\" onchange=\"return fileChg(this);\"></td>\r\n <td>----</td>\r\n <td>----</td>\r\n <td><input type=\"button\" onclick=\"upgread()\" value=\"升级\"/></td>\r\n </tr>\r\n <tr><td colspan=\"4\"><hr/></td></tr>\r\n <tr><td colspan=\"4\">&nbsp;</td></tr>\r\n <tr><th colspan=\"4\">网页升级</th></tr>\r\n <tr><td colspan=\"4\"><hr/></td></tr>\r\n <tr><td colspan=\"4\"><hr/></td></tr>\r\n <tr>\r\n <td colspan=\"3\"></td>\r\n <td>\r\n <input type=\"button\" onclick=\"addFile()\" value=\"添加\"/>\r\n <input type=\"button\" onclick=\"uploadFile()\" value=\"上传\"/>\r\n <input type=\"button\" onclick=\"reboot()\" value=\"重启\"/>\r\n </td>\r\n </tr>\r\n </table>\r\n <script type=\"text/javascript\">\r\n window.onload = function() {\r\n addFile();\r\n }\r\n function addFile() {\r\n var t = document.getElementById('tab');\r\n var r = t.insertRow(t.rows.length-2);\r\n r.insertCell(0).innerHTML=\"<input type=\\\"file\\\" onchange=\\\"return fileChg(this);\\\">\";\r\n r.insertCell(1).innerHTML=\"----\";\r\n r.insertCell(2).innerHTML=\"----\";\r\n r.insertCell(3).innerHTML=\"<a href=\\\"javascript:void(0);\\\" onclick=\\\"this.parentNode.parentNode.parentNode.removeChild(this.parentNode.parentNode)\\\">删除</a>\";\r\n }\r\n function fileChg(obj) {\r\n var fz=obj.files[0].size;\r\n if( fz > 1024*1024 ){\r\n fz=(fz/1024/1024).toFixed(1) + \"MB\";\r\n }else if(fz > 1024){\r\n fz=(fz/1024).toFixed(1) + \"KB\";\r\n }else{\r\n fz=fz+\"B\";\r\n }\r\n var sta = obj.parentNode.parentNode.cells;\r\n sta[1].innerHTML = fz;\r\n sta[2].innerHTML = \"等待上传\";\r\n }\r\n\r\n function uploadFile() {\r\n var files = new Array();\r\n var tableObj = document.getElementById(\"tab\");\r\n for (var i = 8; i < tableObj.rows.length-2; i++) {\r\n file = tableObj.rows[i].cells[0].getElementsByTagName(\"input\")[0];\r\n if ( file.files[0] == null ){\r\n continue;\r\n }\r\n files.push(file.files[0]);\r\n tableObj.rows[i].cells[2].innerHTML = \"等待上传\";\r\n }\r\n if (files.length == 0){\r\n alert(\"请选择文件！\");\r\n return;\r\n }\r\n if( sendHead(files)){\r\n sendFile(files, 0);\r\n }\r\n\r\n }\r\n function sendHead(fileObj) {\r\n var dataArr=[];\r\n for ( var i in fileObj ){\r\n var data = {};\r\n data.Name = fileObj[i].name;\r\n data.Length = parseInt(fileObj[i].size);\r\n dataArr.push(data);\r\n }\r\n xhr = new XMLHttpRequest();\r\n xhr.open(\"post\", \"/html/header\", false);\r\n xhr.send(JSON.stringify(dataArr));\r\n return true;\r\n }\r\n function sendFile(fileObj, index) {\r\n if ( index >= fileObj.length){\r\n alert(\"上传完成\");\r\n return;\r\n }\r\n var t = document.getElementById('tab');\r\n xhr = new XMLHttpRequest();\r\n url = \"/html/\"+fileObj[index].name\r\n xhr.open(\"put\", url, true);\r\n xhr.upload.onprogress = function progressFunction(evt) {\r\n if (evt.lengthComputable) {\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = Math.round(evt.loaded / evt.total * 100) + \"%\";\r\n }\r\n };\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = \"%0\";\r\n xhr.onreadystatechange = function () {\r\n if ( xhr.readyState == 2 ){\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = \"正在校验\";\r\n }else if (xhr.readyState == 4) {\r\n if( xhr.status == 201){\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = \"上传成功\";\r\n index=index+1;\r\n sendFile(fileObj, index);\r\n }else{\r\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML = \"上传失败\";\r\n }\r\n }\r\n }\r\n xhr.send(fileObj[index]);\r\n }\r\n function reboot(){\r\n xhr = new XMLHttpRequest();\r\n xhr.open(\"post\", \"/control\", true);\r\n xhr.onreadystatechange = function () {\r\n if (xhr.readyState == 4) {\r\n if( xhr.status == 200){\r\n alert(\"设备正在重启\");\r\n }else{\r\n alert(\"设备重启失败\");\r\n }\r\n }\r\n }\r\n xhr.send(\"{\\\"Action\\\":0}\");\r\n }\r\n function upgread(){\r\n var file = document.getElementById(\"binFile\").files[0];\r\n if(file == null){\r\n alert(\"请选择固件\");\r\n return;\r\n }\r\n var t = document.getElementById('tab');\r\n xhr = new XMLHttpRequest();\r\n xhr.upload.onprogress = function progressFunction(evt) {\r\n if (evt.lengthComputable) {\r\n t.rows[3].cells[2].innerHTML= Math.round(evt.loaded / evt.total * 100) + \"%\";\r\n }\r\n };\r\n xhr.open(\"put\", \"/upgrade\", true);\r\n t.rows[3].cells[2].innerHTML = \"0%\";\r\n xhr.onreadystatechange = function () {\r\n if ( xhr.readyState == 2 ){\r\n t.rows[3].cells[2].innerHTML = \"正在校验\";\r\n }else if (xhr.readyState == 4) {\r\n if( xhr.status == 201){\r\n t.rows[3].cells[2].innerHTML = \"上传成功\";\r\n alert(\"升级成功，设备正在重启\");\r\n }else{\r\n t.rows[3].cells[2].innerHTML = \"上传失败\";\r\n alert(\"升级失败\");\r\n }\r\n }\r\n }\r\n xhr.send(file);\r\n }\r\n </script>\r\n</body>\r\n</html>\r\n";
const CHAR HTML_UploadHtml[] ICACHE_RODATA_ATTR   = "<!DOCTYPE html>\n<html>\n<head>\n <meta http-equiv=\"content-type\" content=\"text/html,charset=gb2312\">\n <title>升级</title>\n</head>\n<body>\n <table border=\"0\" width=\"70%\" align=\"center\" cellpadding=\"6\" id=\"tab\" cellspacing=\"0\" >\n <tr><th colspan=\"4\">固件升级</th></tr>\n <tr><td colspan=\"4\"><hr/></td></tr>\n <tr align=\"left\">\n <th width=\"40%\">文件</th>\n <th width=\"15%\">大小</th>\n <th width=\"20%\">状态</th>\n <th width=\"25%\"></th>\n </tr>\n <tr align=\"left\">\n <td><input type=\"file\" id=\"binFile\" accept=\".bin\" onchange=\"return fileChg(this);\"></td>\n <td>----</td>\n <td>----</td>\n <td><input type=\"button\" onclick=\"upgread()\" value=\"升级\"/></td>\n </tr>\n <tr><td colspan=\"4\"><hr/></td></tr>\n <tr><td colspan=\"4\">&nbsp;</td></tr>\n <tr><th colspan=\"4\">网页升级</th></tr>\n <tr><td colspan=\"4\"><hr/></td></tr>\n <tr><td colspan=\"4\"><hr/></td></tr>\n <tr>\n <td colspan=\"3\"></td>\n <td>\n <input type=\"button\" onclick=\"addFile()\" value=\"添加\"/>\n <input type=\"button\" onclick=\"uploadFile()\" value=\"上传\"/>\n <input type=\"button\" onclick=\"reboot()\" value=\"重启\"/>\n </td>\n </tr>\n </table>\n <script type=\"text/javascript\">\n window.onload=function() {\n addFile();\n }\n function addFile() {\n var t=document.getElementById('tab');\n var r=t.insertRow(t.rows.length-2);\n r.insertCell(0).innerHTML=\"<input type=\\\"file\\\" onchange=\\\"return fileChg(this);\\\">\";\n r.insertCell(1).innerHTML=\"----\";\n r.insertCell(2).innerHTML=\"----\";\n r.insertCell(3).innerHTML=\"<a href=\\\"javascript:void(0);\\\" onclick=\\\"this.parentNode.parentNode.parentNode.removeChild(this.parentNode.parentNode)\\\">删除</a>\";\n }\n function fileChg(obj) {\n var fz=obj.files[0].size;\n if( fz > 1024*1024 ){\n fz=(fz/1024/1024).toFixed(1) + \"MB\";\n }else if(fz > 1024){\n fz=(fz/1024).toFixed(1) + \"KB\";\n }else{\n fz=fz+\"B\";\n }\n var sta=obj.parentNode.parentNode.cells;\n sta[1].innerHTML=fz;\n sta[2].innerHTML=\"等待上传\";\n }\n\n function uploadFile() {\n var files=new Array();\n var tableObj=document.getElementById(\"tab\");\n for (var i=8; i < tableObj.rows.length-2; i++) {\n file=tableObj.rows[i].cells[0].getElementsByTagName(\"input\")[0];\n if ( file.files[0]==null ){\n continue;\n }\n files.push(file.files[0]);\n tableObj.rows[i].cells[2].innerHTML=\"等待上传\";\n }\n if (files.length==0){\n alert(\"请选择文件！\");\n return;\n }\n if( sendHead(files)){\n sendFile(files, 0);\n }\n\n }\n function sendHead(fileObj) {\n var dataArr=[];\n for ( var i in fileObj ){\n var data={};\n data.Name=fileObj[i].name;\n data.Length=parseInt(fileObj[i].size);\n dataArr.push(data);\n }\n xhr=new XMLHttpRequest();\n xhr.open(\"post\", \"/html/header\", false);\n xhr.send(JSON.stringify(dataArr));\n return true;\n }\n function sendFile(fileObj, index) {\n if ( index >= fileObj.length){\n alert(\"上传完成\");\n return;\n }\n var t=document.getElementById('tab');\n xhr=new XMLHttpRequest();\n url=\"/html/\"+fileObj[index].name\n xhr.open(\"put\", url, true);\n xhr.upload.onprogress=function progressFunction(evt) {\n if (evt.lengthComputable) {\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML=Math.round(evt.loaded / evt.total * 100) + \"%\";\n }\n };\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML=\"%0\";\n xhr.onreadystatechange=function () {\n if ( xhr.readyState==2 ){\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML=\"正在校验\";\n }else if (xhr.readyState==4) {\n if( xhr.status==201){\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML=\"上传成功\";\n index=index+1;\n sleep(100);\n sendFile(fileObj, index);\n }else{\n t.rows[parseInt(8)+parseInt(index)].cells[2].innerHTML=\"上传失败\";\n }\n }\n }\n xhr.send(fileObj[index]);\n }\n function reboot(){\n xhr=new XMLHttpRequest();\n xhr.open(\"post\", \"/control\", true);\n xhr.onreadystatechange=function () {\n if (xhr.readyState==4) {\n if( xhr.status==200){\n alert(\"设备正在重启\");\n }else{\n alert(\"设备重启失败\");\n }\n }\n }\n xhr.send(\"{\\\"Action\\\":0}\");\n }\n function upgread(){\n var file=document.getElementById(\"binFile\").files[0];\n if(file==null){\n alert(\"请选择固件\");\n return;\n }\n var t=document.getElementById('tab');\n xhr=new XMLHttpRequest();\n xhr.upload.onprogress=function progressFunction(evt) {\n if (evt.lengthComputable) {\n t.rows[3].cells[2].innerHTML= Math.round(evt.loaded / evt.total * 100) + \"%\";\n }\n };\n xhr.open(\"put\", \"/upgrade\", true);\n t.rows[3].cells[2].innerHTML=\"0%\";\n xhr.onreadystatechange=function () {\n if ( xhr.readyState==2 ){\n t.rows[3].cells[2].innerHTML=\"正在校验\";\n }else if (xhr.readyState==4) {\n if( xhr.status==201){\n t.rows[3].cells[2].innerHTML=\"上传成功\";\n alert(\"升级成功，设备正在重启\");\n }else{\n t.rows[3].cells[2].innerHTML=\"上传失败\";\n alert(\"升级失败\");\n }\n }\n }\n xhr.send(file);\n }\n function sleep(time) {\n var s = new Date().getTime() + parseInt(time, 10);\n while(new Date().getTime()<s) {}\n };\n </script>\n</body>\n</html>\n";
const CHAR HTML_RedirectHtml[] ICACHE_RODATA_ATTR = "<!DOCTYPE html>\n <html>\n <head>\n <meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,minimum-scale=1.0,user-scalable=no\">\n </head>\n <style>\n a{text-decoration:none;cursor:pointer}\n table{position:absolute;left:0;top:0;right:0;bottom:50%;margin:auto}\n </style>\n <body bgcolor=\"#33CCFF\" onload=\"ready()\">\n <table border=\"0\" width=\"80%\" cellpadding=\"10\" id=\"tab\" cellspacing=\"0\">\n <tr><td align=\"center\">选择任意一种方式接入设备：</td></tr>\n <tr><td align=\"center\"><a id=\"apMode\" onclick=\"apClick()\">1, 直接点击进入: http://192.168.4.1</a></td></tr>\n <tr><td align=\"center\"><a id=\"stationMode\" onclick=\"stationClick()\"></a></td></tr>\n </table>\n <script type=\"text/javascript\">\n var sysData;\n function apClick() {\n window.location.href = \"http://192.168.4.1\";\n }\n function stationClick() {\n window.location.href = \"http://\" + sysData.IP;\n }\n function ready() {\n sleep(100);\n stat = document.getElementById(\"stationMode\");\n xhr = new XMLHttpRequest();\n xhr.open(\"get\", \"/system\", true);\n xhr.onreadystatechange = function() {\n if(xhr.readyState == XMLHttpRequest.DONE && xhr.status == 200){\n sysData = JSON.parse(xhr.responseText);\n var m = sysData.WifiMode;\n if ((m == 1 || m == 3) && sysData.IP != \"0.0.0.0\") {\n stat.innerHTML = \"2, 先连接到wifi \\\"\" + sysData.WifiSSID + \"\\\", 再点击进入: http://\" + sysData.IP;\n }\n }\n } \n xhr.send(\"{}\")\n }\n function sleep(time) {\n var s = new Date().getTime() + parseInt(time, 10);\n while(new Date().getTime() < s ) { \n }\n };\n </script> \n </body>\n </html>";

const CHAR HTML_NotFound[] ICACHE_RODATA_ATTR   = "<html><head><title>404 Not Found</title></head><center><h1>404 Not Found</h1></center><hr><center>SmartPlug</center></body></html>";
const CHAR HTML_ResultOk[] ICACHE_RODATA_ATTR   = "{\"result\":\"success\", \"msg\":\"\"}";
const CHAR HTML_BadRequest[] ICACHE_RODATA_ATTR = "{\"result\":\"failed\", \"msg\":\"bad request\"}";
const CHAR HTML_InternalServerError[] ICACHE_RODATA_ATTR ="{\"result\":\"failed\", \"msg\":\"internal server error\"}";

HTTP_FILE_LIST_S astHttpHtmlData[HTTP_FILE_NUM_MAX];


HTTP_FILE_S* HTTP_FileOpen( CHAR *pcFileName )
{
    HTTP_FILE_LIST_S* pstHtmlData = NULL;
    HTTP_FILE_S* pstFile = NULL;

    if ( NULL == pcFileName )
    {
        LOG_OUT(LOGOUT_ERROR, "pcFileName:%p", pcFileName);
        return NULL;
    }

    pstHtmlData = HTTP_GetFileList(pcFileName);
    if ( NULL == pstHtmlData )
    {
        LOG_OUT(LOGOUT_ERROR, "%s not found", pcFileName);
        return NULL;
    }

    pstFile = malloc(sizeof(HTTP_FILE_S));
    if ( NULL == pstFile )
    {
        LOG_OUT(LOGOUT_ERROR, "malloc pstFile failed");
        return NULL;
    }

    pstFile->pcName        = pstHtmlData->szName;
    pstFile->eStatus    = STATUS_Open;
    pstFile->uiAddr        = pstHtmlData->uiAddr;
    pstFile->uiLength    = pstHtmlData->uiLength;
    pstFile->uiPos        = 0;
    pstFile->eType        = pstHtmlData->eType;
    pstFile->eEncode    = pstHtmlData->eEncode;
    return pstFile;
}

UINT HTTP_FileClose( HTTP_FILE_S** ppstFile )
{
    if ( NULL == ppstFile || NULL == *ppstFile)
    {
        LOG_OUT(LOGOUT_ERROR, "ppstFile:%p, *ppstFile:%p", ppstFile, *ppstFile);
        return RESULT_Fail;
    }

    memset(*ppstFile, 0, sizeof(HTTP_FILE_S));
    FREE_MEM(*ppstFile);
    *ppstFile = NULL;

    return RESULT_Success;
}

UINT HTTP_ReadFile( HTTP_FILE_S* pstFile, CHAR *pcBuf, UINT uiLen )
{
    UINT uiRet = 0;

    if ( NULL == pstFile )
    {
        LOG_OUT(LOGOUT_ERROR, "pstFile:%p", pstFile);
        return RESULT_Fail;
    }

    if( pstFile->eStatus != STATUS_Open)
    {
        LOG_OUT(LOGOUT_ERROR, "%s not open", pstFile->pcName);
        return RESULT_Fail;
    }

    //memcpy(pcBuf, pstFile->uiAddr + pstFile->uiPos, uiLen);
    uiRet = FlASH_Read(pstFile->uiAddr + pstFile->uiPos,
                        pcBuf,
                        uiLen);
    if ( uiRet != OK  )
    {
        LOG_OUT(LOGOUT_ERROR, "read %s failed, FlASH_Read failed", pstFile->pcName);
        return RESULT_Fail;
    }

    pstFile->uiPos = pstFile->uiPos + uiLen;
    if ( pstFile->uiPos >= pstFile->uiLength )
    {
        return RESULT_Finish;
    }

    return RESULT_Success;
}

UINT HTTP_WriteFile( HTTP_FILE_S* pstFile, CHAR *pcBuf, UINT uiLen )
{
    UINT uiRet = 0;
    CHAR szName[HTTP_FILE_NAME_MAX_LEN];
    HTTP_FILE_LIST_S* pstHtmlData = NULL;

    if ( NULL == pstFile )
    {
        LOG_OUT(LOGOUT_ERROR, "pstFile:%p", pstFile);
        return RESULT_Fail;
    }

    if( pstFile->eStatus != STATUS_Open)
    {
        LOG_OUT(LOGOUT_ERROR, "%s not open", pstFile->pcName);
        return RESULT_Fail;
    }

    uiRet = FlASH_Write(pstFile->uiAddr + pstFile->uiPos,
                        pcBuf,
                        uiLen);
    if ( uiRet != OK  )
    {
        LOG_OUT(LOGOUT_ERROR, "write %s failed, FlASH_Read failed", pstFile->pcName);
        return RESULT_Fail;
    }
    pstFile->uiPos = pstFile->uiPos + uiLen;

    //写入完成
    if ( pstFile->uiPos >= pstFile->uiLength )
    {
        pstHtmlData = HTTP_GetFileList( pstFile->pcName );
        if ( NULL == pstHtmlData )
        {
            LOG_OUT(LOGOUT_ERROR, "%s not found", pstFile->pcName);
            return RESULT_Fail;
        }

        pstHtmlData->bIsUpload = TRUE;

        if ( pstHtmlData->eEncode != HTTP_ENCODING_Buff )
        {
            pstHtmlData->szName[strlen(pstHtmlData->szName)-strlen(aGzipSuffix[pstHtmlData->eEncode])] = '\0';
        }

        if ( HTTP_SaveFileListToFlash() != OK )
        {
            LOG_OUT(LOGOUT_ERROR, "save file list failed, fileName:%s", pstFile->pcName);
            return RESULT_Fail;
        }
        sprintf(szName, "/%s", pstHtmlData->szName);
        uiRet = HTTP_RouterRegiste(HTTP_METHOD_GET, szName, HTTP_GetHtml, szName);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "Route registe:%s failed", szName);
        }
        else
        {
            LOG_OUT( LOGOUT_INFO, "Route registe:%s", szName);
        }

        return RESULT_Finish;
    }

    return RESULT_Success;
}

UINT HTTP_SendFile( HTTP_CTX *pstCtx, HTTP_FILE_S* pstFile )
{
    UINT uiBodyLen = 0;
    UINT uiRet = 0;

    //暂定，后边http响应头长度确定后再调整
    pstCtx->stResp.uiSendTotalLen = pstFile->uiLength + HTTP_HEADER_LEN;

    //发送结束时推出
    while ( !HTTP_IS_SEND_FINISH(pstCtx) )
    {
        //申请内存
        //if ( system_get_free_heap_size() > HTTP_BUF_4K*10 )
        //{
        //    HTTP_Malloc(pstCtx, HTTP_BUF_4K);
            //LOG_OUT(LOGOUT_INFO, "%d, 4K %s", system_get_free_heap_size(), pstFile->pcName);
       // }
       // else
        //{
            HTTP_Malloc(pstCtx, WEB_SENDBUF_SIZE);
            //LOG_OUT(LOGOUT_INFO, "%d, 1K %s", system_get_free_heap_size(), pstFile->pcName);
       // }

        //首次发送时发送http头部
        if ( pstCtx->stResp.uiSentLen == 0 )
        {
            pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
            pstCtx->stResp.eContentType  = pstFile->eType;
            pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_MaxAge_1y;

            HTTP_SetRespHeader( pstCtx );

            if ( pstFile->eEncode != HTTP_ENCODING_Buff )
            {
                HTTP_SetCustomHeader(pstCtx, "Content-encoding", szHttpEncodingStr[pstFile->eEncode]);
            }

            HTTP_SetBodyLen(pstCtx, pstFile->uiLength);
        }

        //确定本次发送body体的长度
        if ( pstCtx->stResp.uiSendTotalLen - pstCtx->stResp.uiSentLen > pstCtx->stResp.uiSendBufLen )
        {
            if ( pstCtx->stResp.uiSentLen == 0 )
            {
                //首次发送因为要发送http响应头
                uiBodyLen = pstFile->uiLength > (pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiHeaderLen) ? (pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiHeaderLen) : pstFile->uiLength;
              }
            else
            {
                uiBodyLen = pstCtx->stResp.uiSendBufLen;
            }
        }
        else
        {
            uiBodyLen = pstCtx->stResp.uiSendTotalLen - pstCtx->stResp.uiSentLen;
        }

        uiRet = HTTP_ReadFile( pstFile,
                pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
                uiBodyLen);
        if ( uiRet == RESULT_Fail )
        {
            LOG_OUT(LOGOUT_ERROR, "HTTP_ReadFile %s failed", pstFile->pcName);
            return FAIL;
        }

        pstCtx->stResp.uiSendCurLen = pstCtx->stResp.uiPos + uiBodyLen;
        pstCtx->stResp.uiPos = 0;

        LOG_OUT(LOGOUT_DEBUG, "send process:%d",
                pstCtx->stResp.uiSentLen * 100 / pstCtx->stResp.uiSendTotalLen);

        uiRet = WEB_WebSend(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT(LOGOUT_ERROR, "send %s failed", pstFile->pcName);
            return FAIL;
        }
    }

    return OK;
}

UINT HTTP_SendMultiple( HTTP_CTX *pstCtx, const CHAR* pcContent )
{
    UINT uiRet = 0;
    UINT uiBodyLen = 0;
    UINT uiContentLen = 0;
    UINT uiPos = 0;

    uiContentLen = strlen(pcContent);
    //暂定，后边http响应头长度确定后再调整
    pstCtx->stResp.uiSendTotalLen = uiContentLen + HTTP_HEADER_LEN;

    //发送结束时推出
    while ( !HTTP_IS_SEND_FINISH(pstCtx) )
    {
        HTTP_Malloc(pstCtx, WEB_SENDBUF_SIZE);

        //首次发送时发送http头部
        if ( pstCtx->stResp.uiSentLen == 0 )
        {
            pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
            pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Html;
            pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_MaxAge_1y;

            uiRet = HTTP_SetRespHeader( pstCtx );
            if ( uiRet != OK )
            {
                LOG_OUT( LOGOUT_ERROR, "set header failed");
                return FAIL;
            }

            uiRet = HTTP_SetBodyLen(pstCtx, uiContentLen);
            if ( uiRet != OK )
            {
                LOG_OUT( LOGOUT_ERROR, "set body len failed");
                return FAIL;
            }
        }

        //确定本次发送body体的长度
        if ( pstCtx->stResp.uiSendTotalLen - pstCtx->stResp.uiSentLen > pstCtx->stResp.uiSendBufLen )
        {
            if ( pstCtx->stResp.uiSentLen == 0 )
            {
                //首次发送因为要发送http响应头
                uiBodyLen = uiContentLen > (pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiHeaderLen) ? (pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiHeaderLen) : uiContentLen;
              }
            else
            {
                uiBodyLen = pstCtx->stResp.uiSendBufLen;
            }
        }
        else
        {
            uiBodyLen = pstCtx->stResp.uiSendTotalLen - pstCtx->stResp.uiSentLen;
        }

        memcpy( pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
                pcContent + uiPos,
                uiBodyLen);

        uiPos += uiBodyLen;
        pstCtx->stResp.uiSendCurLen = pstCtx->stResp.uiPos + uiBodyLen;
        pstCtx->stResp.uiPos = 0;

        LOG_OUT(LOGOUT_INFO, "send process:%d",
                pstCtx->stResp.uiSentLen * 100 / pstCtx->stResp.uiSendTotalLen);

        uiRet = WEB_WebSend(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set body len failed");
            return FAIL;
        }
    }
    return OK;
}


VOID HTTP_FileListInit( VOID )
{
    UINT8 i = 0;

    for ( i = 0; i < HTTP_FILE_NUM_MAX; i ++)
    {
        astHttpHtmlData[i].uiLength     = 0;
        astHttpHtmlData[i].uiAddr        = 0;
        astHttpHtmlData[i].bIsUpload    = FALSE;
        astHttpHtmlData[i].eType         = HTTP_CONTENT_TYPE_Stream;
        astHttpHtmlData[i].eEncode         = HTTP_ENCODING_Buff;
        memset( astHttpHtmlData[i].szName, 0, HTTP_FILE_NAME_MAX_LEN);
    }
}


HTTP_FILE_LIST_S* HTTP_GetFileList( CHAR* pcName )
{
    UINT8 i = 0;

    if ( pcName == NULL )
    {
        return &astHttpHtmlData[0];
    }

    for ( i = 0; i < HTTP_FILE_NUM_MAX; i ++)
    {

        if ( strcmp( pcName, astHttpHtmlData[i].szName) == 0)
        {
            return &astHttpHtmlData[i];
        }
    }
    return NULL;
}

UINT HTTP_SaveFileListToFlash( VOID )
{
    UINT uiRet = 0;

    uiRet = CONFIG_SaveConfig(PLUG_MOUDLE_FILELIST);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "HTTP_SaveFileListToFlash failed.");
        return FAIL;
    }
    return OK;
}

UINT32 HTTP_GetFileListLength()
{
    return sizeof(astHttpHtmlData);
}

VOID HTTP_FileListRegiste( VOID )
{
    UINT8 i = 0;
    CHAR szUrl[HTTP_FILE_NAME_MAX_LEN]={};
    UINT uiRet = 0;

    for ( i = 0; i < HTTP_FILE_NUM_MAX; i ++)
    {
        if ( astHttpHtmlData[i].bIsUpload == TRUE && strlen(astHttpHtmlData[i].szName) != 0 )
        {
            snprintf(szUrl, HTTP_FILE_NAME_MAX_LEN, "/%s", astHttpHtmlData[i].szName);
            uiRet = HTTP_RouterRegiste(HTTP_METHOD_GET, szUrl, HTTP_GetHtml, "HTTP_GetHtml");
            if ( uiRet != OK )
            {
                LOG_OUT( LOGOUT_ERROR, "Route registe:%s failed", szUrl);
            }
            else
            {
                LOG_OUT( LOGOUT_INFO, "Route registe:%s", szUrl);
            }

        }
    }
}

UINT HTTP_NotFound( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_NotFound;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Html;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed" );
        return FAIL;
    }

    uiRet = HTTP_SetResponseBody(pstCtx, HTML_NotFound);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}


UINT HTTP_BadRequest( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_BadRequest;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    uiRet = HTTP_SetResponseBody(pstCtx, HTML_BadRequest);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_InternalServerError( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_InternalServerError;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    uiRet = HTTP_SetResponseBody(pstCtx, HTML_InternalServerError);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_GetHome( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    HTTP_FILE_LIST_S* pstFile = NULL;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Found;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Html;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstFile = HTTP_GetFileList( "index.html" );
    if ( pstFile != NULL )
    {
        uiRet = HTTP_SetCustomHeader(pstCtx, "Location", "http://%s/index.html",
        		HTTP_GetReqHeader(&pstCtx->stReq.stHeader[0], "Host"));
    }
    else
    {
        uiRet = HTTP_SetCustomHeader(pstCtx, "Location", "http://%s/upload",
        		HTTP_GetReqHeader(&pstCtx->stReq.stHeader[0], "Host"));
    }

    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set custom header failed");
        return FAIL;
    }

    uiRet = HTTP_SetResponseBody(pstCtx, "");
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_GetHealth( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    uiRet = HTTP_SetResponseBody(pstCtx, "{\"health\":true}");
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_GetInfo( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += WIFI_DeviceInfoMarshalJson(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_GetTimerData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    CHAR szTimer[HTTP_URL_MAX_LEN];
    UINT uiTimerNum = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_2K);

    if ( OK != HTTP_GetRouterPara(pstCtx, "timer", szTimer))
    {
        LOG_OUT(LOGOUT_ERROR, "get timer failed.");
        goto err;
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
        LOG_OUT(LOGOUT_ERROR, "unknow uiTimerNum:%d", uiTimerNum);
        pstCtx->stResp.eHttpCode = HTTP_CODE_BadRequest;
        goto err;
    }

    goto succ;

err:
    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    uiRet = HTTP_SetResponseBody(pstCtx, HTML_BadRequest);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;

succ:
    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += PLUG_MarshalJsonTimer(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
            uiTimerNum);

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_GetDelayData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    CHAR szDelay[HTTP_URL_MAX_LEN];
    UINT uiDelayNum = 0;

    if ( NULL == pstCtx )
    {
        LOG_OUT( LOGOUT_ERROR, "pstCtx:%p", pstCtx);
        return FAIL;
    }

    pstCtx->stResp.eHttpCode     = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_2K + HTTP_BUF_512);

    if ( OK != HTTP_GetRouterPara(pstCtx, "delay", szDelay))
    {
        LOG_OUT(LOGOUT_ERROR, "get delay failed.");
        goto err;
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
        LOG_OUT(LOGOUT_ERROR, "unknow uiDelayNum:%d", uiDelayNum);
        pstCtx->stResp.eHttpCode = HTTP_CODE_BadRequest;
        goto err;
    }

    goto succ;

err:
    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    uiRet = HTTP_SetResponseBody(pstCtx, HTML_BadRequest);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;

succ:
    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += PLUG_MarshalJsonDelay(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
            uiDelayNum);

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }
    return OK;
}


UINT HTTP_GetInfraredData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    CHAR szinfrared[HTTP_URL_MAX_LEN];
    UINT uiNum = 0;

    if ( NULL == pstCtx )
    {
        LOG_OUT( LOGOUT_ERROR, "pstCtx:%p", pstCtx);
        return FAIL;
    }

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_3K);

    if ( OK != HTTP_GetRouterPara(pstCtx, "infrared", szinfrared))
    {
        LOG_OUT(LOGOUT_ERROR, "get infrared failed.");
        goto err;
    }
    else
    {
        if ( strcmp(szinfrared, "all") == 0 )
        {
            uiNum = INFRARED_ALL;
        }
        else
        {
            uiNum = atoi(szinfrared);
        }
    }

    goto succ;

err:
    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    uiRet = HTTP_SetResponseBody(pstCtx, HTML_BadRequest);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;

succ:
    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += PLUG_MarshalJsonInfrared(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos,
            uiNum);

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }
    return OK;
}


UINT HTTP_GetInfraredValue( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    CHAR szBuf[HTTP_URL_MAX_LEN];
    UINT8 ucNum = 0;
    UINT8 ucSwitch = 0;
    UINT uiInfraredValue = 0;

    if ( NULL == pstCtx )
    {
        LOG_OUT( LOGOUT_ERROR, "pstCtx:%p", pstCtx);
        return FAIL;
    }

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_3K);

    if ( OK != HTTP_GetRouterPara(pstCtx, "infrared", szBuf))
    {
        LOG_OUT(LOGOUT_ERROR, "get infrared failed.");
        goto err;
    }
    else
    {
        ucNum = atoi(szBuf);
        if ( ucNum == 0 || ucNum > INFRARED_MAX )
        {
            LOG_OUT(LOGOUT_ERROR, "unknow ucNum:%d", ucNum);
            pstCtx->stResp.eHttpCode = HTTP_CODE_BadRequest;
            goto err;
        }
    }

    if ( OK != HTTP_GetRouterPara(pstCtx, "switch", szBuf))
    {
        LOG_OUT(LOGOUT_ERROR, "get switch failed.");
        goto err;
    }
    else
    {
        if ( strcmp(szBuf, "on") != 0 )
        {
            ucSwitch = 1;
        }
        else if ( strcmp(szBuf, "off") != 0 )
        {
            ucSwitch = 0;
        }
        else
        {
            pstCtx->stResp.eHttpCode = HTTP_CODE_BadRequest;
            LOG_OUT(LOGOUT_ERROR, "unknown switch :%s", szBuf);
            goto err;
        }
    }

    uiInfraredValue = INFRARED_GetInfraredValue( ucNum, ucSwitch, 30 );

    goto succ;

err:
    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    uiRet = HTTP_SetResponseBody(pstCtx, HTML_InternalServerError);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;

succ:
    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    snprintf(szBuf, sizeof(szBuf), "{\"Num\":%d, \"Value\":\"%X\"}", ucNum, uiInfraredValue);
    uiRet = HTTP_SetResponseBody(pstCtx, szBuf);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}


UINT HTTP_GetSystemData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += PLUG_MarshalJsonSystemSet(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }
    return OK;
}


UINT HTTP_GetCloudPlatformData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += PLUG_MarshalJsonCloudPlatformSet(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}


UINT HTTP_GetTemperature( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    CHAR szBuf[30];

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    snprintf(szBuf, sizeof(szBuf), "{\"Temperature\": %2.1f}", TEMP_GetTemperature());
    uiRet = HTTP_SetResponseBody(pstCtx, szBuf);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set response body failed");
        return FAIL;
    }

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}


UINT HTTP_GetHtmlHeader( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_2K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += PLUG_MarshalJsonHtmlData(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_PostHtmlHeader( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if (pstCtx->stReq.uiRecvCurLen < pstCtx->stReq.uiRecvTotalLen)
    {
        return OK;
    }

    uiRet = PLUG_ParseHtmlData(pstCtx->stReq.pcResqBody);
    if ( uiRet != OK )
    {
        pstCtx->stResp.eHttpCode = HTTP_CODE_BadRequest;
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_2K);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        pstCtx->stResp.uiPos += PLUG_MarshalJsonHtmlData(
                pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
                pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}

UINT HTTP_PutHtml( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    static HTTP_FILE_S* pstFile = NULL;
    CHAR szHtmlName[HTTP_FILE_NAME_MAX_LEN];


    if ( pstFile == NULL )
    {
        pstCtx->stResp.eHttpCode      = HTTP_CODE_Created;
        pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
        pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

        if ( OK != HTTP_GetRouterPara(pstCtx, "html", szHtmlName))
        {
            LOG_OUT(LOGOUT_ERROR, "HTTP_PutHtml, get router para html failed.");
            return HTTP_BadRequest(pstCtx);
        }
        else
        {
            pstFile = HTTP_FileOpen( szHtmlName );
            if ( pstFile == NULL )
            {
                LOG_OUT(LOGOUT_ERROR, "open file failed, %s", &pstCtx->stReq.szURL[1]);
                return HTTP_InternalServerError(pstCtx);
            }
        }
    }

    uiRet = HTTP_WriteFile( pstFile, pstCtx->stReq.pcResqBody, pstCtx->stReq.uiRecvCurLen);
    if ( uiRet == RESULT_Fail )
    {
        HTTP_FileClose( &pstFile );

        LOG_OUT(LOGOUT_ERROR, "wtite file failed, %s", &pstCtx->stReq.szURL[1]);
        return HTTP_InternalServerError(pstCtx);

    }
    else if ( uiRet == RESULT_Finish )
    {
        pstCtx->stReq.eProcess = RES_Process_Finished;
        pstCtx->stResp.eHttpCode = HTTP_CODE_Created;
    }

    if (pstCtx->stReq.uiRecvTotalLen > 0)
    {
        //输出下载进度
        LOG_OUT(LOGOUT_INFO, "upload process:%d",
                pstCtx->stReq.uiRecvLen*100 / pstCtx->stReq.uiRecvTotalLen);
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {

        uiRet = HTTP_FileClose( &pstFile );
        if ( uiRet != RESULT_Success )
        {
            LOG_OUT(LOGOUT_ERROR, "close file failed, %s", pstFile->pcName );
            return HTTP_InternalServerError(pstCtx);
        }

        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        uiRet = HTTP_SetResponseBody(pstCtx, HTML_ResultOk);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set response body failed");
            return FAIL;
        }

        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}

UINT HTTP_GetHtml( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    HTTP_FILE_S* pstFile = NULL;

    pstFile = HTTP_FileOpen( &pstCtx->stReq.szURL[1] );
    if ( pstFile == NULL )
    {
        LOG_OUT(LOGOUT_ERROR, "open file failed, %s", &pstCtx->stReq.szURL[1]);
        return HTTP_InternalServerError( pstCtx );
    }

    uiRet = HTTP_SendFile( pstCtx, pstFile );
    if ( uiRet != OK )
    {
        LOG_OUT(LOGOUT_ERROR, "send file failed, %s", pstFile->pcName);
        return FAIL;
    }

    uiRet = HTTP_FileClose( &pstFile );
    if ( uiRet != RESULT_Success )
    {
        LOG_OUT(LOGOUT_ERROR, "close file failed, %s", pstFile->pcName );
        return FAIL;
    }
    return OK;
}

UINT HTTP_GetDate( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_512);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += PLUG_MarshalJsonDate(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_PostTimerData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if (pstCtx->stReq.uiRecvCurLen < pstCtx->stReq.uiRecvTotalLen)
    {
        return OK;
    }

    if ( OK != PLUG_ParseTimerData(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse timer data failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        uiRet = HTTP_SetResponseBody(pstCtx, HTML_ResultOk);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set response body failed");
            return FAIL;
        }
        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}

UINT HTTP_PostDelayData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if (pstCtx->stReq.uiRecvCurLen < pstCtx->stReq.uiRecvTotalLen)
    {
        return OK;
    }

    if ( OK != PLUG_ParseDelayData(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse daley data failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        uiRet = HTTP_SetResponseBody(pstCtx, HTML_ResultOk);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set response body failed");
            return FAIL;
        }
        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}


UINT HTTP_PostInfraredData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if (pstCtx->stReq.uiRecvCurLen < pstCtx->stReq.uiRecvTotalLen)
    {
        return OK;
    }

    if ( OK != PLUG_ParseInfraredData(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse Infrared data failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_2K);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        uiRet = HTTP_SetResponseBody(pstCtx, HTML_ResultOk);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set response body failed");
            return FAIL;
        }

        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}


UINT HTTP_PostSystemData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if (pstCtx->stReq.uiRecvCurLen < pstCtx->stReq.uiRecvTotalLen)
    {
        return OK;
    }

    if ( OK != PLUG_ParseSystemData(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse system data failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        uiRet = HTTP_SetResponseBody(pstCtx, HTML_ResultOk);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set response body failed");
            return FAIL;
        }
        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}

UINT HTTP_GetWebSet( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += PLUG_MarshalJsonWebSet(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_PostWebSet( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if (pstCtx->stReq.uiRecvCurLen < pstCtx->stReq.uiRecvTotalLen)
    {
        return OK;
    }

    if ( OK != PLUG_ParseWebSetData(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse webset data failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        pstCtx->stResp.uiPos += PLUG_MarshalJsonWebSet(
                pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
                pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}

UINT HTTP_PostCloudPlatformData( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if (pstCtx->stReq.uiRecvCurLen < pstCtx->stReq.uiRecvTotalLen)
    {
        return OK;
    }

    if ( OK != PLUG_ParseCloudPlatformData(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse Platform data failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        uiRet = HTTP_SetResponseBody(pstCtx, HTML_ResultOk);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set response body failed");
            return FAIL;
        }
        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}

UINT HTTP_PostDeviceControl( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if (pstCtx->stReq.uiRecvCurLen < pstCtx->stReq.uiRecvTotalLen)
    {
        return OK;
    }

    if ( OK != PLUG_ParseDeviceControlData(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse device control data failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        uiRet = HTTP_SetResponseBody(pstCtx, HTML_ResultOk);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set response body failed");
            return FAIL;
        }
        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}

UINT HTTP_GetScanWifi( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_4K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += WIFI_WifiScanMarshalJson(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_PostDate( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if ( OK != PLUG_ParseDate(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse date failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        pstCtx->stResp.uiPos += PLUG_MarshalJsonDate(
                pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
                pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}


UINT HTTP_PutUpgrade( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    STATIC UINT uiPos = 0;
    STATIC UINT32 uiAddr = 0;

    //取要升级的user.bin的地址
    if ( uiAddr == 0 )
    {
        uiAddr = UPGRADE_GetUpgradeUserBinAddr();
        if ( uiAddr != 0 )
        {
            LOG_OUT(LOGOUT_INFO, "new bin uiAddr:0x%X, length:%d",
                    uiAddr,
                    pstCtx->stReq.uiRecvTotalLen);
        }
    }

    //将要升级的bin文件写入对应flash地址中
    if ( uiAddr != 0 )
    {
        uiRet = FlASH_Write( uiAddr + uiPos,
                             pstCtx->stReq.pcResqBody,
                             pstCtx->stReq.uiRecvCurLen);
        //写失败直接返回500
        if ( uiRet != OK  )
        {
            uiPos = 0;
            uiAddr = 0;

            LOG_OUT(LOGOUT_ERROR, "upgread failed, FlASH_Write failed.");
            return HTTP_InternalServerError( pstCtx );
        }
        else
        {
            if (pstCtx->stReq.uiRecvTotalLen > 0)
            {
                //输出user.bin下载进度
                LOG_OUT(LOGOUT_INFO, "upgrade process:%d",
                        pstCtx->stReq.uiRecvLen*100 / pstCtx->stReq.uiRecvTotalLen);
            }
        }
    }
    //user.bin的地址无效时返回500
    else
    {
        uiPos = 0;
        uiAddr = 0;

        LOG_OUT(LOGOUT_ERROR, "Get user bin addr failed.");
        return HTTP_InternalServerError( pstCtx );
    }

    //更新偏移地址
    uiPos = pstCtx->stReq.uiRecvLen;

    //bin数据接收完成
    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        uiPos = 0;
        uiAddr = 0;

        pstCtx->stResp.eHttpCode = HTTP_CODE_Created;
        pstCtx->stResp.eContentType = HTTP_CONTENT_TYPE_Json;
        pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        uiRet = HTTP_SetResponseBody(pstCtx, HTML_ResultOk);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set response body failed");
            return FAIL;
        }

        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }

        LOG_OUT(LOGOUT_INFO, "new bin download successed.");
        UPGRADE_StartUpgradeRebootTimer();
    }

    return OK;
}

UINT HTTP_GetRelayStatus( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_512);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += PLUG_MarshalJsonRelayStatus(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }

    return OK;
}

UINT HTTP_PostRelayStatus( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if (pstCtx->stReq.uiRecvCurLen < pstCtx->stReq.uiRecvTotalLen)
    {
        return OK;
    }

    if ( OK != PLUG_ParseRelayStatus(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse relay status failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_512);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        pstCtx->stResp.uiPos += PLUG_MarshalJsonRelayStatus(
                pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
                pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}

UINT HTTP_GetRedirectHtml( HTTP_CTX *pstCtx )
{
    return HTTP_SendMultiple(pstCtx, HTML_RedirectHtml);
}

UINT HTTP_GetUploadHtml( HTTP_CTX *pstCtx )
{
    return HTTP_SendMultiple(pstCtx, HTML_UploadHtml);
}

UINT HTTP_GetMeter( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;
    PLUG_WEBSET_S stWebSet = {"meter", ""};

    pstCtx->stResp.eHttpCode     = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    HTTP_Malloc(pstCtx, HTTP_BUF_1K);

    uiRet = HTTP_SetRespHeader( pstCtx );
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "set header failed");
        return FAIL;
    }

    pstCtx->stResp.uiPos += METER_MarshalJsonMeter(
            pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
            pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

    uiRet = HTTP_SendOnce(pstCtx);
    if ( uiRet != OK )
    {
        LOG_OUT( LOGOUT_ERROR, "send once failed");
        return FAIL;
    }
    PLUG_SetWebSetData(&stWebSet);
    return OK;
}

UINT HTTP_PostMeter( HTTP_CTX *pstCtx )
{
    UINT uiRet = 0;

    pstCtx->stResp.eHttpCode      = HTTP_CODE_Ok;
    pstCtx->stResp.eContentType  = HTTP_CONTENT_TYPE_Json;
    pstCtx->stResp.eCacheControl = HTTP_CACHE_CTL_TYPE_No;

    if ( OK != METER_ParseMeterData(pstCtx->stReq.pcResqBody) )
    {
        LOG_OUT( LOGOUT_ERROR, "parse meter data failed");
        return HTTP_InternalServerError( pstCtx );
    }

    if ( pstCtx->stReq.eProcess == RES_Process_Finished )
    {
        HTTP_Malloc(pstCtx, HTTP_BUF_1K);

        uiRet = HTTP_SetRespHeader( pstCtx );
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "set header failed");
            return FAIL;
        }

        pstCtx->stResp.uiPos += METER_MarshalJsonMeter(
                pstCtx->stResp.pcResponBody + pstCtx->stResp.uiPos,
                pstCtx->stResp.uiSendBufLen - pstCtx->stResp.uiPos );

        uiRet = HTTP_SendOnce(pstCtx);
        if ( uiRet != OK )
        {
            LOG_OUT( LOGOUT_ERROR, "send once failed");
            return FAIL;
        }
    }
    return OK;
}

