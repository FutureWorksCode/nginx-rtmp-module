/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - RTMP protocol definitions
 */

#ifndef _RTMP_PROTOCOL_H_INCLUDED_
#define _RTMP_PROTOCOL_H_INCLUDED_

#include "rtmp_core.h"

/* RTMP Protocol Constants */
#define RTMP_VERSION                    3
#define RTMP_DEFAULT_CHUNK_SIZE         128
#define RTMP_MAX_CHUNK_SIZE             10485760
#define RTMP_MAX_CHUNK_HEADER           18

/* RTMP message types */
#define RTMP_MSG_CHUNK_SIZE             1
#define RTMP_MSG_ABORT                  2
#define RTMP_MSG_ACK                    3
#define RTMP_MSG_USER                   4
#define RTMP_MSG_ACK_SIZE               5
#define RTMP_MSG_BANDWIDTH              6
#define RTMP_MSG_EDGE                   7
#define RTMP_MSG_AUDIO                  8
#define RTMP_MSG_VIDEO                  9
#define RTMP_MSG_AMF3_META              15
#define RTMP_MSG_AMF3_SHARED            16
#define RTMP_MSG_AMF3_CMD               17
#define RTMP_MSG_AMF_META               18
#define RTMP_MSG_AMF_SHARED             19
#define RTMP_MSG_AMF_CMD                20
#define RTMP_MSG_AGGREGATE              22
#define RTMP_MSG_MAX                    22

/* RTMP events */
#define RTMP_CONNECT                    (RTMP_MSG_MAX + 1)
#define RTMP_DISCONNECT                 (RTMP_MSG_MAX + 2)
#define RTMP_HANDSHAKE_DONE             (RTMP_MSG_MAX + 3)
#define RTMP_MAX_EVENT                  (RTMP_MSG_MAX + 4)

/* RTMP user control message types */
#define RTMP_USER_STREAM_BEGIN          0
#define RTMP_USER_STREAM_EOF            1
#define RTMP_USER_STREAM_DRY            2
#define RTMP_USER_SET_BUFLEN            3
#define RTMP_USER_RECORDED              4
#define RTMP_USER_PING_REQUEST          6
#define RTMP_USER_PING_RESPONSE         7
#define RTMP_USER_UNKNOWN               8
#define RTMP_USER_BUFFER_END            31

/* RTMP handshake stages */

/* RTMP Header structure */
typedef struct {
    uint32_t    csid;       /* chunk stream id */
    uint32_t    timestamp;  /* timestamp (delta) */
    uint32_t    mlen;       /* message length */
    uint8_t     type;       /* message type id */
    uint32_t    msid;       /* message stream id */
} rtmp_header_t;

/* RTMP Stream structure */
typedef struct {
    rtmp_header_t   hdr;
    uint32_t        dtime;
    uint32_t        len;        /* current fragment length */
    uint8_t         ext;
    rtmp_chain_t   *in;
} rtmp_stream_t;

/* Forward declarations */
typedef struct rtmp_session_s rtmp_session_t;

/* RTMP handler function type */
typedef rtmp_int_t (*rtmp_handler_pt)(rtmp_session_t *s,
                                      rtmp_header_t *h, 
                                      rtmp_chain_t *in);

/* RTMP Session structure */
struct rtmp_session_s {
    uint32_t                signature;  /* "RTMP" */
    
    rtmp_event_t            close;
    
    void                  **ctx;        /* module contexts */
    
    rtmp_str_t             *addr_text;
    int                     connected;
    
    /* client buffer time in msec */
    uint32_t                buflen;
    uint32_t                ack_size;
    
    /* connection parameters */
    rtmp_str_t              app;
    rtmp_str_t              args;
    rtmp_str_t              flashver;
    rtmp_str_t              swf_url;
    rtmp_str_t              tc_url;
    uint32_t                acodecs;
    uint32_t                vcodecs;
    rtmp_str_t              page_url;
    
    /* handshake data */
    rtmp_buf_t             *hs_buf;
    unsigned char          *hs_digest;
    unsigned                hs_old:1;
    rtmp_uint_t             hs_stage;
    
    /* connection timestamps */
    rtmp_msec_t             epoch;
    rtmp_msec_t             peer_epoch;
    rtmp_msec_t             base_time;
    uint32_t                current_time;
    
    /* ping */
    rtmp_event_t            ping_evt;
    unsigned                ping_active:1;
    unsigned                ping_reset:1;
    
    /* auto-pushed? */
    unsigned                auto_pushed:1;
    unsigned                relay:1;
    unsigned                static_relay:1;
    
    /* input streams */
    rtmp_stream_t          *in_streams;
    uint32_t                in_csid;
    rtmp_uint_t             in_chunk_size;
    rtmp_pool_t            *in_pool;
    uint32_t                in_bytes;
    uint32_t                in_last_ack;
    
    rtmp_pool_t            *in_old_pool;
    rtmp_int_t              in_chunk_size_changing;
    
    rtmp_connection_t      *connection;
    
    /* output */
    rtmp_msec_t             timeout;
    uint32_t                out_bytes;
    size_t                  out_pos, out_last;
    rtmp_chain_t           *out_chain;
    unsigned char          *out_bpos;
    unsigned                out_buffer:1;
    size_t                  out_queue;
    size_t                  out_cork;
    rtmp_chain_t           *out[0];
};

/* Configuration structures */
typedef struct {
    rtmp_array_t            servers;
    rtmp_array_t            listen;
    rtmp_array_t            events[RTMP_MAX_EVENT];
} rtmp_core_main_conf_t;

typedef struct {
    rtmp_array_t            applications;
    rtmp_msec_t             timeout;
    rtmp_msec_t             ping;
    rtmp_msec_t             ping_timeout;
    rtmp_flag_t             so_keepalive;
    rtmp_int_t              max_streams;
    rtmp_uint_t             ack_window;
    rtmp_int_t              chunk_size;
    rtmp_pool_t            *pool;
    rtmp_chain_t           *free;
    rtmp_chain_t           *free_hs;
    size_t                  max_message;
    rtmp_flag_t             play_time_fix;
    rtmp_flag_t             publish_time_fix;
    rtmp_flag_t             busy;
    size_t                  out_queue;
    size_t                  out_cork;
    rtmp_msec_t             buflen;
} rtmp_core_srv_conf_t;

typedef struct {
    rtmp_array_t            applications;
    rtmp_str_t              name;
    void                  **app_conf;
} rtmp_core_app_conf_t;

/* Function declarations */
void rtmp_init_connection(rtmp_connection_t *c);
rtmp_session_t *rtmp_init_session(rtmp_connection_t *c);
void rtmp_finalize_session(rtmp_session_t *s);
void rtmp_handshake(rtmp_session_t *s);
void rtmp_cycle(rtmp_session_t *s);
void rtmp_reset_ping(rtmp_session_t *s);

rtmp_int_t rtmp_fire_event(rtmp_session_t *s, rtmp_uint_t evt,
                          rtmp_header_t *h, rtmp_chain_t *in);

/* Message handling functions */
rtmp_int_t rtmp_receive_message(rtmp_session_t *s,
                               rtmp_header_t *h, rtmp_chain_t *in);
                               
rtmp_int_t rtmp_protocol_message_handler(rtmp_session_t *s,
                                        rtmp_header_t *h, rtmp_chain_t *in);
                                        
rtmp_int_t rtmp_user_message_handler(rtmp_session_t *s,
                                    rtmp_header_t *h, rtmp_chain_t *in);

/* Message sending functions */
void rtmp_prepare_message(rtmp_session_t *s, rtmp_header_t *h,
                         rtmp_header_t *lh, rtmp_chain_t *out);
                         
rtmp_int_t rtmp_send_message(rtmp_session_t *s, rtmp_chain_t *out,
                            rtmp_uint_t priority);

/* Protocol control messages */
rtmp_int_t rtmp_send_chunk_size(rtmp_session_t *s, uint32_t chunk_size);
rtmp_int_t rtmp_send_ack(rtmp_session_t *s, uint32_t seq);
rtmp_int_t rtmp_send_ack_size(rtmp_session_t *s, uint32_t ack_size);
rtmp_int_t rtmp_send_bandwidth(rtmp_session_t *s, uint32_t ack_size, uint8_t limit_type);

/* User control messages */
rtmp_int_t rtmp_send_stream_begin(rtmp_session_t *s, uint32_t msid);
rtmp_int_t rtmp_send_stream_eof(rtmp_session_t *s, uint32_t msid);
rtmp_int_t rtmp_send_ping_request(rtmp_session_t *s, uint32_t timestamp);
rtmp_int_t rtmp_send_ping_response(rtmp_session_t *s, uint32_t timestamp);

/* Chunk size management */
rtmp_int_t rtmp_set_chunk_size(rtmp_session_t *s, rtmp_uint_t size);

/* Bit manipulation functions */
void *rtmp_rmemcpy(void *dst, const void* src, size_t n);

#define rtmp_rcpymem(dst, src, n) \
    (((unsigned char*)rtmp_rmemcpy(dst, src, n)) + (n))

static inline uint16_t
rtmp_r16(uint16_t n)
{
    return (n << 8) | (n >> 8);
}

static inline uint32_t
rtmp_r32(uint32_t n)
{
    return (n << 24) | ((n << 8) & 0xff0000) | ((n >> 8) & 0xff00) | (n >> 24);
}

static inline uint64_t
rtmp_r64(uint64_t n)
{
    return (uint64_t) rtmp_r32((uint32_t) n) << 32 |
                      rtmp_r32((uint32_t) (n >> 32));
}

/* Frame types */
#define RTMP_VIDEO_KEY_FRAME            1
#define RTMP_VIDEO_INTER_FRAME          2
#define RTMP_VIDEO_DISPOSABLE_FRAME     3

static inline rtmp_int_t
rtmp_get_video_frame_type(rtmp_chain_t *in)
{
    return (in->buf->pos[0] & 0xf0) >> 4;
}

static inline rtmp_int_t
rtmp_is_codec_header(rtmp_chain_t *in)
{
    return in->buf->pos + 1 < in->buf->last && in->buf->pos[1] == 0;
}

/* Debug functions */
#ifdef DEBUG
char *rtmp_message_type(uint8_t type);
char *rtmp_user_message_type(uint16_t evt);
#endif

#endif /* _RTMP_PROTOCOL_H_INCLUDED_ */