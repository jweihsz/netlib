#include "common.h"

#undef MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"04_unix_server:"

#define	UNIXSTR_PATH	"/tmp/unix.str"


void sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		dbg_printf("child %d terminated\n", pid);
	return;
}

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


int main(int argc, char **argv)
{
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	int ret = -1;
	struct sockaddr_un	cliaddr, servaddr;
	
	void	sig_chld(int); /*ÉùÃ÷º¯Êý*/

	listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if(listenfd <0 )
	{
		dbg_printf("socket  fail \n");
		goto out;
	}

	unlink(UNIXSTR_PATH);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXSTR_PATH);

	ret = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if(ret < 0 )
	{
		dbg_printf("bind fail \n");
		goto out;
	}

	listen(listenfd, 1024);

	signal(SIGCHLD, sig_chld);

	for ( ; ; )
	{
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0)
		{
			if (errno == EINTR)
				continue;		
			else
				dbg_printf("accept error \n");
		}
		if ( (childpid = fork()) == 0) 
		{	
			close(listenfd);	
			str_echo(connfd);	
			exit(0);
		}
		close(connfd);			
	}

	return(0);
out:

	if(listenfd > 0 )
	{
		close(listenfd);
		listenfd = -1;
	}
	return(-1);
}

