/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - RTMP Handshake implementation
 */

#include "rtmp_protocol.h"
#include <string.h>
#include <stdlib.h>

#define RTMP_HANDSHAKE_BUFSIZE  1537

/* Handshake stages */
#define RTMP_HANDSHAKE_SERVER_RECV_CHALLENGE1     0
#define RTMP_HANDSHAKE_SERVER_SEND_CHALLENGE2     1
#define RTMP_HANDSHAKE_SERVER_RECV_CHALLENGE2     2
#define RTMP_HANDSHAKE_SERVER_SEND_RESPONSE       3

static void rtmp_handshake_recv(rtmp_event_t *rev);
static void rtmp_handshake_send(rtmp_event_t *wev);
static rtmp_int_t rtmp_handshake_create_challenge(rtmp_session_t *s,
    unsigned char *buf, size_t bufsize);
static rtmp_int_t rtmp_handshake_create_response(rtmp_session_t *s,
    unsigned char *buf, size_t bufsize);

void
rtmp_handshake(rtmp_session_t *s)
{
    rtmp_connection_t *c;

    c = s->connection;

    if (c->destroyed) {
        return;
    }

    rtmp_log_debug1(RTMP_LOG_DEBUG, &rtmp_stderr_log, 0,
                   "rtmp handshake: stage %ui", s->hs_stage);

    switch (s->hs_stage) {
        case RTMP_HANDSHAKE_SERVER_RECV_CHALLENGE1:
            if (s->hs_buf == NULL) {
                s->hs_buf = rtmp_create_temp_buf(s->in_pool, RTMP_HANDSHAKE_BUFSIZE);
                if (s->hs_buf == NULL) {
                    rtmp_finalize_session(s);
                    return;
                }
            }
            
            c->read->handler = rtmp_handshake_recv;
            c->write->handler = rtmp_handshake_send;
            
            rtmp_handshake_recv(c->read);
            break;

        case RTMP_HANDSHAKE_SERVER_SEND_CHALLENGE2:
            rtmp_handshake_send(c->write);
            break;

        case RTMP_HANDSHAKE_SERVER_RECV_CHALLENGE2:
            rtmp_handshake_recv(c->read);
            break;

        case RTMP_HANDSHAKE_SERVER_SEND_RESPONSE:
            rtmp_handshake_send(c->write);
            break;

        case RTMP_HANDSHAKE_DONE:
            rtmp_log_debug0(RTMP_LOG_DEBUG, &rtmp_stderr_log, 0,
                           "rtmp handshake done");
            
            rtmp_fire_event(s, RTMP_HANDSHAKE_DONE, NULL, NULL);
            break;
    }
}

static void
rtmp_handshake_recv(rtmp_event_t *rev)
{
    rtmp_connection_t *c;
    rtmp_session_t *s;
    rtmp_buf_t *b;
    rtmp_int_t n;
    size_t required_size;

    c = rev->data;
    s = c->data;

    if (c->destroyed) {
        return;
    }

    if (rev->timedout) {
        rtmp_log_error(RTMP_LOG_INFO, &rtmp_stderr_log, 0,
                      "handshake: recv: client timed out");
        c->timedout = 1;
        rtmp_finalize_session(s);
        return;
    }

    if (rev->timer_set) {
        rtmp_del_timer(rev);
    }

    b = s->hs_buf;

    /* Determine required size based on handshake stage */
    switch (s->hs_stage) {
        case RTMP_HANDSHAKE_SERVER_RECV_CHALLENGE1:
            required_size = 1537;  /* C0 + C1 */
            break;
        case RTMP_HANDSHAKE_SERVER_RECV_CHALLENGE2:
            required_size = 1536;  /* C2 */
            break;
        default:
            rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, 0,
                          "handshake: unexpected stage %ui", s->hs_stage);
            rtmp_finalize_session(s);
            return;
    }

    while (b->last - b->start < (rtmp_int_t) required_size) {
        n = c->recv(c, b->last, b->end - b->last);

        if (n == RTMP_ERROR || n == 0) {
            rtmp_finalize_session(s);
            return;
        }

        if (n == RTMP_AGAIN) {
            rtmp_add_timer(rev, s->timeout);
            if (rtmp_handle_read_event(c->read, 0) != RTMP_OK) {
                rtmp_finalize_session(s);
            }
            return;
        }

        b->last += n;
    }

    if (rev->active) {
        rtmp_del_event(rev, RTMP_READ_EVENT, 0);
    }

    ++s->hs_stage;
    rtmp_log_debug1(RTMP_LOG_DEBUG, &rtmp_stderr_log, 0,
                   "handshake: stage %ui", s->hs_stage);

    switch (s->hs_stage) {
        case RTMP_HANDSHAKE_SERVER_SEND_CHALLENGE2:
            /* Check protocol version */
            if (b->start[0] != 3) {
                rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, 0,
                              "handshake: unsupported RTMP version: %d", 
                              (int) b->start[0]);
                rtmp_finalize_session(s);
                return;
            }
            /* Fall through to send response */
            rtmp_handshake(s);
            break;

        case RTMP_HANDSHAKE_DONE:
            rtmp_handshake(s);
            break;

        default:
            rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, 0,
                          "handshake: unexpected stage %ui", s->hs_stage);
            rtmp_finalize_session(s);
    }
}

static void
rtmp_handshake_send(rtmp_event_t *wev)
{
    rtmp_connection_t *c;
    rtmp_session_t *s;
    rtmp_buf_t *b;
    rtmp_int_t n;

    c = wev->data;
    s = c->data;

    if (c->destroyed) {
        return;
    }

    if (wev->timedout) {
        rtmp_log_error(RTMP_LOG_INFO, &rtmp_stderr_log, 0,
                      "handshake: send: client timed out");
        c->timedout = 1;
        rtmp_finalize_session(s);
        return;
    }

    if (wev->timer_set) {
        rtmp_del_timer(wev);
    }

    b = s->hs_buf;

    /* Create handshake data if needed */
    if (s->hs_stage == RTMP_HANDSHAKE_SERVER_SEND_CHALLENGE2) {
        /* Reset buffer for output */
        b->pos = b->start;
        b->last = b->start;

        /* Create S0 + S1 + S2 */
        if (rtmp_handshake_create_challenge(s, b->last, 
                                           b->end - b->last) != RTMP_OK) {
            rtmp_finalize_session(s);
            return;
        }
        b->last += 1537 + 1536;  /* S0 + S1 + S2 */
        b->pos = b->start;
    }

    while (b->pos != b->last) {
        n = c->send(c, b->pos, b->last - b->pos);

        if (n == RTMP_ERROR) {
            rtmp_finalize_session(s);
            return;
        }

        if (n == RTMP_AGAIN || n == 0) {
            rtmp_add_timer(c->write, s->timeout);
            if (rtmp_handle_write_event(c->write, 0) != RTMP_OK) {
                rtmp_finalize_session(s);
            }
            return;
        }

        b->pos += n;
    }

    if (wev->active) {
        rtmp_del_event(wev, RTMP_WRITE_EVENT, 0);
    }

    ++s->hs_stage;
    rtmp_log_debug1(RTMP_LOG_DEBUG, &rtmp_stderr_log, 0,
                   "handshake: stage %ui", s->hs_stage);

    switch (s->hs_stage) {
        case RTMP_HANDSHAKE_SERVER_RECV_CHALLENGE2:
            /* Reset buffer for input */
            b->pos = b->start;
            b->last = b->start;
            rtmp_handshake(s);
            break;

        case RTMP_HANDSHAKE_DONE:
            /* Free handshake buffer */
            s->hs_buf = NULL;
            rtmp_handshake(s);
            break;

        default:
            rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, 0,
                          "handshake: unexpected stage %ui", s->hs_stage);
            rtmp_finalize_session(s);
    }
}

static rtmp_int_t
rtmp_handshake_create_challenge(rtmp_session_t *s, unsigned char *buf, 
                               size_t bufsize)
{
    unsigned char *p;
    uint32_t timestamp;
    
    if (bufsize < 1537 + 1536) {  /* S0 + S1 + S2 */
        return RTMP_ERROR;
    }

    p = buf;

    /* S0: version */
    *p++ = 3;

    /* S1: timestamp + zero + random */
    timestamp = (uint32_t) time(NULL);
    *p++ = (timestamp >> 24) & 0xFF;
    *p++ = (timestamp >> 16) & 0xFF;
    *p++ = (timestamp >> 8) & 0xFF;
    *p++ = timestamp & 0xFF;

    /* 4 bytes zero */
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;

    /* 1528 bytes random data */
    for (int i = 0; i < 1528; i++) {
        *p++ = (unsigned char) (rand() & 0xFF);
    }

    /* S2: echo of client's C1 */
    if (s->hs_buf && s->hs_buf->last - s->hs_buf->start >= 1537) {
        /* Copy C1 as S2 */
        memcpy(p, s->hs_buf->start + 1, 1536);
    } else {
        /* Fill with zeros if C1 not available */
        memset(p, 0, 1536);
    }

    return RTMP_OK;
}

void
rtmp_free_handshake_buffers(rtmp_session_t *s)
{
    /* Handshake buffers are allocated from session pool,
     * so they will be freed with the pool */
    s->hs_buf = NULL;
    s->hs_digest = NULL;
}