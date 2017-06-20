#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>

#include <sys/prctl.h>
#include <sys/wait.h>

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "list.h"

#include "ha_misc.h"
#include "ha_debug.h"
#include "ha_network.h"
#include "ha_event.h"
#include "ha_timer.h"

#include "ha_apk.h"
#include "ha_winapp.h"
#include "ha_qrCode.h"

#include "ha_http_type.h"
#include "ha_proto.h"
#include "ha_policy.h"
#include "ha_30x.h"
#include "ha_spec.h"
#include "ha_filter.h"
#include "ha_redirect.h"
#include "ha_dns.h"
#include "ha_config.h"
#include "common.h"

#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 		"main:"




static void ha_cleanup(void)
{
#if 0
    ha_apk_destroy();
#endif
	ha_filter_destroy();
    ha_http_type_destroy();

#if 1
    ha_redir_destroy();
#endif

    ha_http_ignore_destroy();
    proto_30x_destroy();
	ha_proto_destroy();
    ha_policy_destroy();

    ha_network_destroy();

    ha_timer_destroy();
    ha_event_destroy();
}

static void ha_prepare(const char *file)
{    
    ha_config_init(file); /*这里是初始化配置文件 */

    ha_event_init();
    ha_timer_init();

    ha_network_init();

    ha_policy_init();
	ha_proto_init();
    proto_30x_init();
	
    ha_http_ignore_init();
#if 1
 	ha_redir_init();
#endif

    ha_http_type_init();
	ha_filter_init();
#if 0
    ha_apk_init();
#endif

    ha_config_destroy();
}



static void nop_loop(void)
{
	for ( ; ;) {
		sleep(300);
	}
}

static char *conf_file;

static const struct option lopts[] = {
		{"log-local",no_argument,NULL,'p'},
		{"no-nop",no_argument,NULL,'n'},
		{"version",no_argument,NULL,'v'},

		{"conffile",no_argument,NULL,'f'},
		{NULL,0,NULL,0}
}; 

static void sig_handler(int sig)
{	
	debug_p("SEG__%d!!!",sig);
	ha_cleanup();
	exit(0);
}


int main(int argc,char **argv)
{
	int c = -1,index = 0;
	bool nop = true;

#if 0  /*jweih*/
#ifdef _CY_
	close(0);
	close(1);
	close(2);
#else
	daemonize();
#endif


#endif

#if 0
	signal(SIGINT,SIG_IGN);
	signal(SIGPIPE,SIG_IGN);

	signal(SIGTERM,sig_handler);
	signal(SIGKILL,sig_handler);
	signal(SIGSEGV,sig_handler);
#endif

	while ((c = getopt_long(argc,argv,"f:pnv",lopts,&index)) != -1) {
			/*check args??*/
			switch (c) {
			case 'p':	
				whos_your_daddy();
			break;
			case 'n':
				nop = false;
			break;
			case 'v':
				set_buildtime();
				return 0;
			break;
			case 'f':
				conf_file = optarg;
			break;
			}
	
		}

	#if 0   /*jweih*/
	if (nop) 
		nop_loop();
	#endif

	
	ha_prepare(conf_file);
	ha_network_ready();
	ha_event_loop();
	ha_cleanup();	
	return 0;
}


