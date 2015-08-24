#include "common.h"

#undef MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"02_server:"


int main(int argc, char **argv)
{
	int					i, maxi, maxfd, listenfd, connfd, sockfd;
	int					nready, client[FD_SETSIZE];
	ssize_t				n;
	fd_set				rset, allset;
	char				buf[MAXLINE];
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(9877);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, netlib_get_listen_max());
	maxfd = listenfd;		
	maxi = -1;					
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;	
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	for ( ; ; )
	{
		rset = allset;		
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		dbg_printf("nready===%d\n",nready);
		if (FD_ISSET(listenfd, &rset)) 
		{
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

			for (i = 0; i < FD_SETSIZE; i++)
			if (client[i] < 0) 
			{
				client[i] = connfd;	
				break;
			}
			
			if (i == FD_SETSIZE)
				dbg_printf("too many clients\n");

			FD_SET(connfd, &allset);
			if (connfd > maxfd)
				maxfd = connfd;
			
			if (i > maxi)
				maxi = i;
			
			if (--nready <= 0)
				continue;			
		}

		for (i = 0; i <= maxi; i++) 
		{
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset))
			{
				if ( (n = read(sockfd, buf, MAXLINE)) == 0)
				{
					dbg_printf("get the close\n");
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} 
				else
				{
					struct sockaddr  client_sockaddr;
					int ret = -1;
					ret = netlib_getpeer_name(sockfd,&client_sockaddr);
					if(ret == 0 )
					{
						char * client = netlib_sock_ntop((struct sockaddr *) &client_sockaddr);
						if(NULL != client)
						dbg_printf("the client is %s\n",client);
					
						free(client);
						client = NULL;	
					}
					netlib_writen(sockfd, buf, n);
				}
					
				
				if (--nready <= 0)
					break;
			}
		}
	}
}

