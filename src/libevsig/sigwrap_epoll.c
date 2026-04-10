#include "libevsig/sigwrap_epoll.h"
#include "libevsig/signals.h"
#include "libevsig/errno_signals.h"
#include <sys/epoll.h>
#include <errno.h>

int sw_epoll_create1(int flags) {
  int out = epoll_create1(flags);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("epoll_create1(): ", errno),
             NULL, NULL);

  return out;
}

int sw_epoll_ctl(int epfd, int op, int fd, struct epoll_event *_Nullable event) {
  int out = epoll_ctl(epfd, op, fd, event);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("epoll_ctl(): ", errno),
             NULL, NULL);

  return out;
}

int sw_epoll_wait(int epfd, struct epoll_event *_Nonnull events, int n, int timeout) {
  int out = epoll_wait(epfd, events, n, timeout);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("epoll_wait(): ", errno),
             NULL, NULL);

  return out;
}
