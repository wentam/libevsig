#include "libevsig/thread_shutdown_signal.h"
#include "libevsig/evsig_mutex.h"
#include <stdlib.h>
#include <stdio.h>
#include "libevsig/util.h"
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// TODO set up shutdown thread

evsig_thread_shutdown_signal evsig_global_thread_shutdown_signal =
EVSIG_THREAD_SHUTDOWN_SIGNAL_INIT;


// For signal-handler-safe printing
static void _print(char* msg) {
  uint64_t len = 0;
  char* count = msg;
  while (*count != '\0') {
    count++;
    len++;
  }

  (void)write(STDERR_FILENO, msg, len);
}

static void* _shutdown_thread(void* ud) {
  evsig_thread_shutdown_signal* s = ud;

  // Block all signals so that this isn't the thread
  // that receives a signal, given that we need to
  // be released to do things by threads that receive a signal.
  //
  // TODO might be better to set this before thread creation, then restore.
  //
  // Technically speaking a tiny moment in time where we could be signalled I think.
  sigset_t set;
  sigfillset(&set); // All signals
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  evsig_await_unlock(&s->shutdown_mutex);

  _print("[libevsig] Asking all threads listening for evsig shutdown signal to exit.\n");
  _print("[libevsig] Waiting up to 5 seconds for threads to exit...\n");

  if (evsig_thread_shutdown_signal_send_block(&evsig_global_thread_shutdown_signal,
        5000)) {
    _print("[libevsig] Timed out, some threads didn't exit. Exiting uncleanly.\n");
    _print("[libevsig] To avoid this, make sure your threads exit when "
        "requested via the evsig_shutdown_signal system.\n");
  }

  // There might be other threads not using our system. Thus we should
  // explicitly exit to make sure everything stops (if the user has configured
  // us this way)

  evsig_lock(&s->master_mutex);
  {
    if (s->shutdown_thread_exit) exit(1);
    evsig_ensure_locked(&s->shutdown_mutex);

    s->shutdown_thread_started = false;
  }
  evsig_unlock(&s->master_mutex);

  return NULL;
}

void evsig_thread_shutdown_signal_register_thread(evsig_thread_shutdown_signal* s,
                                                  pid_t t) {
  evsig_lock(&s->master_mutex);
  do {

    // Check for duplicates
    bool duplicate = false;
    for (uint64_t i = 0; i < s->threadlist_fill; i++)
      if (s->threadlist[i] == t) { duplicate = true; break; }
    if (duplicate) break;

    // Grow threadlist if needed
    if (s->threadlist_fill >= s->threadlist_alloc) {
      if (!s->threadlist_alloc) s->threadlist_alloc = 8;
      else s->threadlist_alloc *= 2;

      s->threadlist = realloc(s->threadlist, sizeof(pid_t)*s->threadlist_alloc);
      if (!s->threadlist) {
        fprintf(stderr, "Failed to (re)allocate threadlist for libevsig "
                        "shutdown signal");
        exit(1);
      }
    }

    // Insert thread
    s->threadlist[s->threadlist_fill++] = t;

    // Start the shutdown thread if it's not running already
    if (!s->shutdown_thread_started) {
      evsig_ensure_locked(&s->shutdown_mutex);

      pthread_t t;
      int r = pthread_create(&t, NULL, _shutdown_thread, s);
      pthread_detach(t); // So we don't need to join it

      if (r != 0) {
        fprintf(stderr, "Failed to create shutdown thread for unwind system\n");
        exit(1);
      }

      s->shutdown_thread_started = true;
    }

  } while (false);
  evsig_unlock(&s->master_mutex);
}

void evsig_thread_shutdown_signal_confirm_shutdown(
    evsig_thread_shutdown_signal* s,
    pid_t t) {

  evsig_lock(&s->master_mutex);
  {
    // Shift all instances of this thread out of the list
    uint64_t shift = 0;
    for (uint64_t i = 0; i < s->threadlist_fill; i++) {
      while (i+shift < s->threadlist_fill && s->threadlist[i+shift] == t)
        shift++;

      if (shift && i+shift < s->threadlist_fill)
        s->threadlist[i] = s->threadlist[i+shift];
    }
    s->threadlist_fill -= shift;

    // Free list if there are no more threads
    if (!s->threadlist_fill) {
      free(s->threadlist);
      s->threadlist       = NULL;
      s->threadlist_alloc = 0;
      s->threadlist_fill  = 0;
    }
  }
  evsig_unlock(&s->master_mutex);
}

uint64_t evsig_thread_shutdown_signal_register_cb(
    evsig_thread_shutdown_signal* s,
    void (*cb)(void*),
    void* ud) {

  uint64_t ret = 0;

  evsig_lock(&s->master_mutex);
  {
    // Grow callbacks list if needed
    if (s->callbacks_fill >= s->callbacks_alloc) {
      if (!s->callbacks_alloc) s->callbacks_alloc  = 8;
      else                     s->callbacks_alloc *= 2;

      s->callbacks =
        realloc(s->callbacks,
                sizeof(evsig_thread_shutdown_cb)*s->callbacks_alloc);

      if (!s->callbacks) {
        fprintf(stderr, "Failed to (re)allocate callback list for libevsig "
                        "shutdown signal");
        exit(1);
      }
    }

    // Insert callback
    s->callbacks[s->callbacks_fill++] = (evsig_thread_shutdown_cb) {
      .id = ++s->callbacks_last_insert_id,
      .cb = cb,
      .ud = ud
    };

    ret = s->callbacks_last_insert_id;
  }
  evsig_unlock(&s->master_mutex);

  return ret;
}

void evsig_thread_shutdown_signal_unregister_cb(evsig_thread_shutdown_signal* s,
                                                uint64_t id) {

  evsig_lock(&s->master_mutex);
  {
    // Shift all instances of this callback out of the list
    uint64_t shift = 0;
    for (uint64_t i = 0; i < s->callbacks_fill; i++) {
      while (i+shift < s->callbacks_fill && s->callbacks[i+shift].id == id)
        shift++;

      if (shift && i+shift < s->callbacks_fill)
        s->callbacks[i] = s->callbacks[i+shift];
    }
    s->callbacks_fill -= shift;

    // Free callback list if empty
    if (!s->callbacks_fill) {
      free(s->callbacks);
      s->callbacks       = NULL;
      s->callbacks_alloc = 0;
      s->callbacks_fill  = 0;
    }
  }
  evsig_unlock(&s->master_mutex);
}

void unwind_handler_evsig_thread_shutdown_signal_unregister_cb(void* ptr) {
  evsig_cb_handle* h = ptr;
  evsig_thread_shutdown_signal_unregister_cb(h->s, h->id);
}

bool evsig_thread_shutdown_signal_send_block(evsig_thread_shutdown_signal* s,
                                             uint64_t timeout_ms) {
  bool ret = false;

  do {
    if (atomic_load(&s->shutdown)) break;

    // Set the shutdown flag and call all callbacks to notify threads to stop
    atomic_store(&s->shutdown, 1);
    evsig_lock(&s->master_mutex);
    for (uint64_t i = 0; i < s->callbacks_fill; i++)
      s->callbacks[i].cb(s->callbacks[i].ud);
    evsig_unlock(&s->master_mutex);

    // Block until threadlist is empty or timeout is elapsed
    bool have_threads = true;
    uint64_t slep = 64;
    uint64_t slep_max = 20000;
    uint64_t start_ms = evsig_time_ms();
    while(have_threads) {
      evsig_lock(&s->master_mutex);
      if (!s->threadlist_fill) have_threads = false;
      evsig_unlock(&s->master_mutex);

      if (!have_threads) break;

      evsig_sleep_ns(slep*1000);
      slep *= 2;
      if (slep > slep_max) slep = slep_max;

      if (evsig_time_ms()-start_ms >= timeout_ms) {
        have_threads = false;
        ret = true;
        break;
      }
    }

    // Free resources associated with this signal

    free(s->callbacks);
    s->callbacks_alloc = 0;
    s->callbacks_fill  = 0;

    free(s->threadlist);
    s->threadlist       = NULL;
    s->threadlist_alloc = 0;
    s->threadlist_fill  = 0;
  } while (false);

  return ret;
}

bool evsig_thread_shutdown_p(evsig_thread_shutdown_signal* s) {
  return atomic_load(&evsig_global_thread_shutdown_signal.shutdown);
}

void evsig_thread_shutdown_signal_send_async(evsig_thread_shutdown_signal* s,
                                             bool     exit_process) {
  evsig_lock(&s->master_mutex);
  s->shutdown_thread_exit = exit_process;
  evsig_unlock(&s->shutdown_mutex);
  evsig_unlock(&s->master_mutex);
}
