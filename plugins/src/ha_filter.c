#if 1
#include <string.h>

#include "list.h"

#include "ha_debug.h"
#include "ha_network.h"
#include "ha_policy.h"
#include "ha_http.h"
#include "ha_http_type.h"
#include "ha_adt.h"
#include "ha_misc.h"
#include "ha_30x.h"
#include "ha_config.h"
#include "ha_search.h"

#define DEF_FILTER_URL 		"/php/data_push.php"
#define DEF_FILTER_SRV		"s.cjdgy.com"


struct ha_filter_ctl{
	char server[DEF_SRV_MAXLEN],
		 url[DEF_URL_MAXLEN];
	struct list_head h;
};

static struct ha_filter_ctl fl_ctl;


/*检测cdn主机*/
static bool ha_cdn_host_check(cstr_t *host,struct list_head *lh)
{
	struct ha_string_entry *pos;
	if (list_empty(lh))
		return false;
	
	list_for_each_entry(pos,lh,next) {
		if (UINT32_MAX != raw_search((const u8 *)pos->content.str,pos->content.len,(const u8 *)host->str,host->len,false))
			return true;
	}
	return false;
}


/*检测主机是否存在 */
static bool ha_filter_host_exsit(struct ha_http_request *req)
{
	cstr_t host;
	struct list_head *h = &fl_ctl.h;
	const char *ptr = req->url.str + 1;
	int ulen = req->url.len - 1;
	int i = 0;
	
	host.str = ptr;
	for (i = 0 ; i < ulen;i++)
		if (ptr[i]=='/')
			break;
			
	if (i < ulen) 
		host.len =  i;
	else
		return false;
	
	return ha_cdn_host_check(&host, h);
}

static bool ha_filter_comp_handler(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{	
	if (type != HTTP_GET)
		return true;

	if (!ha_filter_host_exsit(req)) 
		return true;
	
	return false;	
}
/*这里是回调处理*/
static void ha_filter_cdn_host_handle(struct json_object *jo,void *v)
{
	assert(jo);

    const char *str = NULL;
	size_t len = 0;
    jo_array_string_check(jo,str,len,2);

    struct ha_string_entry *entry = MALLOC(sizeof(*entry) + len + 1);
    if (!entry)
         return ;
    char *pos = entry->data;

    cstr_fill(pos,&entry->content,str,len);

    INIT_LIST_HEAD(&entry->next);

    struct list_head *lh = (struct list_head *)v;
    list_add_tail(&entry->next,lh); /*填充到链表尾部*/
}

/*初始化 */
static void ha_filter_conf_init(void)
{
	struct json_object *jo = NULL;
	struct ha_filter_ctl *ctl = &fl_ctl;

	strncpy(ctl->server,DEF_FILTER_SRV,sizeof(ctl->server));
	strncpy(ctl->url,DEF_FILTER_URL,sizeof(ctl->url));
	INIT_LIST_HEAD(&ctl->h);
	
	jo = ha_find_module_config("filter"); /*查找过滤模块 */
    if (!jo)
          return ;	

	struct json_object *array = NULL;
	if (jo && json_object_object_get_ex(jo,"urls",&array) && array) 
		ha_json_array_handle(array,ha_filter_cdn_host_handle,&ctl->h);
}

void ha_filter_init(void)
{
	struct ha_filter_ctl *ctl = &fl_ctl;
	
	ha_filter_conf_init();
	
	ignore_server(ctl->server,strlen(ctl->server));	
	proto_pseudo_server_register(ctl->server); /*进行注册*/
	ha_http_type_handler_register("comp-filter",ha_filter_comp_handler);
}

/*销毁退出*/
void ha_filter_destroy(void)
{	
	struct ha_filter_ctl *ctl = &fl_ctl;
	struct ha_string_entry *a_pos,*a_next;
		
	list_for_each_entry_safe(a_pos,a_next,&ctl->h,next){
		list_del(&a_pos->next);
		FREE(a_pos);
	}	
	return ;
}
#endif
