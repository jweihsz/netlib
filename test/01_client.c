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


void str_cli(FILE *fp, int sockfd)
{
	char	sendline[MAXLINE], recvline[MAXLINE];

	while (fgets(sendline, MAXLINE, fp) != NULL) {

		netlib_writen(sockfd, sendline, strlen(sendline));

		if (readline(sockfd, recvline, MAXLINE) == 0)
			dbg_printf("str_cli: server terminated prematurely\n");

		fputs(recvline, stdout);
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

