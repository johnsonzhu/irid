#include "assignment.h"

pary_node_t *parse_pary_node_value(void *value,int value_len,pary_type_t value_type)
{
	if(PARY_TYPE_OBJECT == value_type)
	{
		pary_node_t *value_node = create_pary_node_object();
		unserialize(value_node,value);
		return value_node;
	}
	else if(PARY_TYPE_BUFF == value_type)
	{
		buff_t *buff = create_buff(value_len);
		append_buff(buff,value,value_len);
		return create_pary_node_buff(buff);
	}
	else if(PARY_TYPE_INT == value_type)
	{
		return create_pary_node_int(atoi((char *)value));
	}
	else if(PARY_TYPE_BOOL == value_type)
	{
		return create_pary_node_bool(atoi((char *)value));
	}
	else if(PARY_TYPE_DOUBLE == value_type)
	{
		return create_pary_node_double(strtol((char *)value,NULL,10));
	}
	else
	{
		printf("bad type 666666666666666\n");
		return NULL;
	}
}

bool update_pary_node(lrumc_t *lrumc,int pos,char *path,void *value,int value_len,pary_type_t value_type,bool found)
{
	if(0 == strlen(path))
	{
		//path为空那就说明更新根
		if(found)
		{
			//说明以前已经有节点,需要先释放	
			destroy_pary_node(lrumc->bucket[pos].value);
		}
		lrumc->bucket[pos].value = parse_pary_node_value(value,value_len,value_type);
	}
	else
	{
		if(!found)
		{
			//说明是空节点，需要先申请内存
			lrumc->bucket[pos].value = create_pary_node_object();
		}
		pary_node_t *old_pnode = lrumc->bucket[pos].value;//获取当前的根
		pary_node_t *tmp_node = NULL;
		pary_node_t *tmp_node1 = NULL;
		char tmp_key[KEY_LEN];//将path切成一个个key
		char *old_pos = NULL,*curr_pos = path;//curr_pos指向path的开头
		while(1)
		{
			memset(tmp_key,0,KEY_LEN);
			old_pos = curr_pos; //记录在偏移之前的位置
			curr_pos = strstr(curr_pos,".");

			//切分path
			if(NULL == curr_pos) strcpy(tmp_key,old_pos);
			else strncpy(tmp_key,old_pos,curr_pos - old_pos);

			//正常来说是不应该出现tmp_key为空的情况，但是有时候以分割点结尾就！！！
			if(0 == strlen(tmp_key)) return false;

			tmp_node = get_pary_node(old_pnode,tmp_key);
			if(NULL == tmp_node)
			{
				//假如找不到，就需要判断是否已经是最后一个key如果是就加到这个根节点否则再继续递归		
				if(NULL == curr_pos)
				{
					tmp_node1 = parse_pary_node_value(value,value_len,value_type);	
					set_pary_node(old_pnode,string(tmp_key),tmp_node1,true);
					return true;
				}
				else
				{
					tmp_node1 = create_pary_node_object();
					set_pary_node(old_pnode,string(tmp_key),tmp_node1,true);	//生成一个空节点
					old_pnode = tmp_node1;//继续递归
				}
			}
			else
			{
				if(NULL == curr_pos)
				{
					//那说明要替换的就是当前的这个节点，先要释放原的那个
					delete_pary_node(old_pnode,tmp_key);//这里可以优化的
					tmp_node1 = parse_pary_node_value(value,value_len,value_type);	
					set_pary_node(old_pnode,string(tmp_key),tmp_node1,true);	
					return true;
				}
				else
				{
					//说明还不是最后的节点，还需要继续递归
					old_pnode = tmp_node;
				}
			}

			curr_pos++;
		}
	}
	return false;
}

