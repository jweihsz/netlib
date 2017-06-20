#ifndef _HA_DEBUG_H
#define _HA_DEBUG_H

#include <stdio.h>
#include <stdlib.h>

#if 1
#define FREE(_x)   free(_x)
#define MALLOC(_x_) malloc(_x_)
#define CALLOC(_x_,_n_) calloc(_x_,_n_)
#define REALLOC(_ptr_,_n_) realloc(_ptr_,_n_)
#define MEMINFO()

#else
void *dbg_malloc(size_t size);
void *dbg_calloc(size_t n,size_t m);
void dbg_free(void *ptr);
void dbg_meminfo(const char *func,int line);

#define FREE(__x__) 		dbg_free(__x__)
#define MALLOC(__x__) 		dbg_malloc(__x__)
#define CALLOC(__x__,__n__)	dbg_calloc(__x__,__n__)
#define MEMINFO()			dbg_meminfo(__func__,__LINE__)
#endif

#include <assert.h>
#include <errno.h>

#define DEBUG_FILE_PATH 		"/tmp/pfpf.log"
void whos_your_daddy(void);
void set_buildtime(void);
void dbg_print_c(const char *func,int line,const char *s,int len);
void dbg_dump(unsigned char *s,int n);

void dbg_print(const char *func,int line,const char *fmt,...);
#define debug_p(_fmt,...)	 dbg_print(__FILE__,__LINE__,_fmt,##__VA_ARGS__)
#define debug_pc(_s_,_len_)  dbg_print_c(__FILE__,__LINE__,_s_,_len_)
#define debug_d(_s_,_len_)   dbg_dump(_s_,_len_)
#endif
