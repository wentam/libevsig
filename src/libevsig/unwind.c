#define _GNU_SOURCE
#include <unistd.h>

#include "libevsig/unwind.h"
#include <stdio.h>
#include <stdlib.h>
#include "libevsig/sigwrap.h"
#include "setjmp.h"
#include "threads.h"
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "libevsig/evsig_mutex.h"
#include "libevsig/thread_shutdown_signal.h"

// TODO make signal handling optional
//
// TODO expose public interface to call in your own signal handler as an
// alternative to using ours.

thread_local unwind_handler_stack_entry* unwind_stack;
thread_local uint64_t unwind_stack_alloc;
thread_local uint64_t unwind_stack_fill;
thread_local int64_t  unwind_init_ref = 0;

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
[[maybe_unused]]
static void _pthread_mutex_unlock(void* ud) {
  pthread_mutex_unlock((pthread_mutex_t*)ud);
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
  evsig_thread_shutdown_signal_send_async(&evsig_global_thread_shutdown_signal,
                                          true);
}

void unwind_all() {
  evsig_thread_shutdown_signal_send_async(&evsig_global_thread_shutdown_signal,
                                          true);
}

void unwind_init(bool threadlocal) {
  if (unwind_init_ref == 0) {
    unwind_stack_alloc = 32;
    unwind_stack_fill = 0;
    unwind_stack = malloc(sizeof(unwind_handler_stack_entry)*unwind_stack_alloc);
    if (!unwind_stack) {
      fprintf(stderr, "Failed to allocate unwind stack\n");
      exit(1);
    }
  }

  // Start our shutdown thread if it's not running yet (unless we are
  // threadlocal)
  if (!threadlocal) {
    struct sigaction sa;
    sa.sa_handler = _sighandle_dispatch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM,   &sa, NULL);
    sigaction(SIGINT,    &sa, NULL);
    sigaction(SIGHUP,    &sa, NULL);
    sigaction(SIGQUIT,   &sa, NULL);
    sigaction(SIGILL,    &sa, NULL);
    sigaction(SIGPIPE,   &sa, NULL);
    sigaction(SIGALRM,   &sa, NULL);
    sigaction(SIGSYS,    &sa, NULL);
    sigaction(SIGSTKFLT, &sa, NULL);
    sigaction(SIGABRT,   &sa, NULL);
    sigaction(SIGFPE,    &sa, NULL);
  }

  unwind_init_ref++;
}

// This should be the only "run all unwind handlers and exit" path,
// because we always want to make sure our internal state is
// cleaned up in that scenario.
void unwind_cleanup() {
  if (unwind_init_ref > 0) unwind_init_ref--;
  if (unwind_init_ref == 0) {

    // Run all unwind handlers left for this thread
    while (unwind_stack_fill > 0) {
      unwind_handler_stack_entry* e = unwind_stack+(unwind_stack_fill-1);
      e->h(e->userdata);
      unwind_stack_fill--;
    }

    free(unwind_stack);
    evsig_thread_shutdown_signal_confirm_shutdown(&evsig_global_thread_shutdown_signal, gettid());
  }
}

void _unwind(unwind_return_point* p) {
  // Call all unwind handlers down to unwind_to
  if (unwind_stack_fill > 0) {
    for (int64_t i = unwind_stack_fill-1; i >= p->unwind_to; i--) {
      // It is critical to adjust the unwind stack *before*
      // calling the handler in case the handler also chooses
      // to unwind somewhere. This prevents infinite recursion.
      unwind_stack_fill--;
      unwind_handler_stack_entry* e = unwind_stack+i;
      e->h(e->userdata);
    }
  }

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
  // It is critical to adjust the unwind stack *before*
  // calling the handler in case the handler also chooses
  // to unwind somewhere. This prevents infinite recursion.
  unwind_stack_fill--;
  e->h(e->userdata);

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

void unwind_handler_print(void* ptr) { sw_fprintf(stderr, "%s", ptr); }
void unwind_handler_free(void* ptr) { free(ptr); }
void unwind_handler_fclose(void* file) { if(file) sw_fclose((FILE*)file); }
