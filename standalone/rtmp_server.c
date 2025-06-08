/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - TCP Server implementation  
 */

#include "rtmp_protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#define RTMP_DEFAULT_PORT    1935
#define RTMP_LISTEN_BACKLOG  128

static rtmp_int_t rtmp_server_init(int port);
static void rtmp_accept_handler(rtmp_event_t *ev);
static volatile int running = 1;

static void
signal_handler(int sig)
{
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            running = 0;
            break;
        case SIGPIPE:
            /* Ignore broken pipe */
            break;
    }
}

int
main(int argc, char **argv)
{
    int port = RTMP_DEFAULT_PORT;
    rtmp_socket_t listen_fd;
    rtmp_connection_t *ls;
    rtmp_pool_t *pool;

    /* Parse command line arguments */
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[1]);
            return 1;
        }
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, signal_handler);

    /* Initialize logging */
    if (rtmp_log_init() != RTMP_OK) {
        fprintf(stderr, "Failed to initialize logging\n");
        return 1;
    }

    /* Initialize event system */
    if (rtmp_event_init() != RTMP_OK) {
        fprintf(stderr, "Failed to initialize event system\n");
        return 1;
    }

    /* Initialize server socket */
    listen_fd = rtmp_server_init(port);
    if (listen_fd == -1) {
        fprintf(stderr, "Failed to initialize server on port %d\n", port);
        return 1;
    }

    /* Create listening connection */
    pool = rtmp_create_pool(4096);
    if (pool == NULL) {
        fprintf(stderr, "Failed to create memory pool\n");
        close(listen_fd);
        return 1;
    }

    ls = rtmp_create_connection(listen_fd, pool);
    if (ls == NULL) {
        fprintf(stderr, "Failed to create listening connection\n");
        close(listen_fd);
        rtmp_destroy_pool(pool);
        return 1;
    }

    ls->read->handler = rtmp_accept_handler;
    ls->data = (void *) (intptr_t) listen_fd;  /* Store listen socket */

    if (rtmp_add_event(ls->read, RTMP_READ_EVENT, 0) != RTMP_OK) {
        fprintf(stderr, "Failed to add listen event\n");
        close(listen_fd);
        rtmp_destroy_pool(pool);
        return 1;
    }

    printf("RTMP Server listening on port %d\n", port);
    printf("Press Ctrl+C to stop\n");

    /* Main event loop */
    while (running) {
        rtmp_event_loop();
    }

    printf("\nShutting down server...\n");

    /* Cleanup */
    close(listen_fd);
    rtmp_destroy_pool(pool);

    return 0;
}

static rtmp_int_t
rtmp_server_init(int port)
{
    rtmp_socket_t fd;
    struct sockaddr_in addr;
    int reuse = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "socket() failed");
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        rtmp_log_error(RTMP_LOG_WARN, &rtmp_stderr_log, errno,
                      "setsockopt(SO_REUSEADDR) failed");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "bind() failed");
        close(fd);
        return -1;
    }

    if (listen(fd, RTMP_LISTEN_BACKLOG) == -1) {
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "listen() failed");
        close(fd);
        return -1;
    }

    /* Set non-blocking */
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                      "fcntl(O_NONBLOCK) failed");
        close(fd);
        return -1;
    }

    return fd;
}

static void
rtmp_accept_handler(rtmp_event_t *ev)
{
    rtmp_connection_t *ls, *c;
    rtmp_socket_t s;
    struct sockaddr_in addr;
    socklen_t addrlen;
    rtmp_pool_t *pool;
    char addr_str[INET_ADDRSTRLEN];

    ls = ev->data;

    if (ls->destroyed) {
        return;
    }

    for (;;) {
        addrlen = sizeof(addr);
        s = accept(ls->fd, (struct sockaddr *) &addr, &addrlen);

        if (s == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;  /* No more connections */
            }

            if (errno == ECONNABORTED) {
                continue;  /* Try again */
            }

            rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, errno,
                          "accept() failed");
            return;
        }

        /* Create new connection */
        pool = rtmp_create_pool(4096);
        if (pool == NULL) {
            rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, 0,
                          "failed to create pool for new connection");
            close(s);
            continue;
        }

        c = rtmp_create_connection(s, pool);
        if (c == NULL) {
            rtmp_log_error(RTMP_LOG_ERR, &rtmp_stderr_log, 0,
                          "failed to create connection");
            close(s);
            rtmp_destroy_pool(pool);
            continue;
        }

        /* Store client address */
        c->sockaddr = rtmp_palloc(pool, sizeof(struct sockaddr_in));
        if (c->sockaddr) {
            memcpy(c->sockaddr, &addr, sizeof(addr));
            c->socklen = addrlen;
        }

        /* Create address text */
        if (inet_ntop(AF_INET, &addr.sin_addr, addr_str, sizeof(addr_str))) {
            c->addr_text.len = strlen(addr_str);
            c->addr_text.data = rtmp_palloc(pool, c->addr_text.len);
            if (c->addr_text.data) {
                memcpy(c->addr_text.data, addr_str, c->addr_text.len);
            }
        }

        rtmp_log_error(RTMP_LOG_INFO, &rtmp_stderr_log, 0,
                      "accepted connection from %s:%d",
                      addr_str, ntohs(addr.sin_port));

        /* Initialize RTMP connection */
        rtmp_init_connection(c);
    }
}