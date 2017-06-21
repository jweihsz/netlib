#include <unistd.h>
#include <sys/epoll.h>

#include "ha_adt.h"
#include "ha_debug.h"
#include "ha_misc.h"
#include "ha_event.h"
#include "ha_const.h"

/*事件监控器*/
struct ha_event {
	struct list_head next;
	
	int fd;
	uint32_t ev;
	bool reuse; /*标识是否可以再次拿出来分配,flash表示不能拿出来使用 */
	struct ha_event_cb cbs;
};

struct ha_event_ctl{
	int epfd;
	struct epoll_event  ep_events[DEF_IO_EVENTS];
	struct ha_event 	ha_events[DEF_IO_EVENTS];
	struct alloctor 	ac_events;

	int nevent; /*正在使用的event*/
};

static struct ha_event_ctl hec_ctl;

int ha_event_fd_get(struct ha_event *ha_ev)
{
#if 0
	assert (ha_ev);
#else
	if (!ha_ev) return -1;
#endif
	return ha_ev->fd;
}

int ha_event_count(void)
{
	return hec_ctl.nevent;
}
static struct ha_event *ha_event_get(void)
{
	struct ha_event_ctl *ctl = &hec_ctl;
	struct ha_event * ev ;
	
	alloctor_entry_get(ev,&ctl->ac_events,struct ha_event,next);
	if (ev) {
		ev->reuse = false;
		ctl->nevent++;
	}
	
	return ev;
}

static void ha_event_put(struct ha_event *ev)
{
	struct ha_event_ctl *ctl = &hec_ctl;
	assert(ev);
	int no = ev - ctl->ha_events; //need to fix
	assert(no >= 0 && no < DEF_IO_EVENTS);
	
	ev->reuse = true;/*表示可以被再次使用*/
	ev->cbs.error_cb = NULL;
	ev->cbs.write_cb = NULL;
	ev->cbs.read_cb = NULL;
	alloctor_entry_put(ev,&ctl->ac_events,next); /*再次放入队列中*/
	ctl->nevent--; /*减少正在被使用的event*/
}

static void ha_event_callback(struct ha_event *ha_ev,uint32_t ev)
{
	if (ha_ev->reuse) /*进入函数的槽都应该是正在使用的*/
		return ;
	
	if (ev & EV_READ) { /*回调读*/
		assert(ha_ev->ev & EV_READ && ha_ev->cbs.read_cb);
		ha_ev->cbs.read_cb(ha_ev->cbs.arg);
	}
	if ((ha_ev->ev & EV_WRITE) && (ev & EV_WRITE)) { /*回调写*/
		assert(ha_ev->ev & EV_WRITE && ha_ev->cbs.write_cb);
		ha_ev->cbs.write_cb(ha_ev->cbs.arg);
	}

	if ((ha_ev->ev & EV_ERROR) && (ev & EV_ERROR)) { /*回调错误处理*/
		if (ha_ev->cbs.error_cb)
			ha_ev->cbs.error_cb(ha_ev->cbs.arg);
	}
}

/*获取一个新的事件槽*/
struct ha_event *ha_event_new(int fd,uint32_t init_ev,struct ha_event_cb *cb)
{
	struct ha_event *ha_ev = NULL;
	assert(cb);
	
	uint32_t ev = 0;
	if (init_ev & EV_READ)
		ev |= EV_READ;
	if (init_ev & EV_WRITE)
		ev |= EV_WRITE;
	if (init_ev & EV_ERROR)
		ev |= EV_ERROR;
	
	ha_ev = ha_event_get();
	if (!ha_ev)
		return ha_ev;
	if (!ev) 
		return ha_ev;
	
	ha_ev->cbs = *cb;
	ha_ev->fd = fd;
	
	struct epoll_event epev;
	epev.events = ev;
	epev.data.ptr = ha_ev;
	
	if (epoll_ctl(hec_ctl.epfd,EPOLL_CTL_ADD,fd,&epev)) {
		ha_event_put(ha_ev);
		ha_ev = NULL;
	} else {
		ha_ev->ev = ev;
	}
	return ha_ev;
}
uint32_t ha_event_ev_get(struct ha_event *ha_ev)
{
	return ha_ev->ev;
}

/*重新更改监控信息*/
void ha_event_enable(struct ha_event *ha_ev,uint32_t ev)
{
#if 0
	assert(ha_ev);
#else
	if (!ha_ev)
		return ;
#endif
	uint32_t orig_ev = ha_ev->ev;
	if (ev & EV_READ && ha_ev->cbs.read_cb) 
		ha_ev->ev |= EV_READ;
	
	if (ev & EV_WRITE && ha_ev->cbs.write_cb) 
		ha_ev->ev |= EV_WRITE;

	if (ev & EV_ERROR && ha_ev->cbs.error_cb)
		ha_ev->ev |= EV_ERROR;

	if (orig_ev != ha_ev->ev) {
		struct epoll_event epev;
		epev.events = ha_ev->ev;
		epev.data.ptr = ha_ev;
		if (!orig_ev) 
			epoll_ctl(hec_ctl.epfd,EPOLL_CTL_ADD,ha_ev->fd,&epev);
		else		
			epoll_ctl(hec_ctl.epfd,EPOLL_CTL_MOD,ha_ev->fd,&epev);
	}
}

/*禁止功能项*/
void ha_event_disable(struct ha_event *ha_ev,uint32_t ev)
{
#if 1

#if 0
	assert(ha_ev);
#else
	if (!ha_ev)
		return ;
#endif
	uint32_t orig_ev = ha_ev->ev;
	if (!orig_ev)
		return ;
	
	if (ev & EV_READ) 
		ha_ev->ev &= ~EV_READ;

	if (ev & EV_WRITE) 
		ha_ev->ev &= ~EV_WRITE;

	if (ev & EV_ERROR)
		ha_ev->ev &= ~EV_ERROR;

	if (orig_ev != ha_ev->ev) {
		struct epoll_event epev;
		epev.events = ha_ev->ev;
		epev.data.ptr = ha_ev;
		if (epev.events) {
			epoll_ctl(hec_ctl.epfd,EPOLL_CTL_MOD,ha_ev->fd,&epev);
		} else {
			epoll_ctl(hec_ctl.epfd,EPOLL_CTL_DEL,ha_ev->fd,NULL);
		}
	}
#endif
}

void ha_event_del(struct ha_event *ha_ev)
{
	epoll_ctl(hec_ctl.epfd,EPOLL_CTL_DEL,ha_ev->fd,NULL);
	ha_ev->reuse = true;
}
/*重新更改回调函数*/
void ha_event_cb_modify(struct ha_event *ha_ev,uint32_t ev,struct ha_event_cb *cb)
{
#if 1
	if (!ha_ev)
		return ;
#else
	assert(ha_ev && cb) ;
#endif

	if (ev & EV_READ && cb->read_cb) 
		ha_ev->cbs.read_cb = cb->read_cb;
	if (ev & EV_WRITE && cb->write_cb) 
		ha_ev->cbs.write_cb = cb->write_cb;
	if (ev & EV_ERROR && cb->error_cb) 
		ha_ev->cbs.error_cb = cb->error_cb;
}

/*事件监控器的初始化 */
void ha_event_init(void)
{
	struct ha_event_ctl *ctl = &hec_ctl;
	alloctor_init(&ctl->ac_events); /*分配池*/
	
	int fd = epoll_create(7);
	assert(fd >= 0);
	set_closexec(fd);/*close on exec, not on-fork*/
	ctl->epfd = fd;
	
	int i;
	for (i = 0 ; i < DEF_IO_EVENTS ; i++) /*空闲分配列表全部进行添加*/
		alloctor_freelist_add(&ctl->ac_events,&ctl->ha_events[i].next);
}



void ha_event_destroy(void)
{
	close(hec_ctl.epfd);
}

void ha_event_loop(void)
{
	struct ha_event_ctl *ctl = &hec_ctl;

	for ( ; ; ) {
		int n = epoll_wait(ctl->epfd,ctl->ep_events,DEF_IO_EVENTS,-1);	
		if (n <= 0 && errno == EINTR)
			continue;
		int i;
		struct epoll_event *epev;
		for (i = 0 ; i < n ; i++) {
			epev = &ctl->ep_events[i];
			struct ha_event *ha_ev = epev->data.ptr;
			ha_event_callback(ha_ev,epev->events);
		}
		
		int del = 0;
#if 0
		for (i = 0 ; i < n ; i++) {			
			epev = &ctl->ep_events[i];
			struct ha_event *ha_ev = epev->data.ptr;
			if (ha_ev->reuse) {
				ha_event_put(ha_ev);
				del++;
			}
#endif
		struct ha_event *ev_using,*_next;
		alloctor_using_visit(ev_using,_next,&ctl->ac_events,next) {
			if (ev_using->reuse) {
				ha_event_put(ev_using);
				del++;
			}
		}

//		if (del)
//			debug_p("delete %d IO Events hold %d events",del,ha_event_count());
	}
}


