/*
 * Copyright (C) Roman Arutyunyan
 * Standalone RTMP Server - Memory pool implementation
 */

#ifndef _RTMP_POOL_H_INCLUDED_
#define _RTMP_POOL_H_INCLUDED_

#include "rtmp_types.h"
#include <stdlib.h>
#include <string.h>

/* Memory pool structure */
struct rtmp_pool_s {
    rtmp_pool_t *next;
    unsigned char *last;
    unsigned char *end;
    size_t size;
    void *cleanup_list;
};

/* Memory pool functions */
rtmp_pool_t *rtmp_create_pool(size_t size);
void rtmp_destroy_pool(rtmp_pool_t *pool);
void *rtmp_pool_alloc(rtmp_pool_t *pool, size_t size);
void *rtmp_pool_calloc(rtmp_pool_t *pool, size_t size);
void rtmp_pool_reset(rtmp_pool_t *pool);

/* Array functions */
rtmp_array_t *rtmp_array_create(rtmp_pool_t *pool, rtmp_uint_t n, size_t size);
void *rtmp_array_push(rtmp_array_t *a);
void *rtmp_array_push_n(rtmp_array_t *a, rtmp_uint_t n);

/* Chain functions */
rtmp_chain_t *rtmp_alloc_chain_link(rtmp_pool_t *pool);
rtmp_buf_t *rtmp_create_temp_buf(rtmp_pool_t *pool, size_t size);

/* String functions */
rtmp_int_t rtmp_strcmp(const unsigned char *s1, const unsigned char *s2);
void rtmp_memcpy(void *dst, const void *src, size_t n);
void rtmp_memzero(void *buf, size_t n);

#endif /* _RTMP_POOL_H_INCLUDED_ */