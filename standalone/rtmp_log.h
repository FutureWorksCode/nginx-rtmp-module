/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - Logging abstraction
 */

#ifndef _RTMP_LOG_H_INCLUDED_
#define _RTMP_LOG_H_INCLUDED_

#include "rtmp_types.h"
#include <stdio.h>
#include <stdarg.h>

/* Log structure */
typedef struct {
    FILE *file;
    rtmp_uint_t level;
    void *data;
    rtmp_connection_t *connection;
} rtmp_log_t;

/* Global log instance */
extern rtmp_log_t rtmp_stderr_log;

/* Logging functions */
void rtmp_log_error_core(rtmp_uint_t level, rtmp_log_t *log, int err, 
                        const char *fmt, ...);

void rtmp_log_debug_core(rtmp_log_t *log, const char *fmt, ...);

/* Logging macros for compatibility with nginx style */
#define rtmp_log_error(level, log, err, fmt, ...) \
    rtmp_log_error_core(level, log, err, fmt, ##__VA_ARGS__)

#define rtmp_log_debug0(level, log, err, fmt) \
    rtmp_log_debug_core(log, fmt)

#define rtmp_log_debug1(level, log, err, fmt, arg1) \
    rtmp_log_debug_core(log, fmt, arg1)

#define rtmp_log_debug2(level, log, err, fmt, arg1, arg2) \
    rtmp_log_debug_core(log, fmt, arg1, arg2)

#define rtmp_log_debug3(level, log, err, fmt, arg1, arg2, arg3) \
    rtmp_log_debug_core(log, fmt, arg1, arg2, arg3)

#define rtmp_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4) \
    rtmp_log_debug_core(log, fmt, arg1, arg2, arg3, arg4)

#define rtmp_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5) \
    rtmp_log_debug_core(log, fmt, arg1, arg2, arg3, arg4, arg5)

#define rtmp_log_debug6(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6) \
    rtmp_log_debug_core(log, fmt, arg1, arg2, arg3, arg4, arg5, arg6)

#define rtmp_log_debug7(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    rtmp_log_debug_core(log, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)

/* Initialize logging */
rtmp_int_t rtmp_log_init(void);

#endif /* _RTMP_LOG_H_INCLUDED_ */