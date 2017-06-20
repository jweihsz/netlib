#ifndef _HA_POLICY_H
#define _HA_POLICY_H

#include <stdbool.h>

#include "ha_http.h"
#include "ha_network.h"


struct server_suffix{
	struct list_head next;
	cstr_t host;
	char data[0];
};

void ha_policy_init(void);
void ha_policy_destroy(void);
void ha_register_policy_entry(const char *name,
	bool (*handler)(struct ha_http_request *,int ,struct ha_packet_info *));
int ha_http_redirect(struct ha_packet_info *pi,char *scheme,char *svr,char *url,char *arg,int fin);
bool ha_build_redir_args(struct ha_http_request *req,cstr_t *agent,const char *opt,struct ha_packet_info *pi,bool esc);
char *ha_escap_buffer_get(void);
const char *ha_policy_get_http_buf(void);
char *ha_policy_get_customer(void);
void ignore_server(const char *s,size_t len);
void ha_policy_spec_register(bool (*checker)(struct ha_http_request *,struct ha_packet_info *));
const char *ha_redir_header(void);

#if 0
struct ha_http_request *ha_policy_get_req(void);
#endif

#endif
