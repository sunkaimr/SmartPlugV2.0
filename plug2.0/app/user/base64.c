/*********************************************************************************
 *      Copyright:  (C) 2017 Yang Zheng<yz2012ww@gmail.com>  
 *                  All rights reserved.
 *
 *       Filename:  base64.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(08/17/2017~)
 *         Author:  Yang Zheng <yz2012ww@gmail.com>
 *      ChangeLog:  1, Release initial version on "08/17/2017 02:09:12 PM"
 *                 
 ********************************************************************************/
#include "base64.h"
 
const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="; 
 
char *base64Encode(const char* data, int data_len)
{ 
    int prepare = 0; 
    int ret_len; 
    int temp = 0; 
    char *ret = NULL; 
    char *f = NULL; 
    int tmp = 0; 
    unsigned char changed[4]; 
    int i = 0; 
    ret_len = data_len / 3; 
    temp = data_len % 3; 
 
    if (temp > 0) 
        ret_len += 1; 
 
    ret_len = ret_len*4 + 1; 
    ret = (char *)malloc(ret_len); 
    if ( ret == NULL) { 
        printf("ret alloc failure.\n"); 
        return NULL; 
    } 
    memset(ret, 0, ret_len); 
 
    f = ret; 
    while (tmp < data_len) 
    { 
        temp = 0; 
        prepare = 0; 
        memset(changed, '\0', 4); 
        while (temp < 3) 
        { 
            if (tmp >= data_len) 
                break; 
 
            prepare = ((prepare << 8) | (data[tmp] & 0xFF)); 
            tmp++; 
            temp++; 
        } 
 
        prepare = (prepare<<((3-temp)*8)); 
        for (i=0; i<4 ;i++) { 
            if (temp < i) 
                changed[i] = 0x40; 
            else 
                changed[i] = (prepare>>((3-i)*6)) & 0x3F; 
 
            *f = base[changed[i]]; 
            f++; 
        } 
    } 
    *f = '\0'; 
      
    return ret; 
} 
 
static char find_pos(char ch)   
{ 
    char *ptr = (char*)strrchr(base, ch);//the last position (the only) in base[] 
    return (ptr - base); 
} 
 
char *base64Decode(const char *data, int data_len)
{ 
    int ret_len = (data_len / 4) * 3; 
    int equal_count = 0; 
    char *ret = NULL; 
    char *f = NULL; 
    int tmp = 0; 
    int temp = 0; 
    char need[3]; 
    int prepare = 0; 
    int i = 0; 
 
    if (*(data + data_len - 1) == '=') 
        equal_count += 1; 
 
    if (*(data + data_len - 2) == '=') 
        equal_count += 1; 
 
    if (*(data + data_len - 3) == '=') 
        equal_count += 1; 
 
    switch (equal_count) 
    { 
    case 0: 
        ret_len += 4;//3 + 1 [1 for NULL] 
        break; 
    case 1: 
        ret_len += 4;//Ceil((6*3)/8)+1 
        break; 
    case 2: 
        ret_len += 3;//Ceil((6*2)/8)+1 
        break; 
    case 3: 
        ret_len += 2;//Ceil((6*1)/8)+1 
        break; 
    } 
    ret = (char *)malloc(ret_len); 
    if (NULL == ret) { 
        printf("ret alloc failure.\n"); 
        return NULL; 
    } 
    memset(ret, 0, ret_len); 
 
    f = ret; 
    while (tmp < (data_len - equal_count)) 
    { 
        temp = 0; 
        prepare = 0; 
        memset(need, 0, 4); 
        while (temp < 4) 
        { 
            if (tmp >= (data_len - equal_count)) 
                break; 
            prepare = (prepare << 6) | (find_pos(data[tmp])); 
            temp++; 
            tmp++; 
        } 
 
        prepare = prepare << ((4-temp) * 6); 
        for (i=0; i<3; i++) { 
            if (i == temp) 
                break; 
            *f = (char)((prepare>>((2-i)*8)) & 0xFF); 
            f++; 
        } 
    } 
    *f = '\0'; 
    return ret; 
}


int tolower(int c) 	//灏嗗ぇ鍐欏瓧姣嶅瓧绗﹁浆鎹负灏忓啓瀛楁瘝瀛楃
{
    if (c >= 'A' && c <= 'Z') 	//鍒ゆ柇鏄惁涓哄ぇ鍐欏瓧绗�
    {
        return c + 'a' - 'A'; 	//杩斿洖灏忓啓瀛楃
    }
    else
    {
        return c; 				//涓嶄负澶у啓瀛楃鍒欎笉澶勭悊
    }
}

int htoi(const char s[],int start,int len) 		//浼犲叆(16杩涘埗鏁�,璧峰浣嶇疆,闀垮害)杩斿洖int绫诲瀷
{
    int i,j; 		//缁欏惊鐜娇鐢�
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X')) //鍒ゆ柇鏄惁鏈夊墠瀵�0x鎴栬��0X
    {
        i = 2; 		//鏈夊墠瀵�0x鎴栬��0X鍒欒缃甶=2(浠庣3浣嶅紑濮�,鏁扮粍涓嬫爣涓�2)
    }
    else
    {
        i = 0;
    }
    i+=start;	//i+start涓哄疄闄呴渶瑕佽浆鎹㈢殑璧峰浣嶇疆
    j=0;
    for (; (s[i] >= '0' && s[i] <= '9')
            || (s[i] >= 'a' && s[i] <= 'f') || (s[i] >='A' && s[i] <= 'F');++i)
    {
        if(j>=len)	//褰搄澶т簬瑕佽浆鎹㈢殑鏁版嵁闀垮害鍒欒烦鍑哄惊鐜�
        {
            break;
        }
        if (tolower(s[i]) > '9')	//鍒ゆ柇s[i]涓哄瓧姣�
        {
            n = 16 * n + (10 + tolower(s[i]) - 'a');	//褰搒[i]涓哄瓧姣嶆椂n=n*16+10+s[i]杞崲涓哄皬鍐欏瓧姣�-a
        }
        else
        {
            n = 16 * n + (tolower(s[i]) - '0'); 		//褰搒[i]涓嶄负瀛楁瘝鏃秐=n*16+s[i]-9
        }
        j++;
    }
    return n;
}
