
#include <sys/stat.h>
#include <stdio.h>
#include <json-c/json.h>

#include "ha_debug.h"
#include "ha_decrypt.h"
#include "ha_config.h"
#include "common.h"



#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"ha_config:"


static struct json_object *s_jo = NULL;

/*创建个空的json_type_array类型JSON数组值对象*/
struct json_object *ha_parse_memory(const char *mem)
{
	assert(mem);
	return json_tokener_parse(mem);
}

/*对配置文件进行解析 */
struct json_object *ha_parse_file(const char *f)
{	
	struct json_object *res = NULL;
	char *ptr = NULL;
	char *dec_ptr = NULL;
	
	struct stat st; /*检查文件是否存在*/
	if (lstat(f,&st))
		return res;
	FILE *fp = fopen(f,"r");
	if(!fp)
		return res;

	ptr = MALLOC(st.st_size + 1);
	if (!ptr)
		goto out;
	
	fread(ptr,1,st.st_size,fp); /*读取文件*/
	ptr[st.st_size] = '\0';
#if 0
	dec_ptr = ha_decrypt(ptr,st.st_size);
	if (!dec_ptr)
		goto out;
	debug_p("%s",dec_ptr);
	res = json_tokener_parse(dec_ptr);
#else
	debug_p("%s",ptr);
	
	res = json_tokener_parse(ptr); /*对文件进行解析*/
#endif
	if (!res)
		debug_p("config file invalid");
out:
	if (dec_ptr)
		FREE(dec_ptr);
	if (ptr)
		FREE(ptr);
	if (fp)
		fclose(fp);

	dbg_printf("the config file is: %s  and parse the file ok !\n",f);

	
	return res;
}

struct json_object *ha_find_config(json_object *jo,const char *name)
{
	struct json_object *res = NULL;
	json_object_object_get_ex(jo,name,&res);
	return res;
}
struct json_object *ha_find_module_config(const char *name)
{
	if (!s_jo)
		return NULL;
	return ha_find_config(s_jo,name);
}

/*对某一数组配置调用回调函数*/
void ha_json_array_handle(struct json_object *array,
									void (*func)(struct json_object *,void *),
									void *v)
{
	assert(array && func);
	if (json_object_get_type(array) != json_type_array)
		return ;
	
	size_t n = json_object_array_length(array);
	size_t i ;
	for (i = 0 ; i < n ; i++) {
		struct json_object *elt = json_object_array_get_idx(array,i);
		if(!elt)
			continue;
		func(elt,v);
	}
}

/*获取int值*/
int ha_json_get_int(struct json_object *jo,const char *name)
{
	assert(jo && name);
	struct json_object *tmp = NULL;
	int res = JO_INT_INVALID;
	if (!json_object_object_get_ex(jo,name,&tmp) || !tmp)
		return res;
	if (json_object_get_type(tmp) != json_type_int)
		return res;
	res = json_object_get_int(tmp);
	return res;
}

/*获取字符串*/
const char *ha_json_get_string(struct json_object *jo,const char *name)
{
	const char *str = NULL;
	struct json_object *tmp = NULL;
	assert (jo&& name);
	
	if (!json_object_object_get_ex(jo,name,&tmp) || !tmp)
		return str;
	
	if (json_object_get_type(tmp) != json_type_string)
		return str;
	str = json_object_get_string(tmp);
	return str;
}
const char *ha_json_get_global_string(const char *name)
{
	if (!s_jo)
		return NULL;
	
	return ha_json_get_string(s_jo,name);
}

/*初始化配置文件 */
void ha_config_init(const char *file)
{
	s_jo = ha_parse_file(file);
}

/*这里是进行销毁*/
void ha_config_destroy(void)
{
	if (s_jo)
		json_object_put(s_jo);
	s_jo = NULL;
}
