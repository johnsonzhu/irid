#ifndef _EPOLL_SOCKET_H
#define _EPOLL_SOCKET_H

#include "mlist.h"
#include <stdbool.h>

#define EPOLL_EVENT_MAX_SIZE 2048 //事件容量
#define EPOLL_RECV_BUFF_SIZE 1024 //buff的初始化大小

typedef struct epoll_socket_s epoll_socket_t;
typedef enum epoll_socket_status_e epoll_socket_status_t;

enum epoll_socket_status_e
{
	EPOLL_SOCKET_STATUS_CLOSE = 0,	
	EPOLL_SOCKET_STATUS_OK = 1,	
};

struct epoll_socket_s
{
	epoll_socket_status_t status;//状态
	int epfd;//epoll句柄
	int listenfd;//监听句柄	
	pthread_mutex_t *thread_lock;//线程锁，防止空转
	mlist_t *recv_mlist;//接收到的数据（多线程安全链表）
	unsigned long curr_connections;
	unsigned long total_connections;
};

epoll_socket_t *create_epoll_socket(void);//创建一个epoll_socket句柄
void start_epoll_socket(epoll_socket_t *epoll_socket,int port);//开始监听网络
void send_epoll_socket(epoll_socket_t *epoll_socket,mlist_item_t *mlist_item);//发送网络数据
void skip_epoll_socket(epoll_socket_t *epoll_socket,int cfd);
void lock_epoll_socket_thread(epoll_socket_t *epoll_socket);//锁定线程锁，防止空转
void unlock_epoll_socket_thread(epoll_socket_t *epoll_socket);//解锁线程锁，说明有请求
void shutdown_epoll_socket(epoll_socket_t *epoll_socket);//关闭epoll_socket
void destroy_epoll_socket(epoll_socket_t *epoll_socket);//删除epoll_socket
#endif
