#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>  
#include <sys/epoll.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <errno.h>  
#include <assert.h>
#include <time.h>
#include "dalloc.h"
#include "buff.h"
#include "epoll_socket.h"

static int init_sock(int port); //初始化listen socket
static void setnonblocking(int fd); //设置非阻塞模式
static void setreuseaddr(int fd); //端口复用
static int init_epoll(void); //初始化epoll
static int ctl_epoll(int epfd,int cfd,int action,int event_type); //epoll控制函数
static void start_epoll(epoll_socket_t *epoll_socket); //开始监听
static mlist_item_t *recv_data(epoll_socket_t *epoll_socket,int cfd); //接收数据
static bool send_data(epoll_socket_t *epoll_socket,int cfd,void *content,int length); //发送数据

/**
 * @desc 创建epoll_socket句柄
 * @param void
 * @return epoll_socket_t *
 */
epoll_socket_t *create_epoll_socket(void)
{
	epoll_socket_t *epoll_socket = NULL;
	DMALLOC(epoll_socket,epoll_socket_t *,sizeof(epoll_socket_t));
	if(NULL == epoll_socket) return NULL;
	epoll_socket->recv_mlist = create_mlist();//创建队列（线程安全）
	if(NULL == epoll_socket->recv_mlist) return NULL;
	DMALLOC(epoll_socket->thread_lock,pthread_mutex_t *,sizeof(pthread_mutex_t));
	if(NULL == epoll_socket->thread_lock) return NULL;
	pthread_mutex_init(epoll_socket->thread_lock,NULL);//初始化线程锁
	epoll_socket->status = EPOLL_SOCKET_STATUS_OK;
	epoll_socket->curr_connections = 0;
	epoll_socket->total_connections = 0;
	return epoll_socket;	
}

/**
 * @desc 开始网络监听
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @param int port 监听端口
 * @return void
 */
void start_epoll_socket(epoll_socket_t *epoll_socket,int port)
{
	assert(epoll_socket && port > 0 && port < 65535);
	epoll_socket->listenfd = init_sock(port);//初始化网络监听socket
	epoll_socket->epfd = init_epoll();//初始化epoll
	//将网络监听socket句柄添加到epoll
	ctl_epoll(epoll_socket->epfd,epoll_socket->listenfd,EPOLL_CTL_ADD,EPOLLIN|EPOLLET);
	//开始监听
	start_epoll(epoll_socket);
}

/**
 * @desc 发送数据
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @param mlist_item_t *mlist_item 要发送的数据
 * @return void
 */
void send_epoll_socket(epoll_socket_t *epoll_socket,mlist_item_t *mlist_item)
{
	assert(epoll_socket && mlist_item);
    if(0 == ctl_epoll(epoll_socket->epfd,mlist_item->fd,EPOLL_CTL_MOD,EPOLLOUT|EPOLLET))//将状态设置成输出
	{
		//检查发送状态
		if(true == send_data(epoll_socket,mlist_item->fd,mlist_item->buff->data,mlist_item->buff->length))//发送数据
		{
			ctl_epoll(epoll_socket->epfd,mlist_item->fd,EPOLL_CTL_MOD,EPOLLIN|EPOLLET);//将fd放回队列里面复用,长连接
		}
		else
		{
			printf("send data fail\n");
			/*
			忽略错误,这里不去删除会不会耗尽epfd event的资源呢？
			if(0 != ctl_epoll(epoll_socket->epfd,mlist_item->fd,EPOLL_CTL_DEL,0))//将已经关闭的cfd从队列里面删除
			{
				perror("send data fail delete cfd fail\n");
			}
			*/
			epoll_socket->curr_connections--;
		}
	}
	else epoll_socket->curr_connections--;	
}

void skip_epoll_socket(epoll_socket_t *epoll_socket,int cfd)
{
	ctl_epoll(epoll_socket->epfd,cfd,EPOLL_CTL_DEL,0);	
	//epoll_socket->curr_connections--;
}

/**
 * @desc 锁定线程锁
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @return void
 */
void lock_epoll_socket_thread(epoll_socket_t *epoll_socket)
{
	pthread_mutex_lock(epoll_socket->thread_lock);	
}

/**
 * @desc 解锁线程锁
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @return void
 */
void unlock_epoll_socket_thread(epoll_socket_t *epoll_socket)
{
	pthread_mutex_unlock(epoll_socket->thread_lock);	
}

/**
 * @desc 关闭epoll_socket
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @return void
 */
void shutdown_epoll_socket(epoll_socket_t *epoll_socket)
{
	epoll_socket->status = EPOLL_SOCKET_STATUS_CLOSE;//关闭主体socket循环	
}

/**
 * @desc 删除epoll_socket
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @return void
 */
void destroy_epoll_socket(epoll_socket_t *epoll_socket)
{
	assert(epoll_socket);	
	destroy_mlist(epoll_socket->recv_mlist);
	close(epoll_socket->listenfd);//多很多请求的时候，这个会耗点时间
	close(epoll_socket->epfd);
	pthread_mutex_destroy(epoll_socket->thread_lock);
	DFREE(epoll_socket->thread_lock);
	//DFREE(epoll_socket);//不能在这里释放这个，主线程哪里还有要用的
}

/**
 * @desc 初始化网络监听sicket
 * @param int port 监听端口
 * @return int sock句柄
 */
int init_sock(int port)
{
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(struct sockaddr_in));
	int fd = socket(AF_INET,SOCK_STREAM,0);
	if(fd<0) return -1;
	setnonblocking(fd);//设置非阻塞
	setreuseaddr(fd);//设置端口复用

	server_addr.sin_family = AF_INET;     
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   
	server_addr.sin_port = htons(port);     
	//绑定端口
	if(0 != bind(fd,(struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)))
	{
		perror("bind fail\n");	
		close(fd);
		exit(1);
	}
	//监听,准备接收连接   
	if(0 != listen(fd,SOMAXCONN))
	{
		perror("listen fail\n");	
		close(fd);
		exit(1);
	} 
	return fd;
}

/**
 * @desc 设置成非阻塞模式
 * @param int fd socket句柄
 * @return void
 */
void setnonblocking(int fd) 
{     
	int opts;     
	opts = fcntl(fd,F_GETFL);     
	if (opts < 0)     
	{         
		perror("fcntl(sock,GETFL)");         
		exit(1);     
	}     
	opts = opts|O_NONBLOCK;     
	if (fcntl(fd,F_SETFL,opts) < 0)     
	{         
		perror("fcntl(sock,SETFL,opts)");         
		exit(1);     
	}  
}

/**
 * @desc 端口复用
 * @param int fd socket句柄
 * @return void
 */
void setreuseaddr(int fd)
{
	int opt = 1;
	if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(&opt)) < 0)     
	{         
		perror("setsockopt");         
		exit(1);     
	}  
}

/**
 * @desc 初始化epoll
 * @param void
 * @return void
 */
int init_epoll(void)
{
	int epfd;
	epfd = epoll_create(EPOLL_EVENT_MAX_SIZE);//这里的这个参数是没有用的
	if(epfd<0)
	{
		perror("epoll_create fail\n");
		exit(1);
	}
	return epfd;
}

/**
 * @desc epoll控制函数
 * @param int epfd epoll句柄
 * @param int cfd 新连接句柄
 * @param int action 操作命令
 * @param int event_type 参数
 * @return int
 */
int ctl_epoll(int epfd,int cfd,int action,int event_type)
{
	int ret;
	if(action != EPOLL_CTL_DEL)
	{
		struct epoll_event ev;	
		memset(&ev,0,sizeof(struct epoll_event));
		ev.data.fd = cfd;     
		ev.events = event_type;  
		ret = epoll_ctl(epfd, action, cfd, &ev);
	}
	else
	{
		ret = epoll_ctl(epfd, action, cfd, NULL);
	}
	/*
	if(0 != ret)
	{
		printf("action = %d,errno = %d\t",action,errno);
		printf("ctl_epoll fail : %s\t",strerror(errno));
		if(event_type == (EPOLLIN|EPOLLET))
		{
			printf("event_type = %s\n","et | in");
		}
		else if(event_type == (EPOLLOUT|EPOLLET))
		{
			printf("event_type = %s\n","et | out");
		}
		else
		{
			printf("event_type = %d\n",event_type);
		}
		close(cfd);
	}
	*/
	return ret;
}

/**
 * @desc 开始监听网络（这个就是主体循环）
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @return void
 */
void start_epoll(epoll_socket_t *epoll_socket)
{
	struct epoll_event events[EPOLL_EVENT_MAX_SIZE];	
	int i,cfd,nfds,timeout = -1;//-1代表一直在等待，直到有事件发生(有请求)
	struct sockaddr_in sin;  
	socklen_t len = sizeof(struct sockaddr_in);  
	while(EPOLL_SOCKET_STATUS_OK == epoll_socket->status)
	{
		nfds = epoll_wait(epoll_socket->epfd,events,EPOLL_EVENT_MAX_SIZE,timeout);	
		if(-1 != nfds)
		{
			for(i=0;i<nfds;i++)
			{
				if(events[i].data.fd == epoll_socket->listenfd)
				{
					//有新的连接
					if((cfd = accept(epoll_socket->listenfd,(struct sockaddr*)&sin, &len)) == -1)  
					{  
						if(errno != EAGAIN && errno != EINTR)  
						{  
							printf("%s: bad accept strerror = %s\n", __func__,strerror(errno));  
						}  
					}  
					else
					{
						setnonblocking(cfd);//设置成非阻塞模式
						ctl_epoll(epoll_socket->epfd,cfd,EPOLL_CTL_ADD,EPOLLIN|EPOLLET);//添加到epoll			
						epoll_socket->total_connections++;
						epoll_socket->curr_connections++;
					}
				}	
				else
				{
					if(events[i].events & EPOLLIN)
					{
						//有请求事件
						if(events[i].data.fd > 0)
						{
							mlist_item_t *mlist_item = recv_data(epoll_socket,events[i].data.fd);
							if(NULL != mlist_item)
							{
								push_mlist(epoll_socket->recv_mlist,mlist_item);	
								pthread_mutex_unlock(epoll_socket->thread_lock);
							}
						}
					}
					//忽略其他类型
					else if(events[i].events & EPOLLOUT)
					{
						//TODO send_epoll_socket
					}	
					else
					{
						perror("unknow type\n");	
					}
				}
			}
		}
	}	
	DFREE(epoll_socket);
}

/**
 * @desc 接收数据
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @param int cfd 当前连接socket句柄
 * @return mlist_item_t *
 */
mlist_item_t *recv_data(epoll_socket_t *epoll_socket,int cfd)
{
	buff_t *buff = create_buff(EPOLL_RECV_BUFF_SIZE);
	bool err = false,no_data_flag = false;
	char content[EPOLL_RECV_BUFF_SIZE];	
	while(EPOLL_SOCKET_STATUS_OK == epoll_socket->status && false == no_data_flag)
	{
		memset(content,0,EPOLL_RECV_BUFF_SIZE);
		int len = recv(cfd,content,EPOLL_RECV_BUFF_SIZE,0);
		if(len > 0)
		{
			append_buff(buff,content,len);	
			if(len < EPOLL_RECV_BUFF_SIZE) no_data_flag = true; //这里这样写正确吗？能一次读到 EPOLL_RECV_BUFF_SIZE 那么多数据吗？		
		}
		else if(len == 0)
		{
			//客户端主动关闭,这里不需要主动ctl_epoll EPOLL_CTL_DEL,系统会自己处理
			close(cfd);
			destroy_buff(buff);
			err = true;
			epoll_socket->curr_connections--;
			break;
		}
		else
		{
			if(errno != EAGAIN)
			{
				//不等于 "没有内容",有错误
				close(cfd);	
				destroy_buff(buff);
				err = true;
			}
			break;
		}
	}
	if(err == false) return create_mlist_item(cfd,buff);//无错误
	else return NULL;//有错误	
}

/**
 * @desc 发送数据
 * @param epoll_socket_t *epoll_socket epoll_socket句柄
 * @param int cfd 当前连接句柄
 * @param void *content 需要发送的内容
 * @param int length 需要发送内容的长度
 * @return bool
 */
bool send_data(epoll_socket_t *epoll_socket,int cfd,void *content,int length)
{
	int have_send_len = 0;
	int ret = -1;
	while(have_send_len < length && EPOLL_SOCKET_STATUS_OK == epoll_socket->status)
	{
		ret = send(cfd,content + have_send_len,length - have_send_len,0);
		if(-1 == ret) return false;//客户端已经断开连接
		have_send_len += ret;
	}
	return true;
}
