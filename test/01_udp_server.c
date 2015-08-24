#include "common.h"

#undef MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"01_udp_server:"


void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int			n;
	socklen_t	len;
	char		mesg[MAXLINE];

	for ( ; ; ) 
	{
		len = clilen;
		n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		sendto(sockfd, mesg, n, 0, pcliaddr, len);
	}
}


int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr, cliaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(9877);
	int n = 0;
	n = 220 * 1024;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));

	bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	dg_echo(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}

