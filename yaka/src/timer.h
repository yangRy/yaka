
#ifndef ZV_TIMER_H
#define ZV_TIMER_H

#include "priority_queue.h"
#include "http_request.h"

#define ZV_TIMER_INFINITE -1
#define TIMEOUT_DEFAULT 500     /* ms */

typedef int (*timer_handler_pt)(yk_http_request_t *rq);

//定时器节点
typedef struct yk_timer_node_s{
    size_t key; //记录过期的时间
    int deleted;    /* if remote client close the socket first, set deleted to 1 */
    timer_handler_pt handler;
    yk_http_request_t *rq;
} yk_timer_node;

int yk_timer_init();
int yk_find_timer();
void yk_handle_expire_timers();

extern yk_pq_t yk_timer;
extern size_t yk_current_msec;

void yk_add_timer(yk_http_request_t *rq, size_t timeout, timer_handler_pt handler);
void yk_del_timer(yk_http_request_t *rq);

#endif
