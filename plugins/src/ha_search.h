#ifndef _HA_SEARCH_H
#define _HA_SEARCH_H

#include <stdint.h>
#ifndef uint
typedef unsigned int uint;
#endif

#ifndef u8
typedef unsigned char u8;
#endif


#define USE_TS_ALGORITHM "raw"
unsigned int raw_search(const u8 *pat,unsigned int patlen,const u8 *text,unsigned int textlen,bool ign);



#ifndef ASIZE
#define ASIZE 256
#endif

struct ts_ops_s;
struct ts_conf_s;

struct ts_conf_s *ts_conf_new_s(const const u8 *pat,uint patlen,bool ign,const char *name);
void ts_conf_destory_s(struct ts_conf_s *tc);

uint ts_search_s(const struct ts_conf_s *tc,const u8 *text,uint textlen);



#endif


