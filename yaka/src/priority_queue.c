
#include "priority_queue.h"

//优先队列初始化
int yk_pq_init(yk_pq_t *yk_pq, yk_pq_comparator_pt comp, size_t size) {
    yk_pq->pq = (void **)malloc(sizeof(void *) * (size+1));
    if (!yk_pq->pq) {
        log_err("yk_pq_init: malloc failed");
        return -1;
    }
    
    yk_pq->nalloc = 0;
    yk_pq->size = size + 1;
    yk_pq->comp = comp;
    
    return ZV_OK;
}

int yk_pq_is_empty(yk_pq_t *yk_pq) {
    return (yk_pq->nalloc == 0)? 1: 0;
}

size_t yk_pq_size(yk_pq_t *yk_pq) {
    return yk_pq->nalloc;
}

void *yk_pq_min(yk_pq_t *yk_pq) {
    if (yk_pq_is_empty(yk_pq)) {
        return NULL;
    }

    return yk_pq->pq[1];
}

static int resize(yk_pq_t *yk_pq, size_t new_size) {
    if (new_size <= yk_pq->nalloc) {
        log_err("resize: new_size to small");
        return -1;
    }

    void **new_ptr = (void **)malloc(sizeof(void *) * new_size);
    if (!new_ptr) {
        log_err("resize: malloc failed");
        return -1;
    }

    memcpy(new_ptr, yk_pq->pq, sizeof(void *) * (yk_pq->nalloc + 1));
    free(yk_pq->pq);
    yk_pq->pq = new_ptr;
    yk_pq->size = new_size;
    return ZV_OK;
}

static void exch(yk_pq_t *yk_pq, size_t i, size_t j) {
    void *tmp = yk_pq->pq[i];
    yk_pq->pq[i] = yk_pq->pq[j];
    yk_pq->pq[j] = tmp;
}

static void swim(yk_pq_t *yk_pq, size_t k) {
    while (k > 1 && yk_pq->comp(yk_pq->pq[k], yk_pq->pq[k/2])) {
        exch(yk_pq, k, k/2);
        k /= 2;
    }
}

static size_t sink(yk_pq_t *yk_pq, size_t k) {
    size_t j;
    size_t nalloc = yk_pq->nalloc;

    while (2*k <= nalloc) {
        j = 2*k;
        if (j < nalloc && yk_pq->comp(yk_pq->pq[j+1], yk_pq->pq[j])) j++;
        if (!yk_pq->comp(yk_pq->pq[j], yk_pq->pq[k])) break;
        exch(yk_pq, j, k);
        k = j;
    }
    
    return k;
}

//从数组顶端删除最小的元素，并将最后一个元素放到顶端，减小堆的大小并让这个元素下沉到合适的位置
int yk_pq_delmin(yk_pq_t *yk_pq) {
    if (yk_pq_is_empty(yk_pq)) {
        return ZV_OK;
    }

    exch(yk_pq, 1, yk_pq->nalloc);
    yk_pq->nalloc--;
    sink(yk_pq, 1);
    if (yk_pq->nalloc > 0 && yk_pq->nalloc <= (yk_pq->size - 1)/4) {
        if (resize(yk_pq, yk_pq->size / 2) < 0) {
            return -1;
        }
    }

    return ZV_OK;
}

//将新元素插到数组末尾，增加堆的大小，并将元素上浮到合适的位置
int yk_pq_insert(yk_pq_t *yk_pq, void *item) {
    if (yk_pq->nalloc + 1 == yk_pq->size) {
        if (resize(yk_pq, yk_pq->size * 2) < 0) {
            return -1;
        }
    }

    yk_pq->pq[++yk_pq->nalloc] = item;
    swim(yk_pq, yk_pq->nalloc);

    return ZV_OK;
}

int yk_pq_sink(yk_pq_t *yk_pq, size_t i) {
    return sink(yk_pq, i);
}
