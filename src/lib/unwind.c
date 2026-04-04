#include "lib/unwind.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib/sigwrap.h"
#include "setjmp.h"
#include "threads.h"
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "lib/evsig_mutex.h"

// TODO make signal handling optional
//
// TODO expose public interface to call in your own signal handler as an
// alternative to using ours.
//
// TODO go through which signals we handle, decide which to keep - we
// probably don't want all

static evsig_mutex shutdown_mutex;
static pthread_mutex_t threadstart_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile bool shutdown_thread_started = false;

static pthread_mutex_t threadlist_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t* threadlist = NULL;
static volatile uint64_t threadlist_count = 0;
static volatile uint64_t threadlist_alloc = 0;

thread_local unwind_handler_stack_entry* unwind_stack;
thread_local uint64_t unwind_stack_alloc;
thread_local uint64_t unwind_stack_fill;
thread_local int64_t  unwind_init_ref = 0;

// We use pthread's thread-local data system to trigger our unwind_run_all_handlers on SIGTERM
// etc. pthread's provides a 'destructor' mechanism that allows you to write a custom destructor
// that runs in the thread's context upon pthread_exit.
//
// We need this because we're running user-defined unwind code, and not all code is safe
// to run in a signal handler directly (free for example).
//
// So our SIGUSR2 handler per-thread just calls pthread_exit to trigger the destructor for
// this dummy variable.
static pthread_key_t unwind_key;
static pthread_once_t unwind_key_once = PTHREAD_ONCE_INIT;

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

// For pthread_cleanup_push/pop
static void _pthread_mutex_unlock(void* ud) {
  pthread_mutex_unlock((pthread_mutex_t*)ud);
}

// Will do nothing if it's already in the list
static void _threadlist_insert(pthread_t t) {
  pthread_mutex_lock(&threadlist_mutex);
  pthread_cleanup_push(_pthread_mutex_unlock, &threadlist_mutex);
  {
    // Check if the thread is already in the list, we don't want duplicates
    bool duplicate = false;
    for (uint64_t i = 0; i < threadlist_count; i++)
      if (pthread_equal(threadlist[i], t)) duplicate = true;

    if (!duplicate) {

      // Grow threadlist if needed
      if (threadlist_count >= threadlist_alloc) {
        if (!threadlist_alloc) threadlist_alloc = 8;
        else threadlist_alloc *= 2;

        threadlist = realloc(threadlist, sizeof(pthread_t)*threadlist_alloc);
        if (!threadlist) {
          fprintf(stderr, "Failed to (re)allocate threadlist for libevsig's unwind mechanism\n");
          exit(1);
        }
      }

      // Insert thread
      threadlist[threadlist_count++] = t;
    }
  }
  pthread_cleanup_pop(true);
}

// Removes all instances, not just one
static void _threadlist_rm(pthread_t t) {
  //_print("rm\n");

  pthread_mutex_lock(&threadlist_mutex);
  pthread_cleanup_push(_pthread_mutex_unlock, &threadlist_mutex);
  {

    // Shift all instances of this thread id out of our list
    uint64_t orig_len = threadlist_count;
    uint64_t shift = 0;
    for (uint64_t i = 0; i < orig_len; i++) {
      if (i+shift < orig_len && pthread_equal(threadlist[i+shift], t)) shift++;
      if (shift && i+shift < orig_len) threadlist[i] = threadlist[i+shift];
    }
    threadlist_count -= shift;

    // Free list if there are no more threads
    //
    // Important to free the threadlist when the final thread
    // cleans itself up.
    if (threadlist_count == 0) {
      //_print("freeing threadlist\n");
      free(threadlist);
      threadlist = NULL;
      threadlist_alloc = 0;
    }
  }
  pthread_cleanup_pop(true);
}

// Returns true if the specified thread is the only thread in the list,
// otherwise false
[[maybe_unused]]
static bool _threadlist_exclusive_p(pthread_t t) {
  bool exclusive = true;

  pthread_mutex_lock(&threadlist_mutex);
  pthread_cleanup_push(_pthread_mutex_unlock, &threadlist_mutex);
  {
    for (uint64_t i = 0; i < threadlist_count; i++)
      if (!pthread_equal(threadlist[i], t)) { exclusive = false; break; }
  }
  pthread_cleanup_pop(true);

  return exclusive;
}

// Not signal handler safe!
static void _on_exit() {
  unwind_run_all_handlers();
  _threadlist_rm(pthread_self());
}

static void _sighandle_dispatch(int sig) {
  // Let the user know we caught a signal.
  //
  // printf isn't safe for signal handler use, so we'll use 'write' which maps
  // closely to the syscall.
  _print("\n\n---------\n[libevsig] Received ");

  switch (sig) {
    case SIGINT:
      _print("SIGINT");
      break;
    case SIGTERM:
      _print("SIGTERM");
      break;
    case SIGSEGV:
      _print("SIGSEGV");
      break;
    case SIGHUP:
      _print("SIGHUP");
      break;
    case SIGQUIT:
      _print("SIGQUIT");
      break;
    case SIGILL:
      _print("SIGILL");
      break;
    case SIGPIPE:
      _print("SIGPIPE");
      break;
    case SIGALRM:
      _print("SIGALRM");
      break;
    case SIGBUS:
      _print("SIGBUS");
      break;
    case SIGSYS:
      _print("SIGSYS");
      break;
    case SIGSTKFLT:
      _print("SIGSTKFLT");
      break;
    case SIGABRT:
      _print("SIGABRT");
      break;
    case SIGFPE:
      _print("SIGFPE");
      break;
    default:
      _print("unknown signal");
      break;
  }

  _print(". Cancelling all threads, which should run all unwind actions.\n");

  // Tell all threads to exit, running all unwind handlers
  evsig_unlock(&shutdown_mutex);
}

void unwind_all() {
  // Tell all threads to exit, running all unwind handlers
  evsig_unlock(&shutdown_mutex);

  // Sleep until we're cancelled
  uint64_t slep = 64;
  uint64_t slep_max = 10000;
  while (true) {
    pthread_testcancel();
    usleep(slep);
    slep *= 2;
    if (slep > slep_max) slep = slep_max;
  }
}

static void _run_unwind_handlers(void* ptr) { _on_exit(); }

static void init_unwind_key() {
  pthread_key_create(&unwind_key, _run_unwind_handlers);
}

static void* _shutdown_thread(void* ud) {
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

  evsig_await_unlock(&shutdown_mutex);

  _print("[libevsig] Asking all threads utilizing unwind system to cancel and cleanup.\n");

  pthread_mutex_lock(&threadlist_mutex);
  pthread_cleanup_push(_pthread_mutex_unlock, &threadlist_mutex);
  for (uint64_t i = 0; i < threadlist_count; i++) pthread_cancel(threadlist[i]);
  pthread_cleanup_pop(true);

  _print("[libevsig] Waiting for threads to exit. If this hangs, the hanging thread might need cancelation points added with pthread_testcancel().\n");

  // Wait for all threads to stop.
  //
  // We shouldn't use pthread_join because it's UB if another thread is also joining.
  //
  // We'll just wait for the threadlist to be empty.
  uint64_t slep = 64;
  uint64_t slep_max = 20000;
  while (true) {
    bool done = false;

    pthread_mutex_lock(&threadlist_mutex);
    pthread_cleanup_push(_pthread_mutex_unlock, &threadlist_mutex);
    if (threadlist_count == 0) done = true;
    pthread_cleanup_pop(true);

    if (done) break;

    usleep(slep);
    slep *= 2;
    if (slep > slep_max) slep = slep_max;
  }

  return NULL;
}

void unwind_init() {
  if (unwind_init_ref == 0) {
    unwind_stack_alloc = 32;
    unwind_stack_fill = 0;
    unwind_stack = malloc(sizeof(unwind_handler_stack_entry)*unwind_stack_alloc);
    if (!unwind_stack) {
      fprintf(stderr, "Failed to allocate unwind stack\n");
      exit(1);
    }
  }

  // Start our shutdown thread if it's not running yet
  pthread_mutex_lock(&threadstart_mutex);
  pthread_cleanup_push(_pthread_mutex_unlock, &threadstart_mutex);
  {
    if (!shutdown_thread_started) {
      evsig_ensure_locked(&shutdown_mutex);

      pthread_t t;
      int r = pthread_create(&t, NULL, _shutdown_thread, NULL);

      if (r != 0) {
        fprintf(stderr, "Failed to create shutdown thread for unwind system\n");
        exit(1);
      }

      shutdown_thread_started = true;
    }
  }
  pthread_cleanup_pop(true);

  // Insert this thread into our threadlist
  _threadlist_insert(pthread_self());

  // Set up our dummy key for destructor
  pthread_once(&unwind_key_once, init_unwind_key);
  pthread_setspecific(unwind_key, (void *)1);

  // Needed for ending threads in signal handler
  //
  // This is the default, but we need to be sure.
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  struct sigaction sa;
  sa.sa_handler = _sighandle_dispatch;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGTERM, &sa, NULL); // Set
  sigaction(SIGINT,  &sa, NULL); // Set
  //sigaction(SIGSEGV, &sa, NULL); // Set
  sigaction(SIGHUP, &sa, NULL); // Set
  sigaction(SIGQUIT, &sa, NULL); // Set
  sigaction(SIGILL, &sa, NULL); // Set
  sigaction(SIGPIPE, &sa, NULL); // Set
  sigaction(SIGALRM, &sa, NULL); // Set
  sigaction(SIGBUS, &sa, NULL); // Set
  sigaction(SIGSYS, &sa, NULL); // Set
  sigaction(SIGSTKFLT, &sa, NULL); // Set
  sigaction(SIGABRT, &sa, NULL); // Set
  sigaction(SIGFPE, &sa, NULL); // Set

  unwind_init_ref++;
}

void unwind_cleanup() {
  if (unwind_init_ref > 0) unwind_init_ref--;
  if (unwind_init_ref == 0) free(unwind_stack);

  _on_exit();
}

void _unwind(unwind_return_point* p) {
  // Call all unwind handlers down to unwind_to
  if (unwind_stack_fill > 0) {
    for (int64_t i = unwind_stack_fill-1; i >= p->unwind_to; i--) {
      unwind_handler_stack_entry* e = unwind_stack+i;
      e->h(e->userdata);
    }
  }

  // Remove all stack items down to unwind_index
  unwind_stack_fill = p->unwind_to;

  longjmp(p->jbuf, 1);
}

void _unwind_action(on_unwind_handler h, void* userdata) {

  if (unwind_stack_fill+1 > unwind_stack_alloc) {
    unwind_stack_alloc *= 2;
    unwind_stack =
      realloc(unwind_stack, sizeof(unwind_handler_stack_entry)*unwind_stack_alloc);
    if (!unwind_stack) {
      fprintf(stderr, "Failed to reallocate unwind stack");
      exit(1);
    }
  }

  unwind_handler_stack_entry frame = {.h = h, .userdata = userdata };
  unwind_stack[unwind_stack_fill++] = frame;

  //sw_fprintf(stderr, "[+] stack size: %ld\n", unwind_stack.element_count);
}

void unwind_run_handler(unwind_handler_stack_entry* e) {
  e->h(e->userdata);
  unwind_stack_fill--;

  // TODO: we can't assume we just ran the last element with this function design
  //       unless we make this a unwind_run_handler_pop with no args

  //sw_fprintf(stderr, "[-] stack size: %ld\n", unwind_stack.element_count);
}

void unwind_rm_handler(unwind_handler_stack_entry* e) {
  unwind_stack_fill--;

  // TODO: we can't assume we just removed the last element with this function design
  //       unless we make this a unwind_rm_handler_pop with no args

  //sw_fprintf(stderr, "[-] stack size: %ld\n", unwind_stack.element_count);
}


// Not signal handler safe!
void unwind_run_all_handlers() {
  while (unwind_stack_fill > 0) {
    unwind_handler_stack_entry* e = unwind_stack+(unwind_stack_fill-1);
    e->h(e->userdata);
    unwind_stack_fill--;
  }
}

void unwind_handler_print(void* ptr) { sw_fprintf(stderr, "%s", ptr); }
void unwind_handler_free(void* ptr) { free(ptr); }
void unwind_handler_fclose(void* file) { if(file) sw_fclose((FILE*)file); }
