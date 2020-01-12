#ifndef __USER_TYPE_H__
#define __USER_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif


/* user define */
typedef unsigned int     UINT32;
typedef unsigned long    UINT64;
typedef unsigned int     UINT;
typedef int              INT32;
typedef unsigned short   UINT16;
typedef short            INT16;
typedef int              INT;
typedef unsigned char    UINT8;
typedef signed char      INT8;
typedef char             CHAR;

#ifndef BOOL
	typedef unsigned char    BOOL;
#endif

#ifndef NULL
    #define NULL ((void *)0)
#endif


#define VOID     void
#define STATIC    static


#define RET_SUCCESSED        0
#define RET_FAILED            1

/* end user define */

#define FREE_MEM( point ) \
do{\
    if ( point ){\
        free( point );\
        point = NULL;\
    }\
}while(0)




#ifdef __cplusplus
}
#endif

#endif
