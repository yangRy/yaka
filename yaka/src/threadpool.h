
#ifndef THREADPOOL_H
#define THREADPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include "dbg.h"

#define THREAD_NUM 8

//任务链表
typedef struct yk_task_s {
    void (*func)(void *);
    void *arg;
    struct yk_task_s *next;
} yk_task_t;

//线程池的数据结构
typedef struct {
    pthread_mutex_t lock; //一个互斥锁
    pthread_cond_t cond; //一个条件锁
    pthread_t *threads; //线程描述符数组
    yk_task_t *head; //任务队列
    int thread_count; //当前线程池的线程数
    int queue_size; //记录head链表的长度 任务队列的长度
    int shutdown;
    int started;
} yk_threadpool_t;

typedef enum {
    yk_tp_invalid   = -1,
    yk_tp_lock_fail = -2,
    yk_tp_already_shutdown  = -3,
    yk_tp_cond_broadcast    = -4,
    yk_tp_thread_fail       = -5,
    
} yk_threadpool_error_t;

yk_threadpool_t *threadpool_init(int thread_num);

int threadpool_add(yk_threadpool_t *pool, void (*func)(void *), void *arg);

int threadpool_destroy(yk_threadpool_t *pool, int gracegul);

#ifdef __cplusplus
}
#endif

#endif
