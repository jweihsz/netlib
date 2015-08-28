#include "common.h"
#include "ringqueue.h"



#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"threadpool:"

struct threadpool;

typedef struct thread_node
{
    pthread_t tid;
    int id;
    struct threadpool *pool;
}thread_node_t;

typedef struct thread_param
{
	void *object;
	int pti;
} thread_param_t;

typedef struct threadpool
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    thread_node_t *threads;
    thread_param_t *params;
    void *ptr1;
    void *ptr2;
    ring_queue_t queue;
    int thread_num;
    int shutdown;
    volatile unsigned int task_num;
	
    void (*onStart)(struct threadpool *pool, int id);
    void (*onStop)(struct threadpool *pool, int id);
    int (*onTask)(struct threadpool *pool, void *task, int task_len);

} threadpool_t;


#define SW_THREADPOOL_QUEUE_LEN          10000

int threadpool_create(threadpool_t *pool, int thread_num)
{

	int ret = -1;
    bzero(pool, sizeof(threadpool_t));
    pool->threads = (thread_node_t *) calloc(thread_num, sizeof(thread_node_t));
	if(NULL == pool->threads)
	{
		dbg_printf("calloc fail \n");
		return(-1);
	}
    pool->params = (thread_param_t *) calloc(thread_num, sizeof(thread_param_t));

	if(NULL == pool->params)
	{
		dbg_printf("calloc fail \n");
		free(pool->threads);
		pool->threads = NULL;
		return(-2);
	}

	ret = ring_queue_init(&pool->queue, SW_THREADPOOL_QUEUE_LEN);
	if(ret < 0 )
	{
		goto fail;

	}

    pthread_mutex_init(&(pool->mutex), NULL);
    pthread_cond_init(&(pool->cond), NULL);
    pool->thread_num = thread_num;
	pool->shutdown = 0;
    return 0;

fail:

	if(NULL != pool->threads)
	{
		free(pool->threads);
		pool->threads = NULL;
	}

	if(NULL !=  pool->params)
	{
		free(pool->params);
		pool->params = NULL;
	}
	return(-1);
}


int threadpool_dispatch(threadpool_t *pool, void *task)
{
    int i, ret;
    pthread_mutex_lock(&(pool->mutex));

    for (i = 0; i < 1000; i++)
    {
        ret = ring_queue_push(&pool->queue, task);
        if (ret < 0)
        {
            usleep(i);
            continue;
        }
        else
        {
            break;
        }
    }

    pthread_mutex_unlock(&(pool->mutex));

    if (ret < 0)
    {
        return (-1);
    }
    else
    {
        volatile unsigned int *task_num = &pool->task_num;
        fetch_and_add(task_num, 1);
    }
    return pthread_cond_signal(&(pool->cond));
}



static void* threadpool_loop(void *arg)
{
	thread_param_t *param = arg;
	threadpool_t *pool = param->object;

	int id = param->pti;
	int ret;
	void *task;
    if (pool->onStart)
    {
        pool->onStart(pool, id);
    }
	

    while (1)
    {
        pthread_mutex_lock(&(pool->mutex));
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&(pool->mutex));
            dbg_printf("thread [%d] will exit\n", id);
            pthread_exit(NULL);
        }

        if (pool->task_num == 0)
        {
            pthread_cond_wait(&(pool->cond), &(pool->mutex));
        }

        dbg_printf("thread [%d] is starting to work\n", id);

        ret = ring_queue_pop(&pool->queue, &task);
        pthread_mutex_unlock(&(pool->mutex));

        if (ret >= 0)
        {
            volatile unsigned int *task_num = &pool->task_num;
            fetch_and_sub(task_num, 1);
			if(pool->onTask)
			{
				pool->onTask(pool, (void *) task, ret);
			}
            
        }
    }

    if (pool->onStop)
    {
        pool->onStop(pool, id);
    }

    pthread_exit(NULL);
    return NULL;
}


#define threadpool_thread(p,id) (&p->threads[id])
int threadpool_run(threadpool_t *pool)
{
    int i;
    for (i = 0; i < pool->thread_num; i++)
    {
        pool->params[i].pti = i;
        pool->params[i].object = pool;
        if (pthread_create(&(threadpool_thread(pool,i)->tid), NULL, threadpool_loop, &pool->params[i]) < 0)
        {
            dbg_printf("pthread_create failed. Error: %s[%d]\n", strerror(errno), errno);
            return (-1);
        }
    }
    return 0;
}


int threadpool_free(threadpool_t *pool)
{
    int i;
    if (pool->shutdown)
    {
        return -1;
    }
    pool->shutdown = 1;

    pthread_cond_broadcast(&(pool->cond));

    for (i = 0; i < pool->thread_num; i++)
    {
        pthread_join((threadpool_thread(pool,i)->tid), NULL);
    }

    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->cond));
    return 0;
}





static void *myprocess(void *arg)
{
	dbg_printf("NewTask: threadid is 0x%lx, working on task %d\n", pthread_self(), *(int *) arg);
	usleep(1000);
	return NULL;
}

int  main(void)
{
	threadpool_t pool;


	int n =10;
	threadpool_create(&pool, 4);


	
	threadpool_run(&pool);


	int *workingnum = (int *) malloc(sizeof(int) * n);
	int i;

	sleep(1);
	for (i = 0; i < n; i++)
	{
		workingnum[i] = i;
		threadpool_dispatch(&pool, &workingnum[i]);
	}

	sleep(5);
	threadpool_free(&pool);
	free(workingnum);


	return 0;
}





