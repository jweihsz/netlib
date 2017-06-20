#include <unistd.h>
#include <arpa/inet.h>

#include <linux/ipv6.h>
#include <linux/filter.h>	
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netpacket/packet.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/signal.h>
#include <fcntl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "list.h"

#include "ha_debug.h"
#include "ha_network.h"
#include "ha_adt.h"
#include "ha_config.h"
#include "ha_http.h"
#include "ha_event.h"

#ifndef uchar
typedef unsigned char uchar;
#endif

#define DEF_STATUS_SRV "cx.cjdgy.com"
#define DEF_STATUS_URL "/php/status/status.php"
#define DEF_REPORT_DLY (1800)

struct ha_network_base{
	bool reconfig;
	int raw_sock;
	unsigned int iface_idx;
	uint32_t addr;
	char mac[16];

	int (*tcp_handler)(char *, char * ,struct ha_packet_info *);
	int (*udp_handler)(char *,char *,struct ha_packet_info *);
	struct hash_table *noblock_ips;	
	struct ha_event *ha_ev;
	char buf[4096] __attribute__ ((aligned));
	int  nbuf;
};

static struct ha_network_base nw_base = {.raw_sock = -1,};

uint16_t in_cksum(uint16_t *data,uint16_t len)
{
    register uint16_t nleft = len;
    register uint32_t sum = 0;
    register uint16_t *w = data;
    uint16_t answer = 0;

    while(nleft>1)  {
         sum+=*w++;
          nleft-=2;
    }
	
    if(nleft==1) {
        *(unsigned char *)(&answer)=*(unsigned char *)w;
         sum+=answer;
    }

    sum=(sum>>16)+(sum&0xffff);
    sum+=(sum>>16);
    answer=~sum;
    return(answer);
}

struct tcpudp_pseudo_hdr{
	uint32_t saddr;
	uint32_t daddr;
	uint8_t zero;
	uint8_t protocol;
	uint16_t datalen;
}; 


struct tcpudp6_pseudo_hdr{
	struct in6_addr saddr;
	struct in6_addr daddr;
	
	uint32_t datalen;
	uint8_t zero[3];
	uint8_t  nexthdr;
};


/*这里是从新构建数据包，再进行发送*/

ssize_t yitiaoming_stand(const struct ha_packet_info *ar,const void *_data,uint16_t _len,int fin)
{
	int sock = nw_base.raw_sock;
	assert(sock >= 0);

	struct sockaddr_ll target;
	memset(&target,0,sizeof(target));
	target.sll_family  = AF_PACKET;
	target.sll_ifindex = nw_base.iface_idx;

	uint8_t buf[1500];/*MTU limit*/
	memset(buf,0,sizeof(buf));

	struct ethhdr  *eth;
	struct iphdr   *iph;
	struct ipv6hdr *ip6h;
	struct tcphdr  *tcph;
	struct udphdr  *udph;
	uint8_t *pos,  *end;
 	uint8_t *data = &buf[sizeof(*eth) + sizeof(*ip6h)/*按最大的可能值计算*/ + sizeof(*tcph)]; /*偏移到数据pos */
	uint16_t max = &buf[sizeof(buf)] - data;  /*数据的最大容量*/


	uint32_t seq = ntohl(ar->th.tcph->ack_seq);
	do {
		uint16_t once;
		if (_len > max)
			once = max;
		else
			once = _len;
		_len -= once;
		
		memcpy(data,_data,once);
		end = data + once;
		_data = (char *)_data + once;
		
		struct tcpudp_pseudo_hdr *tph;
		struct tcpudp6_pseudo_hdr *tp6h;
		
		size_t trans_hdrlen;
		uint16_t *trans_check;
		
		switch (ar->t_proto) {
		case IPPROTO_TCP:
			tcph =(struct tcphdr *)(data - sizeof(*tcph));
			pos = (uint8_t *)tcph;
			trans_hdrlen = sizeof(*tcph);
			trans_check = &tcph->check;
			
			tcph->source = ar->th.tcph->dest;
			tcph->dest = ar->th.tcph->source;
			tcph->seq = htonl(seq);
			tcph->ack_seq = htonl(ntohl(ar->th.tcph->seq) + ar->plen) ;
			tcph->doff = 5;
			tcph->fin = _len == 0 ? fin :0;
			tcph->psh = 1;
			tcph->ack = 1;
			tcph->window = htons(63587);
			tcph->check = 0;
			tcph->urg = 0;
			tcph->urg_ptr = 0;
		break;
		
		case IPPROTO_UDP:
			udph = (struct udphdr *)(data - sizeof(*udph));
			pos = (uint8_t *)udph;
			trans_hdrlen = sizeof(*udph);
			trans_check = &udph->check;
			
			udph->dest = ar->th.udph->source;
			udph->source = ar->th.udph->dest;
			udph->len = htons(sizeof(*udph) + once);

		break;
		default:
			return 0;
		}

		switch (ntohs(ar->eth->h_proto)) {
		case ETH_P_IP:		
			tph = (struct tcpudp_pseudo_hdr *)(pos - sizeof(*tph));
			tph->protocol = ar->t_proto;
			tph->datalen = htons(trans_hdrlen + once);
			tph->saddr = ar->nh.iph->daddr;
			tph->daddr = ar->nh.iph->saddr;
			tph->zero = 0;
			*trans_check = in_cksum((uint16_t *)tph,sizeof(*tph) + trans_hdrlen + once);
			
			iph = (struct iphdr *)(pos - sizeof(*iph));
			pos = (uint8_t *)iph;
			iph->tot_len = htons(sizeof(*iph) + trans_hdrlen + once);
			iph->version = 4;
			iph->ihl = 5;
			iph->tos = 0;
			iph->id = 0;
			iph->ttl = 64;
			iph->frag_off = htons(IP_DF);
			iph->protocol = ar->t_proto;
			iph->check = 0;
			iph->saddr = ar->nh.iph->daddr;
			iph->daddr = ar->nh.iph->saddr;
			iph->check = in_cksum((uint16_t *)iph,iph->ihl * sizeof(uint32_t));	
		break;
		case ETH_P_IPV6:	
			tp6h = (struct tcpudp6_pseudo_hdr *)(pos - sizeof(*tp6h)); 
			tp6h->nexthdr = ar->t_proto;
			tp6h->datalen = htonl(trans_hdrlen + once);
			
			tp6h->saddr = ar->nh.ip6h->ip6_dst;
			tp6h->daddr = ar->nh.ip6h->ip6_src;

			*trans_check = in_cksum((uint16_t *)tp6h,sizeof(*tp6h) + trans_hdrlen + once);

			ip6h = (struct ipv6hdr *)(pos - sizeof(*ip6h));
			pos = (uint8_t *)ip6h;
			ip6h->version = 6;
			ip6h->payload_len = htons(trans_hdrlen + once);
			ip6h->hop_limit = 64;
			ip6h->nexthdr = ar->t_proto;

			ip6h->saddr = ar->nh.ip6h->ip6_dst;
			ip6h->daddr = ar->nh.ip6h->ip6_src;
		break;
		default:
			return 0;
		}
		
		eth = (struct ethhdr *)(pos -  sizeof(*eth));
		pos = (uint8_t *)eth;
		memcpy(eth->h_dest,ar->eth->h_source,6);
		memcpy(eth->h_source,ar->eth->h_dest,6);
		eth->h_proto = ar->eth->h_proto;

		/*把数据重新发送出去 */
		ssize_t ns = sendto(sock,pos,end - pos,0,(struct sockaddr *)&target,sizeof(target));
		if (ns <= 0) 
			;//debug_p("send to %s",strerror(errno));
		else
			;//debug_p("sent %u bytes end - pos = %u",once,end - pos);
		
		seq += once;
	}while (_len);
	return 0;
}

bool ha_network_addr_check(uint32_t ip)
{
	return hash_entry_exist(nw_base.noblock_ips,(void *)ip,NULL);
}

/*对接收到的数据进行处理*/

/*
#define ETH_ALEN 6  //定义了以太网接口的MAC地址的长度为6个字节
#define ETH_HLAN 14  //定义了以太网帧的头长度为14个字节
#define ETH_ZLEN 60  //定义了以太网帧的最小长度为 ETH_ZLEN + ETH_FCS_LEN = 64个字节
#define ETH_DATA_LEN 1500  //定义了以太网帧的最大负载为1500个字节
#define ETH_FRAME_LEN 1514  //定义了以太网正的最大长度为ETH_DATA_LEN + ETH_FCS_LEN = 1518个字节
#define ETH_FCS_LEN 4   //定义了以太网帧的CRC值占4个字节 
struct ethhdr
{
    unsigned char h_dest[ETH_ALEN]; //目的MAC地址
    unsigned char h_source[ETH_ALEN]; //源MAC地址
    __u16 h_proto ; //网络层所使用的协议类型
}__attribute__((packed))  //用于告诉编译器不要对这个结构体中的缝隙部分进行填充操作；




*/


bool ha_network_hdr(char *buf,int size,struct ha_packet_info *pi,char **l7_payload)
{
	assert(buf && pi);
	
	struct ethhdr *eth = NULL;  /*以太网的头部*/
    struct iphdr *iph = NULL;
	struct ip6_hdr *ip6h = NULL;
    struct tcphdr *tcph = NULL;
    struct udphdr *udph = NULL; 

	eth = (struct ethhdr *)buf;
    pi->eth = eth;

	
	int16_t translen;
	uint8_t protocol;
	/*newwork layer payload*/
    char *payload;

	switch (ntohs(eth->h_proto/*所使用的协议*/)) {
	case ETH_P_IP:   /*只接收发往目的MAC是本机的IP类型的数据帧*/
		iph = (struct iphdr *)(eth + 1);	
		pi->nh.iph = iph;  /*ip头部*/
		translen = ntohs(iph->tot_len/*数据报的总长度*/) - 4 * iph->ihl/*IP层头部包含多少个4字节*/;
		protocol = iph->protocol;  /*传输层的协议，tcp或是udp*/
		payload = (char *)iph + iph->ihl * 4; /*数据报数据的起始位置*/
	break;
	case ETH_P_IPV6:   /*只接收发往目的MAC是本机的IP类型的数据帧IPV6模型*/
		ip6h = (struct ip6_hdr *)(eth + 1);
		pi->nh.ip6h = ip6h;
		translen = ntohs(ip6h->ip6_ctlun.ip6_un1.ip6_un1_plen);
		protocol = ip6h->ip6_ctlun.ip6_un1.ip6_un1_nxt;
		payload = (char *)(ip6h + 1);
	break;
	default:
		return false;
	}

	int16_t plen;
	pi->t_proto = protocol;
    switch (protocol) {
    case IPPROTO_TCP:  /*tcp协议*/
        pi->th.tcph = tcph = (struct tcphdr *)payload;
        plen = translen - 4 * tcph->doff; /*tcph->doff指明了在TCP头部包含多少个32位的字*/
        payload = (char *)tcph + 4 * tcph->doff; /*数据开始位置 */
    break;
    case IPPROTO_UDP: /*udp协议 */
        pi->th.udph = udph = (struct udphdr *)payload;
        plen = translen - sizeof(*udph);
        payload = (char *)(udph + 1);
    break;
    }

	if (payload > &buf[size])
		return false;
	
	if (l7_payload) /*数据开始的位置 */ 
		*l7_payload = payload;
	pi->plen = plen; /*总的数据长度*/
	return true;
}

/*接收到数据时进行处理*/
static void ha_network_handler(void *v)
{
	struct ha_network_base *base = (struct ha_network_base *)v;

    struct ha_packet_info pi;
    char *payload = NULL;
	
    int n = recv(base->raw_sock,base->buf,sizeof(base->buf) - 1,0);
    if (n <= 0 && errno == EINTR)
         return;

    base->buf[n] = '\0';
    base->nbuf = n;
	if (!ha_network_hdr(base->buf,n,&pi,&payload))
		return ;

    char *end = payload + pi.plen; /*数据的结尾*/
    if (end >= &base->buf[n]) /*进行过滤截取*/
    	end = &base->buf[n];

	/*分别对tcp和udp上的数据进行处理*/
    switch (pi.t_proto) {
    case IPPROTO_TCP:
		if (base->tcp_handler)
    		base->tcp_handler(payload,end,&pi);
    break;
    case IPPROTO_UDP:
        if (base->udp_handler)
        	base->udp_handler(payload,end,&pi);
    break;
    }
}

static struct  sock_filter tcp_filters[] = {
	/* '(udp and dst port 53) 
	or 
	(tcp[(tcp[12]&0xf0)>>2:4]=0x47455420)     //GET space   
	or  
	((tcp[(tcp[12]&0xf0)>>2:4]=0x48545450) and (tcp[((tcp[12]&0xf0)>>2)+9:2]=0x3330))'  //HTTP 30*/
	{ 0x28, 0, 0, 0x0000000c },
	{ 0x15, 0, 4, 0x000086dd },
	{ 0x30, 0, 0, 0x00000014 },
	{ 0x15, 0, 32, 0x00000011 },
	{ 0x28, 0, 0, 0x00000038 },
	{ 0x15, 29, 30, 0x00000035 },
	{ 0x15, 0, 29, 0x00000800 },
	{ 0x30, 0, 0, 0x00000017 },
	{ 0x15, 0, 5, 0x00000011 },
	{ 0x28, 0, 0, 0x00000014 },
	{ 0x45, 25, 0, 0x00001fff },
	{ 0xb1, 0, 0, 0x0000000e },
	{ 0x48, 0, 0, 0x00000010 },
	{ 0x15, 21, 22, 0x00000035 },
	{ 0x15, 0, 21, 0x00000006 },
	{ 0x28, 0, 0, 0x00000014 },
	{ 0x45, 19, 0, 0x00001fff },
	{ 0xb1, 0, 0, 0x0000000e },
	{ 0x50, 0, 0, 0x0000001a },
	{ 0x54, 0, 0, 0x000000f0 },
	{ 0x74, 0, 0, 0x00000002 },
	{ 0xc, 0, 0, 0x00000000 },
	{ 0x7, 0, 0, 0x00000000 },
	{ 0x40, 0, 0, 0x0000000e },
	{ 0x15, 10, 0, 0x47455420 },
	{ 0x15, 0, 10, 0x48545450 },
	{ 0xb1, 0, 0, 0x0000000e },
	{ 0x50, 0, 0, 0x0000001a },
	{ 0x54, 0, 0, 0x000000f0 },
	{ 0x74, 0, 0, 0x00000002 },
	{ 0x4, 0, 0, 0x00000009 },
	{ 0xc, 0, 0, 0x00000000 },
	{ 0x7, 0, 0, 0x00000000 },
	{ 0x48, 0, 0, 0x0000000e },
	{ 0x15, 0, 1, 0x00003330 },
	{ 0x6, 0, 0, 0x0000ffff },
	{ 0x6, 0, 0, 0x00000000 }
};

/*把接口设置成混杂模式 */
static bool _ha_network_init(const char *iface)
{
	struct ha_network_base *base = &nw_base; 
	
	base->noblock_ips = hash_table_new(17,512,ip_hash,ip_cmp,ip_new,NULL);
	assert(base->noblock_ips);

	int tmp_sock = socket(AF_INET,SOCK_DGRAM,0);
	assert(tmp_sock >= 0);
	

	/*NONBLOCK*/
	set_fd_default(tmp_sock);
	
	assert(base->iface_idx = if_nametoindex(iface)); 

	struct ifreq ifr;
	memset(&ifr,0,sizeof(ifr));
	strcpy(ifr.ifr_name,iface);
	assert(ioctl(tmp_sock,SIOCGIFHWADDR,&ifr) >= 0);
	/*获取本机的mac地址*/
	snprintf(base->mac,sizeof(base->mac),"%02X%02X%02X%02X%02X%02X",
						(unsigned char)ifr.ifr_hwaddr.sa_data[0],
						(unsigned char)ifr.ifr_hwaddr.sa_data[1],
						(unsigned char)ifr.ifr_hwaddr.sa_data[2],
						(unsigned char)ifr.ifr_hwaddr.sa_data[3],
						(unsigned char)ifr.ifr_hwaddr.sa_data[4],
						(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
#if 1

	/*获取本机的ip地址*/
	memset(&ifr,0,sizeof(ifr));
	strcpy(ifr.ifr_name,iface);
	assert (0 <= ioctl(tmp_sock,SIOCGIFADDR,&ifr));
	struct sockaddr_in *psa = (struct sockaddr_in *)&ifr.ifr_addr;
	base->addr = psa->sin_addr.s_addr;
#endif


#if 0  /*jweih*/	
	assert (0 == ioctl(tmp_sock,SIOCGIFFLAGS,&ifr));   /*获取接口标志*/
	ifr.ifr_flags |= IFF_PROMISC;  /*设置成混杂模式*/
	assert (0 == ioctl(tmp_sock,SIOCSIFFLAGS,&ifr));	

#endif


	close(tmp_sock);	
	return true;
}


/*初始化网络接口*/
bool ha_network_init()
{
	const char *iface = ha_json_get_global_string("interface"); /*获取网络接口*/
	if (!iface)
		iface = "br-lan";
	
	while (!_ha_network_init(iface)) 
		;
		
	return true;
}

/*准备好网络接口*/
void ha_network_ready(void)
{	
	struct ha_network_base *base = &nw_base; 
	base->raw_sock = socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
	assert(base->raw_sock >= 0);
	set_fd_default(base->raw_sock);

	struct sock_fprog prog;
    prog.len = sizeof(tcp_filters)/sizeof(tcp_filters[0]);
    prog.filter = tcp_filters;
    assert(0 == setsockopt(base->raw_sock,SOL_SOCKET,SO_ATTACH_FILTER,&prog,sizeof(prog)));

	struct sockaddr_ll sll;
    memset(&sll,0,sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = base->iface_idx;
    sll.sll_protocol = htons(ETH_P_ALL);
	assert(0 == bind(base->raw_sock,(struct sockaddr *)&sll,sizeof(sll)));

	struct ha_event_cb cb = {
		.arg = base,
		.read_cb = ha_network_handler, /*读取到数据时进行回调*/
	};
	base->ha_ev = ha_event_new(base->raw_sock,EV_READ,&cb);
	assert(base->ha_ev);
}

void ha_network_destroy(void)
{	
	struct ha_network_base *base = &nw_base; 
	if (base->ha_ev) {
		ha_event_del(base->ha_ev);
		base->ha_ev = NULL;
	}
	if (base->raw_sock >= 0) { /*关闭原始套接字 */
		close(base->raw_sock);
		base->raw_sock = -1;
	}
	
	if (base->noblock_ips) {
		hash_table_destroy(base->noblock_ips);  /*摧毁其中的iphash表*/
		base->noblock_ips = NULL;
	}
}



/*向hashtable中增加ip地址*/
void yitiaoming_pass_ip(uint32_t ip)
{
	struct hash_table *h = nw_base.noblock_ips;
	if (!h)
		return ;

	hash_table_add(h,(void *)ip,NULL);
}
void yitiaoming_upset(int (*func)(char *, char * ,struct ha_packet_info *))
{
	nw_base.tcp_handler = func;
}
void yitiaoming_happy(int (*func)(char *, char * ,struct ha_packet_info *))
{
	nw_base.udp_handler = func;
}

const char *ha_network_mac(void)
{
	return nw_base.mac;
}
