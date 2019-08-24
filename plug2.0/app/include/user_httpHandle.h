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
#define HTTP_BUF_10K	(1024*10)
#define HTTP_BUF_20K	(1024*20)
#define HTTP_BUF_30K	(1024*30)


#define HTTP_FILE_NAME_MAX_LEN	50
#define HTTP_FILE_NUM_MAX		20




extern const CHAR szHttpMethodStr[][10];
extern const CHAR szHttpUserAgentStringmap[][10];
extern const CHAR szHttpContentTypeStr[][25];


VOID HTTP_FileListInit( VOID );
HTTP_FILE_LIST_S* HTTP_GetFileList( CHAR* pcName );
UINT HTTP_SaveFileListToFlash( VOID );
UINT32 HTTP_GetFileListLength();
VOID HTTP_FileListRegiste( VOID );
UINT HTTP_NotFound( HTTP_CTX *pstCtx );
UINT HTTP_BadRequest( HTTP_CTX *pstCtx );
UINT HTTP_InternalServerError( HTTP_CTX *pstCtx );
UINT HTTP_GetHome( HTTP_CTX *pstCtx );
UINT HTTP_GetHealth( HTTP_CTX *pstCtx );
UINT HTTP_GetInfo( HTTP_CTX *pstCtx );
UINT HTTP_GetDate( HTTP_CTX *pstCtx );
UINT HTTP_PutTest( HTTP_CTX *pstCtx );
UINT HTTP_GetTest( HTTP_CTX *pstCtx );
UINT HTTP_GetTimerData( HTTP_CTX *pstCtx );
UINT HTTP_GetDelayData( HTTP_CTX *pstCtx );
UINT HTTP_GetInfraredData( HTTP_CTX *pstCtx );
UINT HTTP_GetInfraredValue( HTTP_CTX *pstCtx );
UINT HTTP_GetSystemData( HTTP_CTX *pstCtx );
UINT HTTP_GetCloudPlatformData( HTTP_CTX *pstCtx );
UINT HTTP_GetTemperature( HTTP_CTX *pstCtx );
UINT HTTP_GetHtmlHeader( HTTP_CTX *pstCtx );
UINT HTTP_PostHtmlHeader( HTTP_CTX *pstCtx );
UINT HTTP_PutHtml( HTTP_CTX *pstCtx );
UINT HTTP_GetHtml( HTTP_CTX *pstCtx );
UINT HTTP_PostTimerData( HTTP_CTX *pstCtx );
UINT HTTP_PostDelayData( HTTP_CTX *pstCtx );
UINT HTTP_PostInfraredData( HTTP_CTX *pstCtx );
UINT HTTP_PostSystemData( HTTP_CTX *pstCtx );
UINT HTTP_PostCloudPlatformData( HTTP_CTX *pstCtx );
UINT HTTP_PostDeviceControl( HTTP_CTX *pstCtx );
UINT HTTP_GetScanWifi( HTTP_CTX *pstCtx );
UINT HTTP_PostDate( HTTP_CTX *pstCtx );
UINT HTTP_PutUpgrade( HTTP_CTX *pstCtx );
UINT HTTP_GetRelayStatus( HTTP_CTX *pstCtx );
UINT HTTP_PostRelayStatus( HTTP_CTX *pstCtx );
UINT WIFI_WifiScanMarshalJson( CHAR* pcBuf, UINT uiBufLen);
UINT WIFI_DeviceInfoMarshalJson( CHAR* pcBuf, UINT uiBufLen);
UINT HTTP_GetUploadHtml( HTTP_CTX *pstCtx );



#endif /* __USER_HTTP_HANDLE_H__ */
