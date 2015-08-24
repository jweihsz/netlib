#include "common.h"

#undef MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"04_unix_server:"

#define	UNIXDG_PATH		"/tmp/unix.dg"



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
	struct sockaddr_un	servaddr, cliaddr;
	sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(UNIXDG_PATH);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXDG_PATH);
	bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	dg_echo(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}

