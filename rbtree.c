/*
   Red Black Trees
   (C) 1999  Andrea Arcangeli <andrea@suse.de>
   (C) 2002  David Woodhouse <dwmw2@infradead.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   linux/lib/rbtree.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "dalloc.h"
#include "pary.h"

struct rbnode BEN;
#define BNULL &BEN
//static void rb_lnr_pos(struct rbnode *node,char *arr,int pos,int total);

static void __rb_rotate_left(struct rbnode *node, struct rbroot *root)
{
	struct rbnode *right = node->rbright;
	struct rbnode *parent = rb_parent(node);

	if ((node->rbright = right->rbleft))
		rb_set_parent(right->rbleft, node);
	right->rbleft = node;

	rb_set_parent(right, parent);

	if (parent)
	{
		if (node == parent->rbleft)
			parent->rbleft = right;
		else
			parent->rbright = right;
	}
	else
		root->rbnode = right;
	rb_set_parent(node, right);
}

static void __rb_rotate_right(struct rbnode *node, struct rbroot *root)
{
	struct rbnode *left = node->rbleft;
	struct rbnode *parent = rb_parent(node);

	if ((node->rbleft = left->rbright))
		rb_set_parent(left->rbright, node);
	left->rbright = node;

	rb_set_parent(left, parent);

	if (parent)
	{
		if (node == parent->rbright)
			parent->rbright = left;
		else
			parent->rbleft = left;
	}
	else
		root->rbnode = left;
	rb_set_parent(node, left);
}

void rb_insert_color(struct rbnode *node, struct rbroot *root)
{
	struct rbnode *parent, *gparent;

	while ((parent = rb_parent(node)) && rb_is_red(parent))
	{
		gparent = rb_parent(parent);

		if (parent == gparent->rbleft)
		{
			{
				register struct rbnode *uncle = gparent->rbright;
				if (uncle && rb_is_red(uncle))
				{
					rb_set_black(uncle);
					rb_set_black(parent);
					rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rbright == node)
			{
				register struct rbnode *tmp;
				__rb_rotate_left(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rb_set_black(parent);
			rb_set_red(gparent);
			__rb_rotate_right(gparent, root);
		} else {
			{
				register struct rbnode *uncle = gparent->rbleft;
				if (uncle && rb_is_red(uncle))
				{
					rb_set_black(uncle);
					rb_set_black(parent);
					rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rbleft == node)
			{
				register struct rbnode *tmp;
				__rb_rotate_right(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rb_set_black(parent);
			rb_set_red(gparent);
			__rb_rotate_left(gparent, root);
		}
	}

	rb_set_black(root->rbnode);
}

static void __rb_erase_color(struct rbnode *node, struct rbnode *parent,
		struct rbroot *root)
{
	struct rbnode *other;

	while ((!node || rb_is_black(node)) && node != root->rbnode)
	{
		if (parent->rbleft == node)
		{
			other = parent->rbright;
			if (rb_is_red(other))
			{
				rb_set_black(other);
				rb_set_red(parent);
				__rb_rotate_left(parent, root);
				other = parent->rbright;
			}
			if ((!other->rbleft || rb_is_black(other->rbleft)) &&
					(!other->rbright || rb_is_black(other->rbright)))
			{
				rb_set_red(other);
				node = parent;
				parent = rb_parent(node);
			}
			else
			{
				if (!other->rbright || rb_is_black(other->rbright))
				{
					rb_set_black(other->rbleft);
					rb_set_red(other);
					__rb_rotate_right(other, root);
					other = parent->rbright;
				}
				rb_set_color(other, rb_color(parent));
				rb_set_black(parent);
				rb_set_black(other->rbright);
				__rb_rotate_left(parent, root);
				node = root->rbnode;
				break;
			}
		}
		else
		{
			other = parent->rbleft;
			if (rb_is_red(other))
			{
				rb_set_black(other);
				rb_set_red(parent);
				__rb_rotate_right(parent, root);
				other = parent->rbleft;
			}
			if ((!other->rbleft || rb_is_black(other->rbleft)) &&
					(!other->rbright || rb_is_black(other->rbright)))
			{
				rb_set_red(other);
				node = parent;
				parent = rb_parent(node);
			}
			else
			{
				if (!other->rbleft || rb_is_black(other->rbleft))
				{
					rb_set_black(other->rbright);
					rb_set_red(other);
					__rb_rotate_left(other, root);
					other = parent->rbleft;
				}
				rb_set_color(other, rb_color(parent));
				rb_set_black(parent);
				rb_set_black(other->rbleft);
				__rb_rotate_right(parent, root);
				node = root->rbnode;
				break;
			}
		}
	}
	if (node)
		rb_set_black(node);
}

void rb_erase(struct rbnode *node, struct rbroot *root)
{
	struct rbnode *child, *parent;
	int color;

	if (!node->rbleft)
		child = node->rbright;
	else if (!node->rbright)
		child = node->rbleft;
	else
	{
		struct rbnode *old = node, *left;

		node = node->rbright;
		while ((left = node->rbleft) != NULL)
			node = left;

		if (rb_parent(old)) {
			if (rb_parent(old)->rbleft == old)
				rb_parent(old)->rbleft = node;
			else
				rb_parent(old)->rbright = node;
		} else
			root->rbnode = node;

		child = node->rbright;
		parent = rb_parent(node);
		color = rb_color(node);

		if (parent == old) {
			parent = node;
		} else {
			if (child)
				rb_set_parent(child, parent);
			parent->rbleft = child;

			node->rbright = old->rbright;
			rb_set_parent(old->rbright, node);
		}

		node->rb_parent_color = old->rb_parent_color;
		node->rbleft = old->rbleft;
		rb_set_parent(old->rbleft, node);

		goto color;
	}

	parent = rb_parent(node);
	color = rb_color(node);

	if (child)
		rb_set_parent(child, parent);
	if (parent)
	{
		if (parent->rbleft == node)
			parent->rbleft = child;
		else
			parent->rbright = child;
	}
	else
		root->rbnode = child;

color:
	if (color == RB_BLACK)
		__rb_erase_color(child, parent, root);
}

static void rb_augment_path(struct rbnode *node, rb_augment_f func, void *data)
{
	struct rbnode *parent;

up:
	func(node, data);
	parent = rb_parent(node);
	if (!parent)
		return;

	if (node == parent->rbleft && parent->rbright)
		func(parent->rbright, data);
	else if (parent->rbleft)
		func(parent->rbleft, data);

	node = parent;
	goto up;
}

/*
 * after inserting @node into the tree, update the tree to account for
 * both the new entry and any damage done by rebalance
 */
void rb_augment_insert(struct rbnode *node, rb_augment_f func, void *data)
{
	if (node->rbleft)
		node = node->rbleft;
	else if (node->rbright)
		node = node->rbright;

	rb_augment_path(node, func, data);
}

/*
 * before removing the node, find the deepest node on the rebalance path
 * that will still be there after @node gets removed
 */
struct rbnode *rb_augment_erase_begin(struct rbnode *node)
{
	struct rbnode *deepest;

	if (!node->rbright && !node->rbleft)
		deepest = rb_parent(node);
	else if (!node->rbright)
		deepest = node->rbleft;
	else if (!node->rbleft)
		deepest = node->rbright;
	else {
		deepest = rb_next(node);
		if (deepest->rbright)
			deepest = deepest->rbright;
		else if (rb_parent(deepest) != node)
			deepest = rb_parent(deepest);
	}

	return deepest;
}

/*
 * after removal, update the tree to account for the removed entry
 * and any rebalance damage.
 */
void rb_augment_erase_end(struct rbnode *node, rb_augment_f func, void *data)
{
	if (node)
		rb_augment_path(node, func, data);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
struct rbnode *rb_first(const struct rbroot *root)
{
	struct rbnode  *n;

	n = root->rbnode;
	if (!n)
		return NULL;
	while (n->rbleft)
		n = n->rbleft;
	return n;
}

struct rbnode *rb_last(const struct rbroot *root)
{
	struct rbnode  *n;

	n = root->rbnode;
	if (!n)
		return NULL;
	while (n->rbright)
		n = n->rbright;
	return n;
}

struct rbnode *rb_next(const struct rbnode *node)
{
	struct rbnode *parent;

	if (rb_parent(node) == node)
		return NULL;

	/* If we have a right-hand child, go down and then left as far
	   as we can. */
	if (node->rbright) {
		node = node->rbright; 
		while (node->rbleft)
			node=node->rbleft;
		return (struct rbnode *)node;
	}

	/* No right-hand children.  Everything down and left is
	   smaller than us, so any 'next' node must be in the general
	   direction of our parent. Go up the tree; any time the
	   ancestor is a right-hand child of its parent, keep going
	   up. First time it's a left-hand child of its parent, said
	   parent is our 'next' node. */
	while ((parent = rb_parent(node)) && node == parent->rbright)
		node = parent;

	return parent;
}

struct rbnode *rb_prev(const struct rbnode *node)
{
	struct rbnode *parent;

	if (rb_parent(node) == node)
		return NULL;

	/* If we have a left-hand child, go down and then right as far
	   as we can. */
	if (node->rbleft) {
		node = node->rbleft; 
		while (node->rbright)
			node=node->rbright;
		return (struct rbnode *)node;
	}

	/* No left-hand children. Go up till we find an ancestor which
	   is a right-hand child of its parent */
	while ((parent = rb_parent(node)) && node == parent->rbleft)
		node = parent;

	return parent;
}

void rb_replace_node(struct rbnode *victim, struct rbnode *new,
		struct rbroot *root)
{
	struct rbnode *parent = rb_parent(victim);

	/* Set the surrounding nodes to point to the replacement */
	if (parent) {
		if (victim == parent->rbleft)
			parent->rbleft = new;
		else
			parent->rbright = new;
	} else {
		root->rbnode = new;
	}
	if (victim->rbleft)
		rb_set_parent(victim->rbleft, new);
	if (victim->rbright)
		rb_set_parent(victim->rbright, new);

	/* Copy the pointers/colour from the victim to the replacement */
	*new = *victim;
}

struct rbnode * create_rbnode(char *key,pary_node_t *value,pary_list_t *list_item,struct rbnode *parent)
{
	struct rbnode *node = NULL;
	DMALLOC(node,struct rbnode *,sizeof(struct rbnode));
	if(NULL == node) return NULL;
	node->rbleft = NULL;
	node->rbright = NULL;
	node->rb_parent_color = (unsigned long)parent;
	node->key = key;
	node->value = value;
	node->list_item = list_item;
	rb_set_red(node);
	return node;
}

struct rbnode *rb_search(struct rbroot *root,char *key)
{
	assert(root && key);
	struct rbnode *n = root->rbnode;
	int i;
	while(NULL != n)
	{
		i = strcmp(n->key,key);
		if(0 == i) return n;
		else if(i < 0) n = n->rbright;	
		else n = n->rbleft;	
	}
	return NULL;
}

struct rbnode *rb_insert(struct rbroot *root,char *key,pary_node_t *value,pary_list_t *list_item)
{
	assert(root && key && value && list_item);
	struct rbnode *ret_node = NULL;
	struct rbnode **n = &(root->rbnode);
	int lr; //0=left 1=right
	if(NULL == *n)
	{
		//printf("is root\n");
		*n = create_rbnode(key,value,list_item,NULL);	
		ret_node = *n;
		if(NULL == *n)
		{
			printf("create_node fail\n");
		}
		rb_set_black(*n);
		return ret_node;
	}
	int i;
	while(NULL != *n)
	{
		//printf("cmp key %ld\n",(*n)->key);
		i = strcmp((*n)->key,key);
		if(0 == i)
		{
			printf("have exist ====%s====\n",(*n)->key);
			return NULL;	
		}
		else if(i < 0)
		{
			lr = 1;
			if(NULL == (*n)->rbright)
			{
				//printf("set right key %ld\n",key);
				(*n)->rbright = create_rbnode(key,value,list_item,*n);
				ret_node = (*n)->rbright;
				break;
			}
			n = &(*n)->rbright;	
		}	
		else
		{
			lr = 0;
			if(NULL == (*n)->rbleft)
			{
				//printf("set left key %ld\n",key);
				(*n)->rbleft = create_rbnode(key,value,list_item,*n);
				ret_node = (*n)->rbleft;
				break;
			}
			n = &(*n)->rbleft;	
		}
	}
	if(0 == lr) rb_insert_color((*n)->rbleft,root);
	else rb_insert_color((*n)->rbright,root);

	return ret_node;
}

void rb_lnr(struct rbnode *node)
{
	if(NULL != node)
	{
		rb_lnr(node->rbleft);	
		printf("%s",node->key);
		rb_lnr(node->rbright);
	}
}

/*
void rb_lnr_pos(struct rbnode *node,char *arr,int pos,int total)
{
	if(NULL != node)
	{
		rb_lnr_pos(node->rbleft,arr,2*pos,total);	
		//if(pos<=total) arr[pos-1] = node->key;
		if(pos<=total) strcpy(arr[pos-1],node->key);
		rb_lnr_pos(node->rbright,arr,2*pos+1,total);
	}
}
*/
int depth(struct rbnode *node)
{
	int d = -1,ldepth=0,rdepth=0;
	if(NULL != node){

		ldepth = depth(node->rbleft);
		rdepth = depth(node->rbright);
		d  = 1 + (ldepth > rdepth ? ldepth : rdepth);	
	}
	return d;	
}

void display(struct rbnode *node)
{
	/*
	int d = depth(node) + 1;	
	int num = pow(2,d) - 1;
	char *arr = NULL;
	DMALLOC(arr,char *,sizeof(char)*num);
	memset(arr,0,sizeof(char)*num);
	rb_lnr_pos(node,arr,1,num);
	//display_tree(arr,num);
	*/
}
