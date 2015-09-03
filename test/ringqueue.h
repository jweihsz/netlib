#ifndef   _RINGQUEUE_H
#define  _RINGQUEUE_H




typedef struct ring_queue
{
	void **data;
	char *flags;
	unsigned int size;
	unsigned int num;
	unsigned int head;
	unsigned int tail;

} ring_queue_t;


int  ring_queue_pop(ring_queue_t *queue, void **ele);
int ring_queue_push(ring_queue_t *queue, void * ele);
int ring_queue_init(ring_queue_t *queue, int buffer_size);


#endif  /*_RINGQUEUE_H*/