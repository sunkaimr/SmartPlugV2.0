/*
 * user_http.h
 *
 *  Created on: 2018��11��11��
 *      Author: lenovo
 */

#ifndef __USER_HTTP_H__
#define __USER_HTTP_H__

#define HTTP_HOST_MAX_LEN         30
#define HTTP_URL_MAX_LEN          50
#define HTTP_HANDLE_MAX_LEN       50

#define HTTP_ROUTER_MAP_MAX       45
#define HTTP_HRADER_NUM_MAX       20

#define HTTP_FILE_NAME_MAX_LEN    50
#define HTTP_FILE_NUM_MAX         10


#define HTTP_Malloc(ctx, len)                                            \
do{                                                                      \
    if ( ctx->stResp.pcResponBody != NULL ){                             \
		free(ctx->stResp.pcResponBody);									 \
	}																	 \
	ctx->stResp.pcResponBody = ( CHAR* )malloc( len );                   \
	if ( NULL == ctx->stResp.pcResponBody ){                             \
		LOG_OUT(LOGOUT_ERROR, "malloc buf size:%d failed, Free heap:%d", \
				len, system_get_free_heap_size());                       \
		return FAIL;                                                     \
	}                                                                    \
	ctx->stResp.uiSendBufLen = len;                                      \
}while(0);


#define HTTP_IS_SEND_FINISH(ctx) ((pstCtx->stResp.eProcess == RESP_Process_Finished)||(ctx->stResp.uiSendTotalLen > 0 && ctx->stResp.uiSentLen >= ctx->stResp.uiSendTotalLen))


typedef enum {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_POST,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,

    HTTP_METHOD_BUFF
}HTTP_METHOD_E;

typedef enum {
    RES_Process_None = 0,       //����״̬
    RES_Process_Invalid,        //�յ����������޷�������http header��Ϣ���������ֱ�ӷ���400
    RES_Process_GotHeader,      //http header�������
    RES_Process_GetBody,        //���ڽ���body�壬httpЯ����body������޷�һ��recv�����Ҫ�ֶ�ν���
    RES_Process_Finished,       //��ɽ���,��ʼ���ͳɹ�����Ӧ

    RES_Process_Buff
}HTTP_RES_PROCESS_E;

typedef enum {
    RESP_Process_None = 0,       //δ��ʼ������Ӧ
    RESP_Process_SentHeader,     //��Ӧͷ�ѷ���
    RESP_Process_sendBody,       //���ڷ���body��
    RESP_Process_Finished,       //�������

    RESP_Process_Buff
}HTTP_RESP_PROCESS_E;

typedef enum {
	HTTP_CODE_None,
	HTTP_CODE_Continue,
	HTTP_CODE_SwitchingProtocols,
    HTTP_CODE_Ok,
    HTTP_CODE_Created,
	HTTP_CODE_NoContent,
	HTTP_CODE_Partial_Content,
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

typedef enum {
    HTTP_ENCODING_Gzip = 0,

    HTTP_ENCODING_Buff
}HTTP_ENCODING_E;

typedef enum {
	HTTP_none = 0,
    HTTP_1_1,
	HTTP_websocket,

    HTTP_Protocol_Buff
}HTTP_PROTOCOL_E;

typedef UINT (*ROUTER_FUN)(VOID *pPara);

typedef struct tagHttpResponseHead
{
    HTTP_CODE_E             eHttpCode;
    HTTP_CONTENT_TYPE_E     eContentType;
    HTTP_CACHE_CTL_TYPE_E   eCacheControl;

    HTTP_RESP_PROCESS_E     eProcess;

    UINT                    uiSendBufLen;                    // ���ͻ�������С

    UINT                    uiHeaderLen;                     // ��Ӧͷ����
    UINT                    uiBodyLen;                       // ��Ӧ�峤��
    UINT                    uiPos;                           // λ��

    UINT                    uiSendTotalLen;                  // Ӧ�����ܳ��ȳ���
    UINT                    uiSentLen;                       // �ѷ��ͳ���
    UINT                    uiSendCurLen;                    // ����Ӧ���ͳ���
    CHAR*                   pcResponBody;                    // Ӧ����
    BOOL                    bIsCouldSend;

}HTTP_RESP_S;

typedef struct tagHttpCtxHeader
{
    CHAR*        pcKey;
    CHAR*        pcValue;

}HTTP_HEADER;

typedef struct tagHttpRequest
{
    HTTP_METHOD_E        eMethod;                           // GET, POST PUT DELETE
    CHAR                 szURL[HTTP_URL_MAX_LEN];           // /index.html
    HTTP_HEADER          stHeader[HTTP_HRADER_NUM_MAX];     // http header
    HTTP_PROTOCOL_E		 eProtocol;							// Э������

    CHAR*                pcRouter;                          // ƥ�䵽��Router
    ROUTER_FUN           pfHandler;                         // ������

    HTTP_RES_PROCESS_E   eProcess;
    UINT                 uiRecvTotalLen;                    // ����body�峤�ȣ�������head����
    UINT                 uiRecvLen;                         // ���յ�body�ĳ���
    UINT                 uiRecvCurLen;                      // �����յ�body�ĳ���
    CHAR*                pcResqBody;                        // ������

}HTTP_REQ_S;

typedef struct tagHttpCtx
{
    INT                iClientFd;
    UINT               uiCostTime;
    UINT               uiTimeOut;
    HTTP_REQ_S         stReq;
    HTTP_RESP_S        stResp;

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
    CHAR*                pcName;
    UINT                 uiLength;
    UINT                 uiAddr;
    UINT                 uiPos;
    HTTP_FILE_STATUS_E   eStatus;
    HTTP_CONTENT_TYPE_E  eType;
    HTTP_ENCODING_E      eEncode;
}HTTP_FILE_S;


typedef struct tagHttpRouterMap
{
    HTTP_METHOD_E         eMethod;                                    /* GET, POST PUT DELETE */
    CHAR                  szURL[HTTP_URL_MAX_LEN];                    /* URL��ַ */
    ROUTER_FUN            pfHttpHandler;                              /* �ص����� */
}HTTP_ROUTER_MAP_S;


typedef struct tagHttpFileList
{
    CHAR                 szName[HTTP_FILE_NAME_MAX_LEN];    //�ļ�����
    BOOL                 bIsUpload;                         //�����Ƿ����ϴ�
    UINT32               uiAddr;                            //������FLASH�еĴ�ŵ�ַ
    UINT32               uiLength;                          //���ݳ���
    HTTP_CONTENT_TYPE_E  eType;                             //��������
    HTTP_ENCODING_E      eEncode;                           //ѹ����ʽ
}HTTP_FILE_LIST_S;

typedef enum {
    CLI_ReqProcess_None = 0,       // ����״̬,��δ��ʼ����
	CLI_ReqProcess_SentHeader,     // ����ͷ�������
	CLI_ReqProcess_SentBody,       // �����巢�����
	CLI_ReqProcess_Finish,         // ���������

	CLI_ReqProcess_Buff
}HTTP_CLI_REQ_PROCESS_E;


typedef struct tagHttpCliReq
{
    CHAR*               	pcHeadBuf;                     // ����ͷbuf
    UINT                	uiHeadBufLen;                  // ����ͷ�ĳ���
    UINT                	uiHeadPos;                     // λ��

    CHAR*               	pcBodyBuf;                     // ������buf
    UINT                	uiBodyBufLen;                  // ������ĳ���

    HTTP_CLI_REQ_PROCESS_E 	eProcess;

}HTTP_CLIREQ_S;


typedef enum {
	CLI_ResponProcess_None = 0,       //����״̬
	CLI_ResponProcess_Invalid,        //�յ�����Ӧ�����޷�������http header��Ϣ
	CLI_ResponProcess_GotHeader,      //http header�������
	CLI_ResponProcess_GetBody,        //���ڽ���body�壬httpЯ����body������޷�һ��recv�����Ҫ�ֶ�ν���
	CLI_ResponProcess_Finished,       //��ɽ���

	CLI_ResponProcess_Buff
}HTTP_CLI_RESP_PROCESS_E;

typedef struct tagHttpCliResp
{
    HTTP_CODE_E         	eHttpCode;
    HTTP_CONTENT_TYPE_E 	eContentType;
    HTTP_HEADER         	stHeader[HTTP_HRADER_NUM_MAX];     // http header
    UINT                    uiContentLength;

    HTTP_CLI_RESP_PROCESS_E eProcess;
    CHAR*               	pcHeadBuf;                         // ��Ӧͷbuf
    CHAR*                	pcBodyBuf;                         // ��Ӧ��buf
    CHAR*                	pcBody;                            // ��Ӧ��
    UINT                 	uiHeadBufLen;                      // ��Ӧͷbuf����
    UINT                 	uiBodyBufLen;                      // ��Ӧ��buf����
    UINT                 	uiRecvTotalLen;                    // ��Ӧ�峤�ȣ�������head����
    UINT                 	uiRecvLen;                         // ���յ�body�ĳ���
    UINT                 	uiRecvCurLen;                      // �����յ�body�ĳ���

}HTTP_CLIRESP_S;

typedef struct tagHttpClient
{
    INT                 iSocket;
    UINT                uiTimeOut;
    UINT                uiCost;

    HTTP_CLIREQ_S       stReq;
    HTTP_CLIRESP_S      stReson;

}HTTP_CLIENT_S;


extern const CHAR szHttpMethodStr[][10];
extern const CHAR szHttpUserAgentStringmap[][10];
extern const CHAR szHttpContentTypeStr[][25];
extern const CHAR szHttpEncodingStr[][10];
extern const CHAR szHttpCodeMap[][5];
extern const CHAR aGzipSuffix[][5];

VOID HTTP_RouterInit( VOID );
//extern VOID HTTP_RouterHandle( HTTP_CTX *pstCtx );
//extern INT32 HTTP_ParsingHttpHead( CHAR * pcData, UINT32 uiLen,  HTTP_CTX *pstCtx );
HTTP_FILE_LIST_S* HTTP_GetFileList( CHAR* pcName );
UINT32 HTTP_GetFileListLength();
UINT HTTP_SetReqHeader( HTTP_HEADER *pstHeader, CHAR* pcKey, CHAR*pcValue );
CHAR* HTTP_GetReqHeader( HTTP_HEADER *pstHeader, const CHAR* pcKey );

typedef UINT (*HttpClientHandle)(VOID*);

HTTP_CLIENT_S* HTTP_NewClient(CHAR* pcMethod, CHAR* pcEndpoint, CHAR* pcUrl, CHAR* pcBody, UINT uiLen);
VOID HTTP_DestoryClient(HTTP_CLIENT_S* pstCli);
VOID HTTP_ClientSetHeader(HTTP_CLIENT_S *pstCli, CHAR* pcKey, CHAR* pcValue);
UINT HTTP_ClientDoRequest(HTTP_CLIENT_S *pstCli);
UINT HTTP_ClientDoResponse(HTTP_CLIENT_S *pstCli, HttpClientHandle pfHandle, VOID* pPara);

#endif /* __USER_HTTP_H__ */
