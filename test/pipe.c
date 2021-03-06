#include "common.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"pipe:"


typedef struct pipe_fun
{
    void *object;
    int blocking;
    double timeout;

    int (*read)(struct pipe_fun *, void *recv, int length);
    int (*write)(struct pipe_fun *, void *send, int length);
    int (*getFd)(struct pipe_fun *, int isWriteFd);
    int (*close)(struct pipe_fun *);
} pipe_fun_t;



typedef struct pipe_base
{
    int pipes[2];
} pipe_base_t;



static int pipe_read(pipe_fun_t *p, void *data, int length)
{
    pipe_base_t *object = p->object;
    if (p->blocking == 1 && p->timeout > 0)
    {
        if (netlib_socket_wait(object->pipes[0], p->timeout * 1000, EVENT_READ) < 0)
        {
            return -1;
        }
    }
    return read(object->pipes[0], data, length);
}

static int pipe_write(pipe_fun_t *p, void *data, int length)
{
    pipe_base_t *this = p->object;
    return write(this->pipes[1], data, length);
}

static int pipe_getFd(pipe_fun_t *p, int isWriteFd)
{
    pipe_base_t *this = p->object;
    return (isWriteFd == 0) ? this->pipes[0] : this->pipes[1];
}

static int pipe_close(pipe_fun_t *p)
{
    int ret1, ret2;
    pipe_base_t *this = p->object;
    ret1 = close(this->pipes[0]);
    ret2 = close(this->pipes[1]);
    free(this);
    return 0 - ret1 - ret2;
}


int pipe_create(pipe_fun_t *p, int blocking)
{
    int ret;
    pipe_base_t *object = malloc(sizeof(pipe_base_t));
    if (object == NULL)
    {
        return -1;
    }
    p->blocking = blocking;
    ret = pipe(object->pipes);
    if (ret < 0)
    {
        return -1;
    }
    else
    {

        netlib_fcntl_set_block(object->pipes[0],1);
        netlib_fcntl_set_block(object->pipes[1],1);
        p->timeout = -1;
        p->object = object;
        p->read = pipe_read;
        p->write = pipe_write;
        p->getFd = pipe_getFd;
        p->close = pipe_close;
    }
    return 0;
}



typedef struct  pipe_eventfd
{
    int event_fd;
} pipe_eventfd_t;



static int pipe_eventfd_read(pipe_fun_t *p, void *data, int length)
{
    int ret = -1;
    pipe_eventfd_t *object = p->object;

    if (p->blocking == 1 && p->timeout > 0)
    {
        if (netlib_socket_wait(object->event_fd, p->timeout * 1000, EVENT_READ) < 0)
        {
            return -1;
        }
    }

    while (1)
    {
        ret = read(object->event_fd, data, sizeof(uint64_t));
        if (ret < 0 && errno == EINTR)
        {
            continue;
        }
        break;
    }
    return ret;
}

static int pipe_eventfd_write(pipe_fun_t *p, void *data, int length)
{
    int ret;
    pipe_eventfd_t *this = p->object;
    while (1)
    {
        ret = write(this->event_fd, data, sizeof(uint64_t));
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
        }
        break;
    }
    return ret;
}

static int pipe_eventfd_getFd(pipe_fun_t *p, int isWriteFd)
{
    return ((pipe_eventfd_t *) (p->object))->event_fd;
}

static int pipe_eventfd_close(pipe_fun_t *p)
{
    int ret;
    ret = close(((pipe_eventfd_t *) (p->object))->event_fd);
    free(p->object);
    return ret;
}

int pipe_eventfd_create(pipe_fun_t *p, int blocking, int semaphore, int timeout)
{
    int efd;
    int flag = 0;
    pipe_eventfd_t *object = malloc(sizeof(pipe_eventfd_t));
    if (object == NULL)
    {
        return -1;
    }

    flag = EFD_NONBLOCK;

    if (blocking == 1)
    {
        if (timeout > 0)
        {
            flag = 0;
            p->timeout = -1;
        }
        else
        {
            p->timeout = timeout;
        }
    }

    p->blocking = blocking;
    efd = eventfd(0, flag);
    if (efd < 0)
    {
        return -1;
    }
    else
    {
        p->object = object;
        p->read = pipe_eventfd_read;
        p->write = pipe_eventfd_write;
        p->getFd = pipe_eventfd_getFd;
        p->close = pipe_eventfd_close;
        object->event_fd = efd;
    }
    return 0;
}


typedef struct pipe_unsock
{
    int socks[2];
} pipe_unsock_t;

static int swPipeUnsock_getFd(pipe_fun_t *p, int isWriteFd)
{
    pipe_unsock_t *this = p->object;
    return isWriteFd == 1 ? this->socks[1] : this->socks[0];
}

static int swPipeUnsock_close(pipe_fun_t *p)
{
    int ret1, ret2;
    pipe_unsock_t *object = p->object;

    ret1 = close(object->socks[0]);
    ret2 = close(object->socks[1]);

    free(object);

    return 0 - ret1 - ret2;
}


static int swPipeUnsock_read(pipe_fun_t *p, void *data, int length)
{
    return read(((pipe_unsock_t *) p->object)->socks[0], data, length);
}

static int swPipeUnsock_write(pipe_fun_t *p, void *data, int length)
{
    return write(((pipe_unsock_t *) p->object)->socks[1], data, length);
}


int swPipeUnsock_create(pipe_fun_t *p, int blocking, int protocol)
{
    int ret;
    pipe_unsock_t *object = malloc(sizeof(pipe_unsock_t));
    if (object == NULL)
    {
        return -1;
    }
    p->blocking = blocking;
    ret = socketpair(AF_UNIX, protocol, 0, object->socks);
    if (ret < 0)
    {
        return -2;
    }
    else
    {

        if (blocking == 0)
        {
            netlib_fcntl_set_block(object->socks[0],1);
            netlib_fcntl_set_block(object->socks[1],1);
        }

        int sbsize = 1024;
        netlib_set_buffer_size(object->socks[0], sbsize);
        netlib_set_buffer_size(object->socks[1], sbsize);

        p->object = object;
        p->read = swPipeUnsock_read;
        p->write = swPipeUnsock_write;
        p->getFd = swPipeUnsock_getFd;
        p->close = swPipeUnsock_close;
    }
    return 0;
}







typedef struct queue_data
{
    long mtype; 
    char mdata[10]; 
} queue_data_t;

typedef struct queue_msg
{
    int msg_id;
    int ipc_wait;
    uint8_t delete;
    long type;
} queue_msg_t;


typedef struct queue
{
    void *object;
    int blocking;
    int (*in)(struct queue *, queue_data_t *in, int data_length);
    int (*out)(struct queue *, queue_data_t *out, int buffer_length);
    void (*free)(struct queue *);
    int (*notify)(struct queue *);
    int (*wait)(struct queue *);
} queue_t;


void queue_msg_free(queue_t *p)
{
    queue_msg_t *object = p->object;
    if (object->delete)
    {
        msgctl(object->msg_id, IPC_RMID, 0);
    }
    free(object);
}


void queue_msg_set_blocking(queue_t *p, uint8_t blocking)
{
    queue_msg_t *object = p->object;
    object->ipc_wait = blocking ? 0 : IPC_NOWAIT;
}

void queue_msg_set_destory(queue_t *p, uint8_t destory)
{
    queue_msg_t *object = p->object;
    object->delete = destory;
}



int queue_msg_out(queue_t *p, queue_data_t *data, int length)
{
    queue_msg_t *object = p->object;

    int flag = object->ipc_wait;
    long type = data->mtype;

    return msgrcv(object->msg_id, data, length, type, flag);
}

int queue_msg_in(queue_t *p, queue_data_t *in, int length)
{
    int ret;
    queue_msg_t *object = p->object;

    while (1)
    {
        ret = msgsnd(object->msg_id, in, length, object->ipc_wait);

        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN)
            {
                sched_yield();
                continue;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return ret;
        }
    }
    return 0;
}


int queue_msg_create(queue_t *p, int blocking, key_t msg_key, long type)
{
    int msg_id;
    queue_msg_t *object = malloc(sizeof(queue_msg_t));
    if (object == NULL)
    {
        return -1;
    }
    if (blocking == 0)
    {
        object->ipc_wait = IPC_NOWAIT;
    }
    else
    {
        object->ipc_wait = 0;
    }
    p->blocking = blocking;
    msg_id = msgget(msg_key, IPC_CREAT | O_EXCL | 0666);
    if (msg_id < 0)
    {
        return -1;
    }
    else
    {
        object->msg_id = msg_id;
        object->type = type;
        p->object = object;
        p->in = queue_msg_in;
        p->out = queue_msg_out;
        p->free = queue_msg_free;
    }
    return 0;
}





int main(void)
{

	dbg_printf("this is just a test\n");
	return(0);
}





