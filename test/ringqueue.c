#include "common.h"
#include "ringqueue.h"
#include <pthread.h>


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  		(0x01)
#define 	FILE_NAME 	"ringqueue:"




		



int ring_queue_init(ring_queue_t *queue, int buffer_size)
{
    queue->size = buffer_size;
    queue->flags = (char *)calloc(1,queue->size);
    if (queue->flags == NULL)
    {
        return -1;
    }
    queue->data = (void **)calloc(queue->size, sizeof(void*));
    if (queue->data == NULL)
    {
        free(queue->flags);
        return -1;
    }
    queue->head = 0;
    queue->tail = 0;
    memset(queue->flags, 0, queue->size);
    memset(queue->data, 0, queue->size * sizeof(void*));
    return 0;
}



int ring_queue_push(ring_queue_t *queue, void * ele)
{
    if (!(queue->num < queue->size))
    {
        return -1;
    }
    int cur_tail_index = queue->tail;
    char * cur_tail_flag_index = queue->flags + cur_tail_index;  /*queue->flags[cur_tail_index]*/
    while (!compare_and_swap(cur_tail_flag_index, 0, 1))
    {
        cur_tail_index = queue->tail;
        cur_tail_flag_index = queue->flags + cur_tail_index;
    }

    int update_tail_index = (cur_tail_index + 1) % queue->size;
	
    compare_and_swap(&queue->tail, cur_tail_index, update_tail_index);
    *(queue->data + cur_tail_index) = ele;
	
    fetch_and_add(cur_tail_flag_index, 1);  /*变成2了，注意和下面的相结合*/
    fetch_and_add(&queue->num, 1);
    return 0;
}


int  ring_queue_pop(ring_queue_t *queue, void **ele)
{
    if (!(queue->num > 0))
        return -1;
    int cur_head_index = queue->head;
    char * cur_head_flag_index = queue->flags + cur_head_index;

    while (!compare_and_swap(cur_head_flag_index, 2, 3)) /*这里的2是来自上面的赋值*/
    {
        cur_head_index = queue->head;
        cur_head_flag_index = queue->flags + cur_head_index;
    }

    int update_head_index = (cur_head_index + 1) % queue->size;
    compare_and_swap(&queue->head, cur_head_index, update_head_index);
    *ele = *(queue->data + cur_head_index);
	
    fetch_and_sub(cur_head_flag_index, 3);
    fetch_and_sub(&queue->num, 1);
    return 0;
}



ring_queue_t  test;

void * fun_test(void * arg)
{
	
	while(1)
	{
		char * a= calloc(1,sizeof(char));
		a[0]='A';
		dbg_printf("the id is %ld   begin\n",pthread_self());
		ring_queue_push(&test,(void*)a);
		dbg_printf("the id is %ld   end\n",pthread_self());
		
		sleep(1);

		char * pchar  = NULL;
		dbg_printf("the id is %ld     begin\n",pthread_self());
		ring_queue_pop(&test,(void**)&pchar);
		dbg_printf("the id is %ld   %c  end\n",pthread_self(),*pchar);

		if(NULL != pchar)
		{
			free(pchar);
			pchar = NULL;
		}
		
	}
}



#if 0
int main(void )
{
	dbg_printf("this is a test for ringqueue\n");
	
	ring_queue_init(&test,100);
	
	pthread_t ringbuf_pid_0;
	pthread_create(&ringbuf_pid_0, NULL,fun_test, NULL);
	pthread_detach(ringbuf_pid_0);


	pthread_t ringbuf_pid_1;
	pthread_create(&ringbuf_pid_1,NULL, fun_test, NULL);
	pthread_detach(ringbuf_pid_1);

	while(1)
	{
		sleep(10);
	}
	
	return(0);
	
}

#endif
