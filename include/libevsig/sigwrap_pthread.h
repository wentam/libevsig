#pragma once
#include <pthread.h>
#include "libevsig/errno_signals.h"

int sw_pthread_create(pthread_t *restrict thread,
                      const pthread_attr_t *restrict attr,
                      typeof(void *(void *)) *start_routine,
                      void *restrict arg);

int sw_pthread_join(pthread_t thread, void** retval);

int sw_pthread_cancel(pthread_t thread);
