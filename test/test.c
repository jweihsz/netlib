#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include "common.h"
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


#undef  DBG_ON
#undef  FILE_NAME
#define DBG_ON  		(0x01)
#define FILE_NAME 	"test:"





int main(void)
{

#if 0
	int ret = -1;
	ret = netlib_gethost_byteorder();
	if(ret == NETLIB_LITTLE_ENDIAN)
	{
		dbg_printf("little\n");
	}
	else if(ret == NETLIB_BIG_ENDIAN)
	{
		dbg_printf("big\n");
	}
	else
	{
		dbg_printf("unknow\n");
	}
#endif



#if 0
	
	struct sockaddr_in test;
	int ret = -1;

	test.sin_family = AF_INET;
	ret = netlib_sock_pton("192.168.1.1", (struct sockaddr*)&test);
	if(ret == 0)
	{
		char * addres = netlib_sock_ntop((struct sockaddr*)&test);
		if(NULL != addres)
			dbg_printf("the addres is %s\n",addres);
	}

#endif


#if 0
	dbg_printf("the listen max is %d \n",netlib_get_listen_max());

#endif


#if 0
	netlib_new_unix_socket("/tmp/hello");

#endif

#if 0
	struct addrinfo	*ai;
	ai = netlib_get_addrinfo("www.baidu.com", NULL, 0, 0);
	if(NULL != ai)
	{
		dbg_printf("the addres is %s \n",netlib_sock_ntop(ai->ai_addr));
		
	}

#endif

#if 0

	dbg_printf("the open file num is %d \n",netlib_get_openfile_max());
#endif


#if 1
	while(1)
	{
		dbg_printf("the rand value is %d \n",netlib_rand(0,100));
		sleep(1);
	}

#endif

	return(0);
}
