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


#undef TRUE
#undef FALSE
#define	TRUE	(1u)
#define	FALSE	(0u)

#define DBG_ON  		(0x01)
#define FILE_NAME 	"common:"


#define dbg_printf(fmt,arg...)		do{if(DBG_ON)fprintf(stderr,FILE_NAME"%s(line=%d)->"fmt,__FUNCTION__,__LINE__,##arg);}while(0)


#define NETLIB_LITTLE_ENDIAN		(0x01)
#define NETLIB_BIG_ENDIAN			(0x02) 

int  netlib_gethost_byteorder(void);
int netlib_sock_pton(char * net_addres, struct sockaddr *sa);
char * netlib_sock_ntop(struct sockaddr *sa);
int netlib_get_listen_max(void);
int netlib_getsock_name(int sock_fd,struct sockaddr * sa);
int netlib_getpeer_name(int sock_fd,struct sockaddr * peer);
ssize_t netlib_writen(int fd, const void *vptr, size_t n);
int netlib_signal(int signo, void (*func)(int));



#endif /*_COMMON_H*/
