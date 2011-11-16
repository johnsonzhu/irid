#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "dalloc.h"
#include "lrumc.h"
#include "config.h"
#include "lrumc.h"

#define BUFFER_SIZE 4096
#define KEY_SIZE 50
#define VALUE_SIZE 255

extern int g_thread_num;//线程数量
extern int g_port;//监听端口
extern int g_segment_num;//segment的数量
extern int g_bucket_full_rate;//哈希桶的稀疏度
extern segment_config_t *g_lrumc_config;//配置项
extern lrumc_t *g_lrumc;


static void reload_config_item(char *key,char *value);

/**
 * @desc 配置文件解析函数(修改全局变量)
 * @param char *filename 配置文件
 * @return void
 */
void init_config(char *filename,void (*set_config_item)(char *key,char *value))
{
	assert(filename);
	FILE *fp = NULL;
	if(NULL == (fp = fopen(filename,"r")))
	{
		printf("bad conf file %s\n",filename);	
		exit(0);
	}		
	char buffer[BUFFER_SIZE],key[KEY_SIZE],value[VALUE_SIZE];
	int line = 1;
	while(1)
	{
		if(feof(fp)) break;

		memset(buffer,0,BUFFER_SIZE);
		memset(key,0,KEY_SIZE);
		memset(value,0,VALUE_SIZE);

		fgets(buffer,BUFFER_SIZE,fp);
		char *p1 = buffer;
		while(isspace(*p1)) p1++;
		char *p2 = p1;
		while(p2 && (isalnum(*p2) || '_' == *p2)) p2++;
		strncpy(key,p1,p2-p1);

		char *p3 = strstr(buffer,"=");
		if(NULL == p3)
		{
			line++;
			continue;
		}
		p3++;
		while(p3 && isspace(*p3)) p3++;
		char comma[1];
		memset(comma,0,1);
		int have_comma = false;
		if('"' == *p3 || '\'' == *p3)
		{
			comma[0] = *p3;
			have_comma = true;	
			p3++;
		}

		if(have_comma)
		{
			bool ok = false;
			char c;
			int offset = 0;
			bool have_backslash = false;
			while('\0' != (c = *p3++))
			{
				if(c == comma[0])
				{
					if(have_backslash) 	
					{
						value[offset] = c;	
						have_backslash = false;
						offset++;
					}
					else
					{
						ok = true;
						break;	
					}
				}
				else if('\\' == c)
				{
					if(have_backslash)
					{
						value[offset] = '\\';
						have_backslash = false;
						offset++;	
					}
					else have_backslash = true;	
				}
				else
				{
					value[offset] = c;
					offset++;	
				}
			}
			if(!ok)
			{
				printf("bad config in line %d\n",line);
				exit(0);
			}
		}
		else
		{
			char *p4 = p3;
			while(p4 && isdigit(*p4)) p4++;		
			strncpy(value,p3,p4-p3);
		}
		line++;
		(*set_config_item)(key,value);
	}
	fclose(fp);
}

buff_t *reload_config(char *filename)
{
	//TODO 使配置重新生效	
	init_config(filename,&reload_config_item);
	update_lrumc(g_lrumc,g_lrumc_config,g_segment_num);
	return NULL;
}


void set_config_item(char *key,char *value)
{
	if(0 == strcmp(key,"port")) g_port = atoi(value);	
	else if(0 == strcmp(key,"thread")) g_thread_num = atoi(value);
	else if(0 == strcmp(key,"segment"))
	{
		char *p = value;
		char *p1 = value;
		int num = 0;
		while(NULL != p)
		{
			p = strstr(p,",");	
			num++;
			if(NULL == p) break;
			p++;
		}
		g_segment_num = ceil(num/2);
		DCALLOC(g_lrumc_config,segment_config_t *,g_segment_num,sizeof(segment_config_t));
		int i = 0,j = 0,k = 0;
		char *old_p = NULL;
		char tmp[20];
		while(1)
		{
			memset(tmp,0,20);
			old_p = p1;
			p1++;
			p1 = strstr(p1,",");	
			if(NULL == p1) strcpy(tmp,old_p);	
			else
			{
				p1++;
				strncpy(tmp,old_p,p1 - old_p);	
			}
			k = i%2;
			j = i/2;	
			if(0 == k) g_lrumc_config[j].max_size = atoi(tmp);
			else g_lrumc_config[j].buff_size = atoi(tmp);	
			if(NULL == p1) break;
			i++;
		}
	}
	else if(0 == strcmp(key,"bucket_full_rate")) g_bucket_full_rate = atoi(value);	
}

void reload_config_item(char *key,char *value)
{
	if(0 == strcmp(key,"segment"))
	{
		DFREE(g_lrumc_config);
		char *p = value;
		char *p1 = value;
		int num = 0;
		while(NULL != p)
		{
			p = strstr(p,",");	
			num++;
			if(NULL == p) break;
			p++;
		}
		g_segment_num = ceil(num/2);
		DCALLOC(g_lrumc_config,segment_config_t *,g_segment_num,sizeof(segment_config_t));
		int i = 0,j = 0,k = 0;
		char *old_p = NULL;
		char tmp[20];
		while(1)
		{
			memset(tmp,0,20);
			old_p = p1;
			p1++;
			p1 = strstr(p1,",");	
			if(NULL == p1) strcpy(tmp,old_p);	
			else
			{
				p1++;
				strncpy(tmp,old_p,p1 - old_p);	
			}
			k = i%2;
			j = i/2;	
			if(0 == k) g_lrumc_config[j].max_size = atoi(tmp);
			else g_lrumc_config[j].buff_size = atoi(tmp);	
			if(NULL == p1) break;
			i++;
		}
	}
}
