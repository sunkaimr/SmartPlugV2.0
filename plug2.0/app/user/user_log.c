#include "user_common.h"

#define LOG_PREFIX_BUF_SIZE 	(50)
#define LOG_BUF_SIZE 			(1024)


//日志默认INFO级别
static UINT uiLogLevel = LOGOUT_INFO;
STATIC CHAR *g_pcLogBuf = NULL;

VOID LOG_LogInit( VOID )
{
	//printf("LOG_LogInit start.\r\n");

 	g_pcLogBuf = (CHAR*)malloc( LOG_PREFIX_BUF_SIZE + LOG_BUF_SIZE + 5 );
	if ( NULL == g_pcLogBuf )
	{
		printf("LOG_LogInit failed, malloc failed.\r\n");
		while(1);
	}
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


VOID LOG_Logout(UINT uiLevel, CHAR *pcFileName, INT iLine, CHAR *pcFamt, ...)
{
	va_list Arg;
	CHAR *pcPos = NULL;
	CHAR *pcBuf = NULL;

	pcPos = pcBuf = g_pcLogBuf;

	if ( NULL == pcFamt)
	{
		return;
	}

	if ( uiLevel < uiLogLevel )
	{
		return;
	}

	switch ( uiLevel )
	{
	    case LOGOUT_ERROR :
			pcPos += snprintf(pcBuf, LOG_PREFIX_BUF_SIZE, "[%s][%s:%d]# ", "ERROR", pcFileName, iLine );
			break;
	    case LOGOUT_INFO :
			pcPos += snprintf(pcBuf, LOG_PREFIX_BUF_SIZE, "[%s][%s:%d]# ", "INFO", pcFileName, iLine );
	        break;
	    case LOGOUT_DEBUG :
			pcPos += snprintf(pcBuf, LOG_PREFIX_BUF_SIZE, "[%s][%s:%d]# ", "DEBUG", pcFileName, iLine );
	        break;
	    default:
	        return;
	}

	va_start(Arg, pcFamt);
	pcPos += vsnprintf(pcPos, LOG_BUF_SIZE, pcFamt, Arg);
	va_end(Arg);


	pcPos += sprintf(pcPos,"\r\n");
	printf(pcBuf);

	//判断输出日志是否过大截断
	if ( (pcPos - pcBuf) > (LOG_BUF_SIZE + LOG_PREFIX_BUF_SIZE))
	{
		printf("[%s][%s:%d]# Log too large has been cut off.\r\n", "ERROR", __FILE__, __LINE__);
	}
	return;
}

