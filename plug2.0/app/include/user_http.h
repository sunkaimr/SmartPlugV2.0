/*
 * user_http.h
 *
 *  Created on: 2018年11月11日
 *      Author: lenovo
 */

#ifndef __USER_HTTP_H__
#define __USER_HTTP_H__

#define HTTP_HOST_MAX_LEN 		30
#define HTTP_URL_MAX_LEN 		50
#define HTTP_HANDLE_MAX_LEN 	50

#define HTTP_ROUTER_MAP_MAX 	40

#define HTTP_FILE_NAME_MAX_LEN	50
#define HTTP_FILE_NUM_MAX		20


#define HTTP_Malloc(ctx, len) 											\
do{																		\
	if ( ctx->stResp.pcResponBody == NULL )								\
	{																	\
		ctx->stResp.pcResponBody = ( CHAR* )malloc( len );				\
		if ( NULL == ctx->stResp.pcResponBody ){						\
			LOG_OUT(LOGOUT_ERROR, "malloc buf size:%d failed, Free heap:%d",\
					len, system_get_free_heap_size());				\
			return FAIL;												\
		}																\
		ctx->stResp.uiSendBufLen = len;									\
	}																	\
}while(0);


#define HTTP_IS_SEND_FINISH(ctx) (ctx->stResp.uiSendTotalLen > 0 && ctx->stResp.uiSentLen >= ctx->stResp.uiSendTotalLen)


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
	HTTP_CODE_Ok = 0,
	HTTP_CODE_Created,
	HTTP_CODE_Found,
	HTTP_CODE_BadRequest,
	HTTP_CODE_NotFound,
	HTTP_CODE_InternalServerError,

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

	UINT 					uiSendBufLen;					// 发送缓冲区大小

	UINT 					uiHeaderLen;					// 响应头长度
	UINT 					uiBodyLen;						// 响应体长度
	UINT 					uiPos;							// 位置

	UINT 					uiSendTotalLen;					// 应发送总长度长度
	UINT 					uiSentLen;						// 已发送长度
	UINT					uiSendCurLen;					// 本次应发送长度
	CHAR*					pcResponBody;					// 应答体
	BOOL					bIsCouldSend;

}HTTP_RESP_S;

typedef struct tagHttpRequest
{
	HTTP_METHOD_E 		eMethod;						// GET, POST PUT DELETE
	CHAR 				szURL[HTTP_URL_MAX_LEN];       	// /index.html
	HTTP_USERAGENT_E 	eUserAgent;	    				// windows, Android,
	CHAR 				szHost[HTTP_HOST_MAX_LEN];     	// 192.168.0.102:8080

	CHAR*               pcRouter;						//匹配到的Router

	HTTP_PROCESS_E		eProcess;
	UINT 				uiRecvTotalLen;					// 请求body体长度，不包括head长度
	UINT 				uiRecvLen;						// 已收到body的长度
	UINT 				uiRecvCurLen;					// 本次收到body的长度
	CHAR*				pcResqBody;						// 请求体

}HTTP_REQ_S;

typedef struct tagHttpCtx
{
	INT				iClientFd;
	UINT			uiCostTime;
	HTTP_REQ_S     	stReq;
	HTTP_RESP_S		stResp;

}HTTP_CTX;

typedef enum {
	RESULT_Fail = 0,
	RESULT_Success,
	RESULT_Finish,

	RESULT_Buff
}HTTP_FILE_RESULT_E;

typedef enum {
	STATUS_Close = 0,
	STATUS_Open,

	STATUS_Buff
}HTTP_FILE_STATUS_E;

typedef struct tagHttpFile
{
	CHAR*				pcName;
	UINT				uiLength;
	UINT				uiAddr;
	UINT				uiPos;
	HTTP_FILE_STATUS_E	eStatus;
	HTTP_CONTENT_TYPE_E	eType;
}HTTP_FILE_S;

typedef UINT (*ROUTER_FUN)(HTTP_CTX *pstCtx);

typedef struct tagHttpRouterMap
{
	HTTP_METHOD_E 		eMethod;									/* GET, POST PUT DELETE */
	CHAR 				szURL[HTTP_URL_MAX_LEN];    				/* URL地址 */
	ROUTER_FUN 			pfHttpHandler;								/* 回调函数 */
}HTTP_ROUTER_MAP_S;


typedef struct tagHttpFileList
{
	CHAR 				szName[HTTP_FILE_NAME_MAX_LEN];	//文件名称
	BOOL				bIsUpload;						//数据是否已上传
	UINT32				uiAddr;							//数据在FLASH中的存放地址
	UINT32				uiLength;						//数据长度
	HTTP_CONTENT_TYPE_E eType;							//数据类型
}HTTP_FILE_LIST_S;


extern const CHAR szHttpMethodStr[][10];
extern const CHAR szHttpUserAgentStringmap[][10];
extern const CHAR szHttpContentTypeStr[][25];

VOID HTTP_RouterInit( VOID );
//extern VOID HTTP_RouterHandle( HTTP_CTX *pstCtx );
//extern INT32 HTTP_ParsingHttpHead( CHAR * pcData, UINT32 uiLen,  HTTP_CTX *pstCtx );
HTTP_FILE_LIST_S* HTTP_GetFileList( CHAR* pcName );
UINT32 HTTP_GetFileListLength();


#endif /* __USER_HTTP_H__ */
