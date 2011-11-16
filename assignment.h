#ifndef _ASSIGNMENT_H
#define _ASSIGNMENT_H

#include <stdbool.h>
#include "lrumc.h"
#include "pary.h"

bool update_pary_node(lrumc_t *lrumc,int pos,char *path,void *value,int value_len,pary_type_t value_type,bool found);
pary_node_t *parse_pary_node_value(void *value,int value_len,pary_type_t value_type);

#endif
