#ifndef _HA_NETWORK_H
#define _HA_NETWORK_H

#include <stdbool.h>

#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ether.h>

#include "list.h"

/*http://blog.chinaunix.net/uid-23069658-id-3280895.html*/
struct ha_packet_info{
	uint8_t t_proto;
	bool self;
	struct ethhdr *eth; /*以太网头部: mac地址 + 协议*/
	union { /*ip 头部 http://blog.csdn.net/caofengtao1314/article/details/52753894*/
		struct iphdr *iph;
		struct ip6_hdr *ip6h;
	}nh;
	/*传输层头部*/
	union {
		struct tcphdr *tcph; /*http://blog.chinaunix.net/uid-10540984-id-3265959.html*/
		struct udphdr *udph;
	}th;

	uint16_t  plen;
}__attribute__((packed));

struct ha_packet_hdr{
	struct ethhdr eth;
	union {
		struct iphdr iph;
		struct ip6_hdr ip6h;
	}nh;
	union {
		struct tcphdr tcph;
		struct udphdr udph;
	}th;
}__attribute__((packed));




#include <stdint.h>
bool ha_network_addr_check(uint32_t ip);

ssize_t yitiaoming_stand(const struct ha_packet_info *ar,const void *_data,uint16_t _len,int fin);
bool 	ha_network_init(void);
void yitiaoming_upset(int (*func)(char *, char * ,struct ha_packet_info *));
void yitiaoming_happy(int (*func)(char *, char * ,struct ha_packet_info *));

const char *ha_network_mac(void);
void yitiaoming_pass_ip(uint32_t ip);
void ha_network_destroy(void);
void ha_network_ready(void);
void ha_network_packet_deepcopy(struct ha_packet_info *npi,struct ha_packet_hdr *hdr,const struct ha_packet_info *pi);
bool ha_network_hdr(char *buf,int size,struct ha_packet_info *pi,char **l7_payload);
uint16_t in_cksum(uint16_t *data,uint16_t len);

#endif
