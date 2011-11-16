#ifndef _CONFIG_H
#define _CONFIG_H
#include "buff.h"

void init_config(char *filname,void (*set_config_item)(char *key,char *value));
buff_t *reload_config(char *filename);
void set_config_item(char *key,char *value);

#endif
