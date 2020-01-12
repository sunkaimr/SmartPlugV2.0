/*********************************************************************************
 *      Copyright:  (C) 2017 Yang Zheng<yz2012ww@gmail.com>  
 *                  All rights reserved.
 *
 *       Filename:  sha1.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(08/17/2017~)
 *         Author:  Yang Zheng <yz2012ww@gmail.com>
 *      ChangeLog:  1, Release initial version on "08/17/2017 02:08:20 PM"
 *                 
 ********************************************************************************/
#include "sha1.h"
 
static void sha1_process_message_block(sha1_context*);
static void sha1_pad_message(sha1_context*);
 
static void sha1_reset(sha1_context *context) // 初始化动作
{
    context->length_low             = 0;
    context->length_high            = 0;
    context->message_block_index    = 0;
 
    context->message_digest[0]      = 0x67452301;
    context->message_digest[1]      = 0xEFCDAB89;
    context->message_digest[2]      = 0x98BADCFE;
    context->message_digest[3]      = 0x10325476;
    context->message_digest[4]      = 0xC3D2E1F0;
 
    context->computed   = 0;
    context->corrupted  = 0;
}
 
 
static int sha1_result(sha1_context *context) // 成功返回1，失败返回0
{
    if (context->corrupted) {
        return 0;
    }
    if (!context->computed) {
        sha1_pad_message(context);
        context->computed = 1;
    }
    return 1;
}
 
 
static void sha1_input(sha1_context *context,const char *message_array,unsigned length)
{
    if (!length) return;
 
    if (context->computed || context->corrupted){
        context->corrupted = 1;
        return;
    }
 
    while(length-- && !context->corrupted){
        context->message_block[context->message_block_index++] = (*message_array & 0xFF);
 
        context->length_low += 8;
 
        context->length_low &= 0xFFFFFFFF;
        if (context->length_low == 0){
            context->length_high++;
            context->length_high &= 0xFFFFFFFF;
            if (context->length_high == 0) context->corrupted = 1;
        }
 
        if (context->message_block_index == 64){
            sha1_process_message_block(context);
        }
        message_array++;
    }
}
 
static void sha1_process_message_block(sha1_context *context)
{
    const unsigned K[] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
    int         t;                
    unsigned    temp;             
    unsigned    W[80];            
    unsigned    A, B, C, D, E;    
 
    for(t = 0; t < 16; t++) {
        W[t] = ((unsigned) context->message_block[t * 4]) << 24;
        W[t] |= ((unsigned) context->message_block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) context->message_block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) context->message_block[t * 4 + 3]);
    }
 
    for(t = 16; t < 80; t++)  W[t] = SHA1_CIRCULAR_SHIFT(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
 
    A = context->message_digest[0];
    B = context->message_digest[1];
    C = context->message_digest[2];
    D = context->message_digest[3];
    E = context->message_digest[4];
 
    for(t = 0; t < 20; t++) {
        temp =  SHA1_CIRCULAR_SHIFT(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1_CIRCULAR_SHIFT(30,B);
        B = A;
        A = temp;
    }
    for(t = 20; t < 40; t++) {
        temp = SHA1_CIRCULAR_SHIFT(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1_CIRCULAR_SHIFT(30,B);
        B = A;
        A = temp;
    }
    for(t = 40; t < 60; t++) {
        temp = SHA1_CIRCULAR_SHIFT(5,A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1_CIRCULAR_SHIFT(30,B);
        B = A;
        A = temp;
    }
    for(t = 60; t < 80; t++) {
        temp = SHA1_CIRCULAR_SHIFT(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1_CIRCULAR_SHIFT(30,B);
        B = A;
        A = temp;
    }
    context->message_digest[0] = (context->message_digest[0] + A) & 0xFFFFFFFF;
    context->message_digest[1] = (context->message_digest[1] + B) & 0xFFFFFFFF;
    context->message_digest[2] = (context->message_digest[2] + C) & 0xFFFFFFFF;
    context->message_digest[3] = (context->message_digest[3] + D) & 0xFFFFFFFF;
    context->message_digest[4] = (context->message_digest[4] + E) & 0xFFFFFFFF;
    context->message_block_index = 0;
}
 
static void sha1_pad_message(sha1_context* context)
{
    if (context->message_block_index > 55) {
        context->message_block[context->message_block_index++] = 0x80;
        while(context->message_block_index < 64)  context->message_block[context->message_block_index++] = 0;
        sha1_process_message_block(context);
        while(context->message_block_index < 56) context->message_block[context->message_block_index++] = 0;
    } else {
        context->message_block[context->message_block_index++] = 0x80;
        while(context->message_block_index < 56) context->message_block[context->message_block_index++] = 0;
    }
    context->message_block[56] = (context->length_high >> 24 ) & 0xFF;
    context->message_block[57] = (context->length_high >> 16 ) & 0xFF;
    context->message_block[58] = (context->length_high >> 8 ) & 0xFF;
    context->message_block[59] = (context->length_high) & 0xFF;
    context->message_block[60] = (context->length_low >> 24 ) & 0xFF;
    context->message_block[61] = (context->length_low >> 16 ) & 0xFF;
    context->message_block[62] = (context->length_low >> 8 ) & 0xFF;
    context->message_block[63] = (context->length_low) & 0xFF;
 
    sha1_process_message_block(context);
}
 
#define SHA1_SIZE   128
char *sha1_hash(const char *source)
{
    sha1_context    sha;
    char            *buf = NULL;
 
    sha1_reset(&sha);
    sha1_input(&sha, source, strlen(source));
 
    if (!sha1_result(&sha)){
        printf("SHA1 ERROR: Could not compute message digest");
        return NULL;
    } else {
        buf = (char *)malloc(SHA1_SIZE);
        if (NULL == buf) {
            printf("buf is NULL.\n");
            return NULL;
        }
        memset(buf, 0, sizeof(SHA1_SIZE));
        sprintf(buf, "%08X%08X%08X%08X%08X", sha.message_digest[0],sha.message_digest[1],
                sha.message_digest[2],sha.message_digest[3],sha.message_digest[4]);
        //lr_save_string(buf, lrvar);
 
        //return strlen(buf);
        return buf;
    }
}