#include "user_common.h"

#define LOG_PREFIX_BUF_SIZE      (100)
#define LOG_BUF_SIZE             (1024)


//日志默认INFO级别
static UINT uiLogLevel = LOGOUT_INFO;
static CHAR *g_pcLogBuf = NULL;

static xSemaphoreHandle  xMutex = NULL;


VOID LOG_LogInit( VOID )
{
    //printf("LOG_LogInit start.\r\n");

     g_pcLogBuf = (CHAR*)malloc( LOG_PREFIX_BUF_SIZE + LOG_BUF_SIZE + 5 );
    if ( NULL == g_pcLogBuf )
    {
        printf("[ERROR][%s:%d][%s]# malloc failed, Free heap:%d",
                __FILE__, __LINE__, __func__, system_get_free_heap_size());
        while(1);
    }
    //printf("LOG_LogInit over.\r\n");

    xMutex = xSemaphoreCreateRecursiveMutex();
    if( xMutex == NULL )
    {
        printf("[ERROR][%s:%d][%s]# xSemaphoreCreateRecursiveMutex failed",
                __FILE__, __LINE__, __func__);
        while(1);
    }
}

VOID LOG_SetLogLevel( UINT uiLevel )
{
    if( uiLevel >= LOGOUT_DEBUG && uiLevel <= LOGOUT_NONE )
    {
        uiLogLevel = uiLevel;
    }
    else
    {
        printf("LOG_SetLogLevel failed, uiLevel: %d.\r\n", uiLevel);
    }
}


VOID LOG_Logout(UINT uiLevel, CHAR *pcFileName, INT iLine, CHAR *pcfunc, CHAR *pcFamt, ...)
{
    va_list Arg;
    CHAR *pcPos = NULL;
    CHAR *pcBuf = NULL;
    PLUG_DATE_S stDate;
    UINT uiRet = 0;

    pcPos = pcBuf = g_pcLogBuf;

    if ( NULL == pcFamt)
    {
        return;
    }

    if ( uiLevel < uiLogLevel )
    {
        return;
    }

    PLUG_GetDate( &stDate );
    pcPos += snprintf( pcBuf, LOG_PREFIX_BUF_SIZE, "[%d-%02d-%02d %02d:%02d:%02d][%x]",
                       stDate.iYear, stDate.iMonth, stDate.iDay,
                       stDate.iHour, stDate.iMinute, stDate.iSecond,
                       xTaskGetCurrentTaskHandle());

    switch ( uiLevel )
    {
        case LOGOUT_ERROR :
            pcPos += snprintf(pcPos, LOG_PREFIX_BUF_SIZE, "[ERROR][%s:%d][%s]# ", pcFileName, iLine, pcfunc );
            break;
        case LOGOUT_INFO :
            pcPos += snprintf(pcPos, LOG_PREFIX_BUF_SIZE, "[INFO][%s:%d][%s]# ", pcFileName, iLine, pcfunc );
            break;
        case LOGOUT_DEBUG :
            pcPos += snprintf(pcPos, LOG_PREFIX_BUF_SIZE, "[DEBUG][%s:%d][%s]# ", pcFileName, iLine, pcfunc );
            break;
        default:
            return;
    }

    va_start(Arg, pcFamt);
    pcPos += vsnprintf(pcPos, LOG_BUF_SIZE, pcFamt, Arg);
    va_end(Arg);

    pcPos += sprintf(pcPos,"\r\n");
    printf(pcBuf);

//    if ( pstConsoleCtx != NULL && !strstr(pcfunc, "WEBSOCKET_SendData") && !strstr(pcfunc, "WEB_WebSend"))
//    {
//    	printf("%s:%d[%x]# pstConsoleCtx:0x%p, fd:%d\r\n", __func__, __LINE__, xTaskGetCurrentTaskHandle(),
//    			pstConsoleCtx, pstConsoleCtx->iClientFd);
    	//y有可能是传入的参数不对
//    	uiRet = WEBSOCKET_SendData(pstConsoleCtx, pcBuf, pcPos - pcBuf);
//    	if ( uiRet != OK )
//    	{
//    		pstConsoleCtx = NULL;
//    	}
//    }

    //判断输出日志是否过大截断
    if ( (pcPos - pcBuf) > (LOG_BUF_SIZE + LOG_PREFIX_BUF_SIZE))
    {
        printf("[%s][%s:%d][%s]# Log too large has been cut off.\r\n", "ERROR", __FILE__, __LINE__, __func__);
    }
    return;
}

