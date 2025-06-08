/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - Event and connection abstraction
 */

#ifndef _RTMP_CONNECTION_H_INCLUDED_
#define _RTMP_CONNECTION_H_INCLUDED_

#include "rtmp_types.h"
#include "rtmp_pool.h"
#include <sys/epoll.h>

/* Event types */
#define RTMP_READ_EVENT    EPOLLIN
#define RTMP_WRITE_EVENT   EPOLLOUT

/* Event structure */
struct rtmp_event_s {
    void *data;
    rtmp_event_handler_pt handler;
    
    unsigned write:1;
    unsigned active:1;
    unsigned ready:1;
    unsigned eof:1;
    unsigned error:1;
    unsigned timedout:1;
    unsigned timer_set:1;
    
    rtmp_msec_t timer;
};

/* Connection structure */
struct rtmp_connection_s {
    void *data;
    rtmp_socket_t fd;
    
    rtmp_event_t *read;
    rtmp_event_t *write;
    
    rtmp_recv_pt recv;
    rtmp_send_pt send;
    
    struct sockaddr *sockaddr;
    socklen_t socklen;
    rtmp_str_t addr_text;
    
    rtmp_pool_t *pool;
    
    unsigned destroyed:1;
    unsigned timedout:1;
    unsigned close:1;
    unsigned error:1;
};

/* Configuration context for compatibility */
typedef struct {
    void **main_conf;
    void **srv_conf;
    void **app_conf;
} rtmp_conf_ctx_t;

/* Connection functions */
rtmp_connection_t *rtmp_create_connection(rtmp_socket_t fd, rtmp_pool_t *pool);
void rtmp_close_connection(rtmp_connection_t *c);
rtmp_int_t rtmp_add_event(rtmp_event_t *ev, rtmp_int_t event, rtmp_int_t flags);
rtmp_int_t rtmp_del_event(rtmp_event_t *ev, rtmp_int_t event, rtmp_int_t flags);
void rtmp_add_timer(rtmp_event_t *ev, rtmp_msec_t timer);
void rtmp_del_timer(rtmp_event_t *ev);

/* Socket I/O functions */
rtmp_int_t rtmp_unix_recv(rtmp_connection_t *c, unsigned char *buf, size_t size);
rtmp_int_t rtmp_unix_send(rtmp_connection_t *c, unsigned char *buf, size_t size);

/* Chain sending function */
rtmp_chain_t *rtmp_send_chain(rtmp_connection_t *c, rtmp_chain_t *in, off_t limit);

/* Event loop functions */
rtmp_int_t rtmp_event_init(void);
void rtmp_event_loop(void);

/* Handle functions */
rtmp_int_t rtmp_handle_read_event(rtmp_event_t *rev, rtmp_int_t flags);
rtmp_int_t rtmp_handle_write_event(rtmp_event_t *wev, size_t lowat);

#endif /* _RTMP_CONNECTION_H_INCLUDED_ */