#ifndef _HA_MISC_H
#define _HA_MISC_H

#include <sys/types.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <ctype.h>
#include <netdb.h>

#include "list.h"

#ifndef uchar
typedef unsigned char uchar;
#endif

typedef struct ha_const_str_s{
	const char *str;
	uint len;
}cstr_t;
typedef struct ha_str_s{
	char *str;
	uint len;
}str_t;

struct ha_string_entry{
	struct list_head next;
	cstr_t content;
	char   data[0];
};


/*声明一个字符串 */
#define ha_string(_x_) {_x_,sizeof(_x_) - 1}
/*声明一个空字符串*/
#define ha_nullstring() {NULL,0}
/*忽略开头的空字符串*/
#define ha_strip_space(_start_,_end_)		do {for ( ; _start_ < _end_ && isspace(*_start_) ; _start_++);}while(0)
/*逆向统计*/
#define ha_strip_space_tail(_start_,_end_)  do {for ( ; _start_ > _end_ && isspace(*_end_) ; _end_--);}while(0)
/*统计非空字符串的长度*/
#define ha_grab_text(_start_,_end_)  		do {for ( ; _start_ < _end_ && !isspace(*_start_) ; _start_++);}while(0)
/*跳过结尾*/
#define ha_skip_newline(_start_,_end_) 		do{for (;_start_ < _end_ && *_start_ != '\n';_start_++);}while(0)


#define cstr_fill(_dst_ptr_,__cstr__,_src_ptr_,_src_len_) do{\
	(__cstr__)->str = _dst_ptr_;\
	(__cstr__)->len = _src_len_;\
	memcpy(_dst_ptr_,_src_ptr_,_src_len_);\
	_dst_ptr_ += _src_len_;\
	*(_dst_ptr_) = '\0';\
	_dst_ptr_++;\
}while(0)

bool ha_strcmp(cstr_t *cs,const char *s,uint len);

#if 0
void ha_string_free(cstr_t *str);
bool ha_string_copy(cstr_t *str,cstr_t *orig);
#endif

bool ha_strequ(cstr_t *cs1,cstr_t *cs2);
bool name2addr(const char *s,struct sockaddr_in *sa,struct addrinfo **ai);

int str_strip(char *buf);
uint str_hash(const char *str,int len);
void daemonize(void);
char *split_line(char *s, char *e,int sep,int end,str_t *elts,uint n,uint *res);
void set_nonblock(int fd);
void set_closexec(int fd);
void set_fd_default(int fd);
uint ip_hash(void *v);
int ip_cmp(void *v1,void *v2);
void *ip_new(void *v);
int ha_str2int(cstr_t *cs);
void ha_signal_restore(void);
struct ha_string_entry *ha_string_entry_new(const char *str,size_t len);
bool ha_string_suffix_cmp(struct ha_string_entry *se,cstr_t *host);
int listen_fd(struct sockaddr_in *sa,int backlog);

bool iface2mac(const char *iface,char *mac,int n);
uint32_t iface2ip(const char *iface);
#endif
