#ifndef _BUFF_H
#define _BUFF_H

#include <stdbool.h>

#define BUFF_MIN_LENGTH 1024
typedef struct buff_s buff_t;
//这个buff是2进制安全的
struct buff_s
{
	void *data;
	int max_length;//最大长度
	int length;//当前长度
};

buff_t *create_buff(int length);//创建一个buff
bool append_buff(buff_t *buff,void *data,int length);//添加内容到buff后面
bool prepend_buff(buff_t *buff,void *data,int length);//添加内容到buff前面
void destroy_buff(buff_t *buff);//删除buff
#endif
