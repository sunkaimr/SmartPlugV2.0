#ifndef __USER_LOG_H__
#define __USER_LOG_H__





#ifdef __cplusplus
extern "C" {
#endif





#define LOG_OUT_ENABLE     (1)

#define LOGOUT_NONE       4
#define LOGOUT_ERROR      3
#define LOGOUT_INFO       2
#define LOGOUT_DEBUG      1



#if LOG_OUT_ENABLE

    #define GET_FILE_NAME(str)     ((strrchr(str,'\\')) ? (strrchr(str,'\\')+1) : str)

    #define LOG_OUT(Level, Fmt, Args...) LOG_Logout((UINT)Level, GET_FILE_NAME(__FILE__), __LINE__, __func__, (CHAR*)Fmt, ##Args)
#else
    #define LOG_OUT(Level, Fmt, Args...) 
#endif

VOID LOG_LogInit( VOID );
VOID LOG_SetLogLevel( UINT uiLevel );

#ifdef __cplusplus
}
#endif

#endif

