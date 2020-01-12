/********************************************************************************
 *      Copyright:  (C) 2017 Yang Zheng<yz2012ww@gmail.com>
 *                  All rights reserved.
 *
 *       Filename:  sha1.h
 *    Description:  This head file 
 *
 *        Version:  1.0.0(08/17/2017~)
 *         Author:  Yang Zheng <yz2012ww@gmail.com>
 *      ChangeLog:  1, Release initial version on "08/17/2017 02:11:08 PM"
 *                 
 ********************************************************************************/
#ifndef __SHA1_H__
#define __SHA1_H__
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
typedef struct _sha1_context{
	unsigned        message_digest[5];      
	unsigned        length_low;             
	unsigned        length_high;            
	unsigned char   message_block[64]; 
	int             message_block_index;         
	int             computed;                    
	int             corrupted;                   
}sha1_context;
 
#define SHA1_CIRCULAR_SHIFT(bits,word) ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))
 
char *sha1_hash(const char *source);
#endif