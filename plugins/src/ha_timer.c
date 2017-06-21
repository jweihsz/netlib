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


/*��ʱ����״̬*/
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

/*�Ѷ�ʱ�����·Żص��б���*/
static void ha_timer_put(struct ha_timer *tm)
{
	struct ha_timer_ctl *ctl = &ht_ctl;
	assert(tm);
	int no = tm - ctl->timers;
	assert(no >= 0 && no < DEF_TIMERS);
	alloctor_entry_put(tm,&ctl->ac,next);

	tm->st = TIMER_FREE; /*�ö�ʱ������Ϊδʹ��״̬ */
	ctl->count--;	/*����ʹ�õĶ�ʱ������*/
}

/*��ʱʱ�䵽�����ִ��*/
static void ha_timer_execute(struct ha_timer *timer)
{
	timer->running = true;
	if (timer->timeout)
		timer->timeout(timer->data);  /*ִ�ж�ʱ����Ӧ�Ļص�����*/
	timer->running = false;
	
	ha_timer_put(timer); /*�Ѷ�ʱ���Ż�ȥ*/
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

	/*���������Ҫ������ӵ�*/
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

	/*���ʱ���Ƿ��� */
	while (!heap_empty(hp)) {
		
		struct ha_timer *timer = (struct ha_timer *)heap_top(hp,false);
		if (timer->expire > ctl->tick)
			break;
		timer = (struct ha_timer *)heap_top(hp,true); /*ȡ��С���ϵ�topԪ�� */
		ha_timer_execute(timer);  /*Ȼ��ִ�������ʱ���Ļص�����*/
	}
}

/*�Ƚ�������ʱ��*/
static int timer_cmp(void *v1,void *v2)
{
	struct ha_timer *t1 = (struct ha_timer *)v1;	
	struct ha_timer *t2 = (struct ha_timer *)v2;

	return t1->expire - t2->expire;
}

/*���ö�ʱ��֪ͨ*/
static void ha_timer_notify_no(void *v,int no)
{
	struct ha_timer *tm = (struct ha_timer *)v;
	tm->no = no;
}

/*��ȡһ�����еĶ�ʱ����ʹ�� */
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

/*��ȡĿǰ����ʹ�õĶ�ʱ������ */
int ha_timer_count(void)
{
	return ht_ctl.count;
}
struct ha_timer *ha_timer_new(void *arg,void (*func)(void *),int tick
	,char *dbg_info
	)
{
	struct ha_timer_ctl *ctl = &ht_ctl;
	struct ha_timer *tm = ha_timer_get(); /*��ȡһ�����еĶ�ʱ��*/
	if (!tm)
		return tm;

	tm->data = arg; /*���� */
	tm->timeout = func; /*�ص����� */

	tm->debug_info = dbg_info;  /*������Ϣ*/
	tm->expire = ctl->tick + tick;  /*��ʱ��Ϣ */
	INIT_LIST_HEAD(&tm->locate);
	list_add_tail(&tm->locate,&ctl->wait);
	tm->st = TIMER_QUEUE; /*�Ŷ�*/

	return tm;
}

/*can not call it in tm->timeout*/
void ha_timer_del(struct ha_timer *tm)
{		
	struct ha_timer_ctl *ctl = &ht_ctl;
	if (tm->running) /*delete self*/
		return ;
	
	switch (tm->st) {
	case TIMER_QUEUE:  /*������Ŷ��У�ֱ��ɾ��*/
		list_del(&tm->locate);
	break;
	case TIMER_HEAP: /*����Ѿ����õ���С������*/
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


/*��ʼ����ʱ��*/
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
	ctl->ha_ev = ha_event_new(ctl->tmfd,EV_READ,&cb); /*��ʱ����ӵ������*/
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
	struct itimerval itval;   /*1s�д���һ��?*/
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

