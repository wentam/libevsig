#pragma once
#include <stdatomic.h>

// simple mutex using compare-and-swap so we have control over
// what is async-handler-safe.

typedef _Atomic uint8_t evsig_mutex;

// NOT async-handler-safe.
void evsig_lock(evsig_mutex* m);

// async-handler-safe.
void evsig_unlock(evsig_mutex* m);

// Locks if not locked, non-blocking always.
//
// async-handler-safe.
void evsig_ensure_locked(evsig_mutex* m);

// async-handler-safe. Just waits for a mutex to be unlocked, or returns
// immediately if it's already unlocked.
void evsig_await_unlock(evsig_mutex* m);
