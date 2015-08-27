#include "common.h"
#include "lock.h"

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"lock:"


static int lock_spin_lock(lock_t *lock)
{
	if(NULL == lock)return(-1);
    return pthread_spin_lock(&lock->object.spinlock);
}


static int lock_spin_unlock(lock_t *lock)
{
	if(NULL == lock)return(-1);
    return pthread_spin_unlock(&lock->object.spinlock);
}


static int lock_spin_trylock(lock_t *lock)
{
	if(NULL == lock)return(-1);
    return pthread_spin_trylock(&lock->object.spinlock);
}

static int lock_spin_free(lock_t *lock)
{
    return pthread_spin_destroy(&lock->object.spinlock);
}



static  int lock_spin_create(lock_t * lock)
{
	int ret = -1;
	if(NULL == lock)
	{
		dbg_printf("this is null \n");
		return(-1);
	}
	bzero(lock,sizeof(lock_t));

	ret = pthread_spin_init(&lock->object.spinlock,0);
	if(ret < 0 )
	{
		dbg_printf("pthread_spin_init fail\n");
		return(-2);
	}

	lock->lock = lock_spin_lock;
	lock->unlock = lock_spin_unlock;
	lock->trylock = lock_spin_trylock;
	lock->free = lock_spin_free;

	return(0);
}



static  int lock_mutex_lock(lock_t * lock)
{
	if(NULL == lock)return(-1);
	return(pthread_mutex_lock(&lock->object.mutex));
}


static  int lock_mutex_unlock(lock_t * lock)
{
	if(NULL == lock)return(-1);
	return(pthread_mutex_unlock(&lock->object.mutex));
}


static  int lock_mutex_trylock(lock_t * lock)
{
	if(NULL == lock)return(-1);
	return(pthread_mutex_trylock(&lock->object.mutex));
}


static int lock_mutex_free(lock_t * lock)
{
	if(NULL == lock)return(-1);
    return (pthread_mutex_destroy(&lock->object.mutex));
}


static int lock_mutex_create(lock_t * lock)
{
	int ret = -1;
	if(NULL == lock)
	{
		dbg_printf("this is null \n");
		return(-1);
	}
	bzero(lock,sizeof(lock_t));

	ret = pthread_mutex_init(&lock->object.mutex,NULL);
	if(ret < 0 )
	{
		dbg_printf("pthread_mutex_init fail\n");
		return(-2);
	}

	lock->lock = lock_mutex_lock;
	lock->unlock = lock_mutex_unlock;
	lock->trylock = lock_mutex_trylock;
	lock->free = lock_mutex_free;

	return(0);
}


static int lock_rwlock_rdlock(lock_t * lock)
{
	if(NULL == lock)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	return pthread_rwlock_rdlock(&lock->object.rwlock);
}

static int lock_rwlock_wrlock(lock_t * lock)
{
	if(NULL == lock)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	return pthread_rwlock_wrlock(&lock->object.rwlock);
}


static int lock_rwlock_unlock(lock_t * lock)
{
	if(NULL == lock)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	return pthread_rwlock_unlock(&lock->object.rwlock);
}


static int lock_rwlock_tryrdlock(lock_t * lock)
{
	if(NULL == lock)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	return pthread_rwlock_tryrdlock(&lock->object.rwlock);
}

static int lock_rwlock_trywrlock(lock_t * lock)
{
	if(NULL == lock)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	return pthread_rwlock_trywrlock(&lock->object.rwlock);
}


static int lock_rwlock_free(lock_t * lock)
{
	if(NULL == lock)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	return pthread_rwlock_destroy(&lock->object.rwlock);
}




static  int lock_rwlock_create(lock_t * lock)
{
	int ret = -1;
	if(NULL == lock)
	{
		dbg_printf("this is null \n");
		return(-1);
	}
	bzero(lock,sizeof(lock_t));

	ret = pthread_rwlock_init(&lock->object.spinlock,NULL);
	if(ret < 0 )
	{
		dbg_printf("pthread_rwlock_init fail\n");
		return(-2);
	}

	lock->lock_rd = lock_rwlock_rdlock;
	lock->lock_wr = lock_rwlock_wrlock;
	lock->lock_tryrd = lock_rwlock_tryrdlock;
	lock->lock_trywr = lock_rwlock_trywrlock;
	lock->unlock = lock_rwlock_unlock;
	lock->free = lock_rwlock_free;

	return(0);
}



static int lock_filelock_rdlock(lock_t *lock)
{
	lock->object.filelock.lock_t.l_type = F_RDLCK;
    return fcntl(lock->object.filelock.fd, F_SETLKW, &lock->object.filelock);
}

static int lock_filelock_wrlock(lock_t *lock)
{
    lock->object.filelock.lock_t.l_type = F_WRLCK;
    return fcntl(lock->object.filelock.fd, F_SETLKW, &lock->object.filelock);
}

static int lock_filelock_unlock(lock_t *lock)
{
    lock->object.filelock.lock_t.l_type = F_UNLCK;
    return fcntl(lock->object.filelock.fd, F_SETLKW, &lock->object.filelock);
}

static int lock_filelock_trywrlock(lock_t *lock)
{
    lock->object.filelock.lock_t.l_type = F_WRLCK;
    return fcntl(lock->object.filelock.fd, F_SETLK, &lock->object.filelock);
}

static int lock_filelock_tryrdlock(lock_t *lock)
{
    lock->object.filelock.lock_t.l_type = F_RDLCK;
    return fcntl(lock->object.filelock.fd, F_SETLK, &lock->object.filelock);
}

static int lock_filelock_free(lock_t *lock)
{
    return close(lock->object.filelock.fd);
}


int lock_filelock_create(lock_t *lock, int fd)
{
    bzero(lock, sizeof(lock_t));
    lock->object.filelock.fd = fd;


	lock->lock_rd = lock_filelock_rdlock;
	lock->lock_wr = lock_filelock_wrlock;
	lock->lock_tryrd = lock_filelock_tryrdlock;
	lock->lock_trywr = lock_filelock_trywrlock;
	lock->unlock = lock_filelock_unlock;
	lock->free = lock_filelock_free;
	
    return 0;
}




static int cond_lock_notify(cond_lock_t *cond)
{
    return pthread_cond_signal(&cond->cond);
}

static int cond_lock_broadcast(cond_lock_t *cond)
{
    return pthread_cond_broadcast(&cond->cond);
}

static int cond_lock_timewait(cond_lock_t *cond, long sec, long nsec)
{
    struct timespec timeo;

    timeo.tv_sec = sec;
    timeo.tv_nsec = nsec;

    return pthread_cond_timedwait(&cond->cond, &cond->lock.object.mutex, &timeo);
}

static int cond_lock_wait(cond_lock_t *cond)
{
    return pthread_cond_wait(&cond->cond, &cond->lock.object.mutex);
}

static void cond_lock_free(cond_lock_t *cond)
{
    pthread_cond_destroy(&cond->cond);
    cond->lock.free(&cond->lock);
}


int cond_lock_create(cond_lock_t *cond)
{
    if (pthread_cond_init(&cond->cond, NULL) < 0)
    {
		return(-1);
    }
    if (lock_mutex_create(&cond->lock, 0) < 0)
    {
        return (-2);
    }
    cond->notify = cond_lock_notify;
    cond->broadcast = cond_lock_broadcast;
    cond->timewait = cond_lock_timewait;
    cond->wait = cond_lock_wait;
    cond->free = cond_lock_free;

    return 0;
}





