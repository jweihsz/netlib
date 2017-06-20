#ifndef __WW_COMMON_HEADER_H
#define __WW_COMMON_HEADER_H

#include <stdint.h>
#include "list.h"

typedef enum debug_level
{
	DEBUG_CLOSE = 0,
	DEBUG_ERROR,  /* 1 */
	DEBUG_WARNING,/* 2 */
    DEBUG_NORMAL, /* 3 */
    DEBUG_PAUSE,  /* 4 */
    DEBUG_MAX,
}DEBUG_LEVEL;  

#define WW_MAGIC_NUMBER    0x87877672

#define WW_WEBSITE_DIVERSION_SERVER_PORT        8092
#define WW_WEB_DEBUG_SERVER_PORT                8880


/* start ???? ???????????*/

typedef struct{
    uint32_t magic;
    uint64_t rt_mac;
    uint16_t msgtype;
    uint16_t msglen;
    uint8_t  data[0];
}__attribute__((packed)) web_msg_hdr_t;

typedef enum{
    WEB_MSG_TYPE_SUCCESS_LOG = 0,    /* ????????? */
    WEB_MSG_TYPE_MAX,   
}web_msg_type_t;

typedef enum{
    WEB_TLV_TYPE_HOST = 0,
    WEB_TLV_TYPE_URL,
    WEB_TLV_TYPE_UA,
    WEB_TLV_TYPE_HTML,
    WEB_TLV_TYPE_PHONE_MAC,
}web_tlv_type_t;

typedef struct{
    uint8_t       type;         
    uint16_t      length;
    uint8_t       value[0];
}__attribute__((packed)) web_tlv_node_t;

#define  HTTP_HOST_MAX_LEN              200
#define  HTTP_URL_MAX_LEN               1500
#define  HTTP_UA_MAX_LEN                1000
#define  HTTP_HTML_NAME_MAX_LEN         50

typedef struct{
    uint64_t     rt_mac;
    uint8_t      host[HTTP_HOST_MAX_LEN + 1];
    uint8_t      url[HTTP_URL_MAX_LEN + 1];
    uint8_t      ua[HTTP_UA_MAX_LEN + 1];
    uint8_t      html_name[HTTP_HTML_NAME_MAX_LEN + 1];
}web_success_log_t;

/*end */

/* start ???? ???*/
#define UNIQUE_MAGIC_NUMBER         0x10770112A
#define MAX_BUF_SIZE     1024
#define ROUTER_MAX_NUM   2000000
#define FREE_POOL_ENTRY_MALLOC_SIZE      10000

typedef enum{
    FREE_POOL_TYPE_SOCKET = 0,
    FREE_POOL_TYPE_ROUTER,
    FREE_POOL_TYPE_MAX,          
}free_pool_t;


struct FREE_POOL_TABLE_T{
    struct list_head    list;
    uint32_t            num;
    pthread_mutex_t     mutex;
};

typedef struct {
    struct list_head    fd_list;
    //struct list_head    fd_hash;
    
    int                 sock_fd;
    uint32_t            ip;
    uint16_t            port;
    
    //uint8_t                  send_buf[MAX_BUF_SIZE];
    //int32_t                 n_send;
    
    time_t              create_time;
}socket_entry_t;

typedef enum{
    THREAD_LIST_STATUS_NEW = 0,
    THREAD_LIST_STATUS_INUSE,
    THREAD_LIST_STATUS_MAX,          
}thread_list_status_t;


struct WORKER_THREAD_INFO_T
{
    int                 index; /* ???? */
    pthread_t           thread_id; /* ??id */
    
    struct list_head    list_head[THREAD_LIST_STATUS_MAX];
    uint32_t            list_num[THREAD_LIST_STATUS_MAX];
    
    //struct list_head    hash_table[SOCKET_HASH_TABLE_SIZE];
    
    pthread_mutex_t     mutex;
};
/* end */

/*start ????? */

#define MYSQL_MAX_SQL_BUF_LEN         262144
#define MYSQL_MAX_SELECT_NUM          2000

#define ONE_HOUR_SECONDS    (60 * 60)
#define ONE_DAY_SECONDS     (24 * ONE_HOUR_SECONDS)

#define MYSQL_BUF_LEN        4096

typedef enum{
    MYSQL_DB_TYPE_WEB = 0,
    MYSQL_DB_TYPE_WEB_LOG,
    MYSQL_DB_TYPE_MAX,
        
}mysql_db_type_t;


typedef enum{
    MYSQL_TABLE_TYPE_WEB_SUCCESS_LOG = 0,
    MYSQL_TABLE_TYPE_MAX,
        
}mysql_table_type_t;



struct MYSQL_TABLE_T{    
    char                *p_sql_buf;
    uint32_t            sql_len;
    uint32_t            num;
    
    pthread_mutex_t     mutex;
};

/*end */

/* start debug??  */
#define CLIENT_SERVER_LISTEN_BACKLOG    1000

#define CLIENT_MAX_NUM                  5
#define EPOLL_CLIENT_MAX_EVENTS         (CLIENT_MAX_NUM * 2)

#define PER_CLIENT_PROXY_IP_MAX                  500000 /* ??????????ip?*/
#define PER_CLIENT_PROXY_IP_HASH_MASK            0x1FFFFF
#define PER_CLIENT_PROXY_IP_HASH_TABLE_SIZE      (PER_CLIENT_PROXY_IP_HASH_MASK + 1)
#define CLIENT_PROXY_IP_MAX                      (PER_CLIENT_PROXY_IP_MAX * CLIENT_MAX_NUM)

#define CLIENT_IP_ENTRY_MALLOC_SIZE      10000

#define CLIENT_MAX_BUF_SIZE     1024 * 100


#define HASH_IP_PORT(ip, port)  (ip + port)
#define HASH_UNIQUE_NUMBER(num)  (num % PER_CLIENT_PROXY_IP_HASH_TABLE_SIZE)


#define CLIENT_REQUEST_FILE_MAX_LEN   60
#define CLIENT_USER_NAME_MAX_LEN   60
#define CLIENT_PASSWORD_MAX_LEN    200


#define CLIENT_MAX_IP_LIST_NUM      200

#define CLIENT_REQUEST_DEBUG_NAME   "debug"

#define CLIENT_PARAM_USER          "user_name"
#define CLIENT_PARAM_PASS          "password"
#define CLIENT_PARAM_POS           "pos"
#define CLIENT_PARAM_NUM           "number"
#define CLIENT_PARAM_PUBLIC_IP     "public_ip"

#define CLIENT_PARAM_TARGET      "target"
#define CLIENT_PARAM_OPERATE     "operate"
#define CLIENT_PARAM_PARAM       "param"

#define CLIENT_REQUEST_DEBUG_NAME_LEN   (sizeof(CLIENT_REQUEST_DEBUG_NAME) - 1)

typedef enum{
    DBG_STATUS_SOCKET = 0,
    DBG_STATUS_ROUTER, /* 1 */
    DBG_STATUS_CLIENT, /* 2 */
    DBG_STATUS_MYSQL,  /* 3 */
    DBG_STATUS_THREAD, /* 4 */
    DBG_STATUS_ROUTER_INFO,/* 5 */
    DBG_STATUS_G_DOG_DEBUG,/* 6 */
    DBG_STATUS_MAX,       
        
}debug_status_t;

struct CLIENT_REQUEST_INFO_T{
    int8_t                  request_file[CLIENT_REQUEST_FILE_MAX_LEN + 1];
    int8_t                  user_name[CLIENT_USER_NAME_MAX_LEN + 1];/* ????*/
    int8_t                  password[CLIENT_PASSWORD_MAX_LEN + 1];/* ????*/
    uint16_t                proxy_pos;
    uint16_t                proxy_num;
    int8_t                  public_ip[CLIENT_PASSWORD_MAX_LEN + 1];/* ????*/
    
    uint16_t                target;
    uint16_t                operate;
    uint16_t                param;
};
/* end */

#endif
