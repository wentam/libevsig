#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "libevsig/evsig_mutex.h"

// This is a mechanism to signal that threads should shut down. You are probably
// interested in the global signal, evsig_global_shutdown_signal, as this
// is the signal that well be sent by the libevsig signal system.
//
// A shutdown signal has a list of threads with which to await exit
// when sending such a signal. In libevsig this is used to keep the
// process alive on an unhandled signal until all threads have exited.
// Thus, make sure your thread is registered with the global shutdown
// signal if you want to be able to exit cleanly.
//
// To determine when your thread should exit cleanly, you can register a
// callback to receive the event or use evsig_shutdown_p to check
// if it's time to shut down.

#define EVSIG_THREAD_SHUTDOWN_SIGNAL_INIT (evsig_thread_shutdown_signal)\
  { .shutdown                 = false,\
    .callbacks                = NULL,\
    .callbacks_fill           = 0,\
    .callbacks_alloc          = 0,\
    .callbacks_last_insert_id = 0, \
    .threadlist       = NULL,\
    .threadlist_fill  = 0,\
    .threadlist_alloc = 0,\
    .master_mutex = 0,\
    .shutdown_thread_started = false,\
    .shutdown_mutex = 1,\
    .shutdown_thread_exit = false\
  }

typedef struct {
  uint64_t id;
  void (*cb)(void*);
  void* ud;
} evsig_thread_shutdown_cb;

typedef struct {
  // False until signal sent, then true. Feel free to check this instead
  // of registering a callback as a convenience if you're happy to poll instead.
  //
  // You probably only want to check this flag if you've registered your thread
  // with us.
  _Atomic bool              shutdown;
  evsig_thread_shutdown_cb* callbacks;
  uint64_t                  callbacks_fill;
  uint64_t                  callbacks_alloc;
  uint64_t                  callbacks_last_insert_id;

  pid_t*      threadlist;
  uint64_t    threadlist_fill;
  uint64_t    threadlist_alloc;

  evsig_mutex master_mutex;
  evsig_mutex shutdown_mutex;
  bool        shutdown_thread_started;

  bool shutdown_thread_exit;
} evsig_thread_shutdown_signal;

extern evsig_thread_shutdown_signal evsig_global_thread_shutdown_signal;

// An evsig_therad_shutdown_signal is initialized via the
// EVSIG_THREAD_SHUTDOWN_SIGNAL_INIT macro. It's resources
// are cleaned up when all registered callbacks are unregistered and the
// threadlist becomes empty due to confirm_shutdown calls.

// Sends the shutdown signal
//
// Frees resources associated with this signal. Thus, resending the signal
// is a no-op (but safe)
//
// Blocks until either all threads associated with this signal have stopped
// or timeout_ms has elapsed.
//
// Returns true if timeout elapsed, else false
//
// Call me from any thread.
bool evsig_thread_shutdown_signal_send_block(evsig_thread_shutdown_signal* s,
                                             uint64_t timeout_ms);
// Sends the shutdown signal without blocking.
//
// Async-handler safe.
//
// If exit_process is true, will wait up to 5 seconds for all threads
// to confirm shutdown, then will call exit().
void evsig_thread_shutdown_signal_send_async(evsig_thread_shutdown_signal* s,
                                             bool     exit_process);

// Registers a callback to be called upon shutdown signal send. Will
// be called from an arbitrary thread, likely external to you.
//
// Returns an id that identifies the registration.
//
// You must unregister when done to free associated resources.
// You probably want to do so via an UNWIND_ACTION.
//
// Call me from any thread.
uint64_t evsig_thread_shutdown_signal_register_cb(evsig_thread_shutdown_signal* s,
                                                  void (*cb)(void*),
                                                  void* ud);

// Unregisters a callback. Call this when your thread is shutting down (for
// normal exit or signal reasons both), or when you don't care about
// the event anymore.
//
// You probably want to call this via an UNWIND_ACTION
//
// Call me from any thread.
void evsig_thread_shutdown_signal_unregister_cb(evsig_thread_shutdown_signal* s,
                                                uint64_t id);

// Unwind handler for unregister, accepts evsig_cb_handle.
typedef struct {
  evsig_thread_shutdown_signal* s;
  uint64_t id;
} evsig_cb_handle;

void unwind_handler_evsig_thread_shutdown_signal_unregister_cb(void* ptr);

// Adds a thread for which we will await exit when a shutdown
// signal is sent.
//
// We consider a thread shut-down when
// evsig_thread_shutdown_signal_confirm_shutdown is called.
//
// Call me from any thread.
//
// Safe no-op if this thread is already registered.
void evsig_thread_shutdown_signal_register_thread(evsig_thread_shutdown_signal* s,
                                                  pid_t t);

// Notifies us that this thread has shut down to unblock the signal send function
//
// Should always be called on thread shutdown even if not due to a signal.
//
// Note that sig_cleanup() will call this for you, so if using the
// signal system you normally don't need to call this manually as
// long as you are properly cleaning up.
//
// Call me from any thread.
void evsig_thread_shutdown_signal_confirm_shutdown(evsig_thread_shutdown_signal* s,
                                                   pid_t t);

// True if it's time to shut down
bool evsig_thread_shutdown_p(evsig_thread_shutdown_signal* s);
