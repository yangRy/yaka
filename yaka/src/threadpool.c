
#include "threadpool.h"

typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown = 2
} yk_threadpool_sd_t;

static int threadpool_free(yk_threadpool_t *pool);
static void *threadpool_worker(void *arg);

yk_threadpool_t *threadpool_init(int thread_num) {
    if (thread_num <= 0) {
        log_err("the arg of threadpool_init must greater than 0");
        return NULL;
    }

    yk_threadpool_t *pool;
    if ((pool = (yk_threadpool_t *)malloc(sizeof(yk_threadpool_t))) == NULL) {
        goto err;
    }

    pool->thread_count = 0;
    pool->queue_size = 0;
    pool->shutdown = 0; //shutdown初始为零
    pool->started = 0;
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
    pool->head = (yk_task_t *)malloc(sizeof(yk_task_t));    /* dummy head */

    if ((pool->threads == NULL) || (pool->head == NULL)) {
        goto err;
    }

    pool->head->func = NULL;
    pool->head->arg = NULL;
    pool->head->next = NULL;

    if (pthread_mutex_init(&(pool->lock), NULL) != 0) {
        goto err;
    }

    if (pthread_cond_init(&(pool->cond), NULL) != 0) {
        pthread_mutex_destroy(&(pool->lock));
        goto err;
    }
    
    int i;
    for (i=0; i<thread_num; ++i) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0) {
            threadpool_destroy(pool, 0);
            return NULL;
        }
        log_info("thread: %08x started", (uint32_t) pool->threads[i]);

        pool->thread_count++;
        pool->started++;
    }

    return pool;

err:
    if (pool) {
        threadpool_free(pool);
    }

    return NULL;
}
//应该是生产者，向队列添加任务
int threadpool_add(yk_threadpool_t *pool, void (*func)(void *), void *arg) {
    int rc, err = 0;
    if (pool == NULL || func == NULL) {
        log_err("pool == NULL or func == NULL");
        return -1;
    }
    
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        log_err("pthread_mutex_lock");
        return -1;
    }

    if (pool->shutdown) {
        err = yk_tp_already_shutdown;
        goto out;
    }
    
    // TODO: use a memory pool
    yk_task_t *task = (yk_task_t *)malloc(sizeof(yk_task_t));
    if (task == NULL) {
        log_err("malloc task fail");
        goto out;
    }
    
    // TODO: use a memory pool
    task->func = func;
    task->arg = arg;
    task->next = pool->head->next; //将task任务添加到链表尾
    pool->head->next = task;

    pool->queue_size++;
    
    //rc = pthread_cond_signal(&(pool->cond)); //这里会有上锁冲突
    //check(rc == 0, "pthread_cond_signal");

out:
    if(pthread_mutex_unlock(&pool->lock) != 0) {
        log_err("pthread_mutex_unlock");
        return -1;
    }
    rc = pthread_cond_signal(&(pool->cond)); //这里会有上锁冲突
    check(rc == 0, "pthread_cond_signal");
    
    return err;
}

int threadpool_free(yk_threadpool_t *pool) {
    if (pool == NULL || pool->started > 0) {
        return -1;
    }

    if (pool->threads) {
        free(pool->threads);
    }

    yk_task_t *old;
    /* pool->head is a dummy head */
    while (pool->head->next) {
        old = pool->head->next;
        pool->head->next = pool->head->next->next;
        free(old);
    }

    return 0;
}

int threadpool_destroy(yk_threadpool_t *pool, int graceful) {
    int err = 0;

    if (pool == NULL) {
        log_err("pool == NULL");
        return yk_tp_invalid;
    }
    
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return yk_tp_lock_fail;
    }
    
    do {
        // set the showdown flag of pool and wake up all thread    
        if (pool->shutdown) {
            err = yk_tp_already_shutdown;
            break;
        }

        pool->shutdown = (graceful)? graceful_shutdown: immediate_shutdown;
        
        if (pthread_cond_broadcast(&(pool->cond)) != 0) {
            err = yk_tp_cond_broadcast;
            break;
        }

        if (pthread_mutex_unlock(&(pool->lock)) != 0) {
            err = yk_tp_lock_fail;
            break;
        }
        
        int i;
        for (i=0; i<pool->thread_count; i++) {
            if (pthread_join(pool->threads[i], NULL) != 0) {
                err = yk_tp_thread_fail;
            }
            log_info("thread %08x exit", (uint32_t) pool->threads[i]);
        }
             
    } while(0);

    if (!err) {
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->cond));
        threadpool_free(pool);
    }

    return err;
}
//主线程和工作线程，通过生产者消费者队列进行通信，通信的方式涉及互斥锁和条件变量
static void *threadpool_worker(void *arg) {
    if (arg == NULL) {
        log_err("arg should be type yk_threadpool_t*");
        return NULL;
    }

    yk_threadpool_t *pool = (yk_threadpool_t *)arg;
    yk_task_t *task;

    while (1) {
        pthread_mutex_lock(&(pool->lock));
        
        /*  Wait on condition variable, check for spurious wakeups. */
		//当pthread_cond_wait返回时，我们总是再次测试相应条件是否成立，因为可能发生虚假唤醒
        while ((pool->queue_size == 0) && !(pool->shutdown)){
            pthread_cond_wait(&(pool->cond), &(pool->lock));
			//pthread_cond_wait原子地执行一下两个动作：
			//1，给互斥锁pool->lock解锁
			//2，把当前线程投入睡眠，直到某个线程就本条件变量调用pthread_cond_signal
			//
			//在phthread_cond_wait返回前重新给互斥锁上锁
        }

        if (pool->shutdown == immediate_shutdown) {
            break;
        } else if ((pool->shutdown == graceful_shutdown) && pool->queue_size == 0) {
            break;
        }

        task = pool->head->next;
        if (task == NULL) {
            pthread_mutex_unlock(&(pool->lock));
            continue;
        }

        pool->head->next = task->next;
        pool->queue_size--;

        pthread_mutex_unlock(&(pool->lock));

//        func: do_request();
//        arg:  yk_http_request_t
        (*(task->func))(task->arg); //!!!!!!!!!!!!
        /* TODO: memory pool */
        free(task);
    }

    pool->started--;
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);

    return NULL;
}
