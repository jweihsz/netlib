/*handle http*/
#include <string.h>

#include "list.h"

#include "ha_debug.h"
#include "ha_http.h"
#include "ha_network.h"
#include "ha_config.h"
#include "ha_search.h"
#include "ha_policy.h"

#include "common.h"
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"ha_policy:"




#define DEF_HOST_R		"zYdHost"
#define DEF_UA_R		"zYdUA"
#define DEF_URL_R		"zYdurl"
#define DEF_REF_R		"zYdreferal"
#define DEF_MAC_R		"zYdMAC"
#define DEF_CMAC_R		"zYdCMAC"
#define DEF_CSTM_R		"zYdCustomer"
#define DEF_OPT_R		"zYdOpt"
#define DEF_VER_R		"?zYdVersion"
#define DEF_CUSTOMER    "wWcx"

struct ha_policy_entry {
	const char *name;
	bool (*handler)(struct ha_http_request *,int ,struct ha_packet_info *);
	struct list_head next;
};

struct ha_policy_ctl{
	 struct list_head plist;
	 struct list_head suffix_srv;

	 bool (*spec_checker)(struct ha_http_request *,struct ha_packet_info *);
	 char customer[DEF_CUSTOMER_LEN];
	 int cuslen;

	 struct ha_http_request req;
};

static struct ha_policy_ctl p_ctl ;

static const char *http_redirect_header =
	  "HTTP/1.1 302 Moved Temporarily\r\n"
      "Location: %s://%s%s%s\r\n"
      "Content-Type: text/html; charset=iso-8859-1\r\n"
      "Content-length: 0\r\n"
      "Cache-control: no-cache\r\n"
      "Connection:close\r\n"
      "\r\n";

const char *ha_redir_header(void)
{
	return http_redirect_header;
}

static char encrypt_buf[1536];
static char escape_buf[768];

char *ha_encrypt_buf_get(void)
{
	return encrypt_buf;
}




/*产生随机种子*/
static
uint8_t ha_get_seed(const char *server,int slen)
{
	uint res = 0;
	while (slen-- > 0)
		res = res * 31 + (uint)(*server++);
	return (res%256);
}

/*这里是进行编码*/
static
bool ha_encode(const char *res, char *dest, uint8_t seed)
{
	if(!res || !dest)
		return false;
	for(; *res; ++res){
		uint8_t tmp = *res ^ seed;
		dest += sprintf(dest, "%02X", tmp);
		}
	*dest = '\0';
	return true;
}

/*对参数进行编码*/
static
bool ha_redir_arg_encode(const char *arg,const char *server)
{
	uint8_t seed = ha_get_seed(server, strlen(server)); /*获取种子参数 */
	encrypt_buf[0] = '?';
	char *encptr = encrypt_buf + 1;
	return ha_encode(arg + 1, encptr, seed);
}

/*这里是进行重定向*/
int ha_http_redirect(struct ha_packet_info *pi,char *scheme/*"http"*/,char *svr,char *url,char *arg,int fin)
{

	int n;
	char sndbuf[2048];
	if(ha_redir_arg_encode(arg, svr)){ /*这里是对参数进行了编码*/
		n = snprintf(sndbuf,sizeof(sndbuf),http_redirect_header,scheme,svr,url, ha_encrypt_buf_get( ));
		debug_p("REDIRECT %s",sndbuf);
	}else{
		n = snprintf(sndbuf,sizeof(sndbuf),http_redirect_header,scheme,svr,url, arg);
		debug_p("REDIRECT %s",sndbuf);
	}
	return (int)yitiaoming_stand(pi,sndbuf,n,fin);
}

char *ha_escap_buffer_get(void)
{
	return escape_buf;
}

bool ha_build_redir_args(struct ha_http_request *req,cstr_t *agent,const char *opt,struct ha_packet_info *pi,bool esc)
{
	int escapelen = 0;

	char *ptr = escape_buf,*bound = escape_buf + sizeof(escape_buf) - 1/*这里获取到的是长度 */;
	ptr += sprintf(ptr,"%s%s",DEF_VER_R,__WW_VERSION__);  /*版本号*/
	if (opt)  /*传递进入的参数*/
		ptr += sprintf(ptr,"%s%s",DEF_OPT_R,opt);
	else
		ptr += sprintf(ptr,"%s%s",DEF_OPT_R,"none");

	/*host地址 */
	ptr += sprintf(ptr,DEF_HOST_R);
	if (ptr + req->host->len >= bound)
		return false;
	memcpy(ptr,req->host->str,req->host->len);
	ptr += req->host->len;
#if 0
	if (ptr + sizeof(DEF_UA_R) >= bound)
		return false;
	ptr += sprintf(ptr,DEF_UA_R);
#if 1
	escapelen = http_escape(ptr,bound - ptr,agent->str,agent->len);
	if (!escapelen)
		return false;
	ptr += escapelen;
#else
		if (ptr + agent->len >= bound)
			goto end;
		memcpy(ptr,agent->str,agent->len);
		ptr += agent->len;
#endif

#endif
	/*传递用户参数 */
	if (ptr + sizeof(DEF_CSTM_R)  + p_ctl.cuslen >= bound)
		return false;
	ptr += sprintf(ptr,DEF_CSTM_R"%s",p_ctl.customer);

#if 0
	if (ptr + sizeof(DEF_REF_R) >= bound)
		return false;
	ptr += sprintf(ptr,DEF_REF_R);


	if (esc) {
		escapelen = http_escape(ptr,bound - ptr,req->refer->str,req->refer->len);
		if (!escapelen || ptr + escapelen  >= bound)
			return false;
		ptr += escapelen;
	} else {
		if (ptr + req->refer->len >= bound)
			return false;
		memcpy(ptr,req->refer->str,req->refer->len);
		ptr += req->refer->len;
	}
#endif

	/*源mac地址 */
	if (ptr + sizeof(DEF_MAC_R) + 12 >= bound)
		return false;
	ptr += sprintf(ptr,DEF_MAC_R"%02X%02X%02X%02X%02X%02X",
								pi->eth->h_dest[0],
								pi->eth->h_dest[1],
								pi->eth->h_dest[2],
								pi->eth->h_dest[3],
								pi->eth->h_dest[4],
								pi->eth->h_dest[5]);

	if (ptr + sizeof(DEF_CMAC_R) + 12 >= bound)
		return false;

	/*目的mac地址 */
	ptr += sprintf(ptr,DEF_CMAC_R"%02X%02X%02X%02X%02X%02X",
									pi->eth->h_source[0],
									pi->eth->h_source[1],
									pi->eth->h_source[2],
									pi->eth->h_source[3],
									pi->eth->h_source[4],
									pi->eth->h_source[5]);

	if (ptr + sizeof(DEF_URL_R) >= bound)
		return false;

	/*资源定位符地址 */
	ptr += sprintf(ptr,DEF_URL_R);

	if (esc) {
		escapelen = http_escape(ptr,bound - ptr,req->url.str,req->url.len);  /*进行处理*/
		if (!escapelen || ptr + escapelen >= bound)
			return false;
		ptr += escapelen;
	} else {
		if (ptr + req->url.len >= bound)
			return false;
		memcpy(ptr,req->url.str,req->url.len);
		ptr += req->url.len;
	}

	*ptr++ = '\0';
	return true;
}



static bool ignore_server_suffix(cstr_t *host,struct ha_packet_info * pi)
{
	struct ha_string_entry *pos;
	list_for_each_entry (pos,&p_ctl.suffix_srv,next) {
		if (ha_network_addr_check(pi->nh.iph->daddr)) { /*已经在非阻塞列表 */
			return true;
		}
		if (ha_string_suffix_cmp(pos,host))  /*地址对比 */
			return true;
	}

	return false;
}

static bool ha_http_30xself_check(struct ha_http_request *req,struct ha_packet_info * pi)
{
#if 0
	uint restlen = req->refer->len;
	const char *pos = req->refer->str;

	uint host_loc = raw_search((const u8 *)"http://",7,(const u8 *)pos,restlen,false);
	if (host_loc == UINT32_MAX)
		return false;
	const char *host = pos + host_loc + 7;
	uint path_loc = raw_search((const u8 *)"/",1,(const u8 *)host,req->refer->len - (host - req->refer->str),false);
	if (path_loc == UINT32_MAX)
		return false;

	if (ha_strcmp(req->host,host,path_loc)) {
		debug_p("Self 30x Found");
		ha_http_redirect(pi,"http","www.baidu.com","/","",true);
		return true;
	}
#endif
	return false;
}

static cstr_t unkown_ua = {"unkown",6};
static cstr_t refer_none = {"none",4};


/*tcp 回调处理 */
static int ha_analysis_http_request(char *text, char *end,struct ha_packet_info *pi)
{
	struct ha_policy_ctl *ctl = &p_ctl;
	struct ha_http_request *req = &ctl->req;
	int type = HTTP_IGN;
	memset(req,0,sizeof(*req));


	dbg_printf("%s\n",text);


	if (!proto_ak47_bang(text,end,req,&type))
		return -1;

	/*too long url*/

	if (type == HTTP_GET) { 
		req->host = proto_b51_shot(req,"Host",4);
		if (!req->host)
			return -1;

		/*
			HTTP Referer是header的一部分，
			当浏览器向web服务器发送请求的时候，
			一般会带上Referer，告诉服务器我是从哪个页面链接过来的，
			服务器基此可以获得一些信息用于处理

		*/
		req->refer = proto_b51_shot(req,"Referer",7);
		if (!req->refer)
			req->refer = &refer_none;

		if (ignore_server_suffix(req->host,pi))  {
			 ha_http_30xself_check(req,pi);
			 return -1;
		}

		cstr_t *ua = proto_b51_shot(req,"User-Agent",10);
		if (!ua) {
			ua = &unkown_ua;
		} else {
//			if (ignore_agent_keys(ua))
//				return -1;
		}
		req->ua = ua;

	}

	bool ret = false;
	struct ha_policy_entry *pos;
	list_for_each_entry(pos,&ctl->plist,next) {
		if (pos->handler && !pos->handler(req,type,pi)) {
			ret = true;
			break;
		}
	}

	if (!ret && ctl->spec_checker) {
		ctl->spec_checker(req,pi);
	}

	return 0;
}

void ha_register_policy_entry(const char *name,
	bool (*handler)(struct ha_http_request *,int ,struct ha_packet_info *))
{
	struct 	ha_policy_entry *entry = MALLOC(sizeof(*entry));
	if (!entry)
		return ;
	entry->name = name;
	entry->handler = handler;
	INIT_LIST_HEAD(&entry->next);
	list_add_tail(&entry->next,&p_ctl.plist);
}

void ha_policy_destroy(void)
{
	struct ha_policy_ctl *ctl = &p_ctl;
	struct ha_policy_entry *pos,*_next;
	list_for_each_entry_safe(pos,_next,&ctl->plist,next) {
		list_del(&pos->next);
		FREE(pos);
	}
	struct server_suffix *s_pos,*s_next;
	list_for_each_entry_safe(s_pos,s_next,&ctl->suffix_srv,next) {
		list_del(&s_pos->next);
		FREE(s_pos);
	}
}

/*增加需要忽略的服务器*/
static void _ignore_server_suffix_add(const char *s,size_t len)
{
	assert(s && len > 0);

	struct ha_string_entry *ss = ha_string_entry_new(s,len);
	if (!ss)
		return ;
	list_add_tail(&ss->next,&p_ctl.suffix_srv);
}

/*这也是需要忽略的服务器?*/
static void ignore_server_suffix_add(struct json_object *jo,void *v)
{
	const char *str = NULL;
	size_t len = 0;
	jo_array_string_check(jo,str,len,4);

	_ignore_server_suffix_add(str,len);
}

/*这些是需要忽略的服务器 */
void ignore_server(const char *s,size_t len)
{
	struct sockaddr_in sa;
	if (name2addr(s,&sa,NULL))
		yitiaoming_pass_ip((uint32_t)sa.sin_addr.s_addr);

	_ignore_server_suffix_add(s,len);
}

/*这是要忽略的服务器 */
static void ignore_server_add(struct json_object *jo,void *v)
{
	const char *str = NULL;
	size_t len = 0;
	jo_array_string_check(jo,str,len,4);

	ignore_server(str,len); /*对这些地址进行解析*/
}

/*策略初始化*/
void ha_policy_init(void)
{
	/**/
	struct ha_policy_ctl *ctl = &p_ctl;

	INIT_LIST_HEAD(&ctl->plist);
	INIT_LIST_HEAD(&ctl->suffix_srv);

	/*这里是设置默认的客户*/
	strncpy(ctl->customer,DEF_CUSTOMER,sizeof(ctl->customer));

	/*比如cyir*/
	const char *str = ha_json_get_global_string("customer");  /*如果配置文件里面定义了客户*/
	if (str) {
		strncpy(ctl->customer,str,sizeof(ctl->customer));  /*那么进行覆盖 */
		debug_p("customer = %s",ctl->customer);
	}

	/*要忽略的服务器*/
	struct json_object *jo = ha_find_module_config("ignore-server");
	if (jo)
		ha_json_array_handle(jo,ignore_server_add,NULL);

	jo = ha_find_module_config("ignore-suffix-srv");
	if (jo)
		ha_json_array_handle(jo,ignore_server_suffix_add,NULL);

	yitiaoming_upset(ha_analysis_http_request); /*tcp回调处理*/
}

void ha_policy_spec_register(bool (*checker)(struct ha_http_request *,struct ha_packet_info *))
{
	p_ctl.spec_checker = checker;
}
const char *ha_policy_get_http_buf(void)
{
	if (p_ctl.req.method.str)
		return p_ctl.req.method.str;
	return "http_none";
}

char *ha_policy_get_customer(void)
{
	return p_ctl.customer;
}

