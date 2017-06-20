#ifndef _HA_HTTP_TYPE_H
#define _HA_HTTP_TYPE_H

#include <stdbool.h>

#include "ha_http.h"
#include "ha_network.h"

void ha_http_type_handler_register(const char *s,
												bool (*handler)(struct ha_http_request *,int ,struct ha_packet_info*));
void ha_http_type_init(void);
void ha_http_type_destroy(void);

void ha_http_ignore_destroy(void);
void ha_http_ignore_init(void);

#endif
