#include "user_common.h"

#define LOG_PREFIX_BUF_SIZE      (100)
#define LOG_BUF_SIZE             (1024 - LOG_PREFIX_BUF_SIZE)
#define BUF1                      1
#define BUF2                      2


typedef struct LogBuf_tag{
	CHAR 	*pcBuf;
	UINT16  iLen;	// buf总长度
	UINT16	iPos;   // 已使用长度
}LogBuf_S;

typedef struct LogHeader_tag{
	LogBuf_S 	stBuf1;		// 第一个缓冲区
	LogBuf_S 	stBuf2;     // 第二个缓冲区
	UINT8 		UsingBuf;   // 正在使用的缓冲区
	UINT8 		UsedBuf;    // 已使用的缓冲区
}LogHeader_S;

//日志默认INFO级别
static UINT uiLogLevel = LOGOUT_INFO;
LogHeader_S stLogHeader;
static CHAR *pcLogBuf = NULL;

VOID LOG_LogInit( VOID )
{
    //printf("LOG_LogInit start.\r\n");
	pcLogBuf = (CHAR*)malloc( LOG_PREFIX_BUF_SIZE + LOG_BUF_SIZE + 5 );
    if ( NULL == pcLogBuf )
    {
        printf("[ERROR][%s:%d][%s]# malloc pcLogBuf failed, Free heap:%d",
                __FILE__, __LINE__, __func__, system_get_free_heap_size());
        while(1);
    }

	stLogHeader.UsingBuf = BUF1;
	stLogHeader.UsedBuf = BUF1;

	stLogHeader.stBuf1.pcBuf = (CHAR*)malloc( LOG_PREFIX_BUF_SIZE + LOG_BUF_SIZE + 5 );
    if ( NULL == stLogHeader.stBuf1.pcBuf )
    {
        printf("[ERROR][%s:%d][%s]# malloc stBuf1 failed, Free heap:%d",
                __FILE__, __LINE__, __func__, system_get_free_heap_size());
        while(1);
    }
    stLogHeader.stBuf1.iLen = LOG_PREFIX_BUF_SIZE + LOG_BUF_SIZE;
    stLogHeader.stBuf1.iPos = 0;

    stLogHeader.stBuf2.pcBuf = (CHAR*)malloc( LOG_PREFIX_BUF_SIZE + LOG_BUF_SIZE + 5);
    if ( NULL == stLogHeader.stBuf2.pcBuf )
    {
        printf("[ERROR][%s:%d][%s]# malloc stBuf2 failed, Free heap:%d",
                __FILE__, __LINE__, __func__, system_get_free_heap_size());
        while(1);
    }
    stLogHeader.stBuf2.iLen = LOG_PREFIX_BUF_SIZE + LOG_BUF_SIZE;
    stLogHeader.stBuf2.iPos = 0;

    //printf("LOG_LogInit over.\r\n");
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

LogBuf_S* LOG_GetUsingBuf( VOID )
{
	if ( stLogHeader.UsingBuf == BUF1 )
	{
		return &stLogHeader.stBuf1;
	}
	else if ( stLogHeader.UsingBuf == BUF2 )
	{
		return &stLogHeader.stBuf2;
	}

	printf("LOG_GetUsingBuf failed, UsingBuf: %d is invalid.\r\n", stLogHeader.UsingBuf);
	return NULL;
}

LogBuf_S* LOG_GetUsedBuf( VOID )
{
	if ( stLogHeader.UsedBuf == BUF1 )
	{
		return &stLogHeader.stBuf1;
	}
	else if ( stLogHeader.UsedBuf == BUF2 )
	{
		return &stLogHeader.stBuf2;
	}

	printf("LOG_GetUsedBuf failed, UsedBuf: %d is invalid.\r\n", stLogHeader.UsedBuf);
	return NULL;
}

LogBuf_S* LOG_SwitchUsingBuf( VOID )
{
	if ( stLogHeader.UsingBuf == BUF1 )
	{
		stLogHeader.UsedBuf = BUF1;
		stLogHeader.UsingBuf = BUF2;
		stLogHeader.stBuf2.iPos = 0;
		return &stLogHeader.stBuf2;
	}
	stLogHeader.UsedBuf = BUF2;
	stLogHeader.UsingBuf = BUF1;
	stLogHeader.stBuf1.iPos = 0;
	return &stLogHeader.stBuf1;
}

VOID LOG_Logout(UINT uiLevel, CHAR *pcFileName, INT iLine, CHAR *pcfunc, CHAR *pcFamt, ...)
{
    va_list Arg;
    CHAR *pcPos = NULL;
    CHAR *pcBuf = NULL;
    PLUG_DATE_S stDate;
    UINT uiRet = 0;
    LogBuf_S *UsingBuf = NULL;

    if ( NULL == pcFamt)
    {
        return;
    }

    if ( uiLevel < uiLogLevel )
    {
        return;
    }

    pcPos = pcBuf = pcLogBuf;

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

    if ( (pcPos - pcBuf) >  LOG_PREFIX_BUF_SIZE )
    {
    	pcPos = pcBuf + LOG_PREFIX_BUF_SIZE;
    }

    va_start(Arg, pcFamt);
    pcPos += vsnprintf(pcPos, LOG_BUF_SIZE, pcFamt, Arg);
    va_end(Arg);

    pcPos += sprintf(pcPos, "\r\n");

    UsingBuf = LOG_GetUsingBuf();
    if ( UsingBuf != NULL )
    {
    	// 判断剩余缓冲区不够
    	if ((pcPos - pcBuf) > (UsingBuf->iLen - UsingBuf->iPos))
    	{
    		UsingBuf = LOG_SwitchUsingBuf();
    	}
    	strcpy(UsingBuf->pcBuf + UsingBuf->iPos, pcBuf);
    	UsingBuf->iPos = UsingBuf->iPos + (pcPos - pcBuf);
    }
    else
    {
    	printf("LOG_Logout failed, UsingBuf: %d is NULL.\r\n");
    }

    printf(pcBuf);

    //判断输出日志是否过大截断
    if ( (pcPos - pcBuf) > (LOG_BUF_SIZE + LOG_PREFIX_BUF_SIZE))
    {
        printf("[%s][%s:%d][%s]# Log too large has been cut off.\r\n", "ERROR", __FILE__, __LINE__, __func__);
    }
    return;
}

UINT LOG_PrintHistoryLog(CHAR* pcBuf, UINT uiBufLen)
{
	LogBuf_S *UsedBuf = NULL;
	LogBuf_S *UsingBuf = NULL;
	CHAR *pcPos = NULL;

	pcPos = pcBuf;

	UsedBuf = LOG_GetUsedBuf();
	if ( UsedBuf != NULL )
	{
		strncpy(pcPos, UsedBuf->pcBuf, uiBufLen);
		pcPos += UsedBuf->iPos;
	}

	UsingBuf = LOG_GetUsingBuf();
	if ( UsingBuf != NULL )
	{
		strncpy(pcPos, UsingBuf->pcBuf, uiBufLen - UsedBuf->iPos );
		pcPos += UsingBuf->iPos;
	}
	return pcPos - pcBuf;
}

