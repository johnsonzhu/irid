#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include "dalloc.h"
#include "pary.h"


static pary_rbtree_t *create_pary_rbtree(void);//创建一个rbtree
static pary_list_t *create_pary_list(void);//创建一个list
static bool push_pary_list(pary_list_t *list,pary_list_t *item);//添加到队列(只在队列)
static bool unshift_pary_list(pary_list_t *list,pary_list_t *item);
static pary_list_t *pop_pary_list(pary_list_t *list);
static pary_list_t *shift_pary_list(pary_list_t *list);
static void delete_pary_list_item(pary_list_t *item); //删除一个节点
static void merge_pary_node(pary_node_t *dst_node,pary_node_t *src_node);
static pary_list_t *create_pary_list_item(void);//创建一个队列节点
static void destroy_pary_node_object(pary_object_t *object);
static void display_pary_node_object(pary_object_t *object,int depth);

static pary_object_t *create_pary_object(void);//创建一个object
static void update_pary_object_step(pary_object_t *object,char *key);

char *string(char *str)
{
	char *ret = NULL;
	int len = strlen(str);	
	DMALLOC(ret,char *,len+1);
	memset(ret,0,len+1);
	strncpy(ret,str,len);
	ret[len] = '\0';
	return ret;
}

pary_node_t *create_pary_node_bool(bool data)
{
	pary_node_t *node = NULL;	
	DMALLOC(node,pary_node_t *,sizeof(pary_node_t));
	if(NULL == node) return NULL;
	memset(node,0,sizeof(pary_node_t));
	node->type = PARY_TYPE_BOOL;
	node->data.data_int = data;
	node->list_item = NULL;
	return node;
}

pary_node_t *create_pary_node_int(int data)
{
	pary_node_t *node = NULL;	
	DMALLOC(node,pary_node_t *,sizeof(pary_node_t));
	if(NULL == node) return NULL;
	memset(node,0,sizeof(pary_node_t));
	node->type = PARY_TYPE_INT;
	node->data.data_int = data;
	node->list_item = NULL;
	return node;
}

pary_node_t *create_pary_node_double(double data)
{
	pary_node_t *node = NULL;	
	DMALLOC(node,pary_node_t *,sizeof(pary_node_t));
	if(NULL == node) return NULL;
	memset(node,0,sizeof(pary_node_t));
	node->type = PARY_TYPE_DOUBLE;
	node->data.data_double = data;
	node->list_item = NULL;
	return node;
}

pary_node_t *create_pary_node_buff(buff_t *data)
{
	pary_node_t *node = NULL;	
	DMALLOC(node,pary_node_t *,sizeof(pary_node_t));
	if(NULL == node) return NULL;
	memset(node,0,sizeof(pary_node_t));
	node->type = PARY_TYPE_BUFF;
	node->data.data_buff = data;
	node->list_item = NULL;
	return node;
}

pary_node_t *create_pary_node_object(void)
{
	pary_node_t *node = NULL;	
	DMALLOC(node,pary_node_t *,sizeof(pary_node_t));
	if(NULL == node) return NULL;
	memset(node,0,sizeof(pary_node_t));
	node->type = PARY_TYPE_OBJECT;
	node->data.data_object = create_pary_object();
	node->list_item = NULL;
	return node;
}

/**
 * @desc 删除节点(ok)
 * @param pary_node_t *node 要删除的节点
 * @return void
 */
void destroy_pary_node(pary_node_t *node)
{
	assert(node);
	switch(node->type)
	{
		case PARY_TYPE_BUFF:
			destroy_buff(node->data.data_buff);
		break;
		case PARY_TYPE_OBJECT:	
			destroy_pary_node_object(node->data.data_object);	
		break;
		default:
		break;
	}
	DFREE(node);//释放自己
}

/**
 * @desc 更新pnode里面的一个key(这个函数不支持多层key,需要先get再set)
 * @param pary_node_t *pnode 当前要更新的node
 * @param char *key 键值
 * @param pary_node_t *node 值
 * @param bool push 插入方式
 * @return bool
 */
bool set_pary_node(pary_node_t *pnode,char *key,pary_node_t *node,bool push)
{
	//要求pnode必须是一个object
	if(PARY_TYPE_OBJECT != pnode->type)
	{
		printf("pnode is not a object\n");	
		return false;
	}
	//获取key对应的rbnode
	pary_rbnode_t *rbnode = get_pary_node_pos(pnode,key);
	if(NULL == rbnode || NULL == rbnode->value)
	{
		//说明以前不存在
		pary_list_t *list_item = create_pary_list_item();//创建一个空的链表节点
		//插入到红黑树
		pary_rbnode_t *rbnode = rb_insert(PRB(pnode),key,node,list_item);
		assert(rbnode);
		list_item->rbnode = rbnode; //修正绑定关系
		//添加到list,添加到最好就ok	
		if(true == push) push_pary_list(PL(pnode),list_item);
		else unshift_pary_list(PL(pnode),list_item);
		pnode->data.data_object->child_num++;
		update_pary_object_step(pnode->data.data_object,key);	
	}
	else
	{
		//这个key以前是存在的
		if(PARY_TYPE_OBJECT == node->type && PARY_TYPE_OBJECT == rbnode->value->type)
		{
			//假如原来是object，现在也是object，那就将新和旧的数据合并，只需用支持一层合并
			merge_pary_node(rbnode->value,node);
		}
		else
		{
			//直接更新
			pary_node_t *old_node = rbnode->value;//原来的值
			rbnode->value = node;//更新成新的
			//释放旧的node(不用改变结构，只需要释放内存就ok)
			destroy_pary_node(old_node);
		}
		DFREE(key);
	}
	return true;		
}

/**
 * @desc 根据key在pnode里面找对应的值(支持多层key)
 * @param pary_node_t *pnode 父亲节点
 * @param char *key 键值
 * @return pary_node_t *
 */
pary_node_t *get_pary_node(pary_node_t *pnode,char *key)
{
	assert(pnode && key);
	//pnode必须是object
	if(PARY_TYPE_OBJECT != pnode->type) return NULL;
	char *src_key = key;
	pary_node_t *tmp_pnode = pnode;
	pary_rbnode_t *tmp_rbnode = NULL;
	char tmp_key[255];
	while(1)
	{
		memset(tmp_key,0,255);
		char *p = strstr(src_key,".");
		if(NULL == p)
		{
			strcpy(tmp_key,src_key);	
		}
		else
		{
			strncpy(tmp_key,src_key,p - src_key);
		}
		tmp_rbnode = rb_search(tmp_pnode->data.data_object->rbtree,tmp_key);
		if(NULL == p)
		{
			break;	
		}
		src_key = p + 1;
		tmp_pnode = tmp_rbnode->value;
		if(PARY_TYPE_OBJECT != tmp_pnode->type)
		{
			tmp_pnode = NULL;
			break;	
		}
	}
	if(NULL == tmp_rbnode)
	{
		return NULL;	
	}
	else
	{
		return tmp_rbnode->value;
	}
}

/**
 * @desc 根据key在pnode里面找对应红黑树节点(支持多层key)
 * @param pary_node_t *pnode 父亲节点
 * @param char *key 键值
 * @return pary_node_t *
 */
pary_rbnode_t *get_pary_node_pos(pary_node_t *pnode,char *key)
{
	assert(pnode && key);
	//pnode必须是object
	if(PARY_TYPE_OBJECT != pnode->type) return NULL;
	char *src_key = key;
	pary_node_t *tmp_pnode = pnode;
	pary_rbnode_t *tmp_rbnode = NULL;
	char tmp_key[255];
	while(1)
	{
		memset(tmp_key,0,255);
		char *p = strstr(src_key,".");
		if(NULL == p)
		{
			strcpy(tmp_key,src_key);	
		}
		else
		{
			strncpy(tmp_key,src_key,p - src_key);
		}
		tmp_rbnode = rb_search(tmp_pnode->data.data_object->rbtree,tmp_key);
		if(NULL == p)
		{
			break;	
		}
		src_key = p + 1;
		tmp_pnode = tmp_rbnode->value;
		if(PARY_TYPE_OBJECT != tmp_pnode->type)
		{
			tmp_pnode = NULL;
			break;	
		}
	}
	return tmp_rbnode;
}

/**
 * @desc 删除某个key的节点（含孩子）(支持多层key)
 * @param pary_node_t *pnode 父亲节点
 * @param char *key 键值
 * @return bool
 */
bool delete_pary_node(pary_node_t *pnode,char *key)
{
	assert(pnode && key);	
	//pnode必须是object
	pary_rbtree_t *rbtree = (pnode->data.data_object)->rbtree;
	if(PARY_TYPE_OBJECT != pnode->type) return false;
	//红黑树节点
	pary_rbnode_t *rbnode = get_pary_node_pos(pnode,key);
	if(NULL == rbnode) return false;//找不到
	//删除他的孩子先
	destroy_pary_node(rbnode->value);
	DFREE(rbnode->key);
	//删除自己的链表
	delete_pary_list_item(rbnode->list_item);
	//删除自己的红黑树节点
	rb_erase(rbnode,rbtree);
	DFREE(rbnode);
	pnode->data.data_object->child_num--;
	return true;
}

bool push_pary_node(pary_node_t *pnode,pary_node_t *node)
{
	if(PARY_TYPE_OBJECT != pnode->type) return false;	
	char *key = NULL;
	DMALLOC(key,char *,20);
	memset(key,0,20);
	sprintf(key,"%d",pnode->data.data_object->step+1);
	set_pary_node(pnode,key,node,true);
	return true;	
}

bool unshift_pary_node(pary_node_t *pnode,pary_node_t *node)
{
	if(PARY_TYPE_OBJECT != pnode->type) return false;	
	char *key = NULL;
	DMALLOC(key,char *,20);
	memset(key,0,20);
	sprintf(key,"%d",pnode->data.data_object->step+1);
	set_pary_node(pnode,key,node,false);
	return true;	
}

pary_rbnode_t *pop_pary_node(pary_node_t *pnode)
{
	if(PARY_TYPE_OBJECT != pnode->type) return false;	
	if(pnode->data.data_object->child_num == 0) return NULL;
	pary_list_t *item = pop_pary_list(PL(pnode));//删除链表节点
	pary_rbnode_t *rbnode = item->rbnode;
	rb_erase(rbnode,pnode->data.data_object->rbtree);//删除红黑树节点
	pnode->data.data_object->child_num--;
	DFREE(item);	
	return rbnode;
}

pary_rbnode_t *shift_pary_node(pary_node_t *pnode)
{
	if(PARY_TYPE_OBJECT != pnode->type) return false;	
	if(pnode->data.data_object->child_num == 0) return NULL;
	pary_list_t *item = shift_pary_list(PL(pnode));//删除链表节点
	pary_rbnode_t *rbnode = item->rbnode;
	rb_erase(rbnode,pnode->data.data_object->rbtree);//删除红黑树节点
	pnode->data.data_object->child_num--;
	DFREE(item);	
	return rbnode;
}

/**
 * @desc 创建一个红黑树
 * @param void
 * @return pary_rbtree_t *
 */
pary_rbtree_t *create_pary_rbtree(void)
{
	pary_rbtree_t *root = NULL;
	DMALLOC(root,pary_rbtree_t *,sizeof(pary_rbtree_t));
	if(NULL == root) return NULL;
	memset(root,0,sizeof(pary_rbtree_t));
	//这里不用对root->rbnode分配内存，在红黑树里面会自己搞定
	root->rbnode = NULL;
	return root;	
}

/**
 * @desc 创建一个链表
 * @param void
 * @return pary_list_t *
 */
pary_list_t *create_pary_list(void)
{
	pary_list_t *list = NULL;
	DMALLOC(list,pary_list_t *,sizeof(pary_list_t));
	if(NULL == list) return NULL;
	memset(list,0,sizeof(pary_list_t));

	DMALLOC(list->pre,pary_list_t *,sizeof(pary_list_t));
	if(NULL == list->pre) return NULL;
	memset(list->pre,0,sizeof(pary_list_t));
	
	DMALLOC(list->next,pary_list_t *,sizeof(pary_list_t));
	if(NULL == list->next) return NULL;
	memset(list->next,0,sizeof(pary_list_t));

	list->pre->next = list->next;
	list->next->pre = list->pre;
	return list;
}

/**
 * @desc 创建一个空的node(可以认为一个空数组)
 * @param void
 * @return pary_object_t *
 */
pary_object_t *create_pary_object(void)
{
	pary_object_t *object = NULL;
	DMALLOC(object,pary_object_t *,sizeof(pary_object_t));	
	if(NULL == object) return NULL;
	memset(object,0,sizeof(pary_object_t));
	object->list = create_pary_list();
	if(NULL == object->list) return NULL;
	object->rbtree = create_pary_rbtree();
	if(NULL == object->rbtree) return NULL;
	object->child_num = 0;
	object->step = 0;
	return object;
}

/**
 * @desc 将新节点添加到链表（后面）
 * @param pary_list_t *list 链表
 * @param pary_list_t *item 新节点
 * @return bool
 */
bool push_pary_list(pary_list_t *list,pary_list_t *item)
{
	assert(list && item);
	list->next->pre->next = item;
	item->pre = list->next->pre;
	list->next->pre = item;
	item->next = list->next;
	return true;	
}

/**
 * @desc 删除一个元素（后面）
 * @param pary_list_t *list 链表
 * @return pary_list_t *
 */
pary_list_t *pop_pary_list(pary_list_t *list)
{
	if(list->pre->next == list->next) return NULL;
	pary_list_t *item = list->next->pre;
	item->pre->next = item->next;
	item->next->pre = item->pre;
	return item;	
}

/**
 * @desc 删除一个元素（后面）
 * @param pary_list_t *list 链表
 * @return pary_list_t *
 */
pary_list_t *shift_pary_list(pary_list_t *list)
{
	if(list->pre->next == list->next) return NULL;
	pary_list_t *item = list->pre->next;
	item->pre->next = item->next;
	item->next->pre = item->pre;
	return item;	
}

/**
 * @desc 将新节点添加到链表（前面）
 * @param pary_list_t *list 链表
 * @param pary_list_t *item 新节点
 * @return bool
 */
bool unshift_pary_list(pary_list_t *list,pary_list_t *item)
{
	assert(list && item);
	list->pre->next->pre = item;
	item->next = list->pre->next;
	list->pre->next = item;
	item->pre = list->pre;
	return true;	
}

/**
 * @desc 从链表里面删除一个节点
 * @param pary_list_t *需要删除的节点
 * @return void
 */
void delete_pary_list_item(pary_list_t *item) //删除一个节点
{
	assert(item);
	pary_list_t *item_pre = item->pre;
	pary_list_t *item_next = item->next;
	DFREE(item);
	item_pre->next = item_next;
	item_next->pre = item_pre;
}

/**
 * @desc 合并2个object(存在相同的，就src的覆盖dst,也就是新的覆盖旧的，不支持递归,2个必须都是object)
 * @param pary_node_t *dst_node 旧的node
 * @param pary_node_t *src_node 新的node
 * @return void
 */
void merge_pary_node(pary_node_t *dst_node,pary_node_t *src_node)
{
	assert(dst_node && src_node);	
	if(PARY_TYPE_OBJECT != dst_node->type)
	{
		printf("dst_node is not a object\n");
		return;
	}
	if(PARY_TYPE_OBJECT != src_node->type)
	{
		printf("src_node is not a object\n");
		return;
	}
	//遍历链表
	pary_list_t *item = PL(src_node)->pre->next;
	while(item != PL(src_node)->next)
	{
		char *key = item->rbnode->key; //新的key	
		pary_node_t *value = item->rbnode->value; //新的值
		//判断一下是否存在相同的key
		pary_node_t *tmp_node = get_pary_node(dst_node,key);
		if(NULL == tmp_node)
		{
			//不存在,就在添加到dst里面去
			set_pary_node(dst_node,key,value,true);	
			//需要将key从src_node中删除,从红黑树中删除节点	
			//TODO 这里要小心小心，只删除关系，不删除数据,暂不处理
			item->rbnode->key = NULL;
			item->rbnode->value = NULL;
			
		}
		else
		{
			//这里实际只删除孩子的
			destroy_pary_node(tmp_node);
			//已经存在,新的覆盖旧的
			set_pary_node(dst_node,key,value,true);
		}
		item = item->next;
	}
	destroy_pary_node(src_node);
}

/**
 * @desc 创建一个链接节点
 * @param void
 * @return pary_list_t *
 */
pary_list_t *create_pary_list_item(void)
{
	pary_list_t *item = NULL;
	DMALLOC(item,pary_list_t *,sizeof(pary_list_t));	
	if(NULL == item) return NULL;
	memset(item,0,sizeof(pary_list_t));
	item->pre = NULL;
	item->next = NULL;
	return item;
}

/**
 * @desc 删除节点(只删除孩子,不要删除本身),不知道为什么忘记了
 * @param pary_object_t *object 节点
 * @return void
 */
void destroy_pary_node_object(pary_object_t *object)
{
	assert(object);	
	//链表头
	pary_list_t *old_item = NULL,*item = object->list->pre->next;
	//遍历链表
	while(item != object->list->next)
	{
		switch(item->rbnode->value->type)
		{
			case PARY_TYPE_OBJECT:
				destroy_pary_node_object(item->rbnode->value->data.data_object);//递归
				//DFREE(item->rbnode->value->data.data_object);
			break;	
			case PARY_TYPE_BUFF:
				destroy_buff(item->rbnode->value->data.data_buff);
			break;
			default:
			break;
		}	
		old_item = item;
		item = item->next;
		//释放孩子key
		DFREE(old_item->rbnode->key);
		DFREE(old_item->rbnode->value);//上面的递归已经清除所有的孩子
		//删除孩子红黑树节点（直接删除 破坏结构）
		//rb_erase(old_item->rbnode,rbtree);
		DFREE(old_item->rbnode);
		//删除孩子链表节点(直接删除)
		//delete_pary_list_item(old_item);
		DFREE(old_item);
	}
	//清除本身的链表
	DFREE(object->list->pre);
	DFREE(object->list->next);
	DFREE(object->list);
	DFREE(object->rbtree);//红黑树的节点上面都删除清光了,这里释放根
	DFREE(object);
}

/**
 * @desc 打印node
 * @param pary_node_t *node 需要打印的node
 * @return void
 */
void display_pary_node(pary_node_t *node)
{
	assert(node);	
	switch(node->type)
	{
		case PARY_TYPE_INT:
			printf("int data = %d\n",node->data.data_int);
		break;	
		case PARY_TYPE_BOOL:
			printf("bool data = %d\n",node->data.data_int);
		break;	
		case PARY_TYPE_DOUBLE:
			printf("double data = %lf\n",node->data.data_double);
		break;
		case PARY_TYPE_BUFF:
			printf("buff data = %s\n",(char *)node->data.data_buff->data);
		break;
		case PARY_TYPE_OBJECT:
			display_pary_node_object(node->data.data_object,0);
		break;	
		default:
			printf("node bad type\n");
		break;
	}
}

/**
 * @desc 打印一个object
 * @param pary_object_t *object 对象(数组)
 * @parm int depth 深度（用于显示递归）
 * @return void
 */
void display_pary_node_object(pary_object_t *object,int depth)
{
	assert(object);	
	pary_list_t *item = object->list->pre->next;
	int i = 0;
	char space[255];//最大支持255层，傻了
	memset(space,0,255);
	for(;i<depth;i++)
	{
		strcat(space,"\t");
	}
	printf("Array\n%s    (\n",space);
	while(item != object->list->next)
	{
		char *key = item->rbnode->key;	
		printf("\t%s[%s] => ",space,key);
		pary_node_t *value = item->rbnode->value;
		switch(value->type)
		{
			case PARY_TYPE_BOOL:
			case PARY_TYPE_INT:
				printf("%d\n",value->data.data_int);
			break;
			case PARY_TYPE_DOUBLE:
				printf("%lf\n",value->data.data_double);
			break;
			case PARY_TYPE_BUFF:
				printf("%s\n",(char *)value->data.data_buff->data);
			break;
			case PARY_TYPE_OBJECT:
				display_pary_node_object(value->data.data_object,depth + 1);	
				printf("\n");
			break;
			default:
				printf("bad type 55555555\n");
			break;
		}
		item = item->next;
	}
	printf("%s    )\n",space);
}

void update_pary_object_step(pary_object_t *object,char *key)
{
	char c;
	char *p = key;
	while('\0' != (c = *p++))
	{
		if(!isdigit(c)) return;	
	}
	long num = strtol(key,NULL,10);
	if(LONG_MAX == num || LONG_MIN == num) return ;
	if(num > object->step) object->step = num;
}
