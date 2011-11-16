#ifndef _LRUMC_H
#define _LRUMC_H
#include <stdbool.h>
#include "pary.h"

//long long 最大值
#define L_L_MAX 9223372036854775807LL
//哈希桶稀疏度(72%是有数据的)
#define HASH_FULL_RATE 50
//key的最大长度
#define KEY_LEN 255
#define VALUE_LEN 1024

//data struct
typedef struct segment_config_s segment_config_t;
typedef struct segment_s segment_t;
typedef struct segment_item_s segment_item_t;
typedef struct lrumc_s lrumc_t;
typedef enum bucket_status_e bucket_status_t;
typedef struct bucket_s bucket_t;
typedef struct get_ret_s get_ret_t;
typedef unsigned long ulong;

//segment的配置结构
struct segment_config_s
{
	int max_size;//最大容量
	int	buff_size;//缓冲区的大小
};

//segment的数据结构
struct segment_s
{
	int max_size;//最大容量
	int buff_size;//缓冲区的大小
	int curr_size;//当前有多少数据
	segment_item_t *item_list;//双向链表
};

//segment链表的元素结构
struct segment_item_s
{
	int offset;//哈希桶的偏移值
	long long weight;//权重	
	segment_item_t *pre;
	segment_item_t *next;
};

//哈希桶的状态
enum bucket_status_e
{
	BUCKET_STATUS_NULL = 0,//从未用过
	BUCKET_STATUS_USED = 1,//当前在用	
	BUCKET_STATUS_HAD_USED = 2,//以前用过，现在无用	
};

//哈希桶
struct bucket_s
{
	bucket_status_t status;//状态
	char *key;//key
	int segment_pos;//在那一个segment
	void *value;//值（pary_node_t）
	long value_len;//值的长度
	int bucket_offset;//相对于hash_code 处理的偏移度，越大越不好
	ulong list_pos;//链表节点的地址
	ulong hash_code_b;	
	ulong hash_code_c;	
};

//lrumc总句柄
struct lrumc_s
{
	int total;//一共可容纳多少个元素
	int curr_num;//当前有多少个元素
	int bucket_num;//哈希桶的总大小
	int segment_num;//segment的数量
	segment_t **segment;//segment数组(链表)
	bucket_t *bucket;//哈希桶数组(顺序表)
	ulong get_hits;
	ulong get_misses;
	ulong delete_hits;
	ulong delete_misses;
	ulong incr_hits;
	ulong incr_misses;
	ulong decr_hits;
	ulong decr_misses;
	ulong bytes_read;
	ulong bytes_written;
	ulong evictions;
	ulong cmd_set;
	ulong cmd_flush;
	time_t start_time;
};

struct get_ret_s
{
	long length;
	void *data;
	pary_type_t type;	
};

//function
lrumc_t *init_lrumc( segment_config_t *config,int num);//初始化总句柄
void destroy_lrumc(lrumc_t *lrumc);//删除总句柄
get_ret_t *get_lrumc_data(lrumc_t *lrumc,char *key,char *path);//根据key path查找节点
bool set_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type);//更新节点
bool add_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type);//新增节点
bool replace_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type);//更新节点
bool append_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len);//追加内容(后面)
bool prepend_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len);//追加内容（前面）
bool delete_lrumc_data(lrumc_t *lrumc,char *key,char *path);//删除节点
long long increment_lrumc_data(lrumc_t *lrumc,char *key,char *path,int num);//自增长
long long decrement_lrumc_data(lrumc_t *lrumc,char *key,char *path,int num);//自递减
void display_lrumc(lrumc_t *lrumc);//打印总句柄
void prepare_crypt_table();//初始化crypt_table(暴雪)

int count_lrumc_data(lrumc_t *lrumc,char *key,char *path);
long flush_all(lrumc_t *lrumc);
buff_t *list_lrumc_data(lrumc_t *lrumc,char *match_type,char *key);
buff_t *pop_lrumc_data(lrumc_t *lrumc,char *key,char *path);
buff_t *shift_lrumc_data(lrumc_t *lrumc,char *key,char *path);

bool push_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type);
bool unshift_lrumc_data(lrumc_t *lrumc,char *key,char *path,void *value,long value_len,pary_type_t value_type);
buff_t *page_lrumc_data(lrumc_t *lrumc,char *key,char *path,int offset,int limit);

lrumc_t *update_lrumc(lrumc_t *lrumc,segment_config_t *config,int num);

#endif
