#include <stdio.h>
#include <stdlib.h>
#include "../../lrumc.c"
#include <CUnit/Basic.h>

static lrumc_t *g_lrumc = NULL;
static segment_config_t g_config[] = {
		{5000,50},
		{5000,100},
		{10000,200}
	}; 

int init_suite1(void)
{
	prepare_crypt_table();
	g_lrumc = init_lrumc(g_config,3);
	//CU_ASSERT(NULL != g_lrumc);
	return 0;
}

int clean_suite1(void)
{
	destroy_lrumc(g_lrumc);
	return 0;
}

void test_set_lrumc_data_buff(void)
{
	char *key = string("key1");
	char *path = "";
	char *value = "value1";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_BUFF;

	bool ret = set_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret_t *get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_BUFF == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));
}

void test_set_lrumc_data_int(void)
{
	char *key = string("key2");
	char *path = "";
	char *value = "123";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_INT;
	bool ret = set_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret_t *get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_INT == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));
}

void test_set_lrumc_data_bool(void)
{
	char *key = string("key3");
	char *path = "";
	char *value = "1";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_BOOL;
	bool ret = set_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret_t *get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_BOOL == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));
}

void test_set_lrumc_data_double(void)
{
	char *key = string("key4");
	char *path = "";
	char *value = "899999999999.99";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_DOUBLE;
	bool ret = set_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret_t *get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_DOUBLE == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	//CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));
}

void test_set_lrumc_data_object(void)
{
	char *key = string("key5");
	char *path = "";
	char *value = "a:1:{s:1:\"a\";s:3:\"qwe\";}";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_OBJECT;
	bool ret = set_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret_t *get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_OBJECT == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));
}

void test_add_lrumc_data_object(void)
{
	char *key = string("key6");
	char *path = "";
	char *value = "a:1:{s:1:\"a\";s:3:\"qwe\";}";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_OBJECT;
	bool ret = false;
	get_ret_t *get_ret = NULL;
	get_ret = get_lrumc_data(g_lrumc,key,path);
	if(NULL != get_ret)
	{
		ret = delete_lrumc_data(g_lrumc,key,path);	
		CU_ASSERT(true == ret);
	}

	ret = add_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_OBJECT == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));

	ret = delete_lrumc_data(g_lrumc,key,path);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL == get_ret);
}


void test_add_lrumc_data_buff(void)
{
	char *key = string("key7");
	char *path = "";
	char *value = "今天天气真不错啊";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_BUFF;
	bool ret = false;
	get_ret_t *get_ret = NULL;
	get_ret = get_lrumc_data(g_lrumc,key,path);
	if(NULL != get_ret)
	{
		ret = delete_lrumc_data(g_lrumc,key,path);	
		CU_ASSERT(true == ret);
	}

	ret = add_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_BUFF == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));

	ret = delete_lrumc_data(g_lrumc,key,path);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL == get_ret);
}


void test_add_lrumc_data_int(void)
{
	char *key = string("key8");
	char *path = "";
	char *value = "123456789";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_INT;
	bool ret = false;
	get_ret_t *get_ret = NULL;
	get_ret = get_lrumc_data(g_lrumc,key,path);
	if(NULL != get_ret)
	{
		ret = delete_lrumc_data(g_lrumc,key,path);	
		CU_ASSERT(true == ret);
	}

	ret = add_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_INT == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));

	ret = delete_lrumc_data(g_lrumc,key,path);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL == get_ret);
}

void test_add_lrumc_data_bool(void)
{
	char *key = string("key9");
	char *path = "";
	char *value = "0";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_BOOL;
	bool ret = false;
	get_ret_t *get_ret = NULL;
	get_ret = get_lrumc_data(g_lrumc,key,path);
	if(NULL != get_ret)
	{
		ret = delete_lrumc_data(g_lrumc,key,path);	
		CU_ASSERT(true == ret);
	}

	ret = add_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_BOOL == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));

	ret = delete_lrumc_data(g_lrumc,key,path);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL == get_ret);
}

void test_add_lrumc_data_double(void)
{
	char *key = string("key10");
	char *path = "";
	char *value = "0";
	long value_len = strlen(value);
	pary_type_t value_type = PARY_TYPE_DOUBLE;
	bool ret = false;
	get_ret_t *get_ret = NULL;
	get_ret = get_lrumc_data(g_lrumc,key,path);
	if(NULL != get_ret)
	{
		ret = delete_lrumc_data(g_lrumc,key,path);	
		CU_ASSERT(true == ret);
	}

	ret = add_lrumc_data(g_lrumc,key,path,value,value_len,value_type);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL != get_ret);
	CU_ASSERT(PARY_TYPE_DOUBLE == get_ret->type);
	CU_ASSERT(value_len = get_ret->length);
	//CU_ASSERT(0 == memcmp(get_ret->data,value,value_len));

	ret = delete_lrumc_data(g_lrumc,key,path);	
	CU_ASSERT(true == ret);

	get_ret = get_lrumc_data(g_lrumc,key,path);
	CU_ASSERT(NULL == get_ret);
}

void test2(void)
{
	CU_ASSERT(1 == 1);
	CU_ASSERT(2 == 2);
}

int main()
{
	CU_pSuite ps = NULL;
	if (CUE_SUCCESS != CU_initialize_registry()) return CU_get_error();

	if(NULL == (ps = CU_add_suite("suite_1", init_suite1, clean_suite1)))
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
		(NULL == CU_add_test(ps, "test_set_lrumc_data_buff()", test_set_lrumc_data_buff)) ||
		(NULL == CU_add_test(ps, "test_set_lrumc_data_int()", test_set_lrumc_data_int)) ||
		(NULL == CU_add_test(ps, "test_set_lrumc_data_double()", test_set_lrumc_data_double)) ||
		(NULL == CU_add_test(ps, "test_set_lrumc_data_object()", test_set_lrumc_data_object)) ||
		(NULL == CU_add_test(ps, "test_set_lrumc_data_bool()", test_set_lrumc_data_bool)) ||

		(NULL == CU_add_test(ps, "test_add_lrumc_data_buff()", test_add_lrumc_data_buff)) ||
		(NULL == CU_add_test(ps, "test_add_lrumc_data_int()", test_add_lrumc_data_int)) ||
		(NULL == CU_add_test(ps, "test_add_lrumc_data_double()", test_add_lrumc_data_double)) ||
		(NULL == CU_add_test(ps, "test_add_lrumc_data_object()", test_add_lrumc_data_object)) ||
		(NULL == CU_add_test(ps, "test_add_lrumc_data_bool()", test_add_lrumc_data_bool)) 

	) 
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
