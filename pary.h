#ifndef _PARY_H
#define _PARY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buff.h"

//获取node的红黑树根
#define PRB(pnode) pnode->data.data_object->rbtree
//获取node的链表头
#define PL(pnode) pnode->data.data_object->list

typedef enum pary_type_e pary_type_t;
typedef struct pary_node_s pary_node_t;
typedef union pary_data_u pary_data_t;
typedef struct pary_list_s pary_list_t;
typedef struct rbroot pary_rbtree_t;
typedef struct rbnode pary_rbnode_t;
typedef struct pary_object_s pary_object_t; 

//红黑树部分
struct rbnode
{
        unsigned long  rb_parent_color;
#define RB_RED          0
#define RB_BLACK        1
        struct rbnode *rbright;
        struct rbnode *rbleft;
		char *key;
		//unsigned long value;
		pary_node_t *value;//这个是值来的
		pary_list_t *list_item;//绑定链表
} __attribute__((aligned(sizeof(long))));
    /* The alignment might seem pointless, but allegedly CRIS needs it */

struct rbroot
{
        struct rbnode *rbnode;
};

#define rb_parent(r)   ((struct rbnode *)((r)->rb_parent_color & ~3))
#define rb_color(r)   ((r)->rb_parent_color & 1)
#define rb_is_red(r)   (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)  do { (r)->rb_parent_color &= ~1; } while (0)
#define rb_set_black(r)  do { (r)->rb_parent_color |= 1; } while (0)

static inline void rb_set_parent(struct rbnode *rb, struct rbnode *p)
{
        rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long)p;
}
static inline void rb_set_color(struct rbnode *rb, int color)
{
        rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}

#define RB_ROOT (struct rbroot) { NULL, }
#define rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)     ((root)->rbnode == NULL)
#define RB_EMPTY_NODE(node)     (rb_parent(node) == node)
#define RB_CLEAR_NODE(node)     (rb_set_parent(node, node))

extern void rb_insert_color(struct rbnode *, struct rbroot *);
extern void rb_erase(struct rbnode *, struct rbroot *);

typedef void (*rb_augment_f)(struct rbnode *node, void *data);

extern void rb_augment_insert(struct rbnode *node,
                              rb_augment_f func, void *data);
extern struct rbnode *rb_augment_erase_begin(struct rbnode *node);
extern void rb_augment_erase_end(struct rbnode *node,
                                 rb_augment_f func, void *data);

/* Find logical next and previous nodes in a tree */
extern struct rbnode *rb_next(const struct rbnode *);
extern struct rbnode *rb_prev(const struct rbnode *);
extern struct rbnode *rb_first(const struct rbroot *);
extern struct rbnode *rb_last(const struct rbroot *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void rb_replace_node(struct rbnode *victim, struct rbnode *new, 
                            struct rbroot *root);

static inline void rb_link_node(struct rbnode * node, struct rbnode * parent,
                                struct rbnode ** rb_link)
{
        node->rb_parent_color = (unsigned long )parent;
        node->rbleft = node->rbright = NULL;

        *rb_link = node;
}

extern struct rbnode * create_rbnode(char *key,pary_node_t *value,pary_list_t *list_item,struct rbnode *parent);
extern struct rbnode *rb_search(struct rbroot *root,char *key);
extern struct rbnode *rb_insert(struct rbroot *root,char *key,pary_node_t *value,pary_list_t *list_item);
extern void rb_lnr(struct rbnode *node);
extern int depth(struct rbnode *node);
extern void display(struct rbnode *node);


//数据类型 4种
enum pary_type_e
{
	PARY_TYPE_BUFF = 0,	
	PARY_TYPE_INT = 1,	
	PARY_TYPE_DOUBLE = 2,
	PARY_TYPE_BOOL = 3,
	PARY_TYPE_OBJECT = 4,
	PARY_TYPE_UNKNOWN = 5
};

//复合数据
struct pary_object_s
{
	int step;
	int child_num;//孩子数
	pary_list_t *list;//双向链表的头
	pary_rbtree_t *rbtree;//红黑树的根	
};

//联合数据结构
union pary_data_u
{
	int data_int;
	double data_double;
	buff_t *data_buff;//2进制安全
	pary_object_t *data_object;	
};

//节点node的数据结构
struct pary_node_s
{
	pary_type_t type;//类型
	pary_data_t data;//数据或者地址
	pary_list_t *list_item;//链表节点地址
};

//链表
struct pary_list_s
{
	pary_list_t *pre;
	pary_list_t *next;		
	pary_rbnode_t *rbnode; //红黑树的节点
};

//function
char *string(char *str);//转化字符串（malloc）

//创建不同类型的node
pary_node_t *create_pary_node_int(int data);
pary_node_t *create_pary_node_double(double data);
pary_node_t *create_pary_node_buff(buff_t *data);
pary_node_t *create_pary_node_object(void);//包含一个空的object,实际就是一个数组
pary_node_t *create_pary_node_bool(bool data);

//释放一个node(支持所有类型的node)
void destroy_pary_node(pary_node_t *node);

bool set_pary_node(pary_node_t *pnode,char *key,pary_node_t *node,bool push);//更新一个key
pary_node_t *get_pary_node(pary_node_t *pnode,char *key);//获取一个key
pary_rbnode_t *get_pary_node_pos(pary_node_t *pnode,char *key);//获取一个key
bool delete_pary_node(pary_node_t *pnode,char *key);

bool push_pary_node(pary_node_t *pnode,pary_node_t *node);//在list队列后面添加一个key
bool unshift_pary_node(pary_node_t *pnode,pary_node_t *node);//在list队列前面添加一个key
pary_rbnode_t *pop_pary_node(pary_node_t *pnode);//从list队列后面删除一个key
pary_rbnode_t *shift_pary_node(pary_node_t *pnode);//从list队列前面删除一个key

void display_pary_node(pary_node_t *node); //打印一个节点的数据结构

bool is_serialized(void *str,int len);//判断一个数据是否序列化过
char *unserialize(pary_node_t *node,char *str);//反序列化
buff_t *serialize(const pary_node_t *node);//序列化一个节点

#endif
