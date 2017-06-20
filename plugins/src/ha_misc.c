#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>

#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h> 

#include "ha_debug.h"
#include "ha_misc.h"


bool ha_strcmp(cstr_t *cs,const char *s,uint len)
{
	if (cs->len != len)
		return false;

	return (0 == strncasecmp(cs->str,s,len));  /*忽略大小写*/
}

/*比较两个字符串 */
bool ha_strequ(cstr_t *cs1,cstr_t *cs2)
{
	return ((cs1->len == cs2->len) && (0 == strncasecmp(cs1->str,cs2->str,cs1->len)));
}
#if 0
void ha_string_free(cstr_t *str)
{
	if (str->str)
		FREE(str->str);
	str->len = 0;
}
bool ha_string_copy(cstr_t *str,cstr_t *orig)
{
	char *data = CALLOC(orig->len + 1,1);
	if (!data)
		return false;
	memcpy(data,orig->str,orig->len);
	data[orig->len] = '\0';
	
	str->str = data;
	str->len = orig->len;
	return true;
}
#endif
/*block version*/

/*根据ip地址获取对方socket的信息*/
bool name2addr(const char *s,struct sockaddr_in *sa,struct addrinfo **res)
{
	struct addrinfo *ai = NULL;
	int ret,tries = 10;
	while (tries-- > 0) {
		ret = getaddrinfo(s,NULL,NULL,&ai);
		if (ret != EAI_AGAIN)
			break;
		sleep(3);
	}

	if (ret != 0)
		return false;
	
	memcpy(sa,ai->ai_addr,sizeof(*sa));
	if (!res)
		freeaddrinfo(ai);
	else 
		*res = ai;
	return true;
}
#if 1


/*获取本机mac地址*/
bool iface2mac(const char *iface,char *mac,int n)
{
	bool res = false;
	struct ifreq ifr;
	memset(&ifr,0,sizeof(ifr));
	/*fix eth0*/
	strcpy(ifr.ifr_name,iface);
	int sock = socket(AF_INET,SOCK_DGRAM,0);
	if (sock < 0)
		return res;

	if (0 > ioctl(sock,SIOCGIFHWADDR,&ifr)) 
		goto end;
	
	snprintf(mac,n,"%02X%02X%02X%02X%02X%02X",
				(unsigned char)ifr.ifr_hwaddr.sa_data[0],
				(unsigned char)ifr.ifr_hwaddr.sa_data[1],
				(unsigned char)ifr.ifr_hwaddr.sa_data[2],
				(unsigned char)ifr.ifr_hwaddr.sa_data[3],
				(unsigned char)ifr.ifr_hwaddr.sa_data[4],
				(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
	res = true;
end:
	close(sock);
	return res;
}

/*获取本机ip地址*/
uint32_t iface2ip(const char *iface)
{
	uint32_t res = 0;
	struct ifreq ifr;
	memset(&ifr,0,sizeof(ifr));
	/*fix eth0*/
	strcpy(ifr.ifr_name,iface);
	int sock = socket(AF_INET,SOCK_DGRAM,0);
	if (sock < 0)
		return res;

	if (0 > ioctl(sock,SIOCGIFADDR,&ifr)) 
		goto end;
	struct sockaddr_in *psa = (struct sockaddr_in *)&ifr.ifr_addr;
	res = psa->sin_addr.s_addr;
end:
	close(sock);
	return res;
}
#endif


/*获取ip地址的长度*/
int str_strip(char *buf)
{
	if (!buf)
		return 0;
	int buflen = strlen(buf);
	while (buflen-- > 0) {
		if (isspace(buf[buflen]))
			buf[buflen] = '\0';
		else
			break;
	}

	return buflen+1;
}
/*把套接字设置成非阻塞*/
void set_nonblock(int fd)
{
	int fl = fcntl(fd,F_GETFL);
	fcntl(fd,F_SETFL,fl|O_NONBLOCK);
}
/*close on exec, not on-fork*/
void set_closexec(int fd)
{
	int flags = fcntl(fd, F_GETFD);  
	flags |= FD_CLOEXEC;  
	fcntl(fd, F_SETFD, flags); 
}

void set_fd_default(int fd)
{
	set_nonblock(fd);
	set_closexec(fd);
}

/*计算hash值*/
uint str_hash(const char *str,int len)
{
    uint res = 0;
    while (len-- > 0)
        res = res * 31 + (uint)(*str++);
    return res;
}
void daemonize(void)
{
	pid_t pid = fork();

	switch (pid) {
	case 0:  /*子进程*/
		close(0);
		close(1);
		close(2);
	break;
	default:
		exit(-1);
	}
}
char *split_line(char *s, char *e,int sep,int end,str_t *elts,uint n,uint *res)
{
	uint i;
	bool space;
	
	for (i = 0,space = true ; s < e  && i <= n ; s++) {
		if (*s == end) 
			break;

		if (*s == sep) {
			space = true;
			continue;
		}
		
		if (space) {
			i++;
			space = false;
			elts[i-1].str = s;
			elts[i-1].len = 0;
		} 
		
		elts[i - 1].len++;		
	}

	if (res) 
		*res = i ;
	return s + 1;
}
uint ip_hash(void *v)
{
	return (uint)v;
}
int ip_cmp(void *v1,void *v2)
{
	return (int)((uint)v1 - (uint)v2);
}
void *ip_new(void *v)
{
	return v;
}

/*字符串转换成数字*/
int ha_str2int(cstr_t *cs)
{
	int res = 0;
	assert(cs);
	uint i ;
	for (i = 0 ; i < cs->len ;i++)
		if (isdigit(cs->str[i]))
			break;
		
	for (;i < cs->len ;i++) 
		res = res * 10 + cs->str[i] - '0';
	return res;
}


/*字符串入口*/
struct ha_string_entry *ha_string_entry_new(const char *str,size_t len) 
{
	struct ha_string_entry *entry = MALLOC(sizeof(*entry) + len + 1);
	if (!entry)
		return entry;
	INIT_LIST_HEAD(&entry->next);
	
	entry->content.str  = entry->data;
	strncpy(entry->data,str,len);
	entry->content.len = len;
	return entry;
}
bool ha_string_suffix_cmp(struct ha_string_entry *se,cstr_t *host)
{
	if (se->content.len > host->len)
		return false;
	return 0 == memcmp(se->content.str, /*从最后开始比较*/
						host->str + host->len - se->content.len,se->content.len);
}

void ha_signal_restore(void)
{
	int i;
	for (i = 1 ; i < SIGRTMIN ;i++)
		signal(i,SIG_DFL);
}


/*监听套接字*/
int listen_fd(struct sockaddr_in *sa,int backlog)
{
	int sk = socket(AF_INET,SOCK_STREAM,0);
	if (sk < 0)
		return sk;
	int opt = 1;
	setsockopt(sk,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	
	if (bind(sk,(struct sockaddr *)sa,sizeof(*sa))) 
		goto err;
	
	if (listen(sk,backlog))
		goto err;
	return sk;
err:
	close(sk);
	return -1;
}

