#include "common.h"

#undef MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"01_udp_time_sync:"


#define  TIME_SERVCE_NAME	"time.nist.gov"
#define	 TIME_PORT_NAME		"daytime"


typedef struct sync_time
{
	unsigned int year;
	unsigned int month;
	unsigned int date;
	unsigned int hour;
	unsigned int minute;
	unsigned int seconds;
}sync_time_t;


static sync_time_t * huiwei_time = NULL;


static int time_tcp_connect(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
	{
		return(-1);
	}		
	ressave = res;
	do 
	{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;	

		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;		

		close(sockfd);	
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)return(-2);
	
	freeaddrinfo(ressave);
	return(sockfd);
}



static int time_sync_clock(char * time_data )
{
	if(NULL == time_data)
	{
		dbg_printf("time_data is null \n");
		return(-1);
	}
	sync_time_t * timedata =huiwei_time;
	if(NULL == timedata)return(-1);
	
	int data[6];
	memset(data,0,6*sizeof(int));
	
	sscanf(time_data,"%*s%*[^0-9]%2d-%2d-%2d%*[^0-9]%2d:%2d:%2d",&data[0],&data[1],&data[2],&data[3],&data[4],&data[5]);
	int i = 0;
	for(i=0;i<6;++i)
	{
		dbg_printf("%d \n",data[i]);
	}
	/*57254 15-08-20 15:25:18 50 0 0 507.2 UTC(NIST) * */

	return(0);
}

void * time_fun(void * arg )
{
	int run_flag = -1;
	int res = -1;
	int	sockfd, n;
	char  recvline[MAXLINE + 1];
	socklen_t	len;
	struct sockaddr_storage	ss;
	sync_time_t * time_data = (sync_time_t *)arg;
	if(NULL == time_data)return(NULL);
	
	run_flag = 1;
	while(run_flag)
	{
		sockfd = time_tcp_connect(TIME_SERVCE_NAME, TIME_PORT_NAME);
		if(sockfd < 0 )
		{
			if(-2 == sockfd)
			{
				sleep(2);
				continue;
			}
			else
			{
				run_flag = 0;	
			}
		}
		while ( (n = read(sockfd, recvline, MAXLINE)) > 0)
		{
			dbg_printf("%s \n",recvline);
			recvline[n] = 0;
			time_sync_clock(recvline);
			run_flag = 0;
			break;
		}
		
	}


}

int main(int argc, char **argv)
{
	if(NULL  != huiwei_time)
	{
		dbg_printf("init over \n");
		return(-1);
	}
	huiwei_time = calloc(1,sizeof(*huiwei_time));
	if(NULL == huiwei_time)
	{
		dbg_printf("calloc fail \n");
		return(-2);
	}
	
	pthread_t time_pid;
	pthread_create(&time_pid, NULL, time_fun, huiwei_time);
	while(1)
	{

		sleep(2);
	}
}


