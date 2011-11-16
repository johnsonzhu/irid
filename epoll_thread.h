#ifndef _EPOLL_THREAD_H
#define _EPOLL_THREAD_H

#include "epoll_socket.h"
#include "mlist.h"

typedef struct thread_item_s thread_item_t;
typedef struct thread_s thread_t;
typedef enum thread_status_e thread_status_t;

enum thread_status_e
{
	THREAD_STATUS_OK = 0, //正常运行	
	THREAD_STATUS_CLOSE = 1, //关闭	
	THREAD_STATUS_HAVE_DONE = 2	//已经处理过
};

struct thread_item_s
{
	pthread_t pid;//线程id	
	thread_status_t status;//线程状态
};

struct thread_s
{
	int num;//线程数量
	thread_item_t *thread;//线程列表	
	epoll_socket_t *epoll_socket;//epoll_socket句柄
	mlist_item_t *(*action)(epoll_socket_t *epoll_socket,mlist_item_t *item);//处理函数
};

//创建线程
thread_t *create_epoll_thread(int num,epoll_socket_t *epoll_socket,mlist_item_t *(*action)(epoll_socket_t *epoll_socket,mlist_item_t *item));
void shutdown_epoll_thread(thread_t *thread);//关闭线程
void destroy_epoll_thread(thread_t *thread);//删除线程
#endif
