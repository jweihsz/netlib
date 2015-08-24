#include "common.h"

#undef MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"04_unix_cli:"


#define	UNIXSTR_PATH	"/tmp/unix.str"


void str_cli(FILE *fp, int sockfd)
{
	int			maxfdp1, stdineof;
	fd_set		rset;
	char		buf[MAXLINE];
	int		n;

	stdineof = 0;
	FD_ZERO(&rset);
	for ( ; ; )
	{
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		select(maxfdp1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockfd, &rset))
		{
			if ( (n = read(sockfd, buf, MAXLINE)) == 0)
			{
				if (stdineof == 1)
					return;	
				else
					dbg_printf("str_cli: server terminated prematurely\n");
			}
			write(fileno(stdout), buf, n);
		}

		if (FD_ISSET(fileno(fp), &rset)) 
		{
			if ( (n = read(fileno(fp), buf, MAXLINE)) == 0) /*recv finished*/
			{
				dbg_printf("stdin is ctrl + d\n");
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);	
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			netlib_writen(sockfd, buf, n);
		}
	}
}



int  main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_un	servaddr;

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXSTR_PATH);

	connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	str_cli(stdin, sockfd);		/* do it all */

	exit(0);
}

