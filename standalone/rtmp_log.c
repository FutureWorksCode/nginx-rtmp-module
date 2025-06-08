/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - Logging implementation
 */

#include "rtmp_log.h"
#include <errno.h>
#include <string.h>

rtmp_log_t rtmp_stderr_log = {
    .file = NULL,
    .level = RTMP_LOG_DEBUG,
    .data = NULL,
    .connection = NULL
};

rtmp_int_t
rtmp_log_init(void)
{
    rtmp_stderr_log.file = stderr;
    return RTMP_OK;
}

void
rtmp_log_error_core(rtmp_uint_t level, rtmp_log_t *log, int err, 
                   const char *fmt, ...)
{
    va_list args;
    char buf[4096];
    const char *level_str;
    FILE *out;

    if (log == NULL) {
        log = &rtmp_stderr_log;
    }

    if (log->level < level) {
        return;
    }

    out = log->file ? log->file : stderr;

    switch (level) {
        case RTMP_LOG_STDERR:
        case RTMP_LOG_EMERG:
            level_str = "emerg";
            break;
        case RTMP_LOG_ALERT:
            level_str = "alert";
            break;
        case RTMP_LOG_CRIT:
            level_str = "crit";
            break;
        case RTMP_LOG_ERR:
            level_str = "error";
            break;
        case RTMP_LOG_WARN:
            level_str = "warn";
            break;
        case RTMP_LOG_NOTICE:
            level_str = "notice";
            break;
        case RTMP_LOG_INFO:
            level_str = "info";
            break;
        case RTMP_LOG_DEBUG:
        default:
            level_str = "debug";
            break;
    }

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    fprintf(out, "[%s] %s", level_str, buf);
    
    if (err) {
        fprintf(out, " (%d: %s)", err, strerror(err));
    }
    
    fprintf(out, "\n");
    fflush(out);
}

void
rtmp_log_debug_core(rtmp_log_t *log, const char *fmt, ...)
{
    va_list args;
    char buf[4096];
    FILE *out;

    if (log == NULL) {
        log = &rtmp_stderr_log;
    }

    if (log->level < RTMP_LOG_DEBUG) {
        return;
    }

    out = log->file ? log->file : stderr;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    fprintf(out, "[debug] %s\n", buf);
    fflush(out);
}