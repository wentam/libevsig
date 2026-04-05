#pragma once
#include <sys/epoll.h>

int sw_epoll_create1(int flags);
int sw_epoll_ctl(int epfd, int op, int fd, struct epoll_event *_Nullable event);
int sw_epoll_wait(int epfd, struct epoll_event *_Nonnull events, int n, int timeout);
