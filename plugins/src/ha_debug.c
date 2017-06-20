#include <unistd.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "ha_debug.h"

#define DEBUG_BUILTTIME_FILE 	"/tmp/wWpf.build"

#define DEBUG_FILE_MAXLEN 8096

#if 1
static FILE *debug_fp = NULL;
#else
#define debug_fp stderr
#endif
static const char *buildtime = __DATE__"-"__TIME__;

static bool dbg_use = true;
/*这里是关闭调试*/
void whos_your_daddy(void)
{
	dbg_use = false;
}

/*设置创建时间*/
void set_buildtime(void)
{
	FILE *fp = fopen(DEBUG_BUILTTIME_FILE,"w+");
	if (!fp)
		return ;
	fwrite(buildtime,strlen(buildtime),1,fp);/*把创建时间保存到文件中*/
	fclose(fp);
}


#define dbg_check() do{\
	if (!dbg_use)\
		return;\
 	if (!debug_fp)\
		debug_fp = fopen(DEBUG_FILE_PATH,"w+");\
   	if (!debug_fp)\
		return ;\
	}while(0)
void dbg_print(const char *func,int line,const char *fmt,...)
{
	dbg_check();

	fprintf(debug_fp,"<%20s>[%04d]",func,line);
	va_list ap; 
	va_start(ap,fmt);
	vfprintf(debug_fp,fmt,ap);
	va_end(ap);
	fprintf(debug_fp,"\n");
	fflush(debug_fp);

	if (ftell(debug_fp) >= DEBUG_FILE_MAXLEN) {/*检测是否超过了文件的最大长度*/
		fclose(debug_fp);
		debug_fp = NULL;
	}
}
void dbg_print_c(const char *func,int line,const char *s,int len)
{
	dbg_check();

	fprintf(debug_fp,"<%20s>[%04d]",func,line);
	while (len-- > 0)
		fprintf(debug_fp,"%c",*s++);
	fprintf(debug_fp,"\n");fflush(debug_fp);
}
void dbg_dump(unsigned char *s,int n)
{
	dbg_check();
	int i;
	for (i = 0 ; i < n ; i++) {		
		fprintf(debug_fp,"%02X",s[i]);
		if (0 == (i + 1)%16)
			fprintf(debug_fp,"\n");
	}
	fprintf(debug_fp,"\n");
}

static size_t mem_cnt = 0;
void *dbg_malloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr) 
		mem_cnt++;
	return ptr;
}
void *dbg_calloc(size_t n,size_t m)
{
	void *ptr = calloc(n,m);
	if (ptr)
		mem_cnt++;
	return ptr;
}
void dbg_free(void *ptr)
{
	if (ptr) {
		free(ptr);
		mem_cnt--;
	}
}
void dbg_meminfo(const char *func,int line)
{
	dbg_check();

	fprintf(debug_fp,"<%20s>[%04d] mem_cnt = %d\n",func,line,mem_cnt);
	fflush(debug_fp);
}
