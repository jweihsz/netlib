#ifndef _HA_30X_H
#define _HA_30X_H

#include "ha_http.h"

bool proto_30x_handle(struct ha_http_request *req);
bool proto_30x_init(void);
void proto_30x_destroy(void);
void proto_pseudo_server_register(const char *s);
bool proto_stat_save(const char *file);


#endif
