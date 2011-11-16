#ifndef _MC_PROTOCOL_H
#define _MC_PROTOCOL_H

#include <stdbool.h>
#include "buff.h"
#include "lrumc.h"
#include "pary.h"

#define MC_STORED "STORED\r\n"
#define MC_NOT_STORED "NOT_STORED\r\n"
#define MC_DELETED "DELETED\r\n"
#define MC_NOT_FOUND "NOT_FOUND\r\n"
#define MC_END "END\r\n"
#define MC_RN "\r\n"

#define MC_CMD_GET "get"
#define MC_CMD_ADD "add"
#define MC_CMD_SET "set"
#define MC_CMD_REPLACE "replace"
#define MC_CMD_DELETE "delete"
#define MC_CMD_INCR "incr"
#define MC_CMD_DECR "decr"
#define MC_CMD_APPEND "append"
#define MC_CMD_PREPEND "prepend"
#define MC_CMD_QUIT "quit"
#define MC_CMD_POP "pop"
#define MC_CMD_PUSH "push"
#define MC_CMD_UNSHIFT "unshift"
#define MC_CMD_SHIFT "shift"
#define MC_CMD_COUNT "count"
#define MC_CMD_LIST "list"
#define MC_CMD_PAGE "page"
#define MC_CMD_FLUSH_ALL "flush_all"
#define MC_CMD_VERSION "version"
#define MC_CMD_STATS "stats"
#define MC_CMD_RELOAD "reload"


#define MC_SEP_R "\r"
#define MC_SEP_N "\n"
#define MC_SEP_S " "

#define GET_STR(dst,dst_len,src,sep,sep_len)\
do{\
	char *___p = strstr((char *)src,sep);\
	if(NULL != ___p)\
	{\
		int pos = ___p - (char *)src;\
		strncpy(dst,src,pos>dst_len?dst_len:pos);\
		src += pos + sep_len;\
	}\
	else\
	{\
		strncpy(dst,(char *)src,dst_len);\
		src = NULL;\
	}\
}while(0)


#define GET_INT(dst,src,sep,sep_len)\
do{\
	char tmp[11];\
	memset(tmp,0,11);\
	char *___p = strstr((char *)src,sep);\
	int pos = ___p - (char *)src;\
	strncpy(tmp,src,pos);\
	src += pos + sep_len;\
	dst = atoi(tmp);\
}while(0)

#define GET_LLD(dst,src,sep,sep_len)\
do{\
	char tmp[20];\
	memset(tmp,0,20);\
	char *___p = strstr((char *)src,sep);\
	int pos = ___p - (char *)src;\
	strncpy(tmp,src,pos>19?19:pos);\
	src += pos + sep_len;\
	dst = strtoll(tmp,NULL,10);\
}while(0)

#define GET_DOUBLE(dst,src,sep,sep_len)\
do{\
	char tmp[56];\
	memset(tmp,0,56);\
	char *___p = strstr((char *)src,sep);\
	int pos = ___p - (char *)src;\
	strncpy(tmp,src,pos>55?55:pos);\
	src += pos + sep_len;\
	dst = strtod(tmp,NULL);\
}while(0)

#define PARSE_KEY(str,key,path) \
do{\
	char *___p = strstr(str,"@");\
	if(NULL == ___p)\
	{\
		strcpy(key,str);\
		path[0]='\0';\
	}\
	else\
	{\
		strncpy(key,str,___p - str);\
		___p++;\
		strcpy(path,___p);\
	}\
}while(0)

buff_t *parse(lrumc_t *lrumc,void *data);

#endif
