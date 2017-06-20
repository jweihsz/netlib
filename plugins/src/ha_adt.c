#include <stdbool.h>

#include "ha_adt.h"
#include "ha_debug.h"

typedef unsigned int uint;

struct hash_entry{
	void *data; /*通过hash_new()计算获取到的*/
	uint hval;  /*通过hash_func()计算获取到的*/
	struct list_head next; /*just think it a node*/
};

struct hash_table{
	uint nbuckets; 
	uint entries;
	uint capacity;/*表示最大的entries数量*/
	
	uint (*hash_func)(void *);
	int (*cmp)(void *,void *);
	void *(*hash_new)(void *);
	void (*hash_destroy)(void *);
	struct list_head tables[0];
};

/*这是最小堆 */
struct heap {
	int 	size,
			current;
	int  (*cmp)(void *,void *);
	void (*notify_no)(void *,int);
	void *array[0];
};


/*******************************************************
动态内存的释放和分配
*******************************************************/


/*初始化分配节点*/
void alloctor_init(struct alloctor *ac)
{
	assert(ac);
	INIT_LIST_HEAD(&ac->freelist);/*初始化空闲列表*/
	INIT_LIST_HEAD(&ac->using);/*初始化正在使用的列表*/
}

/*增加一个空闲列表节点*/
void alloctor_freelist_add(struct alloctor *ac,struct list_head *lh)
{
	list_add_tail(lh,&ac->freelist);
}


/*******************************************************
最小堆算法
*******************************************************/

void heap_destroy(struct heap *h)
{
	FREE(h);
}

/*分配一个新的heap*/
struct heap *heap_new(int size,void *invalid,int (*cmp)(void *,void *),
							void (*notify_no)(void *,int))
{
	assert(cmp);
	
	struct heap *res = NULL;
	
	res = MALLOC(sizeof(*res) + size * sizeof(void *));
	if (!res) 
		return res;
	
	res->size = size;/*这个是大小*/
	res->current = 1;
	res->cmp = cmp;
	res->array[0] = invalid;
	res->notify_no = notify_no;
	return res;
}

/*heap是否为空*/
bool heap_empty(struct heap *h)
{
	return h->current <= 1;
}

/*heap是否满了*/
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
	/*从下往上*/
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


/*进行查找*/
void *heap_lookup(struct heap *h,int no,bool del)
{
	if (heap_empty(h)) /*如果heap为空*/
		return NULL;
	
	void *res = h->array[no];

	if (!del) /*是否进行删除 */
		return res;

	/*这里是超出范围*/
	if (!(no > 0 && no < h->current))
		debug_p("no = %d current = %d %p",no,h->current,res);
	assert(no > 0 && no < h->current);
	
	void *last = h->array[--h->current]; /*最后一个*/
	int pos,child;
	
	for (pos = no; 2 * pos < h->current  /*要删除的点不是处于最后一个*/ ; pos = child ) {
		child = 2 * pos;

		
		int result = h->cmp(h->array[child],h->array[child + 1]); /*比较该节点的左右孩子节点*/
		if (result > 0) /*左孩子大于右孩子*/
			child++;

		result = h->cmp(h->array[child],last);
		if (result > 0) 
			break;
		
		h->array[pos] = h->array[child];
		if (h->notify_no)
			h->notify_no(h->array[pos],pos);
	}
	if (pos < h->current) {
		h->array[pos] = last; /*把最后一个移送过去*/
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

/*目前heap的大小*/
int heap_current(struct heap *h)
{
	return h->current;
}

/*浏览heap中的数据*/
void heap_visit(struct heap *h,void (*visit)(void *))
{
	int i;
	for (i = 1 ; i < h->current ; i++)
		visit(h->array[i]);
}



/*******************************************************
hash 表
*******************************************************/


/*创建一个新的入口点*/
static struct hash_entry *hash_entry_new(struct hash_table *h,void *v)
{
	struct hash_entry *entry = MALLOC(sizeof(*entry)); /*分配新内存*/
	if (!entry)
		return entry;

	entry->data = h->hash_new(v); /*这里存储的值进行了改变*/
	if (!entry->data) {
		FREE(entry);
		return NULL;
	}
	entry->hval = h->hash_func(v); /*计算出hash value的值*/
	INIT_LIST_HEAD(&entry->next); /*初始化链表*/
	return entry;
}

/*创建一个新的hash列表*/
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

	res->nbuckets = n; /*总的list*/
	res->entries = 0;
	res->capacity = capacity; /*最大的entries数量*/
	uint i;
	for (i = 0 ; i < n ;i++)
		INIT_LIST_HEAD(&(res->tables[i]));  /*初始化list列表*/

	/*钩子函数*/
	res->hash_func = hash_func;
	res->cmp = cmp;
	res->hash_new = hash_new;
	res->hash_destroy = hash_destroy;
	return res;
}

/*在hash表中进行查找,v代表实际意义上的值*/
bool hash_table_find(struct hash_table *h,void *v,uint *pidx,struct hash_entry **data)
{
	uint hval = h->hash_func(v); /*计算出hash value的值*/
	uint idx = hval%h->nbuckets;
	if (pidx)
		*pidx = idx;
	
	struct list_head *head = &(h->tables[idx]); /*在这个入口点上进行查找*/
	struct hash_entry *pos;

	list_for_each_entry(pos,head,next) {
		if (pos->hval == hval && 0 == h->cmp(pos->data,v)) {  /*这个hash值已经存在了*/
			if (data) /*如果需要返回这个节点*/
				*data = pos;
			return true;
		}
	}
	return false; /*否则就没有找到*/
}

/*这里获取的数据是经过改变的*/
void *hash_entry_data(struct hash_entry *entry)
{
	return entry->data;
}
bool hash_entry_exist(struct hash_table *h,void *v,struct hash_entry **data/*返回值*/)
{
	return hash_table_find(h,v,NULL,data);
}


/*销毁一个hash_table*/
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

/*增加一个入口点*/
int hash_table_add(struct hash_table *h,void *v,struct hash_entry **self/*返回新申请的节点*/)
{
	uint idx = 0;
	if (hash_table_find(h,v,&idx,NULL)) /*如果在hash表中已经有这个值了*/
		return 0;

	if (1 + h->entries > h->capacity) { /*查找容量是否超出极限*/
		return -1;
	}
	
	struct hash_entry *entry = hash_entry_new(h,v); /*创建一个新的entry*/
	if (!entry)
		return 0;
	
	list_add_tail(&entry->next,&(h->tables[idx]));  /*加入到对应列表的末尾*/
	h->entries++;
	
	if (self) 
		*self = entry;  /*返回*/
	return 1;
}

/*删除一个入口点*/
void hash_table_del(struct hash_table *h,struct hash_entry *entry)
{
	list_del(&entry->next);
	if (h->hash_destroy)
		h->hash_destroy(entry->data);
	FREE(entry);
	h->entries--; /*入口点数量减少 */
}


/*hash表的节点入口数*/
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
