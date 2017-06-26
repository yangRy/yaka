
#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>

#define MAXEVENTS 1024

int yk_epoll_create(int flags);
void yk_epoll_add(int epfd, int fs, struct epoll_event *event);
void yk_epoll_mod(int epfd, int fs, struct epoll_event *event);
void yk_epoll_del(int epfd, int fs, struct epoll_event *event);
int yk_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

#endif
