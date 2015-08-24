#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>	
#include <sys/time.h>	
#include <time.h>		

#include <sys/time.h>	
#include <netinet/in.h>	
#include <arpa/inet.h>	
#include <errno.h>
#include <fcntl.h>		
#include <netdb.h>
#include "huiwei_common.h"
#include "anyka_com.h"


#undef  MAXLINE
#define MAXLINE   1024

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"huiwei_sync_time:"


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


static int huiwei_time_connect(const char *host, const char *serv)
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



static int huiwei_time_set(char * time_data )
{

	int count = -1;
	if(NULL == time_data)
	{
		dbg_printf("time_data is null \n");
		return(-1);
	}
	sync_time_t * timedata =huiwei_time;
	if(NULL == timedata)return(-1);
	
	count = sscanf(time_data,"%*s%*[^0-9]%2d-%2d-%2d%*[^0-9]%2d:%2d:%2d",&timedata->year,&timedata->month,&timedata->date,&timedata->hour,&timedata->minute,&timedata->seconds);
	if(count != 6)
	{
		dbg_printf("get time data fail\n");
		return(-2);
	}
	timedata->year +=  2000;

	char buff[128];
	memset(buff,'\0',128);
	snprintf(buff,128,"date -s \"%2d-%2d-%2d  %2d:%2d:%2d\" ",timedata->year,timedata->month,timedata->date,timedata->hour,timedata->minute,timedata->seconds);
	system(buff);
	sleep(1);
	system(" hwclock -w ");
	sleep(1);
	
	
	/*57254 15-08-20 15:25:18 50 0 0 507.2 UTC(NIST) * */

	return(0);
}

void * huiwei_time_fun(void * arg )
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

retry:
		sockfd = huiwei_time_connect(TIME_SERVCE_NAME, TIME_PORT_NAME);
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
			recvline[n] = 0;
			res = huiwei_time_set(recvline);
			if(0 == res )
			{
				run_flag = 0;
				break;
			}
			else
			{
				sleep(1);
				close(sockfd);
				memset(time_data,0,sizeof(*time_data));
				goto retry;
			}

		}
		
	}
	close(sockfd);
	if(NULL != time_data)
	{
		free(time_data);
		time_data = NULL;
	}
	

}

int huiwei_time_sync(void)
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
	anyka_pthread_create(&time_pid, huiwei_time_fun, (void *)huiwei_time,200*1024, -1);
	pthread_detach(time_pid);
	return(0);
		
}


