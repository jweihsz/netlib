#include "common.h"

#undef MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"01_client:"


ssize_t readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) 
	{
again:
		if ( (rc = read(fd, &c, 1)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break;
		} else if (rc == 0) 
		{
			*ptr = 0;
			return(n - 1);	
		} else
		{
			if (errno == EINTR)
				goto again;
			return(-1);		
		}
	}

	*ptr = 0;	
	return(n);
}


void str_cli_1(FILE *fp, int sockfd)
{
	char	sendline[MAXLINE], recvline[MAXLINE];

	while (fgets(sendline, MAXLINE, fp) != NULL) {

		netlib_writen(sockfd, sendline, strlen(sendline));

		if (readline(sockfd, recvline, MAXLINE) == 0)
			dbg_printf("str_cli: server terminated prematurely\n");

		fputs(recvline, stdout);
	}
}


void str_cli_2(FILE *fp, int sockfd)
{
	int			maxfdp1;
	fd_set		rset;
	char		sendline[MAXLINE], recvline[MAXLINE];

	FD_ZERO(&rset);
	for ( ; ; )
	{
		FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		select(maxfdp1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockfd, &rset))
		{
			if (readline(sockfd, recvline, MAXLINE) == 0)
				dbg_printf("str_cli: server terminated prematurely\n");
			fputs(recvline, stdout);
		}

		if (FD_ISSET(fileno(fp), &rset))
		{ 
			if (fgets(sendline, MAXLINE, fp) == NULL)
				return;
			netlib_writen(sockfd, sendline, strlen(sendline));
		}
	}
}



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



int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 2)
		dbg_printf("usage: tcpcli <IPaddress>\n");
	netlib_signal(SIGPIPE,SIG_IGN); /*ºöÂÔSIGPIPE*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9877);
	netlib_sock_pton(argv[1],(struct  sockaddr *)&servaddr);
	connect(sockfd, (struct sockaddr* ) &servaddr, sizeof(servaddr));
	str_cli(stdin, sockfd);
	exit(0);
}

