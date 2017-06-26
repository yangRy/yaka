
#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "dbg.h"
#include "error.h"

#define ZV_PQ_DEFAULT_SIZE 10

typedef int (*yk_pq_comparator_pt)(void *pi, void *pj);

typedef struct {
    void **pq;
    size_t nalloc;
    size_t size;
    yk_pq_comparator_pt comp;
} yk_pq_t;

int yk_pq_init(yk_pq_t *yk_pq, yk_pq_comparator_pt comp, size_t size);
int yk_pq_is_empty(yk_pq_t *yk_pq);
size_t yk_pq_size(yk_pq_t *yk_pq);
void *yk_pq_min(yk_pq_t *yk_pq);
int yk_pq_delmin(yk_pq_t *yk_pq);
int yk_pq_insert(yk_pq_t *yk_pq, void *item);

int yk_pq_sink(yk_pq_t *yk_pq, size_t i);
#endif 
