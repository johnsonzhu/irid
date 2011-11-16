#ifndef _RBTREE_H
#define _RBTREE_H
#include <stdio.h>
#include <stdlib.h>
//#include "pary.h"

struct rbnode
{
        unsigned long  rb_parent_color;
#define RB_RED          0
#define RB_BLACK        1
        struct rbnode *rbright;
        struct rbnode *rbleft;
		char *key;
		//unsigned long value;
		struct pary_node_s *value;//这个是值来的
		struct pary_list_s *list_item;//绑定链表
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

/*
static inline void rb_init_node(struct rbnode *rb)
{
        rb->rb_parent_color = 0;
        rb->rbright = NULL;
        rb->rbleft = NULL;
        RB_CLEAR_NODE(rb);
}
*/
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

extern struct rbnode * create_rbnode(char *key,struct pary_node_s *value,struct pary_list_s *list_item,struct rbnode *parent);
extern struct rbnode *rb_search(struct rbroot *root,char *key);
extern struct rbnode *rb_insert(struct rbroot *root,char *key,struct pary_node_s *value,struct pary_list_s *list_item);
extern void rb_lnr(struct rbnode *node);
extern int depth(struct rbnode *node);
//extern void rb_lnr_pos(struct rbnode *node,long *arr,int pos);
extern void display(struct rbnode *node);
#endif
