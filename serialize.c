#include <assert.h>
#include <ctype.h>
#include "dalloc.h"
#include "pary.h"
#include "mc_protocol.h"

typedef struct serialize_ret_s serialize_ret_t;

//序列化函数的返回值
struct serialize_ret_s
{
	pary_type_t type; //类型
	buff_t *buff;//数据
	int item_num;//有多少个元素，数组
};

//序列化一个节点
static serialize_ret_t *serialize_node(const pary_node_t *node);
//序列化一个一个对象
static serialize_ret_t *serialize_object(const pary_object_t *object);

/**
 * @desc 将序列化过的数据反序列化
 * @param pary_node_t *node 最后返回节点
 * @param char *str 序列化过的字符串
 * @return char *
 */
char *unserialize(pary_node_t *node,char *str)
{
	assert(node && str);
	char *curr_pos = str;
	bool find_begin_mark = true;
	bool find_key = false;
	bool find_value = false;
	char *key = NULL;
	char *value = NULL;
	while(NULL != curr_pos || '\0' != *curr_pos)
	{
		if(true == find_begin_mark)
		{
			curr_pos = strstr(curr_pos,"{");	
			if(NULL == curr_pos) return false;
			curr_pos++;
			find_key = true;
			find_begin_mark = false;
			find_value = false;
		}
		else if(true == find_key)
		{
			char type;
			GET_STR(&type,1,curr_pos,":",1);			

			if('s' == type)
			{
				//字符串的情况
				int len = 0;
				GET_INT(len,curr_pos,":",1);
				curr_pos++;
				DMALLOC(key,char *,len+1);
				memset(key,0,len+1);
				strncpy(key,curr_pos,len);
				key[len] = '\0';
				curr_pos += len + 2;
			}
			else if('i' == type )
			{
				//整数的情况
				//整数最大长度19
				DMALLOC(key,char *,20);
				memset(key,0,20);
				int i = 0;
				GET_INT(i,curr_pos,";",1);
				sprintf(key,"%d",i);	
			}
			else if('}' == type)//空数组
			{
				curr_pos++;
				return curr_pos;
			}
			else 
			{
				printf("unserialize bad type ==%c==\n",type);	
				return NULL;
			}
			find_key = false;
			find_begin_mark = false;
			find_value = true;
		}
		else if(true == find_value)
		{
			//找值
			char type;
			GET_STR(&type,1,curr_pos,":",1);			

			if('s' == type)
			{
				//字符串的情况
				int len = 0;
				GET_INT(len,curr_pos,":",1);
				curr_pos++;
				DMALLOC(value,char *,len+1);
				memset(value,0,len+1);
				strncpy(value,curr_pos,len);
				value[len] = '\0';
				buff_t *buff = create_buff(len);
				append_buff(buff,value,len);
				DFREE(value);
				pary_node_t *tmp_node = create_pary_node_buff(buff);
				set_pary_node(node,key,tmp_node,true);
				curr_pos += len + 1;
				if('}' == *curr_pos)
				{
					curr_pos++;
					return curr_pos;	
				}
				curr_pos++;

				find_key = true;
				find_begin_mark = false;
				find_value = false;
			}
			else if('i' == type)
			{
				//整形的情况
				int i = 0;	
				GET_INT(i,curr_pos,";",1);
				pary_node_t *tmp_node = create_pary_node_int(i);
				set_pary_node(node,key,tmp_node,true);

				find_key = true;
				find_begin_mark = false;
				find_value = false;
			}
			else if('b' == type)
			{
				//整形的情况
				int i = 0;	
				GET_INT(i,curr_pos,";",1);
				pary_node_t *tmp_node = create_pary_node_bool(i);
				set_pary_node(node,key,tmp_node,true);

				find_key = true;
				find_begin_mark = false;
				find_value = false;
			}
			else if('d' == type)
			{
				//double的情况
				double d = 0;		
				GET_DOUBLE(d,curr_pos,";",1);
				pary_node_t *tmp_node = create_pary_node_double(d);
				set_pary_node(node,key,tmp_node,true);

				find_key = true;
				find_begin_mark = false;
				find_value = false;
			}
			else if('a' == type)
			{
				//数组的情况
				int item_num = 0;
				GET_INT(item_num,curr_pos,":",1);
				pary_node_t *tmp_node = create_pary_node_object();
				set_pary_node(node,key,tmp_node,true);
				if(item_num > 0)
				{
					curr_pos = unserialize(tmp_node,curr_pos);	
				}
				else
				{
					curr_pos += 2; //跳过{}这样的情况		
				}
				if(NULL == curr_pos) return NULL;
				find_key = true;
				find_begin_mark = false;
				find_value = false;
			}
			if(NULL == curr_pos)
			{
				return NULL;
			}	
			if(0 == strlen(curr_pos))
			{
				return NULL;
			}
			if('}' == *curr_pos)
			{
				curr_pos++;
				return curr_pos;	
			}		
		}
	}
	return NULL;
}


/**
 * @desc 序列化一个节点
 * @param pary_node_t *node 节点
 * @return buff_t *
 */
buff_t *serialize(const pary_node_t *node)
{
	serialize_ret_t *ret = serialize_node(node);	
	buff_t *buff = ret->buff;
	DFREE(ret);
	return buff;
}

/**
 * @desc 序列化一个节点
 * @param pary_node_t *node 节点
 * @return serialize_ret_t *
 */
serialize_ret_t *serialize_node(const pary_node_t *node)
{
	assert(node);	
	serialize_ret_t *ret = NULL,*ret_item = NULL;	
	DMALLOC(ret,serialize_ret_t *,sizeof(serialize_ret_t));
	if(NULL == ret) return NULL;

	char tmp_value[56];
	memset(tmp_value,0,56);
	int value_buff_len = 0;

	buff_t *buff = create_buff(1024);
	switch(node->type)
	{
		case PARY_TYPE_INT:
			value_buff_len = sprintf(tmp_value,"i:%d;",node->data.data_int);
			append_buff(buff,tmp_value,value_buff_len);
			ret->type = PARY_TYPE_INT;
		break;	
		case PARY_TYPE_BOOL:
			value_buff_len = sprintf(tmp_value,"b:%d;",node->data.data_int);
			append_buff(buff,tmp_value,value_buff_len);
			ret->type = PARY_TYPE_INT;
		break;	
		case PARY_TYPE_DOUBLE:
			value_buff_len = sprintf(tmp_value,"d:%lf;",node->data.data_double);
			append_buff(buff,tmp_value,value_buff_len);
			ret->type = PARY_TYPE_DOUBLE;
		break;	
		case PARY_TYPE_BUFF:
			value_buff_len = sprintf(tmp_value,"s:%d:",node->data.data_buff->length);
			append_buff(buff,tmp_value,value_buff_len);
			append_buff(buff,"\"",1);
			append_buff(buff,node->data.data_buff->data,node->data.data_buff->length);
			append_buff(buff,"\";",2);
			ret->type = PARY_TYPE_BUFF;
		break;	
		case PARY_TYPE_OBJECT:
			ret_item = serialize_object(node->data.data_object);
			value_buff_len = sprintf(tmp_value,"a:%d:",ret_item->item_num);
			append_buff(buff,tmp_value,value_buff_len);
			append_buff(buff,ret_item->buff->data,ret_item->buff->length);
			destroy_buff(ret_item->buff);
			DFREE(ret_item);
			ret->type = PARY_TYPE_OBJECT;
		break;
		default:
			printf("serialize bad type");
		break;
	}
	ret->buff = buff;
	return ret;
}

/**
 * @desc 序列化一个对象
 * @param const pary_object_t *object 对象
 * @return serialize_ret_t *
 */
serialize_ret_t *serialize_object(const pary_object_t *object)
{
	serialize_ret_t *ret = NULL;	
	DMALLOC(ret,serialize_ret_t *,sizeof(serialize_ret_t));
	if(NULL == ret) return NULL;
	buff_t *buff = create_buff(1024);
	append_buff(buff,"{",1);
	pary_list_t *item = NULL,*list = object->list;	
	item = list->pre->next;
	char key[255];
	int item_num = 0,key_len = 0,key_buff_len = 0;
	char tmp_key[300];
	char tmp_value[56];
	char *p = NULL;
	serialize_ret_t *ret_item = NULL;
	while(item != list->next)
	{
		bool is_numeric = true;
		memset(key,0,255);
		memset(tmp_key,0,300);
		memset(tmp_value,0,56);
		strcpy(key,item->rbnode->key);
		p = item->rbnode->key;
		char c;
		while('\0' != (c = *p++))
		{
			if(!isdigit(c)) is_numeric = false;	
		}
		key_len = strlen(key);
		if(is_numeric)
		{
			//数字类型
			key_buff_len = sprintf(tmp_key,"i:%ld;",strtol(key,NULL,10));	
		}
		else
		{
			//字符串类型
			key_buff_len = sprintf(tmp_key,"s:%d:\"%s\";",key_len,key);	
		}
		append_buff(buff,tmp_key,key_buff_len);
		ret_item = serialize_node(item->rbnode->value);	
		append_buff(buff,ret_item->buff->data,ret_item->buff->length);
		destroy_buff(ret_item->buff);
		DFREE(ret_item);
		ret_item = NULL;
		item = item->next;
		item_num++;
	}
	append_buff(buff,"}",1);
	if(NULL != ret_item)
	{
		destroy_buff(ret_item->buff);
		DFREE(ret_item);
	}
	ret->item_num = item_num;
	ret->type = PARY_TYPE_OBJECT;
	ret->buff = buff;
	return ret;
}

/**
 * @desc 判断一个字符串是否是序列化过的
 * @param void *str 数据
 * @param int len str的长度
 * @return bool
 */
bool is_serialized(void *str,int len)
{
	char a = 'a';
	char b = ':';
	char c = '}';
	if(
		0 == memcmp(&a,str,sizeof(char)) && 
		0 == memcmp(&b,str+1,sizeof(char)) && 
		0 == memcmp(&c,str-1+len,sizeof(char))
	) return true;
	else return false;
}




