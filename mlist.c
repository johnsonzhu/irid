#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dalloc.h"
#include "mlist.h"

/**
 * @desc 创建一个链表(多线程安全)
 * @param void
 * @return mlist_t *
 */
mlist_t *create_mlist(void)
{
	mlist_t *mlist = NULL;
	DMALLOC(mlist,mlist_t *,sizeof(mlist_t));
	if(NULL == mlist) return NULL;
	DMALLOC(mlist->pre,mlist_t *,sizeof(mlist_t));
	if(NULL == mlist->pre) return NULL;
	DMALLOC(mlist->next,mlist_t *,sizeof(mlist_t));
	if(NULL == mlist->next) return NULL;
	pthread_mutex_init(&mlist->lock,NULL);
	mlist->pre->pre = NULL;
	mlist->pre->next = mlist->next;
	mlist->next->pre = mlist->pre;
	mlist->next->next = NULL;
	return mlist;
}

/**
 * @desc 创建一个节点
 * @param int fd socket句柄
 * @param buff_t *buff 内容
 * @return mlist_item_t *
 */
mlist_item_t *create_mlist_item(int fd,buff_t *buff)
{
	mlist_item_t *mlist_item = NULL;
	DMALLOC(mlist_item,mlist_item_t *,sizeof(mlist_item_t));
	mlist_item->fd = fd;
	mlist_item->buff = buff;
	return mlist_item;
}

/**
 * @desc 从后面插入一个节点
 * @param mlist_t *mlist 链表句柄
 * @param mlist_item_t *mlist_item 节点
 * @return bool
 */
bool push_mlist(mlist_t *mlist,mlist_item_t *mlist_item)
{
	assert(mlist && mlist_item);	
	pthread_mutex_lock(&mlist->lock);
	mlist_t *tmp_mlist = NULL;
	DMALLOC(tmp_mlist,mlist_t *,sizeof(mlist_t));
	tmp_mlist->data = mlist_item;
	tmp_mlist->next = mlist->next;
	tmp_mlist->pre = mlist->next->pre;
	mlist->next->pre = tmp_mlist;
	tmp_mlist->pre->next = tmp_mlist;
	pthread_mutex_unlock(&mlist->lock);
	return true;
}

/**
 * @desc 从前面弹出一个节点
 * @param mlist_t *mlist 链表句柄
 * @return mlist_item_t *
 */
mlist_item_t *shift_mlist(mlist_t *mlist)
{
	assert(mlist);
	if(mlist->next->pre == mlist->pre) return NULL;
	pthread_mutex_lock(&mlist->lock);
	mlist_item_t *tmp_mlist_item;
	mlist_t *tmp_mlist = mlist->pre->next;
	mlist->pre->next = tmp_mlist->next;
	tmp_mlist->next->pre = mlist->pre;
	pthread_mutex_unlock(&mlist->lock);
	tmp_mlist_item = tmp_mlist->data;
	DFREE(tmp_mlist);
	return tmp_mlist_item;
}

/**
 * @desc 删除链表
 * @param mlist_t *mlist 链表句柄
 * @return void
 */
void destroy_mlist(mlist_t *mlist)
{
	assert(mlist);
	mlist_item_t *tmp_item;
	while(NULL != (tmp_item = shift_mlist(mlist))) destroy_mlist_item(tmp_item);
	DFREE(mlist->pre);
	DFREE(mlist->next);
	DFREE(mlist);
}

/**
 * @desc 删除一个节点
 * @param mlist_item_t *mlist_item 节点
 * @return void
 */
void destroy_mlist_item(mlist_item_t *mlist_item)
{
	assert(mlist_item);	
	DFREE(mlist_item->buff->data);
	DFREE(mlist_item->buff);
	DFREE(mlist_item);
}
