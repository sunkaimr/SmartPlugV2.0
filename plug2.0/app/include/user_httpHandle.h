/*
 * user_httpHandle.h
 *
 *  Created on: 2018Äê11ÔÂ11ÈÕ
 *      Author: lenovo
 */

#ifndef __USER_HTTP_HANDLE_H__
#define __USER_HTTP_HANDLE_H__

#define HTTP_BUF_512	(512)
#define HTTP_BUF_1K		(1024)
#define HTTP_BUF_2K		(1024*2)
#define HTTP_BUF_3K		(1024*3)
#define HTTP_BUF_4K		(1024*4)
#define HTTP_BUF_5K		(1024*5)





#define HTTP_HTML_NAME_MAX_LEN	100
#define HTTP_HTML_DATE_MAX		20




extern const CHAR szHttpMethodStringmap[][10];
extern const CHAR szHttpUserAgentStringmap[][10];
extern const CHAR szHttpContentTypeStr[][30];


VOID HTTP_HtmlDataInit( VOID );
HTTP_HTMLDATA_S* HTTP_GetHtmlData( CHAR* pcName );
UINT HTTP_SaveHtmlData( VOID );
UINT32 HTTP_GetHtmlDataLength();
VOID HTTP_HtmlUrlRegiste( VOID );
VOID HTTP_NotFound( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_BadRequest( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_InternalServerError( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetHome( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetHealth( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetInfo( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetDate( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PutTest( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetTest( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetTimerData( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetDelayData( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetSystemData( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetHtmlHeader( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PostHtmlHeader( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PutHtml( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetHtml( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PostTimerData( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PostDelayData( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PostSystemData( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PostDeviceControl( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetScanWifi( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PostDate( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PutUpgrade( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_GetRelayStatus( HTTP_REQUEST_HEAD_S *pstHeader );
VOID HTTP_PostRelayStatus( HTTP_REQUEST_HEAD_S *pstHeader );
UINT HTTP_WifiScanMarshalJson( CHAR* pcBuf, UINT uiBufLen);
UINT HTTP_DeviceInfoMarshalJson( CHAR* pcBuf, UINT uiBufLen);
VOID HTTP_GetUpload( HTTP_REQUEST_HEAD_S *pstHeader );
















#endif /* __USER_HTTP_HANDLE_H__ */
