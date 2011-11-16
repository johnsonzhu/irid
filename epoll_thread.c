#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dalloc.h"
#include "epoll_thread.h"

static void *epoll_thread_action(void *param); //线程处理函数

/**
 * @desc 创建线程
 * @param int num 线程的数量
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @param mlist_item_t *(*action)(epoll_socket_t *epoll_socket,mlist_item_t *item) 处理函数
 * @return thread_t * 线程句柄
 */
thread_t *create_epoll_thread(int num,epoll_socket_t *epoll_socket,mlist_item_t *(*action)(epoll_socket_t *epoll_socket,mlist_item_t *item))
{
	assert(num>0 && epoll_socket && action);	
	int i = 0;
	thread_t *thread = NULL;
	DMALLOC(thread,thread_t *,sizeof(thread_t));
	if(NULL == thread) return NULL;
	memset(thread,0,sizeof(thread_t));

	thread->num = num; //线程数量
	DCALLOC(thread->thread,thread_item_t *,num,sizeof(thread_item_t));
	if(NULL == thread->thread) return NULL;
	thread->action = action; //处理函数
	thread->epoll_socket = epoll_socket; //epoll_socket句柄
	//创建线程
	for(;i<num;i++) pthread_create(&thread->thread[i].pid,NULL,epoll_thread_action,thread);

	return thread;
}

/**
 * @desc 关闭线程
 * @param thread_t *thread 线程句柄
 * @return void
 */
void shutdown_epoll_thread(thread_t *thread)
{
	assert(thread);
	int i = 0,have_join_num = 0;//当前已经关闭线程的数量
	while(have_join_num < thread->num)
	{
		unlock_epoll_socket_thread(thread->epoll_socket);//解锁线程
		for(i=0;i<thread->num;i++)
		{
			if(THREAD_STATUS_CLOSE == thread->thread[i].status)
			{
				pthread_join(thread->thread[i].pid,NULL);//关闭线程
				thread->thread[i].status = THREAD_STATUS_HAVE_DONE;//更新线程状态
				have_join_num++;
			}	
		}
	}
}

/**
 * @desc 删除线程句柄
 * @param thread_t *thread 线程句柄
 * @return void
 */
void destroy_epoll_thread(thread_t *thread)
{
	assert(thread);
	DFREE(thread->thread);	
	DFREE(thread);
}

/**
 * @desc 线程处理函数
 * @param void *param 参数
 * @return void *
 */
void *epoll_thread_action(void *param)
{
	thread_t *thread = (thread_t *)param;
	pthread_t pid = pthread_self();//当前线程id
	epoll_socket_t *epoll_socket = thread->epoll_socket;
	while(EPOLL_SOCKET_STATUS_OK == epoll_socket->status)
	{
		//从队列中获取数据
		mlist_item_t *mlist_item_out = NULL,*mlist_item_in = shift_mlist(epoll_socket->recv_mlist);
		if(NULL != mlist_item_in) 
		{
			//获取要输出的数据
			mlist_item_out = thread->action(epoll_socket,mlist_item_in);
			//发送数据
			if(NULL != mlist_item_out)
			{
				send_epoll_socket(epoll_socket,mlist_item_out);
				//释放内存
				destroy_mlist_item(mlist_item_out);//考虑一下看看能复用内存不
			}
			destroy_mlist_item(mlist_item_in);
		}
		else
		{
			//防止线程空转，上锁
			if(EPOLL_SOCKET_STATUS_OK == epoll_socket->status) lock_epoll_socket_thread(epoll_socket);
		}
	}
	//更改线程状态
	int i = 0;
	for(;i<thread->num;i++)
	{
		if(thread->thread[i].pid == pid) thread->thread[i].status = THREAD_STATUS_CLOSE;
	}
	return NULL;
}
