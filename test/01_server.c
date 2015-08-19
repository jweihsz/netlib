#include "common.h"

#undef MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"01_server:"


void str_echo(int sockfd)
{
	ssize_t		n;
	char		buf[MAXLINE];

again:
	while ( (n = read(sockfd, buf, MAXLINE)) > 0)
		netlib_writen(sockfd, buf, n);

	if (n < 0 && errno == EINTR)
		goto again;
	else if (n < 0)
		dbg_printf("str_echo: read error\n");
}




void sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		dbg_printf("child %d terminated\n", pid);
	return;
}





int main(int argc, char **argv)
{
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

	netlib_signal(SIGCHLD,sig_chld);
	netlib_signal(SIGPIPE,SIG_IGN);
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(9877);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, netlib_get_listen_max());

	for ( ; ; ) 
	{
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
		if(connfd < 0)
		{
			if(errno == EINTR)continue;
			else
			{
				dbg_printf("error");
				return -1;
			}
				
		}

		if ( (childpid = fork()) == 0)
		{	
			struct sockaddr  client_sockaddr;
			int ret = -1;
			ret = netlib_getpeer_name(connfd,&client_sockaddr);
			if(ret == 0 )
			{
				char * client = netlib_sock_ntop((struct sockaddr *) &client_sockaddr);
				if(NULL != client)
				dbg_printf("the client is %s\n",client);

				free(client);
				client = NULL;	
			}

			close(listenfd);	
			str_echo(connfd);	
			exit(0);
		}
		close(connfd);		
	}
}

