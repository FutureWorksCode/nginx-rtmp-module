/*
 * Copyright (C) Roman Arutyunyan  
 * Standalone RTMP Server - Memory pool implementation
 */

#include "rtmp_pool.h"
#include <stdio.h>

#define RTMP_POOL_ALIGNMENT       16
#define RTMP_DEFAULT_POOL_SIZE    4096

static void *rtmp_palloc_small(rtmp_pool_t *pool, size_t size);
static void *rtmp_palloc_large(rtmp_pool_t *pool, size_t size);

rtmp_pool_t *
rtmp_create_pool(size_t size)
{
    rtmp_pool_t *p;

    if (size < RTMP_DEFAULT_POOL_SIZE) {
        size = RTMP_DEFAULT_POOL_SIZE;
    }

    p = (rtmp_pool_t *) malloc(size);
    if (p == NULL) {
        return NULL;
    }

    p->last = (unsigned char *) p + sizeof(rtmp_pool_t);
    p->end = (unsigned char *) p + size;
    p->next = NULL;
    p->size = size;
    p->cleanup_list = NULL;

    return p;
}

void
rtmp_destroy_pool(rtmp_pool_t *pool)
{
    rtmp_pool_t *p, *n;

    for (p = pool; p; p = n) {
        n = p->next;
        free(p);
    }
}

void *
rtmp_pool_alloc(rtmp_pool_t *pool, size_t size)
{
    if (size <= (size_t) (pool->end - pool->last)) {
        return rtmp_palloc_small(pool, size);
    }

    return rtmp_palloc_large(pool, size);
}

void *
rtmp_pool_calloc(rtmp_pool_t *pool, size_t size)
{
    void *p;

    p = rtmp_pool_alloc(pool, size);
    if (p) {
        rtmp_memzero(p, size);
    }

    return p;
}

void
rtmp_pool_reset(rtmp_pool_t *pool)
{
    rtmp_pool_t *p;

    for (p = pool; p; p = p->next) {
        p->last = (unsigned char *) p + sizeof(rtmp_pool_t);
    }
}

static void *
rtmp_palloc_small(rtmp_pool_t *pool, size_t size)
{
    unsigned char *m;

    m = pool->last;
    
    /* Align to RTMP_POOL_ALIGNMENT */
    m = (unsigned char *) (((uintptr_t) m + (RTMP_POOL_ALIGNMENT - 1))
                          & ~(RTMP_POOL_ALIGNMENT - 1));

    if ((size_t) (pool->end - m) >= size) {
        pool->last = m + size;
        return m;
    }

    return rtmp_palloc_large(pool, size);
}

static void *
rtmp_palloc_large(rtmp_pool_t *pool, size_t size)
{
    void *p;
    rtmp_pool_t *new_pool;
    size_t pool_size;

    p = malloc(size);
    if (p == NULL) {
        return NULL;
    }

    /* If size is reasonable, allocate a new pool */
    if (size < RTMP_DEFAULT_POOL_SIZE) {
        pool_size = RTMP_DEFAULT_POOL_SIZE;
        new_pool = rtmp_create_pool(pool_size);
        if (new_pool) {
            new_pool->next = pool->next;
            pool->next = new_pool;
        }
    }

    return p;
}

rtmp_array_t *
rtmp_array_create(rtmp_pool_t *pool, rtmp_uint_t n, size_t size)
{
    rtmp_array_t *a;

    a = rtmp_pool_alloc(pool, sizeof(rtmp_array_t));
    if (a == NULL) {
        return NULL;
    }

    if (n) {
        a->elts = rtmp_pool_alloc(pool, n * size);
        if (a->elts == NULL) {
            return NULL;
        }
    } else {
        a->elts = NULL;
    }

    a->nelts = 0;
    a->size = size;
    a->nalloc = n;
    a->pool = pool;

    return a;
}

void *
rtmp_array_push(rtmp_array_t *a)
{
    void *elt, *new_elts;
    size_t size;

    if (a->nelts == a->nalloc) {
        /* Array is full, need to expand */
        size = a->size * a->nalloc;
        
        new_elts = rtmp_pool_alloc(a->pool, 2 * size);
        if (new_elts == NULL) {
            return NULL;
        }

        rtmp_memcpy(new_elts, a->elts, size);
        a->elts = new_elts;
        a->nalloc *= 2;
    }

    elt = (unsigned char *) a->elts + a->size * a->nelts;
    a->nelts++;

    return elt;
}

void *
rtmp_array_push_n(rtmp_array_t *a, rtmp_uint_t n)
{
    void *elt, *new_elts;
    size_t size;

    if (a->nelts + n > a->nalloc) {
        /* Array needs expansion */
        size = a->size * a->nalloc;
        
        new_elts = rtmp_pool_alloc(a->pool, 2 * size * n);
        if (new_elts == NULL) {
            return NULL;
        }

        rtmp_memcpy(new_elts, a->elts, size);
        a->elts = new_elts;
        a->nalloc = 2 * a->nalloc * n;
    }

    elt = (unsigned char *) a->elts + a->size * a->nelts;
    a->nelts += n;

    return elt;
}

rtmp_chain_t *
rtmp_alloc_chain_link(rtmp_pool_t *pool)
{
    rtmp_chain_t *cl;

    cl = rtmp_pool_alloc(pool, sizeof(rtmp_chain_t));
    if (cl == NULL) {
        return NULL;
    }

    cl->buf = NULL;
    cl->next = NULL;

    return cl;
}

rtmp_buf_t *
rtmp_create_temp_buf(rtmp_pool_t *pool, size_t size)
{
    rtmp_buf_t *b;

    b = rtmp_pool_calloc(pool, sizeof(rtmp_buf_t));
    if (b == NULL) {
        return NULL;
    }

    b->start = rtmp_pool_alloc(pool, size);
    if (b->start == NULL) {
        return NULL;
    }

    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->temporary = 1;

    return b;
}

rtmp_int_t
rtmp_strcmp(const unsigned char *s1, const unsigned char *s2)
{
    return strcmp((const char *) s1, (const char *) s2);
}

void
rtmp_memcpy(void *dst, const void *src, size_t n)
{
    memcpy(dst, src, n);
}

void
rtmp_memzero(void *buf, size_t n)
{
    memset(buf, 0, n);
}