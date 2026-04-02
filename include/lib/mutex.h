#pragma once
#include <stdatomic.h>

// simple mutex using compare-and-swap so we have control over
// what is async-handler-safe.

typedef _Atomic uint8_t mutex;

// NOT async-handler-safe.
void lock(mutex* m);

// async-handler-safe.
void unlock(mutex* m);

// Locks if not locked, non-blocking always.
//
// async-handler-safe.
void ensure_locked(mutex* m);

// async-handler-safe. Just waits for a mutex to be unlocked, or returns
// immediately if it's already unlocked.
void await_unlock(mutex* m);
