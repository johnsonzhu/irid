#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/epoll.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <errno.h>  
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include "dalloc.h"
#include "epoll_socket.h"
#include "epoll_thread.h"
#include "mc_protocol.h"
#include "lrumc.h"
#include "config.h"
#include "cli.h"

#define CONFIG_FILENAME "irid.conf"
#define PID_FILENAME "irid.pid"

//data struct
typedef enum server_status_e server_status_t;
enum server_status_e
{
	SERVER_STATUS_CLOSE = 0,	
	SERVER_STATUS_RUN = 1	
};

//function
void server_end(int signo);//关闭程序
bool init_signal(void);//初始化关闭信号
void init_param(int argc,char **argv);
void create_pid_file(void);
void unlink_pid_file(void);
mlist_item_t *thread_func(epoll_socket_t *epoll_socket,mlist_item_t *item);//业务处理函数,这个就是route

//global var
char *g_version = "v0.9";
char g_config_file[255];
int g_thread_num = 1; //启动的线程数量
epoll_socket_t *g_epoll_socket; //socket句柄
thread_t *g_thread;//thread句柄
server_status_t g_status; //防止重复关闭
lrumc_t *g_lrumc;
int g_segment_num = 0;
int g_port = 11111;//监听的端口
int g_bucket_full_rate = 72;
segment_config_t *g_lrumc_config;
pthread_mutex_t g_lrumc_lock; //lrumc不是线程安全的，这里为他加个锁，保证线程安全
int main(int argc,char **argv)
{
	init_param(argc,argv);
	init_signal(); //注册关闭信号
	printf("using config file %s\n",g_config_file);
	init_config(g_config_file,&set_config_item);
	//return 0;
	g_status = SERVER_STATUS_RUN; //设置服务器的状态
	pthread_mutex_init(&g_lrumc_lock,NULL);
	create_pid_file();
	prepare_crypt_table();
	g_lrumc = init_lrumc(g_lrumc_config,g_segment_num);
	g_epoll_socket = create_epoll_socket();//创建socket句柄
	g_thread = create_epoll_thread(g_thread_num,g_epoll_socket,thread_func);//创建thread句柄
	start_epoll_socket(g_epoll_socket,g_port);//开始监听
	return 0;	
}

/**
 * @desc 注册关闭信号
 * @param void
 * @param bool
 */
bool init_signal(void)
{
    if(
		signal(SIGTERM, server_end) == SIG_ERR || 
		signal(SIGINT, server_end) == SIG_ERR ||
       	signal(SIGHUP, server_end) == SIG_ERR || 
		signal(SIGPIPE, SIG_IGN) == SIG_ERR || //这是用来忽略客户端端口连接的错误的
       	signal(SIGCHLD, server_end) == SIG_ERR
	)
	{
		return false;
    }
	return true;
}


void init_param(int argc,char **argv)
{
	//strcpy(g_config_file,CONFIG_FILENAME);
	g_config_file[0] = '\0';
	int i = 1;
	for(;i<argc;i++)
	{
		if(0 == strcmp(argv[i],"-c") || 0 == strcmp(argv[i],"--config"))
		{
			strcpy(g_config_file,argv[++i]);	
		}
		else if(0 == strcmp(argv[i],"-s"))
		{
			char *cmd = argv[++i];
			if(0 == strcmp("stop",cmd))
			{
				FILE *fp = fopen(PID_FILENAME,"r");
				if(NULL == fp)
				{
					printf("open pid %s fail\n",PID_FILENAME);	
					exit(0);
				}
				char pid_str[10];
				memset(pid_str,0,10);
				fread(pid_str,10,1,fp);
				fclose(fp);
				kill(atoi(pid_str),SIGTERM);
			}	
			exit(0);
		}
		else if(
			0 == strcmp(argv[i],"get") ||
			0 == strcmp(argv[i],"add") ||
			0 == strcmp(argv[i],"set") ||
			0 == strcmp(argv[i],"replace") ||
			0 == strcmp(argv[i],"delete") ||
			0 == strcmp(argv[i],"append") ||
			0 == strcmp(argv[i],"prepend") ||
			0 == strcmp(argv[i],"increment") ||
			0 == strcmp(argv[i],"decrement") ||
			0 == strcmp(argv[i],"push") ||
			0 == strcmp(argv[i],"pop") ||
			0 == strcmp(argv[i],"shift") ||
			0 == strcmp(argv[i],"unshift") ||
			0 == strcmp(argv[i],"count") ||
			0 == strcmp(argv[i],"stats") ||
			0 == strcmp(argv[i],"page") || 
			0 == strcmp(argv[i],"version") || 
			0 == strcmp(argv[i],"list") || 
			0 == strcmp(argv[i],"reload") || 
			0 == strcmp(argv[i],"flush_all") 
			)
		{
			cli(argc,argv);
			exit(0);
		}
	}	
	if(0 == strlen(g_config_file))
	{
		usage(argc,argv);	
		exit(0);
	}
}

void create_pid_file(void)
{
	if(0 == access(PID_FILENAME,R_OK))
	{
		printf("Irid have running\n");	
		exit(1);
	}	
	FILE *fp = fopen(PID_FILENAME,"w");	
	if(NULL == fp)
	{
		printf("create pid file fail\n");	
		exit(1);
	}
	fprintf(fp,"%d",getpid());
	fclose(fp);
}

void unlink_pid_file(void)
{
	if(0 == access(PID_FILENAME,R_OK)) unlink(PID_FILENAME);	
}

/**
 * @desc 关闭程序
 * @param int signo 信号
 * @return void
 */
void server_end(int signo)
{
	//防止重复关闭
	if(SERVER_STATUS_CLOSE == g_status) return;
	g_status = SERVER_STATUS_CLOSE;
	//关闭socket
	shutdown_epoll_socket(g_epoll_socket);
	//关闭thread
	shutdown_epoll_thread(g_thread);
	//释放内存资源
	destroy_epoll_thread(g_thread);
	destroy_epoll_socket(g_epoll_socket);
	destroy_lrumc(g_lrumc);
	DFREE(g_lrumc_config);
	unlink_pid_file();
}

/**
 * @desc 业务逻辑处理总入口
 * @param epoll_socket_t *epoll_socket socket句柄
 * @param mlist_item_t *item 接收到的数据(raw) 不需要在本函数释放内存
 * @param mlist_item_t * 需要再里面创建的
 */
mlist_item_t *thread_func(epoll_socket_t *epoll_socket,mlist_item_t *item)
{
	pthread_mutex_lock(&g_lrumc_lock);
	buff_t *buff = parse(g_lrumc,item->buff->data);
	pthread_mutex_unlock(&g_lrumc_lock);
	if(NULL == buff) return NULL;
	return create_mlist_item(item->fd,buff);
}

