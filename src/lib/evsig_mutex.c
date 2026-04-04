#include "lib/evsig_mutex.h"
#include <time.h>

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

uint64_t fnv_hash(const char* key, const uint64_t key_len) {
  uint64_t result = FNV_OFFSET;

  for (uint64_t i = 0; i < key_len; i++) {
    uint8_t byte = key[i];
    result ^= byte;
    result *= FNV_PRIME;
  }

  return result;
}

thread_local uint64_t rand64_seed = 0;

static uint64_t rand64() {
  while (rand64_seed == 0) {
    uint64_t tmp = time(NULL);
    rand64_seed = fnv_hash((char*)(&tmp), 8);
  }

  rand64_seed ^= rand64_seed << 13;
  rand64_seed ^= rand64_seed >> 7;
  rand64_seed ^= rand64_seed << 17;
  return rand64_seed;
}

void evsig_lock(evsig_mutex* m) {
  uint8_t expected = 0;
  uint32_t spinwait = 128;

  while (!atomic_compare_exchange_weak_explicit(
    m, &expected, 1,
    memory_order_acquire,  // on success: prevents reordering of critical section
    memory_order_relaxed   // on failure: we're just spinning, who cares
  )) {
    expected = 0; // reset expected for next CAS attempt

    if (spinwait > 1) {
      // Jitter prevents everybody from hitting the lock at the same time
      uint64_t jitter = rand64() & ((spinwait >> 1)-1);

      // Avoid huge jitter if spinwait=1 - to protect against footgun if this
      // code is tweaked in the wrong way.
      if (jitter > (spinwait >> 1)) jitter = 0;
      uint64_t slep = spinwait+jitter;

      if (slep < 1024) {
        for (uint64_t i = 0; i < (slep >> 7); i++) {
          __builtin_ia32_pause();
        }
      } else {
        nanosleep(&(struct timespec){.tv_nsec = slep}, NULL);
      }
    }
    if (spinwait < 1024*1024) spinwait *= 2;
  }
}

void evsig_unlock(evsig_mutex* m) { atomic_store_explicit(m, 0, memory_order_release); }
void evsig_ensure_locked(evsig_mutex* m) { atomic_store_explicit(m, 1, memory_order_release); }

void evsig_await_unlock(evsig_mutex* m) {
  uint32_t spinwait = 128;
  while (atomic_load(m) != 0) {
    if (spinwait > 1) {
      // Jitter prevents everybody from hitting the lock at the same time
      uint64_t jitter = rand64() & ((spinwait >> 1)-1);

      // Avoid huge jitter if spinwait=1 - to protect against footgun if this
      // code is tweaked in the wrong way.
      if (jitter > (spinwait >> 1)) jitter = 0;
      uint64_t slep = spinwait+jitter;

      if (slep < 1024) {
        for (uint64_t i = 0; i < (slep >> 7); i++) {
          __builtin_ia32_pause();
        }
      } else {
        nanosleep(&(struct timespec){.tv_nsec = slep}, NULL);
      }
    }
    if (spinwait < 1024*1024) spinwait *= 2;
  }
}
