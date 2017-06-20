#include <stdbool.h>

#include "ha_adt.h"
#include "ha_debug.h"

typedef unsigned int uint;

struct hash_entry{
	void *data; /*ͨ��hash_new()�����ȡ����*/
	uint hval;  /*ͨ��hash_func()�����ȡ����*/
	struct list_head next; /*just think it a node*/
};

struct hash_table{
	uint nbuckets; 
	uint entries;
	uint capacity;/*��ʾ����entries����*/
	
	uint (*hash_func)(void *);
	int (*cmp)(void *,void *);
	void *(*hash_new)(void *);
	void (*hash_destroy)(void *);
	struct list_head tables[0];
};

/*������С�� */
struct heap {
	int 	size,
			current;
	int  (*cmp)(void *,void *);
	void (*notify_no)(void *,int);
	void *array[0];
};


/*******************************************************
��̬�ڴ���ͷźͷ���
*******************************************************/


/*��ʼ������ڵ�*/
void alloctor_init(struct alloctor *ac)
{
	assert(ac);
	INIT_LIST_HEAD(&ac->freelist);/*��ʼ�������б�*/
	INIT_LIST_HEAD(&ac->using);/*��ʼ������ʹ�õ��б�*/
}

/*����һ�������б�ڵ�*/
void alloctor_freelist_add(struct alloctor *ac,struct list_head *lh)
{
	list_add_tail(lh,&ac->freelist);
}


/*******************************************************
��С���㷨
*******************************************************/

void heap_destroy(struct heap *h)
{
	FREE(h);
}

/*����һ���µ�heap*/
struct heap *heap_new(int size,void *invalid,int (*cmp)(void *,void *),
							void (*notify_no)(void *,int))
{
	assert(cmp);
	
	struct heap *res = NULL;
	
	res = MALLOC(sizeof(*res) + size * sizeof(void *));
	if (!res) 
		return res;
	
	res->size = size;/*����Ǵ�С*/
	res->current = 1;
	res->cmp = cmp;
	res->array[0] = invalid;
	res->notify_no = notify_no;
	return res;
}

/*heap�Ƿ�Ϊ��*/
bool heap_empty(struct heap *h)
{
	return h->current <= 1;
}

/*heap�Ƿ�����*/
bool heap_full(struct heap *h)
{
	return h->current == h->size ;
}

/*heap[father * 2] = heap[leftChild];  heap[father * 2 + 1] = heap[rightChild];*/
bool heap_add(struct heap *h,void *v)
{
	/*expand if neccessray*/	
#if 0	
	h->current++;
	if (h->current >= h->size) {
		void **array = REALLOC(h->array,h->size * 2 * sizeof(*array));
		if (!array) 
			return -1;
		h->array = array;
		h->size *= 2;
	}
#endif
	if (heap_full(h))
		return false;
	/*��������*/
	int pos,father;
	for (pos = h->current++; pos > 1 ; pos = father) {
		father = pos / 2;
		int result = h->cmp(h->array[father],v);
		if (result < 0) /*h->array[father]  < v*/
			break;
		h->array[pos] = h->array[father]; 
		if (h->notify_no)
			h->notify_no(h->array[pos],pos);
	}

	h->array[pos] = v;
	if (h->notify_no)
		h->notify_no(h->array[pos],pos);

//	debug_p("current = %d",h->current);
	return true;
}


/*���в���*/
void *heap_lookup(struct heap *h,int no,bool del)
{
	if (heap_empty(h)) /*���heapΪ��*/
		return NULL;
	
	void *res = h->array[no];

	if (!del) /*�Ƿ����ɾ�� */
		return res;

	/*�����ǳ�����Χ*/
	if (!(no > 0 && no < h->current))
		debug_p("no = %d current = %d %p",no,h->current,res);
	assert(no > 0 && no < h->current);
	
	void *last = h->array[--h->current]; /*���һ��*/
	int pos,child;
	
	for (pos = no; 2 * pos < h->current  /*Ҫɾ���ĵ㲻�Ǵ������һ��*/ ; pos = child ) {
		child = 2 * pos;

		
		int result = h->cmp(h->array[child],h->array[child + 1]); /*�Ƚϸýڵ�����Һ��ӽڵ�*/
		if (result > 0) /*���Ӵ����Һ���*/
			child++;

		result = h->cmp(h->array[child],last);
		if (result > 0) 
			break;
		
		h->array[pos] = h->array[child];
		if (h->notify_no)
			h->notify_no(h->array[pos],pos);
	}
	if (pos < h->current) {
		h->array[pos] = last; /*�����һ�����͹�ȥ*/
		if (h->notify_no)
			h->notify_no(h->array[pos],pos);
	}
//	debug_p("current = %d",h->current);
	return res;
}
void *heap_top(struct heap *h,bool del)
{
	return heap_lookup(h,1,del);
}

/*Ŀǰheap�Ĵ�С*/
int heap_current(struct heap *h)
{
	return h->current;
}

/*���heap�е�����*/
void heap_visit(struct heap *h,void (*visit)(void *))
{
	int i;
	for (i = 1 ; i < h->current ; i++)
		visit(h->array[i]);
}



/*******************************************************
hash ��
*******************************************************/


/*����һ���µ���ڵ�*/
static struct hash_entry *hash_entry_new(struct hash_table *h,void *v)
{
	struct hash_entry *entry = MALLOC(sizeof(*entry)); /*�������ڴ�*/
	if (!entry)
		return entry;

	entry->data = h->hash_new(v); /*����洢��ֵ�����˸ı�*/
	if (!entry->data) {
		FREE(entry);
		return NULL;
	}
	entry->hval = h->hash_func(v); /*�����hash value��ֵ*/
	INIT_LIST_HEAD(&entry->next); /*��ʼ������*/
	return entry;
}

/*����һ���µ�hash�б�*/
struct hash_table *hash_table_new(uint n,uint capacity,
											uint (*hash_func)(void *),
											int (*cmp)(void *,void *),
											void *(*hash_new)(void *),
											void (*hash_destroy)(void *))
{
	assert(hash_func && cmp && hash_new);
	
	struct hash_table *res = MALLOC(sizeof(*res) + n * sizeof(struct list_head));
	if (!res)
		return res;

	res->nbuckets = n; /*�ܵ�list*/
	res->entries = 0;
	res->capacity = capacity; /*����entries����*/
	uint i;
	for (i = 0 ; i < n ;i++)
		INIT_LIST_HEAD(&(res->tables[i]));  /*��ʼ��list�б�*/

	/*���Ӻ���*/
	res->hash_func = hash_func;
	res->cmp = cmp;
	res->hash_new = hash_new;
	res->hash_destroy = hash_destroy;
	return res;
}

/*��hash���н��в���,v����ʵ�������ϵ�ֵ*/
bool hash_table_find(struct hash_table *h,void *v,uint *pidx,struct hash_entry **data)
{
	uint hval = h->hash_func(v); /*�����hash value��ֵ*/
	uint idx = hval%h->nbuckets;
	if (pidx)
		*pidx = idx;
	
	struct list_head *head = &(h->tables[idx]); /*�������ڵ��Ͻ��в���*/
	struct hash_entry *pos;

	list_for_each_entry(pos,head,next) {
		if (pos->hval == hval && 0 == h->cmp(pos->data,v)) {  /*���hashֵ�Ѿ�������*/
			if (data) /*�����Ҫ��������ڵ�*/
				*data = pos;
			return true;
		}
	}
	return false; /*�����û���ҵ�*/
}

/*�����ȡ�������Ǿ����ı��*/
void *hash_entry_data(struct hash_entry *entry)
{
	return entry->data;
}
bool hash_entry_exist(struct hash_table *h,void *v,struct hash_entry **data/*����ֵ*/)
{
	return hash_table_find(h,v,NULL,data);
}


/*����һ��hash_table*/
void hash_table_destroy(struct hash_table *h)
{
	uint idx;
	for (idx = 0 ; idx < h->nbuckets ; idx++) {
		struct list_head *head = &h->tables[idx];
		struct hash_entry *pos,*_next;
		list_for_each_entry_safe(pos,_next,head,next) {
			list_del(&pos->next);
			if (h->hash_destroy && pos->data)
				h->hash_destroy(pos->data);
			FREE(pos);
		}
	}

	FREE(h);
}

/*����һ����ڵ�*/
int hash_table_add(struct hash_table *h,void *v,struct hash_entry **self/*����������Ľڵ�*/)
{
	uint idx = 0;
	if (hash_table_find(h,v,&idx,NULL)) /*�����hash�����Ѿ������ֵ��*/
		return 0;

	if (1 + h->entries > h->capacity) { /*���������Ƿ񳬳�����*/
		return -1;
	}
	
	struct hash_entry *entry = hash_entry_new(h,v); /*����һ���µ�entry*/
	if (!entry)
		return 0;
	
	list_add_tail(&entry->next,&(h->tables[idx]));  /*���뵽��Ӧ�б��ĩβ*/
	h->entries++;
	
	if (self) 
		*self = entry;  /*����*/
	return 1;
}

/*ɾ��һ����ڵ�*/
void hash_table_del(struct hash_table *h,struct hash_entry *entry)
{
	list_del(&entry->next);
	if (h->hash_destroy)
		h->hash_destroy(entry->data);
	FREE(entry);
	h->entries--; /*��ڵ��������� */
}


/*hash��Ľڵ������*/
uint hash_table_entries(struct hash_table *h)
{
	return h->entries;
}

#if 1

void hash_table_print(struct hash_table *h)
{
	uint i;
	for (i = 0; i < h->nbuckets ; i++) {
		struct list_head *l = &h->tables[i];
		struct hash_entry *pos;
		list_for_each_entry(pos,l,next) {
			debug_p("pos = %p data = %p",pos,pos->data);
		}
	}
}
#endif
