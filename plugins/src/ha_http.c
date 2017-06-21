#include <unistd.h>
#include <sys/types.h> /*				*/
#include <sys/stat.h>  /*	for open	*/
#include <fcntl.h>	   /**/

#include <sys/sendfile.h>
#include <sys/socket.h>

#include <string.h>
#include <errno.h>

#include "ha_http.h"
#include "ha_debug.h"
#include "ha_misc.h"
#include "ha_search.h"

/*获取到头部有效的字符串*/
char *ha_trisection_header(char *start,char *end,cstr_t *result3)
{
	uint i;

	/*这里读取3次是因为有:method,url,version;*/
	
	for (i = 0; i < 3 ; i++) {
		ha_strip_space(start,end);
		result3[i].str = start;
		ha_grab_text(start,end);
		result3[i].len = start - result3[i].str;
	}
	return start;
}

/*对头部进行分析*/
static char *ha_analysis_http_header(char *start,char *end,struct ha_http_request *stat,int *type)
{
	char *orig_s = start;
	start = ha_trisection_header(start,end,&stat->method);
	
	if (ha_strcmp(&stat->method,"GET",3)) 
		*type = HTTP_GET;
	else if (stat->url.len > 2 && !memcmp(stat->url.str,"30",2))
		*type = HTTP_30X;
	else
		return orig_s;

	while (start < end && *start != '\n')
		start++;

	if (start >= end)
		return orig_s;
	
	return start;
}


/*这里是对key-value进行解析和获取*/
uint ha_analysis_http_line(char *start,char *end,
											struct keyval *lines,uint n)
{
	uint i;
	/*Fix:strip space*/
	for (i = 0 ; start < end && i < n ; i++) {

		/*从buf所指内存区域的前count个字节查找字符ch*/
		char *sep = memchr(start,':',end - start);
		if (!sep) /*如果没有找到*/
			goto done;
		
		ha_strip_space(start,sep);
		lines[i].key.str = start;
		ha_grab_text(start,sep);
		lines[i].key.len = start - lines[i].key.str;
		
		start = ++sep;
	
		ha_strip_space(start,end);
		lines[i].val.str = start;
//		ha_grab_text(start,end);
		ha_skip_newline(start,end);
		if (*(start - 1) == '\r')
			start--;
		
		lines[i].val.len = start - lines[i].val.str;
	}
done:
	return i;
}

/*从这里查找key的值value*/
cstr_t *proto_m4_shot(struct keyval *lines ,uint n,char *key,uint len)
{
	uint nline ;
	for (nline = 0 ; nline < n ; nline++) {
		if (ha_strcmp(&lines[nline].key,key,len)) {
			return &lines[nline].val;
		}
	}
	return NULL;
}


/*去除一些不需要的字符*/
int http_esc_reverse(char *start,int slen,const char *src,int len)
{
    int i,pos,inc;

    for (i = 0,pos = 0; pos < slen && i < len; i += (1 + inc) ) { 
        inc = 0;
        char current = src[i];

        if (src[i] ==  '%') {
            if (i + 2 < len &&  src[i+1] == '2' && src[i+2] == '0') {
                current = ' ';
                inc = 2;
            }   
        } else if (src[i] == '^') {
                if (i + 1 < len) {
                    if (src[i+1] == '^') { 
                        current = '^';
                        inc = 1;
                    } else if (src[i+1] == '$') {
                        current = '&';
                        inc = 1;
                    }   
                }   
        } else if (src[i] == '$') {
            if (i + 1 < len && src[i+1] == '$') {
                    current = '$';
                    inc = 1;
            }   
        }   
        start[pos++] = current;
    }   

    return pos;
}

/*http_escape*/
int http_escape(char *start,int slen,const char *src,int len)
{
	if (slen <= len)
		return 0;
	int i,pos;
	
	for ( i = 0 ,pos = 0; pos + 2 < slen &&  i < len ; i++) {
		switch (src[i]) {
		case ' ':
			start[pos++] = '%';
			start[pos++] = '2';
			start[pos++] = '0';
		break;
		
		case '&':
			start[pos++] = '^';
			start[pos++] = '$';
		break;
		
		case '^':
			start[pos++] = '^';
			start[pos++] = '^';
		break;

		case '$':
			start[pos++] = '$';
			start[pos++] = '$';
		break;

		default:
			start[pos++] = src[i];
		break;
		}
			
	}

	return pos;
}

cstr_t *proto_b51_shot(struct ha_http_request *req,char *key,uint len)
{
	return proto_m4_shot(req->lines,req->nline,key,len);
}

/*对截获的http数据进行分析*/
bool proto_ak47_bang(char *start,char *end,struct ha_http_request *stat,int *http_type)
{
	char *pos = ha_analysis_http_header(start,end,stat,http_type);
	if (pos == start) 
		return false;
	
	stat->nline = ha_analysis_http_line(pos,end,stat->lines,sizeof(stat->lines)/sizeof(stat->lines[0]));
	return true;
}

static char *proto_header_prompt = 
						"POST %s%s HTTP/1.0\r\n"
                        "User-Agent: Mozilla/4.0\r\n"
                        "Host:%s\r\n"
                        "Content-Type: application/x-www-form-urlencoded\r\n"
                        "Connection:Keep-Alive\r\n"
                        "Content-Length:%lu\r\n\r\n";

static bool proto_post(const char *host,
					const char *url,
					const char *arg,
					const char *data_or_file,
					uint datalen,
					bool (*rcvfunc)(void *,int),
					void *v)
{
	assert(host && url);
	
	bool res = false,file = (datalen == 0 && data_or_file);
	int fd = -1,sock = -1;
	char headbuf[512];
	int  headlen = 0;
	struct stat st;
	
	if (file) {
		fd = open(data_or_file,O_RDONLY);
	    if(fd < 0)
			return false;
	    if (0 > fstat(fd,&st))
			goto fail;

		headlen = snprintf(headbuf,sizeof(headbuf),
						proto_header_prompt,url,arg?arg:"",host,st.st_size);
	} else {
		headlen = snprintf(headbuf,sizeof(headbuf),
						proto_header_prompt,url,arg?arg:"",host,datalen);
	}

	sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock < 0)
		goto fail;
	
	set_closexec(sock);
	
	struct sockaddr_in sin;
   if (!name2addr(host,&sin,NULL))
	   goto fail;
   
   sin.sin_family = AF_INET;
   sin.sin_port = htons(80);

   struct timeval tv;
   tv.tv_sec = 2;
   tv.tv_usec = 0;
   
   setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(char *)&tv,sizeof(tv));
   setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char *)&tv,sizeof(tv));

   if (0 != connect(sock,(struct sockaddr *)&sin,sizeof(sin)))
	   goto fail;

   if (0 >= send(sock,headbuf,headlen,0))
	   goto fail;

   if (file) {
   		off_t offset ,rest = st.st_size;
		for (offset = 0; offset < st.st_size ; ) {
			ssize_t ns =  sendfile(sock,fd,&offset,rest);  /*使用send file进行发送*/
			if (ns <= 0)
				goto fail;
			rest -= ns;
		}
   	} else {
		ssize_t ns = send(sock,data_or_file,datalen,0);
		if (ns <= 0)
			goto fail;		
	}

	if (rcvfunc) {
		res = rcvfunc(v,sock);
	} else {
		char unusebuf[256];
		recv(sock,unusebuf,sizeof(unusebuf) - 1,0);
#if 0
		int nr = recv(sock,unusebuf,sizeof(unusebuf) - 1,0);
		if (nr > 0) {
			unusebuf[nr] = '\0';
			debug_p("%s",unusebuf);
		}
#endif
		res = true;
	}
fail:
	if (sock >= 0)
		close(sock);
	if (fd >= 0)
		close(fd);

	return res;
}
/*post file to server*/
bool proto_file_home(const char *host,
                         const char *url,
                         const char *arg,
                         const char *file)
{	
	return proto_post(host,url,arg,file,0,NULL,NULL);
}

/*往服务器发送数据*/
bool proto_data_home(const char *host,const char *url,const char *arg,
							void *data,int datalen,
							bool (*rcvfunc)(void *v,int ),void *v)
{
	return proto_post(host,url,arg,data,datalen,rcvfunc,v);
}

