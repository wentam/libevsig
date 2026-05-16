#pragma once
#include <time.h>
#include <sys/time.h>

[[maybe_unused]]
static void evsig_sleep_ns(uint64_t nanoseconds) {
    struct timespec ts;
    ts.tv_sec = nanoseconds / 1000000000L;  // Seconds
    ts.tv_nsec = nanoseconds % 1000000000L; // Remaining nanoseconds
    nanosleep(&ts, NULL);
}

[[maybe_unused]]
static uint64_t evsig_time_s() {
  return time(NULL);
}

[[maybe_unused]]
static uint64_t evsig_time_us() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) == -1) {
    fprintf(stderr, "Error: failed to get time from gettimeofday\n");
    exit(1);
  }
  return ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
}

[[maybe_unused]]
static uint64_t evsig_time_ms() {
  return evsig_time_us()/1000;
}

[[maybe_unused]]
static uint64_t evsig_time_ns() {
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
    fprintf(stderr, "Error: failed to get time from clock_gettime\n");
    exit(1);
  }
  return ((uint64_t)ts.tv_sec) * 1000000000 + ts.tv_nsec;
}
