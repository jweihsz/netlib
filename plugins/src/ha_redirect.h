#if 1
#ifndef _HA_REDIRECT_H
#define _HA_REDIRECT_H

void ha_redir_init(void);
void ha_redir_destroy(void);
void ha_redir_set_config(char *s);
void ha_redir_set_server(char *s);
void ha_redir_set_url(char *s);

#endif

#endif
