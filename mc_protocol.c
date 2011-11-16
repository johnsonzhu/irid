#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include "dalloc.h"
#include "config.h"
#include "lrumc.h"
#include "mc_protocol.h"
#include "epoll_socket.h"

static buff_t *handle_cmd_add(lrumc_t *lrumc,char *cmd,void *data); //处理添加数据的请求add set replace push unshift append prepend
static buff_t *handle_cmd_delete(lrumc_t *lrumc,char *cmd,void *data); //处理删除数据的请求 delete pop shift
static buff_t *handle_cmd_inde(lrumc_t *lrumc,char *cmd,void *data); //处理数据增长递减请求 incr decr
static buff_t *handle_cmd_get(lrumc_t *lrumc,char *cmd,void *data); //处理get请求
static buff_t *handle_cmd_list(lrumc_t *lrumc,char *cmd,void *data);//处理list请求
static buff_t *handle_cmd_misc(lrumc_t *lrumc,char *cmd,void *data); //处理杂项请求如 version flush_all
static buff_t *handle_cmd_count(lrumc_t *lrumc,char *cmd,void *data); //处理count请求
static buff_t *handle_cmd_page(lrumc_t *lrumc,char *cmd,void *data); //处理page请求
static buff_t *handle_cmd_stats(lrumc_t *lrumc,char *cmd,void *data); //处理stats请求
static buff_t *handle_cmd_reload(lrumc_t *lrumc,char *cmd,void *data); //处理reload请求

extern char *g_version;
extern int g_thread_num;
extern epoll_socket_t *g_epoll_socket;

/**
 * @desc 解析mc协议
 * @param lrumc_t *lrumc lrumc句柄
 * @param void *data 接收到的数据
 * @return buff_t *
 */
buff_t *parse(lrumc_t *lrumc,void *data)
{
	printf("input=%s\n",(char *)data);
	//获取当前操作指令
	char cmd[11];
	memset(cmd,0,11);

	//获取当前指令
	GET_STR(cmd,11,data,MC_SEP_S,1);
	//printf("cmd=%s\n",cmd);

	buff_t *buff = NULL;
	if(
		0 == strcmp(cmd,MC_CMD_ADD) || 
		0 == strcmp(cmd,MC_CMD_SET) || 
		0 == strcmp(cmd,MC_CMD_REPLACE) ||
		0 == strcmp(cmd,MC_CMD_APPEND) ||
		0 == strcmp(cmd,MC_CMD_PREPEND) ||
		0 == strcmp(cmd,MC_CMD_PUSH) ||
		0 == strcmp(cmd,MC_CMD_UNSHIFT) 
		)
	{
		buff = handle_cmd_add(lrumc,cmd,data);
	}
	else if(0 == strcmp(cmd,MC_CMD_LIST)) buff = handle_cmd_list(lrumc,cmd,data);
	else if(0 == strcmp(cmd,MC_CMD_PAGE)) buff = handle_cmd_page(lrumc,cmd,data);
	else if(0 == strcmp(cmd,MC_CMD_STATS)) buff = handle_cmd_stats(lrumc,cmd,data);
	else if(0 == strcmp(cmd,MC_CMD_RELOAD)) buff = handle_cmd_reload(lrumc,cmd,data);
	else if(
		0 == strcmp(cmd,MC_CMD_DELETE) ||
		0 == strcmp(cmd,MC_CMD_POP) ||
		0 == strcmp(cmd,MC_CMD_SHIFT) 
		)
	{
		buff = handle_cmd_delete(lrumc,cmd,data);
	}
	else if(0 == strcmp(cmd,MC_CMD_COUNT)) buff = handle_cmd_count(lrumc,cmd,data);
	else if(0 == strcmp(cmd,MC_CMD_GET)) buff = handle_cmd_get(lrumc,cmd,data);
	else if(0 == strcmp(cmd,MC_CMD_INCR) || 0 == strcmp(cmd,MC_CMD_DECR)) buff = handle_cmd_inde(lrumc,cmd,data);
	else if(0 == strncmp(cmd,MC_CMD_QUIT,4))
	{
		 //return NULL;//这个是特别处理的	
	}
	else buff = handle_cmd_misc(lrumc,cmd,data);

	if(NULL == buff)
	{
		buff = create_buff(6); 
		append_buff(buff,MC_END,strlen(MC_END));
	}
	lrumc->bytes_read += buff->length;
	printf("output=%s\n",(char *)buff->data);
	return buff;
}

/**
 * @desc 数据add操作(包括 add set replace append prepend push unshift)
 * @param lrumc_t *lrumc lrumc总句柄
 * @param char *cmd 当前操作指令
 * @param void *data 请求的数据
 * @return buff_t *
 */
buff_t *handle_cmd_add(lrumc_t *lrumc,char *cmd,void *data)
{
	buff_t *buff = create_buff(30);
	char *input_key = NULL;
	pary_type_t value_type;
	int expire_time,data_len;
	bool res = false;
	//DMALLOC(key,char *,sizeof(char)*KEY_LEN);
	char key[KEY_LEN];
	memset(key,0,KEY_LEN);
	GET_STR(key,KEY_LEN,data,MC_SEP_S,1);	
	GET_INT(value_type,data,MC_SEP_S,1);	
	GET_INT(expire_time,data,MC_SEP_S,1);	
	GET_INT(data_len,data,MC_SEP_N,1);	
	void *content = NULL;
	DMALLOC(content,void *,data_len);
	memset(content,0,data_len);
	memcpy(content,data,data_len);

	DMALLOC(input_key,char *,KEY_LEN);
	memset(input_key,0,KEY_LEN);
	//DMALLOC(input_path,char *,KEY_LEN);
	char input_path[KEY_LEN];
	memset(input_path,0,KEY_LEN);
	PARSE_KEY(key,input_key,input_path);

	if(0 == strcmp(cmd,MC_CMD_ADD))
	{
		//新增数据
		res = add_lrumc_data(lrumc,input_key,input_path,content,data_len,value_type);		
	}	
	else if(0 == strcmp(cmd,MC_CMD_SET))
	{
		//set数据
		res = set_lrumc_data(lrumc,input_key,input_path,content,data_len,value_type);		
	}	
	else if(0 == strcmp(cmd,MC_CMD_REPLACE))
	{
		//replace数据
		res = replace_lrumc_data(lrumc,input_key,input_path,content,data_len,value_type);		
	}
	else if(0 == strcmp(cmd,MC_CMD_APPEND))
	{
		//append数据
		res = append_lrumc_data(lrumc,input_key,input_path,content,data_len);		
	}
	else if(0 == strcmp(cmd,MC_CMD_PREPEND))
	{
		//append数据
		res = prepend_lrumc_data(lrumc,input_key,input_path,content,data_len);		
	}
	else if(0 == strcmp(cmd,MC_CMD_PUSH))
	{
		//push数据
		res = push_lrumc_data(lrumc,input_key,input_path,content,data_len,value_type);		
	}
	else if(0 == strcmp(cmd,MC_CMD_UNSHIFT))
	{
		//unshift数据
		res = unshift_lrumc_data(lrumc,input_key,input_path,content,data_len,value_type);		
	}
	else
	{
		printf("bad cmd \n");	
		exit(0);
	}
	if(true == res) append_buff(buff,MC_STORED,strlen(MC_STORED));
	else append_buff(buff,MC_NOT_STORED,strlen(MC_NOT_STORED));
	
	//DFREE(key);
	DFREE(content);
	//DFREE(input_path);
	return buff;	
}

/**
 * @desc 数据delete操作(包括 delete pop shift)
 * @param lrumc_t *lrumc lrumc总句柄
 * @param char *cmd 当前操作指令
 * @param void *data 请求的数据
 * @return buff_t *
 */
buff_t *handle_cmd_delete(lrumc_t *lrumc,char *cmd,void *data)
{
	buff_t *buff = NULL;
	char key[KEY_LEN];
	memset(key,0,KEY_LEN);
	GET_STR(key,KEY_LEN,data,MC_SEP_R,1);	

	char input_key[KEY_LEN];
	char input_path[KEY_LEN];
	memset(input_key,0,KEY_LEN);
	memset(input_path,0,KEY_LEN);
	PARSE_KEY(key,input_key,input_path);

	//返回结果
	if(0 == strcmp(cmd,MC_CMD_DELETE))
	{
		buff = create_buff(30);
		bool res = delete_lrumc_data(lrumc,input_key,input_path);
		if(true == res) append_buff(buff,MC_DELETED,strlen(MC_DELETED));
		else append_buff(buff,MC_NOT_FOUND,strlen(MC_NOT_FOUND));
	}
	else if(0 == strcmp(cmd,MC_CMD_POP))
	{
		buff = pop_lrumc_data(lrumc,input_key,input_path);
	}
	else if(0 == strcmp(cmd,MC_CMD_SHIFT))
	{
		buff = shift_lrumc_data(lrumc,input_key,input_path);
	}
	return buff;
}

/**
 * @desc 数据增长递减操作(包括 incr decr)
 * @param lrumc_t *lrumc lrumc总句柄
 * @param char *cmd 当前操作指令
 * @param void *data 请求的数据
 * @return buff_t *
 */
buff_t *handle_cmd_inde(lrumc_t *lrumc,char *cmd,void *data)
{
	buff_t *buff = create_buff(30);
	int num = 0;
	long long ret_num = 0;
	char key[KEY_LEN];	
	memset(key,0,KEY_LEN);
	GET_STR(key,KEY_LEN,data,MC_SEP_S,1);	
	GET_INT(num,data,MC_SEP_R,1);	

	char input_key[KEY_LEN];
	char input_path[KEY_LEN];
	memset(input_key,0,KEY_LEN);
	memset(input_path,0,KEY_LEN);
	PARSE_KEY(key,input_key,input_path);

	if(0 == strcmp(cmd,MC_CMD_INCR)) ret_num = increment_lrumc_data(lrumc,input_key,input_path,num);	
	else ret_num = decrement_lrumc_data(lrumc,input_key,input_path,num);	

	if(ret_num != L_L_MAX) 
	{
		char ret[30];
		memset(ret,0,30);
		int len = sprintf(ret,"%lld\r\n",ret_num);
		append_buff(buff,ret,len);
	}
	else append_buff(buff,MC_NOT_FOUND,strlen(MC_NOT_FOUND));
	return buff;	
}

/**
 * @desc 数据get操作(包括 get)
 * @param lrumc_t *lrumc lrumc总句柄
 * @param char *cmd 当前操作指令
 * @param void *data 请求的数据
 * @return buff_t *
 */
buff_t *handle_cmd_get(lrumc_t *lrumc,char *cmd,void *data)
{
	buff_t *buff = create_buff(1024);
	char key[KEY_LEN];
	int curr_length = 0;
	while(NULL != data && 0 != memcmp(data,MC_SEP_R,1) && 0 != memcmp(data,MC_SEP_N,1))
	{	
		memset(key,0,KEY_LEN);
		GET_STR(key,KEY_LEN,data,MC_SEP_S,1);	
		char input_key[KEY_LEN];
		char input_path[KEY_LEN];
		memset(input_key,0,KEY_LEN);
		memset(input_path,0,KEY_LEN);
		PARSE_KEY(key,input_key,input_path);

		//printf("key=%s\n",key);	
		//printf("input_key=%s\n",input_key);	
		//printf("input_path=%s\n",input_path);	

		get_ret_t *get_ret = get_lrumc_data(lrumc,input_key,input_path);

		if(NULL != get_ret)
		{
			if(get_ret->length > 0)
			{
				//找到数据
				curr_length += 35 + get_ret->length + strlen(key);	
				if(buff->max_length < curr_length)
				{
					DREALLOC(buff->data,char *,curr_length);
					buff->max_length = curr_length;
				}
					
				buff->length += sprintf(buff->data + buff->length,"VALUE %s %d %ld\n",key,get_ret->type,get_ret->length);
				memcpy(buff->data + buff->length,get_ret->data,get_ret->length);
				buff->length += get_ret->length;
				strcpy(buff->data + buff->length,MC_RN);
				buff->length += 2;
			}
			DFREE(get_ret->data);
			DFREE(get_ret);
		}
	}
	//返回结果
	strcpy(buff->data + buff->length,MC_END);
	buff->length += 5;
	return buff;	
}

/**
 * @desc 数据list操作(包括 list)
 * @param lrumc_t *lrumc lrumc总句柄
 * @param char *cmd 当前操作指令
 * @param void *data 请求的数据
 * @return buff_t *
 */
buff_t *handle_cmd_list(lrumc_t *lrumc,char *cmd,void *data)
{
	char match_type[4];
	char match_str[KEY_LEN];
	memset(match_type,0,4);
	memset(match_str,0,KEY_LEN);
	GET_STR(match_type,4,data,MC_SEP_S,1);	
	GET_STR(match_str,KEY_LEN,data,MC_SEP_S,1);	

	return list_lrumc_data(lrumc,match_type,match_str);
}

/**
 * @desc 数据杂项操作(包括 version flush_all)
 * @param lrumc_t *lrumc lrumc总句柄
 * @param char *cmd 当前操作指令
 * @param void *data 请求的数据
 * @return buff_t *
 */
buff_t *handle_cmd_misc(lrumc_t *lrumc,char *cmd,void *data)
{
	buff_t *buff = create_buff(50);
	char ret[30];
	memset(ret,0,30);
	int len = 0;
	if(0 == strcmp(cmd,MC_CMD_FLUSH_ALL))
	{
		//删除全部数据
		len = sprintf(ret,"flush %ld key\r\n",flush_all(lrumc));
	}	
	else if(0 == strcmp(cmd,MC_CMD_VERSION))
	{
		len = sprintf(ret,"%s\r\n",g_version);		
	}
	else
	{
		len = sprintf(ret,"ERROR\r\n");
	}
	append_buff(buff,ret,len);
	return buff;
}

/**
 * @desc 数据count操作(包括 count)
 * @param lrumc_t *lrumc lrumc总句柄
 * @param char *cmd 当前操作指令
 * @param void *data 请求的数据
 * @return buff_t *
 */
buff_t *handle_cmd_count(lrumc_t *lrumc,char *cmd,void *data)
{
	buff_t *buff = create_buff(30);
	char key[KEY_LEN];
	memset(key,0,KEY_LEN);
	GET_STR(key,KEY_LEN,data,MC_SEP_R,1);	

	char input_key[KEY_LEN];
	char input_path[KEY_LEN];
	memset(input_key,0,KEY_LEN);
	memset(input_path,0,KEY_LEN);
	PARSE_KEY(key,input_key,input_path);
	//返回结果
	int num = count_lrumc_data(lrumc,input_key,input_path);
	char ret[30];
	memset(ret,0,30);
	int len = sprintf(ret,"%d\r\n",num);
	append_buff(buff,ret,len);
	return buff;
}

buff_t *handle_cmd_page(lrumc_t *lrumc,char *cmd,void *data)
{
	int offset = -1;
	int limit = -1;
	char key[KEY_LEN];
	memset(key,0,KEY_LEN);
	GET_STR(key,KEY_LEN,data,MC_SEP_S,1);	
	GET_INT(offset,data,MC_SEP_S,1);	
	GET_INT(limit,data,MC_SEP_S,1);	

	char input_key[KEY_LEN];
	char input_path[KEY_LEN];
	memset(input_key,0,KEY_LEN);
	memset(input_path,0,KEY_LEN);
	PARSE_KEY(key,input_key,input_path);
	//返回结果
	return page_lrumc_data(lrumc,input_key,input_path,offset,limit);
}

buff_t *handle_cmd_reload(lrumc_t *lrumc,char *cmd,void *data)
{
	buff_t *buff = NULL;
	char filename[255];
	memset(filename,0,255);
	GET_STR(filename,255,data,MC_SEP_S,1);	
	char tip[255];
	memset(tip,0,255);
	int len = 0;
	if(0 != access(filename,R_OK))
	{
		buff = create_buff(255);
		len = sprintf(tip,"bad config file %s\n",filename);	
		append_buff(buff,tip,len);
	}
	else
	{
		buff = reload_config(filename);	
	}
	return buff;
}

buff_t *handle_cmd_stats(lrumc_t *lrumc,char *cmd,void *data)
{
	buff_t *buff = create_buff(10000);	
	int len = 0;
	char tmp[10000];
	memset(tmp,0,10000);
	time_t now = time(NULL);
	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	len += sprintf(tmp+len,"STAT pid %d\n",getpid());
	len += sprintf(tmp+len,"STAT uptime %ld\n",now - lrumc->start_time);
	len += sprintf(tmp+len,"STAT time %ld\n",now);
	len += sprintf(tmp+len,"STAT version %s\n",g_version);
	len += sprintf(tmp+len,"STAT pointer_size %d\n",8*sizeof(unsigned long));
	len += sprintf(tmp+len,"STAT rusage_user %ld.%06ld\n",(long)usage.ru_utime.tv_sec,(long)usage.ru_utime.tv_usec);
	len += sprintf(tmp+len,"STAT rusage_system %ld.%06ld\n",(long)usage.ru_stime.tv_sec,(long)usage.ru_stime.tv_usec);
	len += sprintf(tmp+len,"STAT curr_connections %lu\n",g_epoll_socket->curr_connections);
	len += sprintf(tmp+len,"STAT total_connections %lu\n",g_epoll_socket->total_connections);
	len += sprintf(tmp+len,"STAT cmd_get %lu\n",lrumc->get_hits + lrumc->get_misses);
	len += sprintf(tmp+len,"STAT cmd_set %lu\n",lrumc->cmd_set);
	len += sprintf(tmp+len,"STAT cmd_flush %lu\n",lrumc->cmd_flush);
	len += sprintf(tmp+len,"STAT get_hits %lu\n",lrumc->get_hits);
	len += sprintf(tmp+len,"STAT get_misses %lu\n",lrumc->get_misses);
	len += sprintf(tmp+len,"STAT delete_hits %lu\n",lrumc->delete_hits);
	len += sprintf(tmp+len,"STAT delete_misses %lu\n",lrumc->delete_misses);
	len += sprintf(tmp+len,"STAT incr_hits %lu\n",lrumc->incr_hits);
	len += sprintf(tmp+len,"STAT incr_misses %lu\n",lrumc->incr_misses);
	len += sprintf(tmp+len,"STAT decr_hits %lu\n",lrumc->decr_hits);
	len += sprintf(tmp+len,"STAT decr_misses %lu\n",lrumc->decr_misses);
	len += sprintf(tmp+len,"STAT bytes_read %lu\n",lrumc->bytes_read);
	len += sprintf(tmp+len,"STAT bytes_written %lu\n",lrumc->bytes_written);
	len += sprintf(tmp+len,"STAT threads %d\n",g_thread_num);
	len += sprintf(tmp+len,"STAT curr_items %d\n",lrumc->curr_num);
	len += sprintf(tmp+len,"STAT total_items %lu\n",lrumc->curr_num + lrumc->evictions);
	len += sprintf(tmp+len,"STAT evictions %lu\n",lrumc->evictions);
	len += sprintf(tmp+len,"STAT segment num %d\n",lrumc->segment_num);
	int i = 0;
	for(;i<lrumc->segment_num;i++)
	{
		len += sprintf(tmp+len,"STAT segment %d max_num %d buff_num %d curr_num %d\n",i+1,lrumc->segment[i]->max_size,lrumc->segment[i]->buff_size,lrumc->segment[i]->curr_size);
	}

	append_buff(buff,tmp,len);
	return buff;
}
