#ifndef _HA_EVENT_H
#define _HA_EVENT_H

#include <sys/epoll.h>
#include <stdbool.h>


#define EV_READ  EPOLLIN
#define EV_WRITE EPOLLOUT
#define EV_ERROR (EPOLLRDHUP|EPOLLERR)

struct ha_event_cb{
	void (*read_cb)(void *v);
	void (*write_cb)(void *v);
	void (*error_cb)(void *v);
	void *arg;
};
struct ha_event;
void ha_event_cb_modify(struct ha_event *ha_ev,uint32_t ev,struct ha_event_cb *cb);

int ha_event_fd_get(struct ha_event *ha_ev);
void ha_event_init(void);
void ha_event_loop(void);
struct ha_event *ha_event_new(int fd,uint32_t init_ev,struct ha_event_cb *cb);
	
void ha_event_del(struct ha_event *);
void ha_event_destroy(void);
void ha_event_enable(struct ha_event *ha_ev,uint32_t ev);
void ha_event_disable(struct ha_event *ha_ev,uint32_t ev);
uint32_t ha_event_ev_get(struct ha_event *ha_ev);
int ha_event_count(void);

#endif
