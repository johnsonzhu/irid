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
#include "buff.h"
#include "cli.h"

#define PARSE_HOST_PORT(str,host,port) \
do{\
	char *__p = strstr(str,":");\
	if(NULL != __p)\
	{\
		strncpy(host,str,__p - str);\
		__p++;\
		port = atoi(__p);\
	}\
	else\
	{\
		strcpy(host,"0.0.0.0");\
		port = 0;\
	}\
}while(0)

static bool call(const char *host,int port,char *content,int content_size);
static cmd_ret_t *cli_get(int argc,char **argv);
static cmd_ret_t *cli_set(int argc,char **argv);
static cmd_ret_t *cli_add(int argc,char **argv);
static cmd_ret_t *cli_replace(int argc,char **argv);
static cmd_ret_t *cli_delete(int argc,char **argv);
static cmd_ret_t *cli_increment(int argc,char **argv);
static cmd_ret_t *cli_decrement(int argc,char **argv);
static cmd_ret_t *cli_append(int argc,char **argv);
static cmd_ret_t *cli_prepend(int argc,char **argv);
static cmd_ret_t *cli_stats(int argc,char **argv);
static cmd_ret_t *cli_count(int argc,char **argv);
static cmd_ret_t *cli_push(int argc,char **argv);
static cmd_ret_t *cli_pop(int argc,char **argv);
static cmd_ret_t *cli_shift(int argc,char **argv);
static cmd_ret_t *cli_unshift(int argc,char **argv);
static cmd_ret_t *cli_flush_all(int argc,char **argv);
static cmd_ret_t *cli_version(int argc,char **argv);
static cmd_ret_t *cli_page(int argc,char **argv);
static cmd_ret_t *cli_list(int argc,char **argv);
static cmd_ret_t *cli_reload(int argc,char **argv);

bool call(const char *host,int port,char *content,int content_size)
{
	assert(host && port >= 0);
	struct sockaddr_in sain;
	memset(&sain, 0, sizeof(sain));
	sain.sin_family = AF_INET;
	if(inet_aton(host, &sain.sin_addr) == 0) return false;	
	uint16_t snum = port;
	sain.sin_port = htons(snum);
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if(fd == -1) return false;
	if(-1 == connect(fd, (struct sockaddr *)&sain, sizeof(sain))) //连接
	{
		printf("cann't connect to %s:%d\n",host,port);
		close(fd); 
		return false;	
	}
	char buffer[65535];
	memset(buffer,0,65535);
	//printf("\nsend data = %s\n",(char *)content);
	send(fd,content,content_size,0); 
	recv(fd,buffer,65535,0); 
	printf("%s",buffer);
	close(fd);
	return true;	
}

void cli(int argc,char **argv)
{
	if(argc < 3) usage(argc,argv);	
	cmd_ret_t *cmd_ret = NULL;
	char host[16];
	memset(host,0,16);
	int port = 0;
	PARSE_HOST_PORT(argv[2],host,port);

	if(0 == strcmp(argv[1],"get")) cmd_ret = cli_get(argc,argv);	
	else if(0 == strcmp(argv[1],"set")) cmd_ret = cli_set(argc,argv);
	else if(0 == strcmp(argv[1],"add")) cmd_ret = cli_add(argc,argv);
	else if(0 == strcmp(argv[1],"replace")) cmd_ret = cli_replace(argc,argv);
	else if(0 == strcmp(argv[1],"delete")) cmd_ret = cli_delete(argc,argv);
	else if(0 == strcmp(argv[1],"append")) cmd_ret = cli_append(argc,argv);
	else if(0 == strcmp(argv[1],"prepend")) cmd_ret = cli_prepend(argc,argv);
	else if(0 == strcmp(argv[1],"increment")) cmd_ret = cli_increment(argc,argv);
	else if(0 == strcmp(argv[1],"decrement")) cmd_ret = cli_decrement(argc,argv);
	else if(0 == strcmp(argv[1],"count")) cmd_ret = cli_count(argc,argv);
	else if(0 == strcmp(argv[1],"pop")) cmd_ret = cli_pop(argc,argv);
	else if(0 == strcmp(argv[1],"push")) cmd_ret = cli_push(argc,argv);
	else if(0 == strcmp(argv[1],"shift")) cmd_ret = cli_shift(argc,argv);
	else if(0 == strcmp(argv[1],"unshift")) cmd_ret = cli_unshift(argc,argv);
	else if(0 == strcmp(argv[1],"stats")) cmd_ret = cli_stats(argc,argv);
	else if(0 == strcmp(argv[1],"flush_all")) cmd_ret = cli_flush_all(argc,argv);
	else if(0 == strcmp(argv[1],"version")) cmd_ret = cli_version(argc,argv);
	else if(0 == strcmp(argv[1],"page")) cmd_ret = cli_page(argc,argv);
	else if(0 == strcmp(argv[1],"list")) cmd_ret = cli_list(argc,argv);
	else if(0 == strcmp(argv[1],"reload")) cmd_ret = cli_reload(argc,argv);
	else
	{
		usage(argc,argv);	
	} 	
	if(NULL != cmd_ret)
	{
		call(host,port,cmd_ret->data,cmd_ret->length);
	}
}

cmd_ret_t *cli_get(int argc,char **argv)
{
	int i = 4;
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < i) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"get %s ",argv[3]);
	if(argc >i)
	{
		for(;i<argc;i++)
		{
			cmd_ret->length += sprintf(cmd_ret->data+cmd_ret->length,"%s ",argv[i]);
		}
	}
	cmd_ret->length += sprintf(cmd_ret->data+cmd_ret->length,"\r\n");
	return cmd_ret;	
}

cmd_ret_t *cli_set(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 5) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"set %s 0 0 %d\n%s\r\n",argv[3],strlen(argv[4]),argv[4]);
	return cmd_ret;	
}

cmd_ret_t *cli_add(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 5) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"add %s 0 0 %d\n%s\r\n",argv[3],strlen(argv[4]),argv[4]);
	return cmd_ret;	
}

cmd_ret_t *cli_replace(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 5) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"replace %s 0 0 %d\n%s\r\n",argv[3],strlen(argv[4]),argv[4]);
	return cmd_ret;	
}

cmd_ret_t *cli_delete(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 4) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"delete %s\r\n",argv[3]);
	return cmd_ret;	
}

cmd_ret_t *cli_increment(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 5) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"incr %s %d",argv[3],atoi(argv[4]));
	return cmd_ret;	
}

cmd_ret_t *cli_decrement(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 5) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"decr %s %d",argv[3],atoi(argv[4]));
	return cmd_ret;	
}

cmd_ret_t *cli_append(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 5) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"append %s 0 0 %d\n%s\r\n",argv[3],strlen(argv[4]),argv[4]);
	return cmd_ret;	
}

cmd_ret_t *cli_prepend(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 5) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"prepend %s 0 0 %d\n%s\r\n",argv[3],strlen(argv[4]),argv[4]);
	return cmd_ret;	
}

cmd_ret_t *cli_stats(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 3) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"stats \r\n");
	return cmd_ret;	
}

cmd_ret_t *cli_count(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 4) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"count %s\r\n",argv[3]);
	return cmd_ret;	
}

cmd_ret_t *cli_push(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 5) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"push %s 0 0 %d\n%s\r\n",argv[3],strlen(argv[4]),argv[4]);
	return cmd_ret;	
}

cmd_ret_t *cli_pop(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 4) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"pop %s\r\n",argv[3]);
	return cmd_ret;	
}

cmd_ret_t *cli_shift(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 4) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"shift %s\r\n",argv[3]);
	return cmd_ret;	
}

cmd_ret_t *cli_unshift(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 5) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"unshift %s 0 0 %d\n%s\r\n",argv[3],strlen(argv[4]),argv[4]);
	return cmd_ret;	
}

cmd_ret_t *cli_flush_all(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 3) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"flush_all \r\n");
	return cmd_ret;	
}

cmd_ret_t *cli_version(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 3) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"version \r\n");
	return cmd_ret;	
}

cmd_ret_t *cli_page(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 6) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"page %s %d %d \r\n",argv[3],atoi(argv[4]),atoi(argv[5]));
	return cmd_ret;	
}

cmd_ret_t *cli_list(int argc,char **argv)
{
	int i = 3;
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < i) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"list ");

	if(argc>i)
	{
		if(5 == argc)
		{
			if(0 == strcmp(argv[3],"-fm") || 0 == strcmp(argv[3],"-mm") || 0 == strcmp(argv[3],"-em"))
			{
				cmd_ret->length += sprintf(cmd_ret->data+cmd_ret->length,"%s %s ",argv[3],argv[4]);
			}	
			else usage(argc,argv);	
		}
		else usage(argc,argv);	
	}
	else
	{
		cmd_ret->length += sprintf(cmd_ret->data+cmd_ret->length,"NO NO ");
	}
	cmd_ret->length += sprintf(cmd_ret->data+cmd_ret->length,"\r\n");
	return cmd_ret;	
}

cmd_ret_t *cli_reload(int argc,char **argv)
{
	cmd_ret_t *cmd_ret = NULL;
	DMALLOC(cmd_ret,cmd_ret_t *,sizeof(cmd_ret_t));	
	memset(cmd_ret,0,sizeof(cmd_ret_t));
	if(argc < 4) usage(argc,argv); 
	cmd_ret->length = sprintf(cmd_ret->data,"reload %s \r\n",argv[3]);
	return cmd_ret;	
}

void usage(int argc,char **argv)
{
	printf("\n%s Usage:\n",argv[0]);
	printf("  start server\t%s -c irid.conf\n",argv[0]);
	printf("  stop server\t%s -s stop\n",argv[0]);
	printf("\n");
	printf("%s CLI API:\n",argv[0]);
	printf("  %s add host:port key value\n",argv[0]);
	printf("  %s append host:port key value\n",argv[0]);
	printf("  %s count host:port key\n",argv[0]);
	printf("  %s decrement host:port key num\n",argv[0]);
	printf("  %s delete host:port key\n",argv[0]);
	printf("  %s flush_all host:port\n",argv[0]);
	printf("  %s get host:port key1 [key2]\n",argv[0]);
	printf("  %s increment host:port key num\n",argv[0]);
	printf("  %s list host:port [-fm str] [-mm str] [-em str]\n",argv[0]);
	printf("  %s page host:port key offset limit\n",argv[0]);
	printf("  %s prepend host:port key value\n",argv[0]);
	printf("  %s pop host:port key\n",argv[0]);
	printf("  %s push host:port key value\n",argv[0]);
	printf("  %s reload host:port filename\n",argv[0]);
	printf("  %s replace host:port key value\n",argv[0]);
	printf("  %s set host:port key value\n",argv[0]);
	printf("  %s shift host:port key\n",argv[0]);
	printf("  %s stats host:port\n",argv[0]);
	printf("  %s unshift host:port key value\n",argv[0]);
	printf("  %s version host:port\n",argv[0]);
	exit(0);
}

