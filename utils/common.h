#ifndef _COMMON_H
#define _COMMON_H


#include 	<stdio.h>
#include 	<stdlib.h>
#include	<unistd.h>
#include 	<sys/stat.h>
#include 	<sys/types.h>

#include	<sys/types.h>	
#include	<sys/socket.h>	
#include	<sys/time.h>	
#include	<time.h>		

#include	<sys/time.h>	
#include	<netinet/in.h>	
#include	<arpa/inet.h>	
#include	<errno.h>
#include	<fcntl.h>		
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	
#include	<sys/uio.h>		
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		
#include	<sys/select.h>	
#include	<sys/sysctl.h>
#include	<poll.h>	
#include	<strings.h>		
#include	<sys/ioctl.h>
#include	<pthread.h>
#include 	<sys/resource.h>

#undef TRUE
#undef FALSE
#define	TRUE	(1u)
#define	FALSE	(0u)

#define DBG_ON  		(0x01)
#define FILE_NAME 	"common:"


#define dbg_printf(fmt,arg...)		do{if(DBG_ON)fprintf(stderr,FILE_NAME"%s(line=%d)->"fmt,__FUNCTION__,__LINE__,##arg);}while(0)


#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))



#define NETLIB_LITTLE_ENDIAN		(0x01)
#define NETLIB_BIG_ENDIAN			(0x02) 

#define  EVENT_READ		(0x01<<1)
#define  EVENT_WRITE	(0x01<<2)


#define  compare_and_swap(lock,old,set)		__sync_bool_compare_and_swap(lock,old,set)
#define  fetch_and_add(value,add)			__sync_fetch_and_add(value,add)
#define	 fetch_and_sub(value,sub)			__sync_fetch_and_sub(value,sub)	


int  netlib_gethost_byteorder(void);
int netlib_sock_pton(char * net_addres, struct sockaddr *sa);
char * netlib_sock_ntop(struct sockaddr *sa);
int netlib_get_listen_max(void);
int netlib_getsock_name(int sock_fd,struct sockaddr * sa);
int netlib_getpeer_name(int sock_fd,struct sockaddr * peer);
ssize_t netlib_writen(int fd, const void *vptr, size_t n);
int netlib_signal(int signo, void (*func)(int));
int netlib_new_unix_socket(char * unix_sock_path);
int netlib_get_openfile_max(void);
int netlib_rand(int min,int max);
int netlib_fcntl_set_block(int sock,int nonblock);
int netlib_socket_wait(int fd, int timeout_ms, int events);
int netlib_set_buffer_size(int fd, int buffer_size);

#endif /*_COMMON_H*/
