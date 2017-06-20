#include <stdbool.h>
#include <string.h>

#include "list.h"

#include "ha_search.h"
#include "ha_http.h"
#include "ha_network.h"
#include "ha_policy.h"

#include "ha_debug.h"
#include "ha_config.h"

/*这是要处理的类型*/
struct ha_http_type_handler{
	struct list_head next;
	const char *name;
	bool (*handler)(struct ha_http_request *,int ,struct ha_packet_info*);
};

/*这是需要忽略的类型*/
struct ha_http_type_ignore{
	struct list_head next;
	const char *name;
	uint namlen;
};

struct ha_http_type_ctl{
	struct list_head focus_list;
	struct list_head ignore_list;
};

static struct ha_http_type_ctl hth_ctl;

/*这里是尽心注册 */
void ha_http_type_handler_register(const char *s,
										bool (*handler)(struct ha_http_request *,int ,struct ha_packet_info*))
{
	assert(s && handler);
	struct ha_http_type_handler *hth = MALLOC(sizeof(*hth));
	if (!hth)
		return ;
	INIT_LIST_HEAD(&hth->next);
	hth->name = s;
	hth->handler = handler;

	list_add_tail(&hth->next,&hth_ctl.focus_list);
}

/*需要处理类型的回调函数*/
static bool ha_type_handler(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{
	if (type != HTTP_GET)
		return true;
	
	struct ha_http_type_handler *h_pos;
	list_for_each_entry(h_pos,&hth_ctl.focus_list,next) {
		if (!h_pos->handler(req,type,pi))
			return false;
	}

	return true;
}

/*需要忽略的回调函数 */
static bool ha_type_ignore(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{
	struct ha_http_type_ignore *i_pos;
	list_for_each_entry (i_pos,&hth_ctl.ignore_list,next) {
#if 1
		if (UINT32_MAX != raw_search((const u8 *)i_pos->name,i_pos->namlen,(const u8 *)req->url.str,req->url.len,false)) {
			return false;
		}
			
#else
		if (req->url.len < i_pos->namlen)
			continue;

		if (0 == memcmp(req->url.str + req->url.len - i_pos->namlen,i_pos->name,i_pos->namlen))
			return false;
#endif		
	}

	return true;
}

/*增加不需要处理的类型*/
static void ignore_type_add(struct json_object *jo,void *v)
{
	const char *str = NULL;
	size_t len = 0;
	jo_array_string_check(jo,str,len,2);
	
	struct list_head *h = (struct list_head *)v;
	struct ha_string_entry *entry = MALLOC(sizeof(*entry) + len + 1);
	if (!entry)
		return ;
	INIT_LIST_HEAD(&entry->next);

	char *ptr = entry->data;
	cstr_fill(ptr,&entry->content,str,len);

	list_add_tail(&entry->next,h);
}

void ha_http_ignore_init(void)
{
	struct ha_http_type_ctl *ctl = &hth_ctl;
	INIT_LIST_HEAD(&ctl->ignore_list);
	/*configure*/
	struct json_object	*jo = ha_find_module_config("ignore-type");
	if (jo) 
		ha_json_array_handle(jo,ignore_type_add,&ctl->ignore_list);
	
	ha_register_policy_entry("type_ignore",ha_type_ignore);  /*注册一种策略*/
}

void ha_http_ignore_destroy(void)
{
	struct ha_http_type_ignore *i_pos,*i_next;

	list_for_each_entry_safe(i_pos,i_next,&hth_ctl.ignore_list,next) {
		list_del(&i_pos->next);
		FREE(i_pos);
	}
}

/*这是需要处理的类型*/
void ha_http_type_init(void)
{
	struct ha_http_type_ctl *ctl = &hth_ctl;
	INIT_LIST_HEAD(&ctl->focus_list);

	ha_register_policy_entry("type_handler",ha_type_handler); /*注册一种策略*/
}


void ha_http_type_destroy(void)
{	
	struct ha_http_type_ctl *ctl = &hth_ctl;
	struct ha_http_type_handler *h_pos,*h_next;
	list_for_each_entry_safe(h_pos,h_next,&ctl->focus_list,next) {
		list_del(&h_pos->next);
		FREE(h_pos);
	}
}
