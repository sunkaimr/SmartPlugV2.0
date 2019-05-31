/*
 * user_http.h
 *
 *  Created on: 2018年11月11日
 *      Author: lenovo
 */

#ifndef __USER_HTTP_H__
#define __USER_HTTP_H__

#define HTTP_HOST_MAX_LEN 		30
#define HTTP_URL_MAX_LEN 		100
#define HTTP_HANDLE_MAX_LEN 	100

#define HTTP_ROUTER_MAP_MAX 	40

#define HTTP_HTML_NAME_MAX_LEN	100
#define HTTP_HTML_DATE_MAX		20


typedef enum {
	HTTP_METHOD_GET = 0,
	HTTP_METHOD_POST,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,

	HTTP_METHOD_BUFF
}HTTP_METHOD_E;


typedef enum {
	HTTP_USERAGENT_WINDOWNS = 0,	/* windowns 平台的浏览器发起的请求如IE、chrome登*/
	HTTP_USERAGENT_ANDROID,			/* 安卓手机端发起的请求 */
	HTTP_USERAGENT_CURL,			/* curl命令发起的请求 */
	HTTP_USERAGENT_WGET,			/* wget命令发起的请求 */

	HTTP_USERAGENT_BUFF
}HTTP_USERAGENT_E;


typedef enum {
	HTTP_PROCESS_None = 0,		//空闲状态
	HTTP_PROCESS_Invalid,		//收到了请求但是无法解析出http header信息，这种情况直接返回400
	HTTP_PROCESS_GotHeader,		//http header解析完成
	HTTP_PROCESS_GetBody,		//http携带的body体过长无法一次recv完成需要分多次接收
	HTTP_PROCESS_Finished,		//完成接收,开始发送成功的响应

	HTTP_PROCESS_Buff
}HTTP_PROCESS_E;


typedef enum {
	HTTP_CODE_Ok 					= 200,
	HTTP_CODE_Created 				= 201,
	HTTP_CODE_Found 				= 302,
	HTTP_CODE_BadRequest			= 400,
	HTTP_CODE_NotFound				= 404,
	HTTP_CODE_InternalServerError	= 500,

	HTTP_CODE_Buff
}HTTP_CODE_E;

typedef enum {
	HTTP_CONTENT_TYPE_Html = 0,
	HTTP_CONTENT_TYPE_Js,
	HTTP_CONTENT_TYPE_Css,
	HTTP_CONTENT_TYPE_Json,
	HTTP_CONTENT_TYPE_Icon,
	HTTP_CONTENT_TYPE_Png,
	HTTP_CONTENT_TYPE_Gif,

	HTTP_CONTENT_TYPE_Stream,
	HTTP_CONTENT_TYPE_Buff
}HTTP_CONTENT_TYPE_E;

typedef enum {
	HTTP_CACHE_CTL_TYPE_No = 0,
	HTTP_CACHE_CTL_TYPE_MaxAge_1h,
	HTTP_CACHE_CTL_TYPE_MaxAge_1d,
	HTTP_CACHE_CTL_TYPE_MaxAge_1w,
	HTTP_CACHE_CTL_TYPE_MaxAge_1m,
	HTTP_CACHE_CTL_TYPE_MaxAge_1y,

	HTTP_CACHE_CTL_TYPE_Buff
}HTTP_CACHE_CTL_TYPE_E;


typedef struct tagHttpResponseHead
{
	HTTP_CODE_E				eHttpCode;
	HTTP_CONTENT_TYPE_E 	eContentType;
	HTTP_CACHE_CTL_TYPE_E	eCacheControl;

}HTTP_RESPONSE_HEAD_S;

typedef struct tagHttpRequestHead
{
	HTTP_METHOD_E 		eMethod;						// GET, POST PUT DELETE
	CHAR 				szURL[HTTP_URL_MAX_LEN];       	// /index.html
	HTTP_USERAGENT_E 	eUserAgent;	    				// windows, Android,
	CHAR 				szHost[HTTP_HOST_MAX_LEN];     	// 192.168.0.102:8080
	UINT 				uiContentLenth;					// 请求body体长度，不包括head长度

	CHAR*               pcRouter;						//匹配到的Router

	HTTP_PROCESS_E		eProcess;
	UINT 				uiRecvPresentLenth;				// 已收到body的长度
	UINT 				uiRecvCurLenth;					// 本次收到body的长度
	CHAR*				pcResqBody;						// 请求体

	BOOL				bIsCouldSend;
	UINT 				uiSendTotalLength;				// 应发送总长度长度
	UINT 				uiSentLength;					// 已发送长度
	UINT				uiSendCurLength;				// 本次应发送长度
	CHAR*				pcResponBody;					// 应答体
}HTTP_REQUEST_HEAD_S;

typedef VOID(*ROUTER)(HTTP_REQUEST_HEAD_S*);

typedef struct tagHttpRouterMap
{
	HTTP_METHOD_E 		eMethod;									/* GET, POST PUT DELETE */
	CHAR 				szURL[HTTP_URL_MAX_LEN];    				/* URL地址 */
	CHAR 				szHttpHandlerStr[HTTP_HANDLE_MAX_LEN];    	/* 路由名称 */
	ROUTER 				pfHttpHandler;								/* 回调函数 */
}HTTP_ROUTER_MAP_S;



typedef struct tagHttpHtmlData
{
	CHAR 				szName[HTTP_HTML_NAME_MAX_LEN];	//html名称
	BOOL				bIsUpload;						//数据是否已上传
	UINT32				uiAddr;							//数据在FLASH中的存放地址
	UINT32				uiLength;						//数据长度
	HTTP_CONTENT_TYPE_E eType;							//数据类型
}HTTP_HTMLDATA_S;


extern const CHAR szHttpMethodStringmap[][10];
extern const CHAR szHttpUserAgentStringmap[][10];
extern const CHAR szHttpContentTypeStr[][30];

VOID HTTP_RouterInit( VOID );
VOID HTTP_RouterHandle( HTTP_REQUEST_HEAD_S *pstHeader );
INT32 HTTP_ParsingHttpHead( CHAR* pcData, UINT32 uiLen, HTTP_REQUEST_HEAD_S *pstHttpHead );
HTTP_HTMLDATA_S* HTTP_GetHtmlData( CHAR* pcName );
UINT32 HTTP_GetHtmlDataLength();


#endif /* __USER_HTTP_H__ */
