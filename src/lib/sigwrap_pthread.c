#include "lib/sigwrap_pthread.h"
#include "lib/signals.h"
#include <errno.h>


int sw_pthread_create(pthread_t *restrict thread,
                      const pthread_attr_t *restrict attr,
                      typeof(void *(void *)) *start_routine,
                      void *restrict arg) {
  int out = pthread_create(thread, attr, start_routine, arg);

  if (out != 0) {
    switch (out) {
      case EAGAIN:
        SIG_SEND(SIGNAL_INSUFFICIENT_RESOURCES, "pthread_create(): Insufficient resources",
                 NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "pthread_create(): Invalid input", NULL, NULL);
        break;
      case EPERM:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "pthread_create(): Permission denied", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "pthread_create(): Unknown error", NULL, NULL);
        break;
    }
  }

  return out;
}

int sw_pthread_join(pthread_t thread, void** retval) {
  int out = pthread_join(thread, retval);

  if (out != 0) {
    switch (out) {
      case EDEADLK:
        SIG_SEND(SIGNAL_WOULD_DEADLOCK,
                 "pthread_join(): Would deadlock (maybe 2 threads trying to join eachother?)",
                 NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "pthread_join(): Invalid input, not joinable, or another thread is already waiting to join with this thread", NULL, NULL);
        break;
      case ESRCH:
        SIG_SEND(SIGNAL_NOT_FOUND, "pthread_join(): No thread with this id could be found",
                 NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "pthread_join(): Unknown error", NULL, NULL);
        break;
    }
  }

  return out;
}

int sw_pthread_cancel(pthread_t thread) {
  int out = pthread_cancel(thread);

  if (out != 0) {
    switch (out) {
      case ESRCH:
        SIG_SEND(SIGNAL_NOT_FOUND, "pthread_cancel(): No thread with this id could be found",
                 NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "pthread_cancel(): Unknown error", NULL, NULL);
        break;
    }
  }

  return out;
}
