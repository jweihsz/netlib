
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


int netlib_get_openfile_max(void)
{
	int ret = 0;
	struct rlimit fileopen_limit;
	ret = getrlimit(RLIMIT_NOFILE, &fileopen_limit);
	if(ret < 0)return(0);

	return(fileopen_limit.rlim_cur);
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

void netlib__sigchild_handler(int signo)
{
    while (waitpid(-1, NULL, WNOHANG) > 0) {};
}



/*
//it is suit able for tcp and udp
//error==-1  0==timeout  right

*/
int netlib_readable_timeout(int fd,int sec)
{
	if(fd < 0)
	{
		dbg_printf("please check the socket\n");
		return(-1);
	}
	fd_set			rset;
	struct timeval	tv;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	return(select(fd+1, &rset, NULL, NULL, &tv));
 
}




inline unsigned long netlib_hash_value(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c;
    return hash;
}






int netlib_writeable_timeout(int fd, int sec)
{
	if(fd < 0)
	{
		dbg_printf("please check the socket\n");
		return(-1);
	}
	fd_set			wset;
	struct timeval	tv;
	FD_ZERO(&wset);
	FD_SET(fd, &wset);
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	return(select(fd+1, NULL, &wset, NULL, &tv));
}




int netlib_new_unix_socket(char * unix_sock_path)
{
	int  sockfd =  -1;
	int ret = -1;
	struct sockaddr_un	addr;
	
	unlink(unix_sock_path);
	
	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if(sockfd < 0 )
	{
		dbg_printf("socket  fail \n");
		goto out;
	}
	bzero(&addr, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, unix_sock_path, sizeof(addr.sun_path)-1);
	ret = bind(sockfd, (struct sockaddr *) &addr, SUN_LEN(&addr));
	if(ret < 0 )
	{
		dbg_printf("bind fail \n");
		goto out;
	}

	return(sockfd);

out:

	if(sockfd > 0 )
	{
		close(sockfd);
	}

	return(-1);
}




int netlib_connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec)
{
	int				flags, n, error;
	socklen_t		len;
	fd_set			rset, wset;
	struct timeval	tval;

	flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	error = 0;
	if ( (n = connect(sockfd, saptr, salen)) < 0)
		if (errno != EINPROGRESS) /*如果错误不是正在处理,说明是其它致命性错误*/
			return(-1);

	if (n == 0) /*局域网情况*/
		goto done;	/* connect completed immediately */

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;
	
	if ( (n = select(sockfd+1, &rset, &wset, NULL,nsec ? &tval : NULL)) == 0) /*没有活动选项，说明是真正的超时*/
	{
					 
		close(sockfd);		/* timeout */
		errno = ETIMEDOUT;
		return(-1);
	}

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) 
	{
		#if 0
		len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) /*利用该函数来获取真正的错误码*/
			return(-1);	
		#else
			char check_buff[2];
			if(read(sockfd,check_buff,0) != 0 )
			{
				return(-1);
			}
		
		#endif
	}
	else
		dbg_printf("select error: sockfd not set\n");

done:
	fcntl(sockfd, F_SETFL, flags);/*恢复成阻塞模式*/

	if (error) /*有错误*/
	{
		close(sockfd);		/* just in case */
		errno = error;
		return(-1);
	}
	return(0);
}



struct addrinfo * netlib_get_addrinfo(const char *host, const char *serv, int family, int socktype)
{
	int				n;
	struct addrinfo	hints, *res;
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;	/* always return canonical name */
	hints.ai_family = family;		/* AF_UNSPEC, AF_INET, AF_INET6, etc. */
	hints.ai_socktype = socktype;	/* 0, SOCK_STREAM, SOCK_DGRAM, etc. */
	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		return(NULL);

	return(res);
}


int  netlib_create_dir(const   char   *sPathName)  
{  
	char   DirName[256];  

	int   i=0;
	int len = 0 ;  
	strcpy(DirName,   sPathName);  

	if(DirName[len-1] !='/')  
		strcat(DirName,   "/");    
	len   =   strlen(DirName);  
   
	for(i=1;   i<len;   i++)  
	{  
		if(DirName[i]=='/')  
		{  
			DirName[i]   =   0;  
			
			if(access(DirName,   F_OK)!=0)  
			{  
				if(mkdir(DirName,   0755)==-1) 
				{   
					dbg_printf("create the dir fail \n");
					return   -1;   
				}  
			}  
			DirName[i]   =   '/';  
		}  
	}  
  	return   0;  
} 


char * netlib_get_dirname(char * dirname)
{
	if(NULL == dirname)
	{
		dbg_printf("the parem is null\n");
		return(NULL);
	}
	char * name_temp = strdup(dirname);
	int length = strlen(name_temp);
	if('/' == name_temp[length-1])
	{
		length -= 2;	
	}

    for (; length > 0; length--)
    {
        if ('/' == dirname[length])
        {
            dirname[length] = 0;
            break;
        }
    }
	return(dirname);
}



#define  WRITE_MAX_BYTE		(512)
int netlib_write_data(int fd,char * data,int length)
{
	if(fd<0 || NULL == data)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	int count = 0;
	int write_byte = 0;
	int n = 0;
	while(count < length)
	{
		write_byte = length;
		if(write_byte > WRITE_MAX_BYTE)write_byte = WRITE_MAX_BYTE;
		n = write(fd,data,write_byte);
		if(n > 0)
		{
			count += n;	
			data += n;
		}
		else
		{
			dbg_printf("some thing wrong happened \n");
			break;
		}
	}
	return(0);
}

int netlib_read_data(int fd, void *buf, int len)
{

	int n = 0;
	int count_bytes = 0;
	while(n < len)
	{
		count_bytes = read(fd,buf+n,len);
		if(count_bytes > 0 )
		{
			n += count_bytes;
		}
		else if(count_bytes < 0)
		{
            if (errno == EINTR)
            {
                continue;
            }
			else
				break;
		}

	}

	return (n);
}

	
  


int netlib_soft_rand(int min, int max)
{

	if(min >= max)
	{
		dbg_printf("please check the pram \n");
		return(-1);
	}
	static int seed = 0;
    if (seed == 0)
    {
        seed = time(NULL);
        srand(seed);
    }

	int rand_value = (int)rand()%max;
	if(rand_value < min)
		return(min + rand_value);
	return(rand_value);

}


int netlib_rand(int min,int max)
{
	static int dev_fd = -1;
	static int ret = 1;
	if(dev_fd == -1 )
	{
		dev_fd  = open("/dev/urandom", O_RDONLY);
		if(dev_fd < 0 )
		{
			ret = 0;
			dev_fd = 0;
		}
	}

	unsigned int rand_value = 0;
	if(ret == 0 )
	{
		return(netlib_soft_rand(min,max));	
	}
	else
	{
	    if (read(dev_fd, (char*)&rand_value, sizeof(rand_value)) < 0)
	    	return(-1);
	}
	int return_value = rand_value%max;
	if(rand_value < min)
		return(min + rand_value);
	
	return(return_value);

}


long netlib_get_file_szie(FILE * fp)
{
	if(NULL == fp)return(0);
	long pos = ftell(fp);
	fseek(fp,0L,SEEK_END);
	long size = ftell(fp);
	fseek(fp,pos,SEEK_SET);
	return(size);

}



int  netlib_ioctl_set_block(int sock,int nonblock)
{
	int ret = -1;
	do
	{
		ret = ioctl(sock,FIONBIO,&nonblock);
	}while(ret==-1 && errno==EINTR);

	return(ret);

}


int netlib_fcntl_set_block(int sock,int nonblock)
{

	int value;
	int ret = -1;
	do
	{
		value = fcntl(sock,F_GETFL);
	}while(value<0 && errno==EINTR);

	if(value < 0)
	{
		dbg_printf("this is wrong to get \n");
		return (-1);
	}

    if (nonblock)
    {
        value = value | O_NONBLOCK;
    }
    else
    {
        value = value & ~O_NONBLOCK;
    }

	do
	{
		ret = fcntl(sock,F_SETFL,value);
		
	}while(ret <0 && errno==EINTR );

	if(ret <0)
	{
		dbg_printf("set the fanctl fail\n");
		return(-2);
	}
	return(0);


}







