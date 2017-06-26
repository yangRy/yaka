
#include "epoll.h"
#include "dbg.h"

struct epoll_event *events; //epoll事件数组

int yk_epoll_create(int flags) {
    int fd = epoll_create1(flags);
    check(fd > 0, "yk_epoll_create: epoll_create1");

    events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    check(events != NULL, "yk_epoll_create: malloc");
    return fd;
}
//添加要监听的描述符
//fd:被监听的描述符
void yk_epoll_add(int epfd, int fd, struct epoll_event *event) {
    int rc = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event);
    check(rc == 0, "yk_epoll_add: epoll_ctl");
    return;
}

//修改已经注册的fd监听事件
void yk_epoll_mod(int epfd, int fd, struct epoll_event *event) {
    int rc = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, event);
    check(rc == 0, "yk_epoll_mod: epoll_ctl");
    return;
}

void yk_epoll_del(int epfd, int fd, struct epoll_event *event) {
    int rc = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, event);
    check(rc == 0, "yk_epoll_del: epoll_ctl");
    return;
}

int yk_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    int n = epoll_wait(epfd, events, maxevents, timeout); //timeout为0，立即返回；为-1，永久阻塞
    check(n >= 0, "yk_epoll_wait: epoll_wait");
    return n;
}
//epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout)
//参数events用来从内核得到的事件的集合，maxevents告知内核这个events有多大,这个maxevents的值不能大于创建epoll_create()的size
