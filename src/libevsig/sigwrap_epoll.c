#include "libevsig/sigwrap_epoll.h"
#include "libevsig/signals.h"
#include <sys/epoll.h>
#include <errno.h>

int sw_epoll_create1(int flags) {
  int out = epoll_create1(flags);

  if (out == -1) {
    switch (errno) {
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "epoll_create1(): Invalid input", NULL, NULL);
        break;
      case EMFILE:
        SIG_SEND(SIGNAL_TOO_MANY_OPEN_FILES, "epoll_create1(): Too many open files", NULL, NULL);
        break;
      case ENFILE:
        SIG_SEND(SIGNAL_TOO_MANY_OPEN_FILES, "epoll_create1(): Too many open files", NULL, NULL);
        break;
      case ENOMEM:
        SIG_SEND(SIGNAL_NOT_ENOUGH_MEMORY, "epoll_create1(): Insufficient memory available",
                 NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "epoll_create1(): Unknown error", NULL, NULL);
        break;
    }
  }

  return out;
}

int sw_epoll_ctl(int epfd, int op, int fd, struct epoll_event *_Nullable event) {
  int out = epoll_ctl(epfd, op, fd, event);

  if (out == -1) {
    switch (out) {
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "epoll_ctl(): Bad file descriptor", NULL, NULL);
        break;
      case EEXIST:
        SIG_SEND(SIGNAL_FILE_EXISTS, "epoll_ctl(): fd already exists", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "epoll_ctl(): Invalid input", NULL, NULL);
        break;
      case ELOOP:
        SIG_SEND(SIGNAL_LOOP, "epoll_ctl(): Would result in infinite loop", NULL, NULL);
        break;
      case ENOENT:
        SIG_SEND(SIGNAL_NOT_FOUND, "epoll_ctl(): fd not registered with this epoll instance",
                 NULL, NULL);
        break;
      case ENOMEM:
        SIG_SEND(SIGNAL_NOT_ENOUGH_MEMORY, "epoll_ctl(): Insufficient memory available",
                 NULL, NULL);
        break;
      case ENOSPC:
        SIG_SEND(SIGNAL_INSUFFICIENT_RESOURCES, "epoll_ctl(): Would exceed max fd watch limit",
                 NULL, NULL);
        break;
      case EPERM:
        SIG_SEND(SIGNAL_UNSUPPORTED, "epoll_ctl(): Specified fd does not support epoll", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "epoll_ctl(): Unknown error", NULL, NULL);
        break;
    }
  }

  return out;
}

int sw_epoll_wait(int epfd, struct epoll_event *_Nonnull events, int n, int timeout) {
  int out = epoll_wait(epfd, events, n, timeout);

  if (out == -1) {
    switch(out) {
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "epoll_wait(): Bad file descriptor", NULL, NULL);
        break;
      case EFAULT:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "epoll_wait(): 'events' argument not writable",
                 NULL, NULL);
        break;
      case EINTR:
        SIG_SEND(SIGNAL_INTERRUPTED, "epoll_wait(): Interrupted by signal handler", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "epoll_wait(): Invalid input", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "epoll_wait(): Unknown error", NULL, NULL);
        break;
    }
  }

  return out;
}
