#include <unistd.h>

#include <sys/timerfd.h>
#include <time.h>
#include <string.h>

#if defined(NO_EPOLL)
#include <sys/time.h>
#include <sys/signal.h>
#endif

#include "ha_debug.h"
#include "ha_adt.h"
#include "ha_event.h"
#include "ha_const.h"


/*定时器的状态*/
enum timer_state{
	TIMER_FREE,
	TIMER_QUEUE,
	TIMER_HEAP,
	TIMER_ALONE,
};

struct ha_timer {
	struct list_head next;
	struct list_head locate;
	enum timer_state st;
	bool running;
	
	int no;
	int expire;
	void *data;
	void (*timeout)(void *);
	
	char *debug_info;
};

struct ha_timer_ctl{
#if !defined(NO_EPOLL)
	int tmfd;
#endif
	int tick;
	struct ha_timer timers[DEF_TIMERS];
	struct alloctor ac;
	struct heap *th;
	struct ha_event *ha_ev;
	struct list_head wait;

	int count;
};

static struct ha_timer_ctl ht_ctl;

/*把定时器重新放回到列表中*/
static void ha_timer_put(struct ha_timer *tm)
{
	struct ha_timer_ctl *ctl = &ht_ctl;
	assert(tm);
	int no = tm - ctl->timers;
	assert(no >= 0 && no < DEF_TIMERS);
	alloctor_entry_put(tm,&ctl->ac,next);

	tm->st = TIMER_FREE; /*该定时器设置为未使用状态 */
	ctl->count--;	/*正在使用的定时器减少*/
}

/*定时时间到后进行执行*/
static void ha_timer_execute(struct ha_timer *timer)
{
	timer->running = true;
	if (timer->timeout)
		timer->timeout(timer->data);  /*执行定时器对应的回调函数*/
	timer->running = false;
	
	ha_timer_put(timer); /*把定时器放回去*/
#if 0
	if (timer->debug_info)
		debug_p("tm = %p %p",timer,timer->debug_info);
#endif
}

static void ha_timer_process(void *v)
{
	struct ha_timer_ctl *ctl = (struct ha_timer_ctl *)v;
#if !defined(NO_EPOLL)
	uint64_t cnt;
	read(ctl->tmfd,&cnt,sizeof(cnt));
#endif
	ctl->tick++;

	/*如果还有需要进行添加的*/
	struct heap *hp = ctl->th;
	if (!list_empty(&ctl->wait)) {
		struct ha_timer *pos,*_next;
		list_for_each_entry_safe(pos,_next,&ctl->wait,locate) {
			list_del(&pos->locate);
			if (heap_add(hp,pos))
				pos->st = TIMER_HEAP;
			else
				ha_timer_put(pos);
		}
	}

	/*检测时间是否到了 */
	while (!heap_empty(hp)) {
		
		struct ha_timer *timer = (struct ha_timer *)heap_top(hp,false);
		if (timer->expire > ctl->tick)
			break;
		timer = (struct ha_timer *)heap_top(hp,true); /*取最小堆上的top元素 */
		ha_timer_execute(timer);  /*然后执行这个定时器的回调函数*/
	}
}

/*比较两个定时器*/
static int timer_cmp(void *v1,void *v2)
{
	struct ha_timer *t1 = (struct ha_timer *)v1;	
	struct ha_timer *t2 = (struct ha_timer *)v2;

	return t1->expire - t2->expire;
}

/*设置定时器通知*/
static void ha_timer_notify_no(void *v,int no)
{
	struct ha_timer *tm = (struct ha_timer *)v;
	tm->no = no;
}

/*获取一个空闲的定时器供使用 */
static struct ha_timer *ha_timer_get(void)
{
	struct ha_timer_ctl *ctl = &ht_ctl;
	struct ha_timer *tm;
	alloctor_entry_get(tm,&ctl->ac,struct ha_timer,next);
	if (tm) {
		assert(tm->st == TIMER_FREE);
		assert(tm->running == false);
		ctl->count++;
	}
	return tm;
}

/*获取目前正在使用的定时器数量 */
int ha_timer_count(void)
{
	return ht_ctl.count;
}
struct ha_timer *ha_timer_new(void *arg,void (*func)(void *),int tick
	,char *dbg_info
	)
{
	struct ha_timer_ctl *ctl = &ht_ctl;
	struct ha_timer *tm = ha_timer_get(); /*获取一个空闲的定时器*/
	if (!tm)
		return tm;

	tm->data = arg; /*参数 */
	tm->timeout = func; /*回调函数 */

	tm->debug_info = dbg_info;  /*调试信息*/
	tm->expire = ctl->tick + tick;  /*超时信息 */
	INIT_LIST_HEAD(&tm->locate);
	list_add_tail(&tm->locate,&ctl->wait);
	tm->st = TIMER_QUEUE; /*排队*/

	return tm;
}

/*can not call it in tm->timeout*/
void ha_timer_del(struct ha_timer *tm)
{		
	struct ha_timer_ctl *ctl = &ht_ctl;
	if (tm->running) /*delete self*/
		return ;
	
	switch (tm->st) {
	case TIMER_QUEUE:  /*如果在排队中，直接删除*/
		list_del(&tm->locate);
	break;
	case TIMER_HEAP: /*如果已经放置到最小堆中了*/
		heap_lookup(ctl->th,tm->no,true);
	break;
	default:
		return ;
	break;
	}
	ha_timer_put(tm);
}
#if defined(NO_EPOLL)
static void sig_alarm_handler(int sig)
{
	ha_timer_process(&ht_ctl);
}
#endif


/*初始化定时器*/
void ha_timer_init(void)
{
	struct ha_timer_ctl *ctl = &ht_ctl;
	alloctor_init(&ctl->ac);

	INIT_LIST_HEAD(&ctl->wait);
	
	ctl->th = heap_new(DEF_TIMERS + 1,NULL,timer_cmp,ha_timer_notify_no);
	assert(ctl->th);
	
#if !defined(NO_EPOLL)	
	ctl->tmfd = timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
	assert(ctl->tmfd >= 0);
	struct ha_event_cb cb = {
		.read_cb = ha_timer_process,
		.arg = ctl,
	};
	ctl->ha_ev = ha_event_new(ctl->tmfd,EV_READ,&cb); /*定时器添加到监控中*/
	assert(ctl->ha_ev);
	
#endif
	
	int i;
	for (i = 0 ; i < DEF_TIMERS; i++) {
		alloctor_freelist_add(&ctl->ac,&ctl->timers[i].next);
		ctl->timers[i].st = TIMER_FREE;
	}
	
#if !defined(NO_EPOLL)
	struct itimerspec itmp;
	long sec = DEF_TICK_MS * 1000000/1000000000;
	long nsec = DEF_TICK_MS * 1000000%1000000000;
	
	itmp.it_interval.tv_nsec = nsec;
	itmp.it_interval.tv_sec =sec;
	
	clock_gettime(CLOCK_MONOTONIC,&itmp.it_value);
	itmp.it_value.tv_nsec += nsec;
	itmp.it_value.tv_sec += sec + itmp.it_value.tv_nsec/1000000000;
	itmp.it_value.tv_nsec = itmp.it_value.tv_nsec%1000000000;

	timerfd_settime(ctl->tmfd,TFD_TIMER_ABSTIME,&itmp,NULL);
#else
	struct itimerval itval;   /*1s中处理一次?*/
	long sec = DEF_TICK_MS * 1000/1000000;
	long usec = DEF_TICK_MS * 1000%1000000;
	itval.it_interval.tv_sec = sec;
	itval.it_interval.tv_usec = usec;

	itval.it_value.tv_sec = sec;
	itval.it_value.tv_usec = usec;

	signal(SIGALRM,sig_alarm_handler);
	setitimer(ITIMER_REAL,&itval,NULL);
#endif
}

void ha_timer_destroy(void)
{	
	struct ha_timer_ctl *ctl = &ht_ctl;

	ha_event_del(ctl->ha_ev);
#if !defined(NO_EPOLL)
	close(ctl->tmfd);
#endif

	heap_destroy(ctl->th);	
}

int ha_timer_expire(struct ha_timer *tm)
{
	return tm->expire;
}
int ha_timer_tick(void)
{
	return ht_ctl.tick;
}

