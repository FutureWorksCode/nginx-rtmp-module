/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - Basic type definitions
 */

#ifndef _RTMP_TYPES_H_INCLUDED_
#define _RTMP_TYPES_H_INCLUDED_

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

/* Basic result codes */
#define RTMP_OK          0
#define RTMP_ERROR      -1
#define RTMP_AGAIN      -2
#define RTMP_DONE       -3
#define RTMP_DECLINED   -4

/* Socket/connection types */
typedef int rtmp_socket_t;
typedef ssize_t rtmp_int_t;
typedef size_t rtmp_uint_t;
typedef uint8_t rtmp_flag_t;
typedef time_t rtmp_msec_t;

/* Memory management */
typedef struct rtmp_pool_s rtmp_pool_t;
typedef struct rtmp_chain_s rtmp_chain_t;
typedef struct rtmp_buf_s rtmp_buf_t;

/* Event system */
typedef struct rtmp_event_s rtmp_event_t;
typedef struct rtmp_connection_s rtmp_connection_t;

/* Configuration */
typedef struct rtmp_conf_s rtmp_conf_t;
typedef struct rtmp_str_s rtmp_str_t;
typedef struct rtmp_array_s rtmp_array_t;

/* String structure */
struct rtmp_str_s {
    size_t len;
    unsigned char *data;
};

/* Buffer structure - compatible with ngx_buf_t essentials */
struct rtmp_buf_s {
    unsigned char *pos;
    unsigned char *last;
    unsigned char *start;
    unsigned char *end;
    rtmp_flag_t temporary:1;
    rtmp_flag_t memory:1;
    rtmp_flag_t mmap:1;
    rtmp_flag_t recycled:1;
};

/* Chain structure - compatible with ngx_chain_t */
struct rtmp_chain_s {
    rtmp_buf_t *buf;
    rtmp_chain_t *next;
};

/* Array structure */
struct rtmp_array_s {
    void *elts;
    rtmp_uint_t nelts;
    size_t size;
    rtmp_uint_t nalloc;
    rtmp_pool_t *pool;
};

/* Basic macros for compatibility */
#define rtmp_string(str) { sizeof(str) - 1, (unsigned char *) str }
#define rtmp_null_string { 0, NULL }

/* Memory allocation macros */
#define rtmp_palloc(pool, size) rtmp_pool_alloc(pool, size)
#define rtmp_pcalloc(pool, size) rtmp_pool_calloc(pool, size)

/* Logging levels */
#define RTMP_LOG_STDERR            0
#define RTMP_LOG_EMERG             1
#define RTMP_LOG_ALERT             2
#define RTMP_LOG_CRIT              3
#define RTMP_LOG_ERR               4
#define RTMP_LOG_WARN              5
#define RTMP_LOG_NOTICE            6
#define RTMP_LOG_INFO              7
#define RTMP_LOG_DEBUG             8

#define RTMP_LOG_DEBUG_RTMP        RTMP_LOG_DEBUG

/* Function pointer types */
typedef rtmp_int_t (*rtmp_recv_pt)(rtmp_connection_t *c, unsigned char *buf, size_t size);
typedef rtmp_int_t (*rtmp_send_pt)(rtmp_connection_t *c, unsigned char *buf, size_t size);
typedef void (*rtmp_event_handler_pt)(rtmp_event_t *ev);

#endif /* _RTMP_TYPES_H_INCLUDED_ */