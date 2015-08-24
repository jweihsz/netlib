#include "common.h"

#undef MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"01_udp_client:"



static void dg_cli_1(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
	int	n;
	char	sendline[MAXLINE], recvline[MAXLINE + 1];

	while (fgets(sendline, MAXLINE, fp) != NULL) 
	{

		sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);

		n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
		recvline[n] = 0;	/* null terminate */
		fputs(recvline, stdout);
	}
}


void dg_cli_2(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
	int				n;
	char			sendline[MAXLINE], recvline[MAXLINE + 1];
	socklen_t		len;
	struct sockaddr	*preply_addr;

	preply_addr = malloc(servlen);

	while (fgets(sendline, MAXLINE, fp) != NULL)
	{

		sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);

		len = servlen;
		n = recvfrom(sockfd, recvline, MAXLINE, 0, preply_addr, &len);
		if (len != servlen || memcmp(pservaddr, preply_addr, len) != 0)
		{
			dbg_printf("this is not right\n");
			continue;
		}

		recvline[n] = 0;
		fputs(recvline, stdout);
	}
}


void dg_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
	int		n;
	char	sendline[MAXLINE], recvline[MAXLINE + 1];

	connect(sockfd, (struct sockaddr *) pservaddr, servlen);  /*!!!!!!!!!!!!*/

	while (fgets(sendline, MAXLINE, fp) != NULL) {

		write(sockfd, sendline, strlen(sendline));

		n = read(sockfd, recvline, MAXLINE);

		recvline[n] = 0;	/* null terminate */
		fputs(recvline, stdout);
	}
}




int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 2)
		dbg_printf("usage: udpcli <IPaddress>\n");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9877);
	netlib_sock_pton(argv[1],(struct  sockaddr *)&servaddr);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	dg_cli(stdin, sockfd, (struct  sockaddr *) &servaddr, sizeof(servaddr));

	exit(0);
}



