#ifndef _MLIST_H
#define _MLIST_H
#include <pthread.h>
#include <stdbool.h>
#include "buff.h"

typedef struct mlist_s mlist_t;
typedef struct mlist_item_s mlist_item_t;

struct mlist_item_s
{
	int fd;//连接id
	buff_t *buff;//内容	
};

//这个是一个链表来的
struct mlist_s
{
	mlist_item_t *data;
	mlist_t *pre;
	mlist_t *next;
	pthread_mutex_t lock;//这个列表类是线程安全的
};

mlist_t *create_mlist(void);//创建一个链表
mlist_item_t *create_mlist_item(int fd,buff_t *buff);//创建一个节点
bool push_mlist(mlist_t *mlist,mlist_item_t *mlist_item);//从后面插入一个节点
mlist_item_t *shift_mlist(mlist_t *mlist);//从前面弹出一个节点
void destroy_mlist(mlist_t *mlist);//删除链表
void destroy_mlist_item(mlist_item_t *mlist_item);//删除节点
#endif
