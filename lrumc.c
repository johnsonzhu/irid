#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include "dalloc.h"
#include "lrumc.h"
#include "assignment.h"

//static var
static unsigned long crypt_table[0x500];//(暴雪)

//static function
static segment_t *create_segment(int max_size,int buff_size);//创建一个segment
static segment_item_t *create_segment_item(int offset);//创建一个segment_item节点
static segment_item_t *create_segment_item_list(int offset);//创建一个segment列表
static int discard_segment(lrumc_t *lrumc,segment_t *segment);//丢弃节点
static void insert_segment(lrumc_t *lrumc,int offset,segment_t *segment);//插入segment
static segment_t *move_segment(lrumc_t *lrumc,int offset,segment_t **src_segment);//移动节点
static void destroy_segment_item(segment_item_t *item);//删除segment_item节点
static segment_t *create_one_segment(segment_item_t *item,int num);//创建一个segment
static void increment_weight(lrumc_t *lrumc,int offset);//增加权重
static void delete_segment_item(segment_item_t *item);//删除segment_item
static unsigned long hash_code(char *str, unsigned long hash_type);//计算hash_code(暴雪) 这样的数据碰撞的情况很严重
static void fix_have_used(lrumc_t *lrumc,int pos);//修正哈希桶的位置
static int find_bucket_pos(lrumc_t *lrumc,char *key);//根据key找出对应的bucket pos (get count push unshift pop shift)
static bool found_bucket_pos(lrumc_t *lrumc,char *key,int *pos); //根据key找出对应的bucket pos (delete append prepend replace incrdment decrement)
static bool _append_prepend_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,bool append);
static long long _increment_decrement_lrumc_data(lrumc_t *lrumc,char *key,char *path,int num,bool increment);
static buff_t *_pop_shift_lrumc_data(lrumc_t *lrumc,char *key,char *path,bool pop);
static bool _push_unshift_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type,bool push);

/**
 * @desc 创建总句柄
 * @param segment_config_t *config 配置
 * @param int num 段的数量
 * @return lrumc_t *
 */
lrumc_t *init_lrumc(segment_config_t *config,int num)
{
	assert(config && num >0);
	int i = 0,total = 0;

	lrumc_t *lrumc = NULL;
	DMALLOC(lrumc,lrumc_t *,sizeof(lrumc_t));
	if(NULL == lrumc) return NULL;
	memset(lrumc,0,sizeof(lrumc_t));
	DCALLOC(lrumc->segment,segment_t **,num,sizeof(segment_t *));
	lrumc->segment_num = num;
	if(NULL == lrumc->segment) return NULL;
	for(;i<num;i++)
	{
		lrumc->segment[i] = create_segment(config[i].max_size,config[i].buff_size);
		if(NULL == lrumc->segment+i) return NULL;
		total += config[i].max_size + config[i].buff_size;	
	}
	lrumc->total = total;
	lrumc->curr_num = 0;
	lrumc->bucket_num = ceil(total * 100 /HASH_FULL_RATE);  
	DCALLOC(lrumc->bucket,bucket_t *,lrumc->bucket_num,sizeof(bucket_t));
	if(NULL == lrumc->bucket) return NULL;
	for(i=0;i<lrumc->bucket_num;i++) lrumc->bucket[i].status = BUCKET_STATUS_NULL;	
	lrumc->start_time = time(NULL);
	lrumc->get_hits = 0;
	lrumc->get_misses = 0;
	lrumc->delete_hits = 0;
	lrumc->delete_misses = 0;
	lrumc->incr_hits = 0;
	lrumc->incr_misses = 0;
	lrumc->decr_hits = 0;
	lrumc->decr_misses = 0;
	lrumc->bytes_read = 0;
	lrumc->bytes_written = 0;
	lrumc->cmd_set = 0;
	lrumc->cmd_flush = 0;
	lrumc->evictions = 0;
	return lrumc;	
}

/**
 * @desc 更新lrumc
 * @param segment_config_t *config 配置
 * @param int num 段的数量
 * @return lrumc_t *
 */
lrumc_t *update_lrumc(lrumc_t *lrumc,segment_config_t *config,int num)
{
	int i = 0;
	int total = 0;
	segment_t *segment = NULL;
	if(num > lrumc->segment_num)
	{
		//增加段
		DREALLOC(lrumc->segment,segment_t **,num*sizeof(segment_t *));
		for(i=lrumc->segment_num;i < num;i++)
		{
			lrumc->segment[i] = create_segment(config[i].max_size,config[i].buff_size);
			if(NULL == lrumc->segment+i) return NULL;
		}
		lrumc->segment_num = num;
	}	
	else if(num == lrumc->segment_num)
	{
	}
	else
	{
		//减少段
		for(i=num;i<lrumc->segment_num;i++)
		{
			segment = lrumc->segment[i];	
			if(segment->item_list->pre->next !=  segment->item_list->next)
			{
				segment_item_t *old_item = NULL,*tmp_item = segment->item_list->pre->next;
				while(tmp_item != segment->item_list->next)
				{
					old_item = tmp_item;
					lrumc->bucket[old_item->offset].status = BUCKET_STATUS_HAD_USED;//更新bucket的状态
					DFREE(lrumc->bucket[old_item->offset].key);
					destroy_pary_node(lrumc->bucket[old_item->offset].value);
					lrumc->bucket[old_item->offset].hash_code_b = 0;
					lrumc->bucket[old_item->offset].hash_code_c = 0;
					tmp_item = tmp_item->next;	
					destroy_segment_item(old_item);
				}
			}
			DFREE(segment->item_list->pre);//这里才是真的要删除pre 和 next
			DFREE(segment->item_list->next);
			DFREE(segment->item_list);
			DFREE(segment);
		}		
		lrumc->segment_num = num;
	}

	for(i=0;i<num;i++)
	{
		lrumc->segment[i]->max_size = config[i].max_size;
		lrumc->segment[i]->buff_size = config[i].buff_size;
		total += lrumc->segment[i]->max_size + lrumc->segment[i]->buff_size;
	}

	int new_bucket_num = ceil(total *100 / HASH_FULL_RATE);
	bucket_t *new_bucket = NULL;
	DMALLOC(new_bucket,bucket_t *,new_bucket_num*sizeof(bucket_t));
	if(NULL == new_bucket)
	{
		printf("create new_bucket fail\n");	
	}
	memset(new_bucket,0,new_bucket_num*sizeof(bucket_t));
	for(i=0;i<new_bucket_num;i++)
	{
		new_bucket[i].status = BUCKET_STATUS_NULL;	
		new_bucket[i].hash_code_b = 0;	
		new_bucket[i].hash_code_c = 0;	
	}

	int item_total = 0;
	lrumc->bucket_num = new_bucket_num;
	for(i=0;i<num;i++)
	{
		segment = lrumc->segment[i];	
		if(segment->item_list->pre->next !=  segment->item_list->next)
		{
			segment_item_t *old_item = NULL,*tmp_item = segment->item_list->pre->next;
			while(tmp_item != segment->item_list->next)
			{
				old_item = tmp_item;
				int offset = old_item->offset;
				char *key = lrumc->bucket[offset].key;
				void *value = lrumc->bucket[offset].value;
				ulong hash_code_b = lrumc->bucket[offset].hash_code_b;
				ulong hash_code_c = lrumc->bucket[offset].hash_code_c;
				ulong hash_code_a = hash_code(key,0);
				int new_pos = hash_code_a % new_bucket_num;
				bool full = false;
				bool err = false;
				int old_pos = new_pos;
				while(BUCKET_STATUS_USED == new_bucket[new_pos].status)
				{
					if(hash_code_b == new_bucket[new_pos].hash_code_b && hash_code_c == new_bucket[new_pos].hash_code_c)
					{
						//找到自己了
						err = true;
						break;
					}
					new_pos++;		
					if(new_pos >= new_bucket_num) new_pos = 0;//回环	
					if(new_pos == old_pos)
					{
						//缩小装满了
						full = true;
						err = true;
						break;//不应该存在的这样的情况的
					}
				}
				if(false == err)
				{
					new_bucket[new_pos].status = BUCKET_STATUS_USED;
					new_bucket[new_pos].key = key;
					new_bucket[new_pos].list_pos = (ulong)old_item;
					new_bucket[new_pos].value = value;
					new_bucket[new_pos].value_len = lrumc->bucket[offset].value_len;
					new_bucket[new_pos].hash_code_b = hash_code_b;
					new_bucket[new_pos].hash_code_c = hash_code_c;
					new_bucket[new_pos].segment_pos = i;

					new_bucket[new_pos].bucket_offset = new_pos - old_pos;
					old_item->offset = new_pos;
					item_total++;
				}


				if(true == full || true == err)
				{
					old_item->pre->next = old_item->next;
					old_item->next->pre = old_item->pre;			
					segment->curr_size--;
					DFREE(key);
					destroy_pary_node(value);
				}

				tmp_item = tmp_item->next;	
				if(true == full || true == err)
				{
					destroy_segment_item(old_item);
				}
			}
		}
	}
	DFREE(lrumc->bucket);
	lrumc->bucket = new_bucket;
	lrumc->total = item_total;
	return NULL;
}

/**
 * @desc 删除lrumc总句柄
 * @param lrumc_t *lrumc 总句柄
 * @return void
 */
void destroy_lrumc(lrumc_t *lrumc)
{
	assert(lrumc);
	int i = 0;
	for(;i<lrumc->bucket_num;i++)
	{
		if(BUCKET_STATUS_USED != lrumc->bucket[i].status) continue;
		segment_item_t *item = (segment_item_t *)lrumc->bucket[i].list_pos;	
		destroy_segment_item(item);
		DFREE(lrumc->bucket[i].key);
		destroy_pary_node(lrumc->bucket[i].value);
	}
	for(i=0;i<lrumc->segment_num;i++)
	{
		DFREE(lrumc->segment[i]->item_list->pre);	
		DFREE(lrumc->segment[i]->item_list->next);	
		DFREE(lrumc->segment[i]->item_list);	
		DFREE(lrumc->segment[i]);
	}	
	DFREE(lrumc->segment);
	DFREE(lrumc->bucket);
	DFREE(lrumc);
}

/**
 * @desc 获取数据
 * @param lrumc_t *lrumc 总句柄
 * @param char *key key
 * @return void *
 */
get_ret_t *get_lrumc_data(lrumc_t *lrumc,char *key,char *path)
{
	assert(key);	
	int pos = -1;
	if(-1 == (pos = find_bucket_pos(lrumc,key)))
	{
		lrumc->get_misses++;
		return NULL;
	}

	increment_weight(lrumc,pos);	
	int length = 0;
	get_ret_t *get_ret = NULL;
	DMALLOC(get_ret,get_ret_t *,sizeof(get_ret_t));
	memset(get_ret,0,sizeof(get_ret_t));
	get_ret->length = 0;
	if(NULL == get_ret) return NULL;

	pary_node_t *ret_node = NULL;
	if(strlen(path) > 0) ret_node = get_pary_node(lrumc->bucket[pos].value,path);
	else ret_node = (pary_node_t *)lrumc->bucket[pos].value;	
	
	if(NULL != ret_node)
	{
		buff_t *buff = NULL;
		void *ret_str = NULL;
		get_ret->type = ret_node->type;
		switch(ret_node->type)
		{
			case PARY_TYPE_OBJECT:
				buff = serialize(ret_node);		
				get_ret->data = buff->data;
				get_ret->length = buff->length;
				DFREE(buff);
			break;
			case PARY_TYPE_BUFF:
				length = ret_node->data.data_buff->length;
				DMALLOC(ret_str,void *,length);
				memcpy(ret_str,ret_node->data.data_buff->data,length);
				get_ret->data = ret_str;
				get_ret->length = length;
			break;
			case PARY_TYPE_BOOL:
			case PARY_TYPE_INT:
				//length = sizeof(int);
				DMALLOC(ret_str,char *,20);
				length = sprintf(ret_str,"%d",ret_node->data.data_int);
				//memcpy(ret_str,&(ret_node->data.data_int),length);
				get_ret->data = ret_str;
				get_ret->length = length;
			break;
			case PARY_TYPE_DOUBLE:
				length = sizeof(double);
				DMALLOC(ret_str,void *,length);
				memcpy(ret_str,&(ret_node->data.data_double),length);
				get_ret->data = ret_str;
				get_ret->length = length;
			break;
			default:
				printf("bad type 22222\n");
				DFREE(get_ret);
				return NULL;
			break;
		}
	}
	lrumc->get_hits++;
	return get_ret;
}

/**
 * @desc 新增数据(key已经存在就会添加失败)
 * @param lrumc_t *lrumc 总句柄
 * @param char *key key
 * @param char *value 新的值
 * @return bool
 */
bool add_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type)
{
	assert(key && value);	
	ulong hash_code_a = hash_code(key,0);
	ulong hash_code_b = hash_code(key,1);
	ulong hash_code_c = hash_code(key,2);
	int pos = hash_code_a % lrumc->bucket_num;
	int old_pos = pos;
	while(BUCKET_STATUS_USED == lrumc->bucket[pos].status)
	{
		if(hash_code_b == lrumc->bucket[pos].hash_code_b && hash_code_c == lrumc->bucket[pos].hash_code_c)
		{
			DFREE(key);
			return false;//已经存在
		}
		pos++;		
		if(pos >= lrumc->bucket_num) pos = 0;//回环	
		if(pos == old_pos)
		{
			DFREE(key);
			return false;//不应该存在的这样的情况的
		}
	}
	lrumc->bucket[pos].status = BUCKET_STATUS_USED;

	update_pary_node(lrumc,pos,path,value,value_len,value_type,false);

	lrumc->bucket[pos].value_len = value_len;
	lrumc->bucket[pos].key = key;
	lrumc->bucket[pos].hash_code_b = hash_code_b;
	lrumc->bucket[pos].hash_code_c = hash_code_c;
	lrumc->bucket[pos].bucket_offset = old_pos - pos;//有可能是负数
	segment_item_t *item = create_segment_item(pos);
	segment_t *segment = create_one_segment(item,1);
	lrumc->bucket[pos].list_pos = (ulong)item; //队列的前端
	lrumc->bucket[pos].segment_pos = lrumc->segment_num - 1;//一定是在最后的那个	
	insert_segment(lrumc,lrumc->segment_num-1,segment);
	discard_segment(lrumc,segment);
	lrumc->bytes_written += value_len;
	return true;
}

/**
 * @desc 更新数据(强制)
 * @param lrumc_t *lrumc 总句柄
 * @param char *key key
 * @param char *value 新的值
 * @return bool
 */
bool set_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type)
{
	assert(key);	
	ulong hash_code_a = hash_code(key,0);
	ulong hash_code_b = hash_code(key,1);
	ulong hash_code_c = hash_code(key,2);
	int pos = hash_code_a % lrumc->bucket_num;
	int old_pos = pos;
	bool found = false,find_have_used = false;
	while(BUCKET_STATUS_NULL != lrumc->bucket[pos].status)
	{
		if(
			hash_code_b == lrumc->bucket[pos].hash_code_b && 
			hash_code_c == lrumc->bucket[pos].hash_code_c &&
			BUCKET_STATUS_HAD_USED != lrumc->bucket[pos].status)
		{
			found = true;
			break;
		}
		pos++;		
		if(pos >= lrumc->bucket_num) pos = 0;	
		if(pos == old_pos)
		{
			find_have_used = true;	
		}
		if(find_have_used && BUCKET_STATUS_HAD_USED == lrumc->bucket[pos].status)
		{
			break;	
		}
	}

	update_pary_node(lrumc,pos,path,value,value_len,value_type,found);
	if(true == found)
	{
		DFREE(key);
		//lrumc->bucket[pos].key = key;
		lrumc->bucket[pos].value_len = value_len;
		increment_weight(lrumc,pos);//增加权重
	}
	else //if(true == found)
	{
		//不存在的
		lrumc->bucket[pos].status = BUCKET_STATUS_USED;
		lrumc->bucket[pos].hash_code_b = hash_code_b;
		lrumc->bucket[pos].hash_code_c = hash_code_c;

		lrumc->bucket[pos].value_len = value_len;
		lrumc->bucket[pos].key = key;
		lrumc->bucket[pos].bucket_offset = old_pos - pos;//有可能是负数

		segment_item_t *item = create_segment_item(pos);
		segment_t *segment = create_one_segment(item,1);
		lrumc->bucket[pos].list_pos = (ulong)segment->item_list->pre->next;
		insert_segment(lrumc,lrumc->segment_num-1,segment);
		discard_segment(lrumc,segment);
	}
	lrumc->cmd_set++;
	lrumc->bytes_written += value_len;
	return true;
}

/**
 * @desc 更新数据(key不存在，就会更新失败)
 * @param lrumc_t *lrumc 总句柄
 * @param char *key key
 * @param char *value 新的值
 * @return bool
 */
bool replace_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type)
{
	assert(lrumc && key && value);	
	int pos = -1;
	bool found = found_bucket_pos(lrumc,key,&pos);

	if(true == found)
	{
		update_pary_node(lrumc,pos,path,value,value_len,value_type,found);
		lrumc->bucket[pos].value_len = value_len;
		increment_weight(lrumc,pos);//增加权重
		lrumc->bytes_written += value_len;
	}
	DFREE(key);	
	return found;
}

/**
 * @desc 后面追加内容
 * @param lrumc_t *lrumc 总句柄
 * @param char *key 键值
 * @param char *path 
 * @param void *value 值
 * @param long value_len 值的长度
 * @return bool
 */
bool append_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len)
{
	return _append_prepend_lrumc_data(lrumc,key,path,value,value_len,true);
}

/**
 * @desc 前面追加内容
 * @param lrumc_t *lrumc 总句柄
 * @param char *key 键值
 * @param char *path 
 * @param void *value 值
 * @param long value_len 值的长度
 * @return bool
 */
bool prepend_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len)
{
	return _append_prepend_lrumc_data(lrumc,key,path,value,value_len,false);
}

/**
 * @desc 追加内容
 * @param lrumc_t *lrumc 总句柄
 * @param char *key 键值
 * @param char *path 
 * @param void *value 值
 * @param long value_len 值的长度
 * @return bool
 */
bool _append_prepend_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,bool append)
{
	assert(lrumc && key && value);	
	int pos = -1;
	bool found = found_bucket_pos(lrumc,key,&pos);
	if(true == found)
	{
		pary_node_t *node = NULL;
		if(0 != strlen(path)) node = get_pary_node(lrumc->bucket[pos].value,path);	
		else node = lrumc->bucket[pos].value;	

		if(NULL != node)
		{
			//要是字符类型才能prepend
			if(PARY_TYPE_BUFF != node->type) return false;
			if(true == append) append_buff(node->data.data_buff,value,value_len);
			else prepend_buff(node->data.data_buff,value,value_len);
			increment_weight(lrumc,pos);//增加权重
		}
		else found = false;
		lrumc->bytes_written += value_len;
	}
	DFREE(key);
	return found;
}

/**
 * @desc 删除缓存中的数据
 * @param lrumc_t *lrumc 总句柄
 * @param char *key key
 * @param char *path path
 * @return bool
 */
bool delete_lrumc_data(lrumc_t *lrumc,char *key,char *path)
{
	assert(key);	
	int pos = -1;
	bool found = found_bucket_pos(lrumc,key,&pos);
	if(true == found)
	{
		if(0 == strlen(path))
		{
			//释放值
			DFREE(lrumc->bucket[pos].key);//下面不能有这个代码,只是释放孩子不是释放自己
			destroy_pary_node(lrumc->bucket[pos].value);
			int segment_pos = lrumc->bucket[pos].segment_pos;
			delete_segment_item((segment_item_t *)lrumc->bucket[pos].list_pos);//删除一个节点
			lrumc->segment[segment_pos]->curr_size--;//数据减少一
			lrumc->curr_num--;
			lrumc->bucket[pos].status = BUCKET_STATUS_HAD_USED;
			fix_have_used(lrumc,pos);//不能在fix_have_used 后面有任何pos的相关操作
		}
		else
		{
			found = delete_pary_node(lrumc->bucket[pos].value,path);
		}
		lrumc->delete_hits++;
	}
	else lrumc->delete_misses++;
	return found;
}

/**
 * @desc 自增长
 * @param lrumc_t *lrumc 总句柄
 * @param char *key 键值
 * @param char *path 
 * @param int num 数量
 * @return long long 当前值
 */
long long increment_lrumc_data(lrumc_t *lrumc,char *key,char *path,int num)
{
	return _increment_decrement_lrumc_data(lrumc,key,path,num,true);
}

/**
 * @desc 自递减
 * @param lrumc_t *lrumc 总句柄
 * @param char *key 键值
 * @param char *path 
 * @param int num 数量
 * @return long long 当前值
 */
long long decrement_lrumc_data(lrumc_t *lrumc,char *key,char *path,int num)
{
	return _increment_decrement_lrumc_data(lrumc,key,path,num,false);
}

/**
 * @desc 自增减
 * @param lrumc_t *lrumc 总句柄
 * @param char *key 键值
 * @param char *path 
 * @param int num 数量
 * @return long long 当前值
 */
long long _increment_decrement_lrumc_data(lrumc_t *lrumc,char *key,char *path,int num,bool increment)
{
	assert(lrumc && key);	
	int pos = -1;
	bool found = found_bucket_pos(lrumc,key,&pos);
	long long lld = L_L_MAX;
	if(true == found)
	{
		pary_node_t *node = NULL;
		if(0 != strlen(path)) node = get_pary_node(lrumc->bucket[pos].value,path);	
		else node = lrumc->bucket[pos].value;	

		//要是数字类型才能decrment
		if(NULL != node)
		{
			if(true == increment)
			{
				lrumc->incr_hits++;
				switch(node->type)
				{
					case PARY_TYPE_INT:
						node->data.data_int += num;
						lld = node->data.data_int;	
					break;
					case PARY_TYPE_DOUBLE:
						node->data.data_double += num;
						lld = node->data.data_double;	
					break;
					default:
					break;
				}
			}
			else
			{
				lrumc->decr_hits++;
				switch(node->type)
				{
					case PARY_TYPE_INT:
						node->data.data_int -= num;
						if(node->data.data_int < 0) node->data.data_int = 0;
						lld = node->data.data_int;	
					break;
					case PARY_TYPE_DOUBLE:
						node->data.data_double -= num;
						if(node->data.data_double < 0) node->data.data_double = 0;
						lld = node->data.data_double;	
					break;
					default:
					break;
				}
			}
		}
		increment_weight(lrumc,pos);//增加权重
	}
	else
	{
		if(true == increment) lrumc->incr_misses++;	
		else lrumc->decr_misses++;	
	}
	return lld;
}

/**
 * @desc 打印当前数据分布情况
 * @param lrumc_t *lrumc 总句柄
 * @return void
 */
void display_lrumc(lrumc_t *lrumc)
{
	int i = 0;
	printf("total=%d\n",lrumc->total);	
	printf("curr_num=%d\n",lrumc->curr_num);	
	printf("bucket_num=%d\n",lrumc->bucket_num);	
	printf("segment_num=%d\n",lrumc->segment_num);	
	for(;i<lrumc->segment_num;i++)
	{
		printf("segment %d max_size %d\n",i,lrumc->segment[i]->max_size);	
		printf("segment %d buff_size %d\n",i,lrumc->segment[i]->buff_size);	
		printf("segment %d curr_size %d\n",i,lrumc->segment[i]->curr_size);	
		segment_item_t *tmp_item,*item = (segment_item_t *)lrumc->segment[i]->item_list;
		tmp_item = item->pre->next;
		int j = 0;
		while(tmp_item != item->next)
		{
			int offset = tmp_item->offset;	
			char *key = (char *)lrumc->bucket[offset].key;
			char *value = (char *)lrumc->bucket[offset].value;
			printf("\t\titem %d key = %s offset %d weight %lld value = %s\n",j,key,offset,tmp_item->weight,value);
			tmp_item = tmp_item->next;	
			j++;
		}
		printf("\n");
		/*
		tmp_item = item->next->pre;
		j = 0;
		while(tmp_item != item->pre)
		{
			int offset = tmp_item->offset;	
			char *key = (char *)lrumc->bucket[offset].key;
			char *value = (char *)lrumc->bucket[offset].value;
			printf("\t\titem %d key = %s offset %d weight %lld value = %s\n",j,key,offset,tmp_item->weight,value);
			tmp_item = tmp_item->pre;	
			j++;
		}
		printf("\n");
		*/
	}
}

/**
 * @desc 创建一个段
 * @param int max_size 最大size
 * @param int buff_size buff_size
 * @return segment_t *
 */
segment_t *create_segment(int max_size,int buff_size)
{
	segment_t *segment = NULL;
	DMALLOC(segment,segment_t *,sizeof(segment_t));
	if(NULL == segment) return NULL;
	memset(segment,0,sizeof(segment_t));
	segment->max_size = max_size;
	segment->buff_size = buff_size;
	segment->curr_size = 0;
	segment->item_list = create_segment_item_list(-1);
	if(NULL == segment->item_list) return NULL;
	return segment;
}

/**
 * @desc 创建一个空节点
 * @param int offset 偏移值
 * @return segment_item_t *
 */
segment_item_t *create_segment_item(int offset)
{
	segment_item_t *item = NULL;
	DMALLOC(item,segment_item_t *,sizeof(segment_item_t));
	if(NULL == item) return NULL;
	memset(item,0,sizeof(segment_item_t));
	item->offset = offset;
	item->weight = L_L_MAX;
	item->pre = NULL;
	item->next = NULL;
	return item;
}

/**
 * @desc 创建一个segment_item_t链表（注意是链表）,这个函数是复合，应该拆
 * @param int offset 偏移值
 * @return segment_item_t *
 */
segment_item_t *create_segment_item_list(int offset)
{
	segment_item_t *item = NULL;
	DMALLOC(item,segment_item_t *,sizeof(segment_item_t));
	if(NULL == item) return NULL;
	memset(item,0,sizeof(segment_item_t));
	item->offset = -1;
	item->weight = 0;
	DMALLOC(item->pre,segment_item_t *,sizeof(segment_item_t));
	if(NULL == item->pre) return NULL;
	memset(item->pre,0,sizeof(segment_item_t));
	item->pre->offset = -2;
	item->pre->weight = -3;
	DMALLOC(item->next,segment_item_t *,sizeof(segment_item_t));
	if(NULL == item->next) return NULL;
	memset(item->next,0,sizeof(segment_item_t));
	item->next->offset = -4;
	item->next->weight = -5;

	if(-1 == offset)
	{
		//空链表
		item->pre->pre = NULL;
		item->pre->next = item->next;
		item->next->next = NULL;
		item->next->pre = item->pre;
	}
	else
	{
		//有一个元素的链表
		segment_item_t *data_item = create_segment_item(offset);
		item->pre->pre = NULL;
		item->pre->next = data_item;
		item->next->next = NULL;
		item->next->pre = data_item;
		data_item->pre = item->pre;
		data_item->next = item->next;
	}
	return item;
}

/**
 * @desc 删除多余的节点
 * @param lrumc_t *lrumc 总句柄
 * @param segment_t *segment 需要删除的句柄
 * @return int 删除的数量
 */
int discard_segment(lrumc_t *lrumc,segment_t *segment)
{
	assert(lrumc && segment);	
	int num = 0;
	if(segment->item_list->pre->next !=  segment->item_list->next)
	{
		segment_item_t *old_item = NULL,*tmp_item = segment->item_list->pre->next;
		while(tmp_item != segment->item_list->next)
		{
			num++;	
			old_item = tmp_item;
			lrumc->bucket[old_item->offset].status = BUCKET_STATUS_HAD_USED;//更新bucket的状态
			DFREE(lrumc->bucket[old_item->offset].key);
			destroy_pary_node(lrumc->bucket[old_item->offset].value);
			lrumc->bucket[old_item->offset].hash_code_b = 0;
			lrumc->bucket[old_item->offset].hash_code_c = 0;
			fix_have_used(lrumc,old_item->offset);
			tmp_item = tmp_item->next;	
			destroy_segment_item(old_item);
		}
	}
	DFREE(segment->item_list->pre);//这里才是真的要删除pre 和 next
	DFREE(segment->item_list->next);
	DFREE(segment->item_list);
	DFREE(segment);
	lrumc->evictions += num;
	return num;
}

/**
 * @desc 在段间移动节点
 * @param lrumc_t *lrumc 总句柄
 * @param int int offset 偏移
 * @param segment_t *segment 新增的段
 * @return void
 */
void insert_segment(lrumc_t *lrumc,int offset,segment_t *segment)
{
	assert(lrumc && segment && offset>=0 && offset<lrumc->segment_num);	
	while(offset < lrumc->segment_num && segment->item_list->pre->next != segment->item_list->next)
	{
		lrumc->curr_num += segment->curr_size;
		move_segment(lrumc,offset,&segment);	
		lrumc->curr_num -= segment->curr_size;
		offset++;
	}
}

/**
 * @desc 在段间移动节点
 * @param lrumc_t *lrumc 总句柄
 * @param int int offset 偏移
 * @param segment_t *src_segment 新增的段
 * @return void
 */
segment_t *move_segment(lrumc_t *lrumc,int offset,segment_t **src_segment)
{
	assert(src_segment);	
	segment_t *dst_segment = lrumc->segment[offset];
	int i = 0,weight = dst_segment->item_list->pre->next->weight;//获取当前最前端的权重
	dst_segment->curr_size += (*src_segment)->curr_size;//修改size

	//下面1 3 对应 2 4对应
	segment_item_t *dst_tmp_item = dst_segment->item_list->pre->next;
	segment_item_t *src_tmp_item = (*src_segment)->item_list->pre->next;

	/*
	// src 尾巴                                      dst 头
	src_segment->item_list->next->pre->next = dst_segment->item_list->pre->next;//对接尾巴
	// src 头                                    dst 头
	src_segment->item_list->pre->next->pre = dst_segment->item_list->pre;//对接头
	// dst 头                                   src 尾
	dst_segment->item_list->pre->next->pre = src_segment->item_list->next->pre;
	// dst 总头 
	dst_segment->item_list->pre->next = src_segment->item_list->pre->next;
	*/
	(*src_segment)->item_list->next->pre->next = dst_tmp_item;
	dst_tmp_item->pre = (*src_segment)->item_list->next->pre;
	dst_segment->item_list->pre->next = src_tmp_item;
	src_tmp_item->pre = dst_segment->item_list->pre;

	//TODO 这里要修正那个weight
	segment_item_t *item = dst_segment->item_list->pre->next;
	for(;i<(*src_segment)->curr_size;i++)
	{
		lrumc->bucket[item->offset].segment_pos = offset;//当前在第几段 	
		item->weight = weight + (*src_segment)->curr_size - i;	
		item = item->next;
	}
	//空列表
	(*src_segment)->item_list->pre->next = (*src_segment)->item_list->next;
	(*src_segment)->item_list->pre->pre = NULL;//这里不是多余的不知道
	(*src_segment)->item_list->next->pre = (*src_segment)->item_list->pre;	
	(*src_segment)->item_list->next->next = NULL;	
	(*src_segment)->curr_size = 0;
	int num = dst_segment->curr_size - dst_segment->max_size - dst_segment->buff_size;
	if(num>0)
	{
		//也就是还要进一步移动
		num += dst_segment->buff_size;	
		dst_segment->curr_size -= num;//修正大小
		(*src_segment)->max_size = num;
		(*src_segment)->buff_size = 0;
		(*src_segment)->curr_size = num;
		segment_item_t *tmp_item = dst_segment->item_list->next; 
		int i = 0;
		while(i<num)
		{
			tmp_item = tmp_item->pre;
			i++;		
		}
		(*src_segment)->item_list->pre->next = tmp_item;
		(*src_segment)->item_list->next->pre = dst_segment->item_list->next->pre;
		//修复队列
		tmp_item->pre->next = dst_segment->item_list->next;
		dst_segment->item_list->next->pre->next = (*src_segment)->item_list->next;
		dst_segment->item_list->next->pre = tmp_item->pre;
		tmp_item->pre = (*src_segment)->item_list->pre;
	}
	return *src_segment;
}

/**
 * @desc 删除一个节点（释放内存），不用维护链表结构，上层循环调用去维护
 * @param segment_item_t *item 节点
 * @return void
 */
void destroy_segment_item(segment_item_t *item)
{
	assert(item);	
	DFREE(item);
}

/**
 * @desc 创建一个segment
 * @param segment_item_t *item ltem_list的内容
 * @param int num 数量
 * @return segment_t *
 */
segment_t *create_one_segment(segment_item_t *item,int num)
{
	segment_t *segment = create_segment(1,0);		
	if(NULL == segment) return NULL;
	segment->item_list->pre->next = item;
	segment->item_list->next->pre = item;
	item->pre = segment->item_list->pre;
	item->next = segment->item_list->next;
	segment->curr_size = num;
	return segment;
}

/**
 * @desc 增加节点的权重
 * @param lrumc_t *lrumc 总句柄
 * @param int offset 偏移
 * return void
 */
void increment_weight(lrumc_t *lrumc,int offset)
{
	assert(lrumc && offset>=0 && offset < lrumc->bucket_num);	
	segment_item_t *tmp_item = NULL,*item_pre = NULL,*item_next = NULL,*item = (segment_item_t *)lrumc->bucket[offset].list_pos;
	item->weight++;//权重加1
	tmp_item = item->pre;
	int segment_pos = lrumc->bucket[offset].segment_pos;//在那一段
	//交换位置
	if(NULL != tmp_item->pre && segment_pos != lrumc->segment_num - 1) //最后那段直接上去
	{
		//找出比当前节点大的节点
		while(NULL != tmp_item->pre && tmp_item->weight < item->weight) tmp_item = tmp_item->pre;	
		if(tmp_item->next != item || tmp_item->weight < item->weight)
		{
			//说明需要偏移
			//交换2个节点
			item_pre = item->pre;
			item_next = item->next;
			item->pre = tmp_item;
			item->next = tmp_item->next;
			item->next->pre = item;
			tmp_item->next = item;

			item_pre->next = item_next;
			item_next->pre = item_pre;
		}
	}

	//交换段位置	
	if(item->weight + lrumc->segment[segment_pos]->max_size/5 > lrumc->segment[segment_pos]->item_list->pre->next->weight || 
		NULL == item->pre->pre || segment_pos == lrumc->segment_num - 1)
	{
		//已经是最开始的那一段就不处理了
		if(0 != segment_pos)
		{
			//从原来队列脱离
			//tmp_item 就是当前段的总头,item就是要脱离的那一段
			tmp_item->next = tmp_item->next->next;
			tmp_item->next->pre = tmp_item;
			lrumc->segment[segment_pos]->curr_size--;//当前的这个要减1
			lrumc->curr_num--;
			//保证item干净
			item->pre = NULL;
			item->next = NULL;
			segment_t *segment = create_one_segment(item,1);		
			insert_segment(lrumc,segment_pos - 1,segment);//添加到上一个段
			discard_segment(lrumc,segment);
		}
	}
}

/**
 * @desc 删除一个节点（不能DFREE pre next）,维护原来链表的结构
 * @param segment_item_t *item 节点地址
 * @return void
 */
void delete_segment_item(segment_item_t *item)
{
	assert(item);	
	segment_item_t *item_pre = item->pre;
	segment_item_t *item_next = item->next;
	DFREE(item);
	item_pre->next = item_next;
	item_next->pre = item_pre;
}

/**
 * @desc 初始化crypt_table(暴雪)
 * @param void
 * @return void
 */
void prepare_crypt_table(void)
{ 
    unsigned long seed = 0x00100001, index1 = 0, index2 = 0, i;
    for(index1 = 0; index1 < 0x100; index1++)
    { 
        for(index2 = index1, i = 0; i < 5; i++, index2 += 0x100)
        { 
            unsigned long temp1, temp2;
            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp1 = (seed & 0xFFFF) << 0x10;
            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp2 = (seed & 0xFFFF);
            crypt_table[index2] = (temp1 | temp2); 
        } 
    } 
}

/**
 * @desc 计算hash_code(暴雪)
 * @param char *str 键
 * @param unsigned long hash_type 
 * @return unsigned long
 */
unsigned long hash_code(char *str, unsigned long hash_type)
{ 
    unsigned char *key = (unsigned char *)str;
    unsigned long seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
    int ch;

    while(*key != 0)
    { 
        ch = toupper(*key++);
        seed1 = crypt_table[(hash_type << 8) + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3; 
    }
    return seed1; 
} 

/**
 * @desc 调整bucket
 * @param lrumc_t *lrumc lrumc句柄
 * @param int pos 位置
 * @return void
 */
void fix_have_used(lrumc_t *lrumc,int pos)
{
	assert(lrumc && pos >= 0 && pos < lrumc->bucket_num);		
	int last_find_pos = -1,find_pos = pos + 1;
	if(find_pos == lrumc->bucket_num) find_pos = 0;//到达上限	
	int old_pos = find_pos;
	int b_offset = 0;
	while(BUCKET_STATUS_NULL != lrumc->bucket[find_pos].status)
	{
		b_offset = find_pos - pos;
		if(
			BUCKET_STATUS_USED == lrumc->bucket[find_pos].status && 
			b_offset < lrumc->bucket[find_pos].bucket_offset) 
		{
			last_find_pos = find_pos;
		}
		find_pos++;	
		if(find_pos >= lrumc->bucket_num) find_pos = 0;
		if(find_pos == old_pos)
		{
			//已经循环了一圈
			break;	
		}
	}
	if(-1 != last_find_pos)
	{
		segment_item_t *item = (segment_item_t *)lrumc->bucket[last_find_pos].list_pos;	
		item->offset = pos;

		lrumc->bucket[pos].status = lrumc->bucket[last_find_pos].status;
		lrumc->bucket[pos].key = lrumc->bucket[last_find_pos].key;
		lrumc->bucket[pos].value = lrumc->bucket[last_find_pos].value;
		lrumc->bucket[pos].value_len = lrumc->bucket[last_find_pos].value_len;
		lrumc->bucket[pos].segment_pos = lrumc->bucket[last_find_pos].segment_pos;
		lrumc->bucket[pos].list_pos = lrumc->bucket[last_find_pos].list_pos;
		lrumc->bucket[pos].hash_code_b = lrumc->bucket[last_find_pos].hash_code_b;
		lrumc->bucket[pos].hash_code_c = lrumc->bucket[last_find_pos].hash_code_c;
		lrumc->bucket[pos].bucket_offset -= b_offset;

		lrumc->bucket[last_find_pos].status = BUCKET_STATUS_NULL;
		lrumc->bucket[last_find_pos].hash_code_b = 0;
		lrumc->bucket[last_find_pos].hash_code_c = 0;
	}
	else
	{
		//从pos到find_pos - 1 都置成BUCKET_STATUS_NULL		
		int i;
		if(find_pos > pos)
		{
			//小到大 一段就OK
			for(i=pos;i<find_pos;i++)
			{
				if(BUCKET_STATUS_HAD_USED == lrumc->bucket[i].status)
				{
					lrumc->bucket[i].status = BUCKET_STATUS_NULL;
					lrumc->bucket[i].hash_code_b = 0;
					lrumc->bucket[i].hash_code_c = 0;
				}
			}
		}
		else
		{
			//分成2段处理
			for(i=0;i<find_pos;i++)
			{
				if(BUCKET_STATUS_HAD_USED == lrumc->bucket[i].status)
				{
					lrumc->bucket[i].status = BUCKET_STATUS_NULL;
					lrumc->bucket[i].hash_code_b = 0;
					lrumc->bucket[i].hash_code_c = 0;
				}
			}

			for(i=pos;i<lrumc->bucket_num;i++)
			{
				if(BUCKET_STATUS_HAD_USED == lrumc->bucket[i].status)
				{
					lrumc->bucket[i].status = BUCKET_STATUS_NULL;
					lrumc->bucket[i].hash_code_b = 0;
					lrumc->bucket[i].hash_code_c = 0;
				}
			}
		}
	}
}

/**
 * @desc 计算数组里面的元素有多少个
 * @param lrumc_t *lrumc
 * @param char *key 键
 * @param char *path 路径
 * @return int
 */
int count_lrumc_data(lrumc_t *lrumc,char *key,char *path)
{
	assert(key);	
	int pos = -1;
	if(-1 == (pos = find_bucket_pos(lrumc,key))) return -1;

	int ret_num = -1;
	pary_node_t *ret_node = NULL;

	if(strlen(path) > 0) ret_node = get_pary_node(lrumc->bucket[pos].value,path);
	else ret_node = (pary_node_t *)lrumc->bucket[pos].value;		

	if(NULL != ret_node)
	{
		if(PARY_TYPE_OBJECT == ret_node->type) ret_num = ret_node->data.data_object->child_num;	
		else ret_num = 1;	
	}
	return ret_num;	
}

/**
 * @desc 清除所有的数据
 * @param lrumc_t *lrumc lrumc总句柄
 * @return long
 */
long flush_all(lrumc_t *lrumc)
{
	lrumc->curr_num = 0;	
	int i = 0;
	long num = 0;
	//遍历所有的segment
	for(;i<lrumc->segment_num;i++)
	{
		//遍历segment链表
		segment_t *segment = lrumc->segment[i];
		if(segment->item_list->pre->next !=  segment->item_list->next)
		{
			segment_item_t *old_item = NULL,*tmp_item = segment->item_list->pre->next;
			while(tmp_item != segment->item_list->next)
			{
				num++;
				old_item = tmp_item;
				lrumc->bucket[old_item->offset].status = BUCKET_STATUS_NULL;
				DFREE(lrumc->bucket[old_item->offset].key);
				destroy_pary_node(lrumc->bucket[old_item->offset].value);
				lrumc->bucket[old_item->offset].hash_code_b = 0;
				lrumc->bucket[old_item->offset].hash_code_c = 0;
				tmp_item = tmp_item->next;	
				destroy_segment_item(old_item);
			}
			//修正关系
			segment->item_list->pre->next = segment->item_list->next;
			segment->item_list->pre->pre = NULL;
			segment->item_list->next->pre = segment->item_list->pre;
			segment->item_list->next->next = NULL;
			segment->curr_size = 0;
		}
	}
	for(i=0;i<lrumc->bucket_num;i++)
	{
		if(lrumc->bucket[i].status == BUCKET_STATUS_USED) printf("have error as pos %d\n",i); 
	}
	lrumc->cmd_flush++;
	return num;
}

/**
 * @desc list数据可匹配前缀 后缀 中缀
 * @param lrumc_t *lrumc lrumc总句柄
 * @param char *match_type 匹配方式-fm -mm -em
 * @param char *key 需要匹配的字符串
 * @return buff *
 */
buff_t *list_lrumc_data(lrumc_t *lrumc,char *match_type,char *key)
{
	buff_t * buff = NULL; 
	int i = 0;
	int key_len = strlen(key);
	bool found = false;
	//遍历整个哈希桶
	for(;i<lrumc->bucket_num;i++)
	{
		if(lrumc->bucket[i].status != BUCKET_STATUS_USED) continue;	
		if(0 != strcmp(match_type,"NO"))
		{
			char *p = strstr(lrumc->bucket[i].key,key);
			if(NULL == p) continue;
			int offset = p - lrumc->bucket[i].key;
			if(0 == strcmp(match_type,"-fm"))
			{
				if(0 != offset) continue;
			}
			else if(0 == strcmp(match_type,"-em"))
			{
				if(offset + key_len != strlen(lrumc->bucket[i].key))	continue;	
			}
		}
		if(NULL == buff) buff = create_buff(1024*1024);
		append_buff(buff,lrumc->bucket[i].key,strlen(lrumc->bucket[i].key));		
		append_buff(buff,"\n",1);
		found = true;
	}
	return buff;
}

/**
 * @desc 删除数组中最后的那个元素
 * @param lrumc_t *lrumc
 * @param char *key 键值
 * @param char *path 路径
 * @return buff_t *
 */
buff_t *pop_lrumc_data(lrumc_t *lrumc,char *key,char *path)
{
	return _pop_shift_lrumc_data(lrumc,key,path,true);
}

/**
 * @desc 删除数组中最前的那个元素
 * @param lrumc_t *lrumc
 * @param char *key 键值
 * @param char *path 路径
 * @return buff_t *
 */
buff_t *shift_lrumc_data(lrumc_t *lrumc,char *key,char *path)
{
	return _pop_shift_lrumc_data(lrumc,key,path,false);
}

/**
 * @desc 删除数组中的元素
 * @param lrumc_t *lrumc
 * @param char *key 键值
 * @param char *path 路径
 * @return buff_t *
 */
buff_t *_pop_shift_lrumc_data(lrumc_t *lrumc,char *key,char *path,bool pop)
{
	assert(key);	
	int pos = -1;
	if(-1 == (pos = find_bucket_pos(lrumc,key))) return NULL;

	pary_rbnode_t *rbnode = NULL;
	pary_node_t *ret_node = NULL;
	if(strlen(path) > 0) ret_node = get_pary_node(lrumc->bucket[pos].value,path);
	else ret_node = (pary_node_t *)lrumc->bucket[pos].value;	
	
	if(NULL != ret_node)
	{
		if(true == pop) rbnode = pop_pary_node(ret_node);
		else rbnode = shift_pary_node(ret_node);

		if(NULL != rbnode)
		{
				buff_t *buff = create_buff(1024);
				char tmp[2048];		
				memset(tmp,0,2048);
				buff_t *seri_buff = serialize(rbnode->value);
				int len = sprintf(tmp,"key=%s\nvalue=%s\r\n",rbnode->key,(char *)seri_buff->data);
				DFREE(rbnode->key);
				destroy_pary_node(rbnode->value);
				DFREE(rbnode);
				destroy_buff(seri_buff);
				append_buff(buff,tmp,len);
				return buff;
		}
	}
	return NULL;	
}

/**
 * @desc 将元素压到数组的后面
 * @param lrumc_t *lrumc
 * @param char *key 键值
 * @param char *path 路径
 * @param void *value 值
 * @param long value_len 值的长度
 * @param pary_type_t value_type 值的类型
 * @return bool
 */
bool push_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type)
{
	return _push_unshift_lrumc_data(lrumc,key,path,value,value_len,value_type,true);
}

/**
 * @desc 将元素压到数组的前面
 * @param lrumc_t *lrumc
 * @param char *key 键值
 * @param char *path 路径
 * @param void *value 值
 * @param long value_len 值的长度
 * @param pary_type_t value_type 值的类型
 * @return bool
 */
bool unshift_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type)
{
	return _push_unshift_lrumc_data(lrumc,key,path,value,value_len,value_type,false);
}

/**
 * @desc 将元素添加到数组
 * @param lrumc_t *lrumc
 * @param char *key 键值
 * @param char *path 路径
 * @param void *value 值
 * @param long value_len 值的长度
 * @param pary_type_t value_type 值的类型
 * @param bool push 是否用push的方式
 * @return bool
 */
bool _push_unshift_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type,bool push)
{
	assert(key);	
	bool ret = false;
	int pos = -1;
	if(-1 != (pos = find_bucket_pos(lrumc,key)))
	{
		pary_node_t *node = NULL;
		pary_node_t *ret_node = NULL;
		if(strlen(path) > 0) ret_node = get_pary_node(lrumc->bucket[pos].value,path);
		else ret_node = (pary_node_t *)lrumc->bucket[pos].value;
			
		if(NULL != ret_node)
		{
			if(PARY_TYPE_OBJECT == ret_node->type)
			{
				lrumc->bytes_written += value_len;
				ret_node->data.data_object->child_num++;
				node = parse_pary_node_value(value,(int)value_len,value_type);
				if(true == push) ret = push_pary_node(ret_node,node);		
				else ret = unshift_pary_node(ret_node,node);		
			}
		}
	}
	DFREE(key);
	return ret;
}


/**
 * @desc 根据key找出在bucket的位置
 * @param lrumc_t *lrumc 总句柄
 * @param char *key 键值
 * @return int
 */
int find_bucket_pos(lrumc_t *lrumc,char *key)
{
	ulong hash_code_a = hash_code(key,0);
	ulong hash_code_b = hash_code(key,1);
	ulong hash_code_c = hash_code(key,2);
	int pos = hash_code_a % lrumc->bucket_num;
	int old_pos = pos;
	if(NULL == lrumc->bucket+pos) return -1;
	while(1)
	{
		if(
				hash_code_b == lrumc->bucket[pos].hash_code_b &&
				hash_code_c == lrumc->bucket[pos].hash_code_c && 
				lrumc->bucket[pos].status != BUCKET_STATUS_HAD_USED)
		{
			break;
		}
		if(BUCKET_STATUS_NULL == lrumc->bucket[pos].status) return -1;
		pos++;
		if(pos == lrumc->bucket_num) pos = 0;
		if(pos == old_pos)
		{
			return -1;	
		}
	}
	return pos;	
}

/**
 * @desc 根据key找出在bucket的位置
 * @param lrumc_t *lrumc 总句柄
 * @param char *key 键值
 * @param int *pos 位置
 * @return bool
 */
bool found_bucket_pos(lrumc_t *lrumc,char *key,int *pos)
{
	ulong hash_code_a = hash_code(key,0);
	ulong hash_code_b = hash_code(key,1);
	ulong hash_code_c = hash_code(key,2);
	*pos = hash_code_a % lrumc->bucket_num;
	int old_pos = *pos;
	bool found = false;
	while(BUCKET_STATUS_NULL != lrumc->bucket[*pos].status)
	{
		if(
			hash_code_b == lrumc->bucket[*pos].hash_code_b && 
			hash_code_c == lrumc->bucket[*pos].hash_code_c &&
			BUCKET_STATUS_HAD_USED != lrumc->bucket[*pos].status)
		{
			found = true;
			break;
		}
		(*pos)++;		
		if((*pos) >= lrumc->bucket_num) *pos = 0;	
		if(old_pos == *pos)
		{
			break;
		}
	}
	return found;
}

/**
 * @desc 分页获取数据
 * @param lrumc_t *lrumc 句柄
 * @param char *key 键
 * @param char *path 路径
 * @param int offset 偏移(开始坐标，从0开始)
 * @param int limit 一页显示多少条数据
 * @return buff_t *
 */
buff_t *page_lrumc_data(lrumc_t *lrumc,char *key,char *path,int offset,int limit)
{
	assert(lrumc && key && path && offset >= 0 && limit >= 0);
	buff_t *buff = NULL;
	int pos = -1;
	bool found = found_bucket_pos(lrumc,key,&pos);
	if(true == found)
	{
		pary_node_t *node = NULL;
		if(0 != strlen(path)) node = get_pary_node(lrumc->bucket[pos].value,path);	
		else node = lrumc->bucket[pos].value;	

		if(NULL != node)
		{
			if(PARY_TYPE_OBJECT != node->type) return NULL;

			pary_list_t *list = PL(node);
			pary_list_t *tmp_item = list->pre->next;
			int i = 0,j = 0,len = 0;
			char tmp[2048];
			while(tmp_item != list->next && j < limit)
			{
				if(i<offset)
				{
					i++;
					tmp_item = tmp_item->next;
					continue;	
				}
				memset(tmp,0,2048);
				len = sprintf(tmp,"key=%s\n",tmp_item->rbnode->key);	
				if(NULL == buff) buff = create_buff(2048);
				append_buff(buff,tmp,len);
				i++;
				j++;
				tmp_item = tmp_item->next;
			}
			increment_weight(lrumc,pos);//增加权重
		}
	}
	return buff;	
}
