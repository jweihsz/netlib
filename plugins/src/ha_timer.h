#ifndef _HA_TIMER_H
#define _HA_TIMER_H

struct ha_timer;
void ha_timer_destroy(void);
void ha_timer_init(void);
void ha_timer_del(struct ha_timer *tm);
struct ha_timer *ha_timer_new(void *arg,void (*func)(void *),int ms
	,char *dbginfo
	);
int ha_timer_expire(struct ha_timer *tm);
int ha_timer_tick(void);
int ha_timer_count(void);

#if 0
int ha_timer_get_no(struct ha_timer *tm);
void ha_timer_visit(void);

#endif

#endif

