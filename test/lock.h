#ifndef _LOCK_H
#define _LOCK_H


enum locks
{
    MUTEX_LOCK = 0,
    RW_LOCK = 1,
    SPIN_LOCK = 2,
    FILE_LOCK = 3,
    UNDEF_LOC,
};


typedef struct file_lock
{
    struct flock lock_t;
    int fd;
} file_lock_t;

typedef struct lock
{
    union
    {
        pthread_mutex_t mutex;
        pthread_rwlock_t rwlock;
        pthread_spinlock_t spinlock;
        file_lock_t filelock;
    } object;
    int (*lock)(struct lock *);
    int (*unlock)(struct lock *);
    int (*trylock)(struct lock *);
    int (*free)(struct lock *);
	
    int (*lock_rd)(struct lock *);
    int (*lock_tryrd)(struct lock *);
    int (*lock_wr)(struct lock *);
    int (*lock_trywr)(struct lock *);
} lock_t;



typedef struct cond_lock
{
    lock_t lock;
    pthread_cond_t cond;

    int (*wait)(struct cond_lock *object);
    int (*timewait)(struct cond_lock *object, long, long);
    int (*notify)(struct cond_lock *object);
    int (*broadcast)(struct cond_lock *object);
    void (*free)(struct cond_lock *object);
} cond_lock_t;



#endif  /*_LOCK_H*/