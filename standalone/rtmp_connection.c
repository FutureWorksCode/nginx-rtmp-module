/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - Connection and event implementation
 */

#include "rtmp_connection.h"
#include "rtmp_log.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#define RTMP_CHAIN_ERROR ((rtmp_chain_t *) -1)

/* Global event system variables */
static int epoll_fd = -1;
static rtmp_event_t *timer_queue = NULL;

rtmp_int_t
rtmp_event_init(void)
{
    epoll_fd = epoll_create(1024);
    if (epoll_fd == -1) {
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "epoll_create() failed");
        return RTMP_ERROR;
    }

    return RTMP_OK;
}

rtmp_connection_t *
rtmp_create_connection(rtmp_socket_t fd, rtmp_pool_t *pool)
{
    rtmp_connection_t *c;

    c = rtmp_pcalloc(pool, sizeof(rtmp_connection_t));
    if (c == NULL) {
        return NULL;
    }

    c->fd = fd;
    c->pool = pool;

    c->read = rtmp_pcalloc(pool, sizeof(rtmp_event_t));
    if (c->read == NULL) {
        return NULL;
    }

    c->write = rtmp_pcalloc(pool, sizeof(rtmp_event_t));
    if (c->write == NULL) {
        return NULL;
    }

    c->read->data = c;
    c->write->data = c;
    c->write->write = 1;

    c->recv = rtmp_unix_recv;
    c->send = rtmp_unix_send;

    /* Set socket to non-blocking */
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "fcntl(O_NONBLOCK) failed");
        return NULL;
    }

    return c;
}

void
rtmp_close_connection(rtmp_connection_t *c)
{
    if (c->fd != -1) {
        if (c->read->active) {
            rtmp_del_event(c->read, RTMP_READ_EVENT, 0);
        }
        if (c->write->active) {
            rtmp_del_event(c->write, RTMP_WRITE_EVENT, 0);
        }

        close(c->fd);
        c->fd = -1;
    }

    c->destroyed = 1;
}

rtmp_int_t
rtmp_add_event(rtmp_event_t *ev, rtmp_int_t event, rtmp_int_t flags)
{
    struct epoll_event epev;
    rtmp_connection_t *c;
    int op;

    if (epoll_fd == -1) {
        return RTMP_ERROR;
    }

    c = ev->data;

    epev.events = event | EPOLLET;  /* Edge-triggered */
    epev.data.ptr = ev;

    if (ev->active) {
        op = EPOLL_CTL_MOD;
    } else {
        op = EPOLL_CTL_ADD;
        ev->active = 1;
    }

    if (epoll_ctl(epoll_fd, op, c->fd, &epev) == -1) {
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "epoll_ctl(%d) failed", op);
        return RTMP_ERROR;
    }

    return RTMP_OK;
}

rtmp_int_t
rtmp_del_event(rtmp_event_t *ev, rtmp_int_t event, rtmp_int_t flags)
{
    struct epoll_event epev;
    rtmp_connection_t *c;

    if (epoll_fd == -1 || !ev->active) {
        return RTMP_OK;
    }

    c = ev->data;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->fd, &epev) == -1) {
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "epoll_ctl(EPOLL_CTL_DEL) failed");
        return RTMP_ERROR;
    }

    ev->active = 0;
    return RTMP_OK;
}

void
rtmp_add_timer(rtmp_event_t *ev, rtmp_msec_t timer)
{
    ev->timer = time(NULL) + timer / 1000;
    ev->timer_set = 1;
    
    /* Add to timer queue - simplified implementation */
    ev->timer = timer;
    /* TODO: Implement proper timer management */
}

void
rtmp_del_timer(rtmp_event_t *ev)
{
    ev->timer_set = 0;
    /* TODO: Remove from timer queue */
}

rtmp_int_t
rtmp_unix_recv(rtmp_connection_t *c, unsigned char *buf, size_t size)
{
    ssize_t n;

    n = recv(c->fd, buf, size, 0);

    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return RTMP_AGAIN;
        }
        
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "recv() failed");
        return RTMP_ERROR;
    }

    if (n == 0) {
        return RTMP_ERROR;  /* Connection closed */
    }

    return n;
}

rtmp_int_t
rtmp_unix_send(rtmp_connection_t *c, unsigned char *buf, size_t size)
{
    ssize_t n;

    n = send(c->fd, buf, size, 0);

    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return RTMP_AGAIN;
        }
        
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "send() failed");
        return RTMP_ERROR;
    }

    return n;
}

rtmp_chain_t *
rtmp_send_chain(rtmp_connection_t *c, rtmp_chain_t *in, off_t limit)
{
    rtmp_chain_t *cl;
    rtmp_buf_t *b;
    rtmp_int_t n;
    size_t size;

    for (cl = in; cl; cl = cl->next) {
        b = cl->buf;
        
        while (b->pos < b->last) {
            size = b->last - b->pos;
            if (limit && size > (size_t) limit) {
                size = (size_t) limit;
            }

            n = c->send(c, b->pos, size);

            if (n == RTMP_ERROR) {
                return RTMP_CHAIN_ERROR;
            }

            if (n == RTMP_AGAIN) {
                return cl;
            }

            b->pos += n;
            
            if (limit) {
                limit -= n;
                if (limit == 0) {
                    return cl;
                }
            }
        }
    }

    return NULL;
}

rtmp_int_t
rtmp_handle_read_event(rtmp_event_t *rev, rtmp_int_t flags)
{
    if (!rev->active) {
        return rtmp_add_event(rev, RTMP_READ_EVENT, flags);
    }
    
    return RTMP_OK;
}

rtmp_int_t
rtmp_handle_write_event(rtmp_event_t *wev, size_t lowat)
{
    if (!wev->active) {
        return rtmp_add_event(wev, RTMP_WRITE_EVENT, 0);
    }
    
    return RTMP_OK;
}

void
rtmp_event_loop(void)
{
    struct epoll_event events[1024];
    int nev, i;
    rtmp_event_t *ev;

    for (;;) {
        nev = epoll_wait(epoll_fd, events, 1024, 1000);  /* 1 second timeout */

        if (nev == -1) {
            if (errno == EINTR) {
                continue;
            }
            
            rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                          "epoll_wait() failed");
            break;
        }

        for (i = 0; i < nev; i++) {
            ev = events[i].data.ptr;

            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                ev->error = 1;
            }

            if (events[i].events & EPOLLIN) {
                ev->ready = 1;
            }

            if (ev->handler) {
                ev->handler(ev);
            }
        }

        /* Process timers - simplified */
        /* TODO: Implement proper timer processing */
    }
}