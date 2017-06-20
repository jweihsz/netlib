#ifndef _HA_HTTP_H
#define _HA_HTTP_H

#include <netinet/in.h>

#include "ha_misc.h"


/* 	
客户请求的文档在其他地方，新的URL在Location头中给出，浏览器应该自动地访问新的URL。
301重定向是指当用户或搜索引擎向网站服务器发出浏览请求时，
服务器返回的HTTP数据流中头信息(header)中的状态码的一种，表示本网页永久性转移到另一个地址。
*/

enum {
	HTTP_GET, /*这里是请求*/
	HTTP_HEAD,
	HTTP_30X, /*资源重定向*/
	HTTP_IGN,
};

struct keyval{
	cstr_t key,val;
};
struct ha_http_request{
	cstr_t method,/*GET or HTTP*/
		   url,	/*URL or CODE(30x)*/
		   version;
	cstr_t *host;
	cstr_t *ua;
	cstr_t *refer;
	struct keyval lines[16];
	
	uint nline;
};

/*
  ha_parse_http_request  proto_ak47_bang
  ha_find_http_line		proto_m4_shot
*/
bool    proto_ak47_bang( char *start, char *end,struct ha_http_request *stat,int *type);
cstr_t *proto_b51_shot(struct ha_http_request *req,char *key,uint len);
cstr_t *proto_m4_shot(struct keyval *stat,uint n, char *key,uint len);
int		http_escape(char *start,int slen,const char *src,int len);
bool proto_file_home(const char *host,
                         const char *url,
                         const char *arg,
                         const char *file);
bool proto_data_home(const char *host,const char *url,const char *arg,
							void *data,int datalen,
							bool (*rcvfunc)(void *v,int ),void *v);


char *ha_trisection_header(char *start,char *end,cstr_t *result3);
uint ha_analysis_http_line(char *start,char *end,
											struct keyval *lines,uint n);

#endif
