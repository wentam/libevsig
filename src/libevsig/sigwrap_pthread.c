#include "libevsig/sigwrap_pthread.h"
#include "libevsig/signals.h"
#include "libevsig/errno_signals.h"
#include <errno.h>

int sw_pthread_create(pthread_t *restrict thread,
                      const pthread_attr_t *restrict attr,
                      typeof(void *(void *)) *start_routine,
                      void *restrict arg) {
  int out = pthread_create(thread, attr, start_routine, arg);
  if (out)
    SIG_SEND(sig_from_errno(out), str_from_errno("pthread_create(): ", out),
             NULL, NULL);
  return out;
}

int sw_pthread_join(pthread_t thread, void** retval) {
  int out = pthread_join(thread, retval);
  if (out)
    SIG_SEND(sig_from_errno(out), str_from_errno("pthread_join(): ", out),
             NULL, NULL);
  return out;
}

int sw_pthread_cancel(pthread_t thread) {
  int out = pthread_cancel(thread);
  if (out)
    SIG_SEND(sig_from_errno(out), str_from_errno("pthread_cancel(): ", out),
             NULL, NULL);
  return out;
}
