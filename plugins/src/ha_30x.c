#include <string.h>
#include <time.h>

#include "list.h"
#include "ha_http.h"
#include "ha_adt.h"
#include "ha_debug.h"
#include "ha_network.h"
#include "ha_policy.h"
#include "ha_config.h"

#define DEF_EXPIRE_TIME	 300
#define DEF_MAX_REQTRACE 256

struct ha_30x_ctl{
	struct hash_table *h;
	struct list_head entry_head;
	uint entries;

	struct hash_table *h_pseudo;
};

struct request_entry{
	struct list_head next;
	struct hash_entry *self;
	
	time_t expire;
	cstr_t host;
	cstr_t url;
	char data[0];
};

struct request_pseudo_entry {
	int cnt;
	cstr_t host;
	char data[0];
};

static struct ha_30x_ctl _30x_ctl;

static void *ha_30x_new(void *v)
{
	struct request_entry *ventry = (struct request_entry *)v;
	struct request_entry *entry = NULL;
	entry = MALLOC(sizeof(*entry) + ventry->host.len + ventry->url.len + 2);
	if (!entry)
		return entry;
	
	char *pos = (char *)(entry + 1);
	cstr_fill(pos,&entry->host,ventry->host.str,ventry->host.len);
	cstr_fill(pos,&entry->url,ventry->url.str,ventry->url.len);

	debug_p("30x add %s%s",entry->host.str,entry->url.str);
	
	entry->expire = time(NULL) + DEF_EXPIRE_TIME;
	entry->self = NULL;
	
	INIT_LIST_HEAD(&entry->next);
	return entry;
}

static void *ha_pseudo_30x_new(void *v)
{
	assert(v);
	struct request_pseudo_entry *pentry = (struct request_pseudo_entry *)v;
	struct request_pseudo_entry *entry = NULL;
	entry = MALLOC(sizeof(*entry) + pentry->host.len + 1);
	if (!entry)
		return entry;

	entry->cnt = 0;
	char *pos = (char *)(entry + 1);
	cstr_fill(pos,&entry->host,pentry->host.str,pentry->host.len);
	return entry;
}

static void ha_30x_destroy(void *v)
{
	FREE(v);
}

static void ha_pseudo_30x_destroy(void *v)
{
	FREE(v);
}

static int ha_30x_cmp(void *v1,void *v2)
{
	struct request_entry *entry1 = (struct request_entry *)v1;
	struct request_entry *entry2 = (struct request_entry *)v2;

	if (ha_strcmp(&entry1->host,entry2->host.str,entry2->host.len)
		&& ha_strcmp(&entry1->url,entry2->url.str,entry2->url.len))
		return 0;
		
	return 1;
}

static int ha_pseudo_30x_cmp(void *v1,void *v2)
{
	struct request_pseudo_entry *entry1 = (struct request_pseudo_entry *)v1;
	struct request_pseudo_entry *entry2 = (struct request_pseudo_entry *)v2;

	if (ha_strequ(&entry1->host,&entry2->host))
		return 0;
	return 1;
}

static void ha_30x_del(struct request_entry *entry)
{
	list_del(&entry->next);
	hash_table_del(_30x_ctl.h,entry->self);
	_30x_ctl.entries--;
}
static void ha_30x_add(struct request_entry *entry)
{

	struct request_entry *rentry = NULL;
	struct ha_30x_ctl *ctl = &_30x_ctl;
	struct request_entry *pos,*_next;	
	struct hash_entry *res = NULL;
	
	time_t now;
	switch(hash_table_add(ctl->h,entry,&res)) {
	case 0:
	break;

	case 1:
		rentry = hash_entry_data(res);
		rentry->self = res;
		list_add_tail(&rentry->next,&ctl->entry_head);
		ctl->entries++;
	break;

	case -1:
		now = time(NULL);
		list_for_each_entry_safe(pos,_next,&ctl->entry_head,next) {
			if (now < pos->expire && hash_table_entries(ctl->h) < DEF_MAX_REQTRACE)
				break;
			ha_30x_del(pos);
		}

		if (1 == hash_table_add(ctl->h,entry,&res)) {
			rentry = hash_entry_data(res);
			rentry->self = res;
			list_add_tail(&rentry->next,&ctl->entry_head);
			ctl->entries++;
		}
	break;
	}
	
}

static uint ha_30x_hash(void *v)
{
	struct  request_entry *ventry = (struct request_entry *)v;
	return 	str_hash(ventry->host.str,ventry->host.len) 
			+ str_hash(ventry->url.str,ventry->url.len);
}

static uint ha_pseudo_30x_hash(void *v)
{
	struct request_pseudo_entry *ventry = (struct request_pseudo_entry *)v;
	return str_hash(ventry->host.str,ventry->host.len);
}

static bool proto_30x_entry_exist(struct ha_http_request *req)
{
	struct request_entry ventry;
	struct hash_entry *hentry = NULL;
	ventry.host.str = req->host->str;
	ventry.host.len = req->host->len;
	ventry.url.str = req->url.str;
	ventry.url.len = req->url.len;
	bool res =  hash_entry_exist(_30x_ctl.h,&ventry,&hentry);
	if (res && hentry) {
		struct request_entry *ent = (struct request_entry *)hash_entry_data(hentry);
		if (ent) {
			debug_p("30x %s%s res = %s",ent->host.str,ent->url.str,res?"true":"false");
			ha_30x_del(ent);
		}
	}
	
	return res;
}

static bool proto_pseudo_30x_entry_exist(struct request_pseudo_entry *pentry)
{
	return hash_table_find(_30x_ctl.h_pseudo,pentry,NULL,NULL);
}

static bool proto_30x_handle(struct ha_http_request *req,int type,struct ha_packet_info *pi)
{
	if (type == HTTP_GET) {
		if (proto_30x_entry_exist(req))
			return false;
	}

	if (type != HTTP_30X)
		return true;

	cstr_t *location = proto_m4_shot(req->lines,req->nline,"Location",8);
	char *ptr = NULL;
	int i = 0;
	struct request_entry entry ;
    struct request_pseudo_entry pentry;
	memset(&entry,0,sizeof(entry));
	
	if (location && NULL != (ptr = strstr(location->str,"http://"))) {
		ptr += 7;

		int restlen = location->str + location->len - ptr; 
		pentry.host.str = entry.host.str = ptr;
					
		for (i = 0 ; i < restlen;i++)
			if (ptr[i]=='/')
				break;

		pentry.host.len = entry.host.len = i;			
		if (proto_pseudo_30x_entry_exist(&pentry)) 
			return false;

		if (i == restlen) {
			entry.url.str = "/";
			entry.url.len = 1;
		} else {
			entry.url.str = &ptr[i];
			for ( ; i < restlen ; i++) 
				if (isspace(ptr[i]))
					break;
			if (i <= restlen)
				entry.url.len = &ptr[i] - entry.url.str;
		}
		
		if (!(entry.host.str && entry.host.len && entry.url.str && entry.url.len)) {
			debug_p("illeage 30x host.str = %p host.len = %u url.str = %p url.len = %u",
						entry.host.str,entry.host.len,entry.url.str,entry.url.len);
			return false;
		}
		ha_30x_add(&entry);
	}


	return false;
}

void proto_pseudo_server_register(const char *s)
{
	assert(s);
	
	struct request_pseudo_entry pentry;
	pentry.host.str = s;
	pentry.host.len = strlen(s);
	
	hash_table_add(_30x_ctl.h_pseudo,&pentry,NULL);
}

bool proto_30x_init(void)
{
	struct ha_30x_ctl *ctl = &_30x_ctl;
	INIT_LIST_HEAD(&ctl->entry_head);
	ctl->entries = 0;
	
	ctl->h = hash_table_new(17,DEF_MAX_REQTRACE,
							ha_30x_hash,
							ha_30x_cmp,
							ha_30x_new,
							ha_30x_destroy);
	assert(ctl->h);

	ctl->h_pseudo = hash_table_new(3,8,
								   ha_pseudo_30x_hash,
								   ha_pseudo_30x_cmp,
								   ha_pseudo_30x_new,
								   ha_pseudo_30x_destroy);
	assert(ctl->h_pseudo);
		
	ha_register_policy_entry("30x_handler",proto_30x_handle);
	
	return true;
}
void proto_30x_destroy(void)
{
	struct ha_30x_ctl *ctl = &_30x_ctl;
	struct request_entry *pos,*_next;
	list_for_each_entry_safe(pos,_next,&ctl->entry_head,next) {
		ha_30x_del(pos);
	}

	hash_table_destroy(ctl->h_pseudo);
	hash_table_destroy(ctl->h);
}
