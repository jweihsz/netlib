#ifndef _HA_CONFIG_H
#define _HA_CONFIG_H

#include <json-c/json.h>

#define DEF_SRV_MAXLEN 		(64)
#define DEF_URL_MAXLEN 		(64)
#define DEF_CUSTOMER_LEN	(32)

struct json_object *ha_find_config(json_object *jo,const char *name);
struct json_object *ha_find_module_config(const char *name);
void ha_config_init(const char *file);
void ha_json_array_handle(struct json_object *array,void (*func)(struct json_object *,void *),void *v);
const char *ha_json_get_string(struct json_object *jo,const char *name);
const char *ha_json_get_global_string(const char *name);
int ha_json_get_int(struct json_object *jo,const char *name);
struct json_object *ha_parse_file(const char *f);
struct json_object *ha_parse_memory(const char *mem);

void ha_config_destroy(void);

#define JO_INT_INVALID (0x7FFFFFF)

#define jo_array_string_check(_jo_,__str__,_len_,__len__) do {\
	if (!_jo_)\
		return ;\
	if (json_object_get_type(_jo_) != json_type_string)\
		return ;\
	__str__ = json_object_get_string(_jo_);\
	if (!__str__)\
		return ;\
	_len_ = strlen(__str__);\
	if (_len_ < __len__)\
		return;\
	}while(0)


#endif
