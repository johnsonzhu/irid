#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "dalloc.h"
#include "buff.h"

/**
 * @desc 创建一个buff
 * @param int length 长度
 * @return buff_t *
 */
buff_t *create_buff(int length)
{
	assert(length>=0);
	if(length < BUFF_MIN_LENGTH) length = BUFF_MIN_LENGTH;	
	buff_t *buff = NULL;
	DMALLOC(buff,buff_t *,sizeof(buff_t));
	if(NULL == buff) return NULL;
	buff->length = 0;
	buff->max_length = length;
	DCALLOC(buff->data,void *,buff->max_length,sizeof(char));
	if(NULL == buff->data) return NULL;
	return buff;
}

/**
 * @desc 添加内容到buff
 * @param buff_t *buff buff
 * @param void *data 数据
 * @param int length 长度
 * @return bool
 */
bool append_buff(buff_t *buff,void *data,int length)
{
	assert(buff && data);	
	if(buff->length + length > buff->max_length)
	{
		while(buff->max_length < buff->length + length) buff->max_length *= 2;
		DREALLOC(buff->data,void *,buff->max_length*sizeof(char));
		if(NULL == buff->data) return false;
	}
	memcpy(buff->data+buff->length,data,length);
	buff->length += length;
	return true;
}

/**
 * @desc 添加内容到buff
 * @param buff_t *buff buff
 * @param void *data 数据
 * @param int length 长度
 * @return bool
 */
bool prepend_buff(buff_t *buff,void *data,int length)
{
	assert(buff && data);	
	if(buff->length + length > buff->max_length)
	{
		while(buff->max_length < buff->length + length) buff->max_length *= 2;
		DREALLOC(buff->data,void *,buff->max_length*sizeof(char));
		if(NULL == buff->data) return false;
	}
	memmove(buff->data+length,buff->data,buff->length);
	memcpy(buff->data,data,length);
	buff->length += length;
	return true;
}

/**
 * @desc 删除buff
 * @param buff_t *buff buff
 * @return void
 */
void destroy_buff(buff_t *buff)
{
	assert(buff);
	DFREE(buff->data);	
	DFREE(buff);
}
