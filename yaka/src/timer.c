
#include <sys/time.h>
#include "timer.h"

//从比较函数看到，应该是个小顶堆
//到期时间最小的放置到堆顶
static int timer_comp(void *ti, void *tj) {
    yk_timer_node *timeri = (yk_timer_node *)ti;
    yk_timer_node *timerj = (yk_timer_node *)tj;

    return (timeri->key < timerj->key)? 1: 0;
}

yk_pq_t yk_timer; //优先级队列
size_t yk_current_msec;

//更新yk_current_msec的值
static void yk_time_update() {
    // there is only one thread calling yk_time_update, no need to lock?
    struct timeval tv;
    int rc;

    rc = gettimeofday(&tv, NULL);
    check(rc == 0, "yk_time_update: gettimeofday error");
//        毫秒            秒                    微秒
    yk_current_msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    debug("in yk_time_update, time = %zu", yk_current_msec);
}


int yk_timer_init() {
    int rc;
    rc = yk_pq_init(&yk_timer, timer_comp, ZV_PQ_DEFAULT_SIZE);
    check(rc == ZV_OK, "yk_pq_init error");
   
    yk_time_update();
    return ZV_OK;
}

//找到对顶的计时器，将其作为epoll的监听时间
int yk_find_timer() {
    yk_timer_node *timer_node;
    int time = ZV_TIMER_INFINITE;
    int rc;

    while (!yk_pq_is_empty(&yk_timer)) {
        debug("yk_find_timer");
        yk_time_update();
        timer_node = (yk_timer_node *)yk_pq_min(&yk_timer);
        check(timer_node != NULL, "yk_pq_min error");

        if (timer_node->deleted) {
            rc = yk_pq_delmin(&yk_timer); 
            check(rc == 0, "yk_pq_delmin");
            free(timer_node);
            continue;
        }
             
        time = (int) (timer_node->key - yk_current_msec);
        debug("in yk_find_timer, key = %zu, cur = %zu",
                timer_node->key,
                yk_current_msec);
        time = (time > 0? time: 0); //超时了，time置为0
        break;
    }
    
    return time;
}

//删除失效的时钟
void yk_handle_expire_timers() {
    debug("in yk_handle_expire_timers");
    yk_timer_node *timer_node;
    int rc;

    while (!yk_pq_is_empty(&yk_timer)) {
        debug("yk_handle_expire_timers, size = %zu", yk_pq_size(&yk_timer));
        yk_time_update();
        timer_node = (yk_timer_node *)yk_pq_min(&yk_timer);
        check(timer_node != NULL, "yk_pq_min error");

        if (timer_node->deleted) {
            rc = yk_pq_delmin(&yk_timer); 
            check(rc == 0, "yk_handle_expire_timers: yk_pq_delmin error");
            free(timer_node);
            continue;
        }
        
        if (timer_node->key > yk_current_msec) {
            return;
        }

        if (timer_node->handler) {
            timer_node->handler(timer_node->rq);
        }
        rc = yk_pq_delmin(&yk_timer); 
        check(rc == 0, "yk_handle_expire_timers: yk_pq_delmin error");
        free(timer_node);
    }
}

void yk_add_timer(yk_http_request_t *rq, size_t timeout, timer_handler_pt handler) {
    int rc;
    yk_timer_node *timer_node = (yk_timer_node *)malloc(sizeof(yk_timer_node));
    check(timer_node != NULL, "yk_add_timer: malloc error");

    yk_time_update();
    rq->timer = timer_node;
    timer_node->key = yk_current_msec + timeout;
    debug("in yk_add_timer, key = %zu", timer_node->key);
    timer_node->deleted = 0; //删除标志位初始化为0
    timer_node->handler = handler;
    timer_node->rq = rq;

    rc = yk_pq_insert(&yk_timer, timer_node);
    check(rc == 0, "yk_add_timer: yk_pq_insert error");
}

//将删除标志位置1
void yk_del_timer(yk_http_request_t *rq) {
    debug("in yk_del_timer");
    yk_time_update();
    yk_timer_node *timer_node = rq->timer;
    check(timer_node != NULL, "yk_del_timer: rq->timer is NULL");

    timer_node->deleted = 1;
}
