/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - Core header with compatibility layer
 */

#ifndef _RTMP_CORE_H_INCLUDED_
#define _RTMP_CORE_H_INCLUDED_

/* Include standalone types first */
#include "rtmp_types.h"
#include "rtmp_pool.h"
#include "rtmp_connection.h"
#include "rtmp_log.h"
#include <time.h>

/* Map nginx types to standalone types for compatibility */
#define ngx_int_t           rtmp_int_t
#define ngx_uint_t          rtmp_uint_t
#define ngx_flag_t          rtmp_flag_t
#define ngx_msec_t          rtmp_msec_t

#define ngx_str_t           rtmp_str_t
#define ngx_pool_t          rtmp_pool_t
#define ngx_chain_t         rtmp_chain_t
#define ngx_buf_t           rtmp_buf_t
#define ngx_array_t         rtmp_array_t

#define ngx_connection_t    rtmp_connection_t
#define ngx_event_t         rtmp_event_t
#define ngx_log_t           rtmp_log_t

#define ngx_conf_t          rtmp_conf_t
#define ngx_conf_ctx_t      rtmp_conf_ctx_t

/* Map nginx constants to standalone constants */
#define NGX_OK              RTMP_OK
#define NGX_ERROR           RTMP_ERROR
#define NGX_AGAIN           RTMP_AGAIN
#define NGX_DONE            RTMP_DONE
#define NGX_DECLINED        RTMP_DECLINED

#define NGX_READ_EVENT      RTMP_READ_EVENT
#define NGX_WRITE_EVENT     RTMP_WRITE_EVENT

#define NGX_LOG_STDERR      RTMP_LOG_STDERR
#define NGX_LOG_EMERG       RTMP_LOG_EMERG
#define NGX_LOG_ALERT       RTMP_LOG_ALERT
#define NGX_LOG_CRIT        RTMP_LOG_CRIT
#define NGX_LOG_ERR         RTMP_LOG_ERR
#define NGX_LOG_WARN        RTMP_LOG_WARN
#define NGX_LOG_NOTICE      RTMP_LOG_NOTICE
#define NGX_LOG_INFO        RTMP_LOG_INFO
#define NGX_LOG_DEBUG       RTMP_LOG_DEBUG
#define NGX_LOG_DEBUG_RTMP  RTMP_LOG_DEBUG_RTMP

/* Map nginx memory functions */
#define ngx_palloc          rtmp_palloc
#define ngx_pcalloc         rtmp_pcalloc
#define ngx_create_pool     rtmp_create_pool
#define ngx_destroy_pool    rtmp_destroy_pool

#define ngx_array_create    rtmp_array_create
#define ngx_array_push      rtmp_array_push
#define ngx_array_push_n    rtmp_array_push_n

#define ngx_alloc_chain_link rtmp_alloc_chain_link
#define ngx_create_temp_buf rtmp_create_temp_buf

/* Map nginx string functions */
#define ngx_strcmp          rtmp_strcmp
#define ngx_memcpy          rtmp_memcpy
#define ngx_memzero         rtmp_memzero
#define ngx_string          rtmp_string
#define ngx_null_string     rtmp_null_string

/* Map nginx connection functions */
#define ngx_add_event       rtmp_add_event
#define ngx_del_event       rtmp_del_event
#define ngx_add_timer       rtmp_add_timer
#define ngx_del_timer       rtmp_del_timer
#define ngx_close_connection rtmp_close_connection
#define ngx_handle_read_event rtmp_handle_read_event
#define ngx_handle_write_event rtmp_handle_write_event

/* Map nginx logging functions */
#define ngx_log_error       rtmp_log_error
#define ngx_log_debug0      rtmp_log_debug0
#define ngx_log_debug1      rtmp_log_debug1
#define ngx_log_debug2      rtmp_log_debug2
#define ngx_log_debug3      rtmp_log_debug3
#define ngx_log_debug4      rtmp_log_debug4
#define ngx_log_debug5      rtmp_log_debug5
#define ngx_log_debug6      rtmp_log_debug6
#define ngx_log_debug7      rtmp_log_debug7

/* Time functions - simplified */
#define ngx_time()          time(NULL)
#define ngx_current_msec    (time(NULL) * 1000)

/* Chain constants */
#define NGX_CHAIN_ERROR     ((rtmp_chain_t *) -1)
#define RTMP_CHAIN_ERROR    NGX_CHAIN_ERROR

/* Socket error constants */
#define NGX_EAGAIN          EAGAIN
#define NGX_ETIMEDOUT       ETIMEDOUT

/* Some utility macros that nginx uses */
#define ngx_conf_merge_value(conf, prev, default) \
    if (conf == NGX_CONF_UNSET) { \
        conf = (prev != NGX_CONF_UNSET) ? prev : default; \
    }

#define ngx_conf_merge_uint_value(conf, prev, default) \
    if (conf == NGX_CONF_UNSET_UINT) { \
        conf = (prev != NGX_CONF_UNSET_UINT) ? prev : default; \
    }

#define ngx_conf_merge_msec_value(conf, prev, default) \
    if (conf == NGX_CONF_UNSET_MSEC) { \
        conf = (prev != NGX_CONF_UNSET_MSEC) ? prev : default; \
    }

#define ngx_conf_merge_size_value(conf, prev, default) \
    if (conf == NGX_CONF_UNSET_SIZE) { \
        conf = (prev != NGX_CONF_UNSET_SIZE) ? prev : default; \
    }

/* Configuration unset values */
#define NGX_CONF_UNSET       -1
#define NGX_CONF_UNSET_UINT  ((rtmp_uint_t) -1)
#define NGX_CONF_UNSET_MSEC  ((rtmp_msec_t) -1)
#define NGX_CONF_UNSET_SIZE  ((size_t) -1)

#endif /* _RTMP_CORE_H_INCLUDED_ */