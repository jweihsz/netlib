/*abstruct data type*/
#ifndef _HA_ADT_H
#define _HA_ADT_H

#include <stdbool.h>
#include "list.h"

typedef unsigned int uint;

/*这个是分配池*/
struct alloctor{
	struct list_head freelist;
	struct list_head using;
};
void alloctor_freelist_add(struct alloctor *ac,struct list_head *lh);
void alloctor_init(struct alloctor *ac);

/*从这里获取一个空闲的内存空间节点*/
#define alloctor_entry_get(_ptr_,_a_,_type_,_mb_) \
	({\
		_ptr_ = NULL;\
		if (!list_empty(&(_a_)->freelist)) {\
			_ptr_ = list_first_entry(&((_a_)->freelist),_type_,_mb_);\
			list_del(&(_ptr_)->_mb_);\
			list_add(&(_ptr_)->_mb_,&(_a_)->using);\
		}\
	})

/*这里是重新放置回去*/
#define alloctor_entry_put(_ptr_,_a_,_mb_) \
		({list_del(&(_ptr_)->_mb_);list_add(&(_ptr_)->_mb_,&(_a_)->freelist);})

/*遍历已经正在使用的节点*/
#define alloctor_using_visit(_pos_,_next_,_a_,_mb_) list_for_each_entry_safe(_pos_,_next_,&(_a_)->using,_mb_)	
/*便利空闲的使用节点*/
#define alloctor_free_visit(_pos_,_next_,_a_,_mb_) list_for_each_entry_safe(_pos_,_next_,&(_a_)->freelist,_mb_)	

struct heap;
struct hash_table;
struct hash_entry;
struct hash_table *hash_table_new(uint n,uint capacity,
										uint (*hash_func)(void *),
										int (*cmp)(void *,void *),
										void *(*hash_new)(void *),
										void (*hash_destroy)(void *));

bool hash_entry_exist(struct hash_table *h,void *v,struct hash_entry **he);
void hash_table_destroy(struct hash_table *h);
int hash_table_add(struct hash_table *h,void *v,struct hash_entry **self);
void hash_table_del(struct hash_table *h,struct hash_entry *entry);
uint hash_table_entries(struct hash_table *h);
void *hash_entry_data(struct hash_entry *entry);
bool hash_table_find(struct hash_table *h,void *v,uint *pidx,struct hash_entry **data);

struct heap *heap_new(int size,void *invalid,int (*cmp)(void *,void *),void (*notify)(void *,int));
void heap_destroy(struct heap *h);
bool heap_add(struct heap *h,void *v);
void *heap_top(struct heap *h,bool del);
void *heap_lookup(struct heap *h,int no,bool del);

bool heap_empty(struct heap *h);
void heap_visit(struct heap *h,void (*visit)(void *));
int heap_current(struct heap *h);

void hash_table_print(struct hash_table *h);

#endif
