
#include "common.h"


#undef  DBG_ON
#undef  FILE_NAME
#define DBG_ON  		(0x01)
#define FILE_NAME 	"common:"


int em_isdir(const char * path)
{
	int ret = -1;
	if(NULL == path)return(FALSE);
	struct stat status;
	ret = lstat(path,&status);
	if(ret != 0)return(FALSE);
	return (S_ISDIR( status.st_mode ) && !S_ISLNK( status.st_mode ));
}


int em_read_number(char * filename, int * num)
{
	FILE * file = fopen( filename, "r" );
	if ( !file )
	{
		dbg_printf("fopen fail\n");
		return (-1);
	}
	if ( EOF == fscanf( file, "%d", num ))/*格式化输入*/
	{
		dbg_printf("fscanf fail\n");
		return (-2);
	}
	fclose( file );
	return 0;	
}




int  netlib_gethost_byteorder(void)
{
	union 
	{
	  short  s;
      char   c[sizeof(short)];
    } un;
	un.s = 0x0102;
	if (sizeof(short) == 2)
	{
		if (un.c[0] == 1 && un.c[1] == 2)/*big*/
			return(2);
		else if (un.c[0] == 2 && un.c[1] == 1)/*little*/
			return(1); 
		else
			return(-1);
	} 
	return(-1);
		
}


char * netlib_sock_ntop(struct sockaddr *sa)
{

	if(NULL == sa  )
	{
		dbg_printf("check the param \n");
		return(NULL);
	}
	#define  DATA_LENGTH	128
	
	char * str  = calloc(1,sizeof(char)*DATA_LENGTH);
	if(NULL == str)
	{
		dbg_printf("calloc is fail\n");
		return(NULL);
	}

	switch(sa->sa_family)
	{
		case AF_INET:
		{

			struct sockaddr_in * sin = (struct sockaddr_in*)sa;

			if(inet_ntop(AF_INET,&(sin->sin_addr),str,DATA_LENGTH) == NULL)
			{
				free(str);
				str = NULL;
				return(NULL);	
			}
			return(str);

		}
		case AF_INET6:
		{

			struct sockaddr_in6 * sin6 = (struct sockaddr_in6 *)sa;
			if(inet_ntop(AF_INET,&sin6->sin6_addr,str,DATA_LENGTH) == NULL )
			{
				free(str);
				str = NULL;
				return(NULL);		
			}
			return(str);
		}

		case AF_UNIX:
		{
			struct sockaddr_un	*unp = (struct sockaddr_un *) sa;
			if (unp->sun_path[0] == 0)
				strcpy(str, "(no pathname bound)");
			else
				snprintf(str, DATA_LENGTH, "%s", unp->sun_path);
			return(str);
		}
		default:
		{
			free(str);
			str = NULL;
			return(NULL);
		}


	}
			
    return (NULL);
}


int netlib_sock_pton(char * net_addres, struct sockaddr *sa)
{
	if(NULL == net_addres || NULL == sa)
	{
		dbg_printf("please check the param\n");
		return(-1);
	}
	switch(sa->sa_family)
	{
		case AF_INET:
		{
			struct sockaddr_in * sin = (struct sockaddr_in*)sa;
			if(inet_pton(AF_INET,net_addres,&sin->sin_addr) != 1)
				return(-1);
			else
				return(0);
		}
		case AF_INET6:
		{
			struct sockaddr_in6 * sin6 = (struct sockaddr_in6 *)sa;
			if(inet_pton(AF_INET6,net_addres,&sin6->sin6_addr) != 1)
				return(-1);
			else
				return(0);

		}
		default:
		{
			return(-1);
		}
	}
	return(-1);

}




int  netlib_sock_get_port(const struct sockaddr *sa)
{
	if(NULL == sa)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	switch (sa->sa_family)
	{
		case AF_INET: 
		{
			struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

			return(ntohs(sin->sin_port));
		}
		case AF_INET6: 
		{
			struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;
			return(ntohs(sin6->sin6_port));
		}

	}

    return(-1);
}




int  netlib_sock_cmp_addr(const struct sockaddr *sa1, const struct sockaddr *sa2)			 
{

	if(NULL ==sa1 || NULL ==  sa2)return(-1);
	if (sa1->sa_family != sa2->sa_family)return(-1);
		
	switch (sa1->sa_family)
	{
		case AF_INET: 
		{
			return(memcmp( &((struct sockaddr_in *) sa1)->sin_addr,
				&((struct sockaddr_in *) sa2)->sin_addr,sizeof(struct in_addr)));				   
		}
		case AF_INET6: 
		{
			return(memcmp( &((struct sockaddr_in6 *) sa1)->sin6_addr,
				&((struct sockaddr_in6 *) sa2)->sin6_addr, sizeof(struct in6_addr)));
					   
					  
		}
		case AF_UNIX:
		{
			return(strcmp( ((struct sockaddr_un *) sa1)->sun_path,
						   ((struct sockaddr_un *) sa2)->sun_path));
		}
	}
    return (-1);
}



		
ssize_t netlib_readn(int fd, void *vptr, size_t n)
{
	size_t	nleft;
	ssize_t	nread;
	char	*ptr;
	if(NULL == vptr || n <= 0)return(-1);
	ptr = vptr;
	nleft = n;
	while (nleft > 0) 
	{
		if ( (nread = read(fd, ptr, nleft)) < 0) 
		{
			if (errno == EINTR)
				nread = 0;		
			else
				return(-1);
		} else if (nread == 0)
			break;				
		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);
}


		
ssize_t netlib_writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t 	nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) 
		{
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		
			else
				return(-1); 
		}
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}



int netlib_get_listen_max(void)
{

	int  max = -1;
	int length = -1;
	char data[16];
	memset(data,'\0',16);
	FILE * backlog = NULL;
	backlog = fopen("/proc/sys/net/ipv4/tcp_max_syn_backlog","r");
	if(NULL == backlog)
	{
		dbg_printf("open fail\n");
		return(-1);
	}
	length = fread(data,1,sizeof(data),backlog);
	if(length<=0)return(-1);
	fclose(backlog);
	max = atoi(data);
	return(max);

}



int netlib_getsock_name(int sock_fd,struct sockaddr * sa)
{
	if(NULL ==sa || sock_fd <= 0 )return(-1);
	socklen_t len = sizeof(struct sockaddr);
	if(getsockname(sock_fd,sa,&len) < 0 )
	{
		return(-1);
	}
	return(0);
}

int netlib_getpeer_name(int sock_fd,struct sockaddr * peer)
{
	if(NULL ==peer || sock_fd <= 0 )return(-1);
	socklen_t len = sizeof(struct sockaddr);
	if(getpeername(sock_fd,peer,&len) < 0 )
	{
		return(-1);
	}
	return(0);
}



int netlib_signal(int signo, void (*func)(int))
{
	struct sigaction	act, oact;
	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) 
		act.sa_flags |= SA_RESTART;	
	if (sigaction(signo, &act, &oact) < 0)
		return(-1);
	
	return(0);
}





