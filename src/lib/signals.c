#include "lib/signals.h"
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include "lib/sigwrap.h"
#include "lib/evsig_util.h"
#include "stdio.h"
#include "threads.h"
#include "stdbool.h"
#include <stdint.h>
#include "lib/_signals.h"
#include "lib/unwind.h"

thread_local sig_handler_stack_entry* sig_handler_stack;
thread_local int64_t sig_handler_stack_alloc;
thread_local int64_t sig_handler_stack_fill;

SIG_DEFTYPE(SIGNAL_NOTHING);
SIG_DEFTYPE(SIGNAL_ALL);
SIG_DEFTYPE(SIGNAL_SUCCESS);
SIG_DEFTYPE(SIGNAL_FAIL);
SIG_DEFTYPE(SIGNAL_UNKNOWN_ERROR);
SIG_DEFTYPE(SIGNAL_EOF);
SIG_DEFTYPE(SIGNAL_INVALID_INPUT);
SIG_DEFTYPE(SIGNAL_UNSUPPORTED_RESTART);
SIG_DEFTYPE(SIGNAL_ALLOC_FAILED);
SIG_DEFTYPE(SIGNAL_NO_SIG_HANDLER);
SIG_DEFTYPE(SIGNAL_READ_ERROR);
SIG_DEFTYPE(SIGNAL_WRITE_ERROR);
SIG_DEFTYPE(SIGNAL_PERMISSION_DENIED);
SIG_DEFTYPE(SIGNAL_FILE_EXISTS);
SIG_DEFTYPE(SIGNAL_IO_ERROR);
SIG_DEFTYPE(SIGNAL_IS_DIRECTORY);
SIG_DEFTYPE(SIGNAL_TOO_MANY_OPEN_FILES);
SIG_DEFTYPE(SIGNAL_NO_SUCH_FILE_OR_DIR);
SIG_DEFTYPE(SIGNAL_NOT_ENOUGH_MEMORY);
SIG_DEFTYPE(SIGNAL_NO_SUCH_DEVICE);
SIG_DEFTYPE(SIGNAL_READ_ONLY_FS);
SIG_DEFTYPE(SIGNAL_BAD_FILE_DESCRIPTOR);
SIG_DEFTYPE(SIGNAL_INTERRUPTED);
SIG_DEFTYPE(SIGNAL_NOT_ENOUGH_SPACE);
SIG_DEFTYPE(SIGNAL_WOULD_BLOCK);
SIG_DEFTYPE(SIGNAL_FILE_TOO_BIG);
SIG_DEFTYPE(SIGNAL_BUSY);
SIG_DEFTYPE(SIGNAL_TOO_MANY_SYMLINKS);
SIG_DEFTYPE(SIGNAL_IS_NOT_DIRECTORY);
SIG_DEFTYPE(SIGNAL_UNSUPPORTED_OP);
SIG_DEFTYPE(SIGNAL_NOT_SEEKABLE);
SIG_DEFTYPE(SIGNAL_CORRUPT_DATA);
SIG_DEFTYPE(SIGNAL_UNSUPPORTED);

SIG_DEFTYPE(SIG_RESTART_CONTINUE);
SIG_DEFTYPE(SIG_RESTART_RETURN);
SIG_DEFTYPE(SIG_RESTART_EXIT);
SIG_DEFTYPE(SIG_RESTART_UNWIND);
SIG_DEFTYPE(SIG_RESTART_NULL);

static uint32_t one = 1;

static sig_restart catchall_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
  sw_fprintf(stderr, CLR_BOLD "\n------------------------------\n" CLR_RESET, sig_type);
  sw_fprintf(stderr,
             "Unhandled signal of type %s\n" CLR_BOLD CLR_RED "%s" CLR_RESET "\n\n",
             sig_type, msg);

  // Print backtrace
  void *buffer[1024];
  int nptrs = backtrace(buffer, 1024);
  char **strings = backtrace_symbols(buffer, nptrs);

  sw_fprintf(stderr, "Backtrace:\n");
  for (int i = 0; i < nptrs; i++) {
    sw_fprintf(stderr, "  %s\n", strings[i]);
  }

  free(strings);

  sw_fprintf(stderr, "\nRaising SIGINT (this should stop debuggers/the program)\n", sig_type);
  sw_fprintf(stderr, CLR_BOLD "------------------------------\n" CLR_RESET, sig_type);

  // Raise signal so GDB/the program stops
  raise(SIGINT);

  return (sig_restart){ .restart_type = SIG_RESTART_EXIT,
                        .restart_data = &one,
                        .restart_data_cleanup = NULL};
}

void sig_init() {
  sig_handler_stack_alloc = 32;
  sig_handler_stack_fill  = 0;
  sig_handler_stack       = malloc(sizeof(sig_handler_stack_entry)*sig_handler_stack_alloc);
  if (!sig_handler_stack) {
    fprintf(stderr, "Failed to allocate signal handler stack. Exiting.\n");
    exit(1);
  }
  SIG_PERSISTENT_HANDLER(SIGNAL_ALL, catchall_handler, NULL);
  unwind_init();
}

void sig_cleanup() {
  free(sig_handler_stack);
  unwind_cleanup();
}

void _sig_free_restart(sig_restart* s) {
  if (s->restart_data_cleanup) s->restart_data_cleanup(s->restart_data);
}

sig_restart _sig_send(const char* sig_type,
                      char* msg,
                      void* signal_data,
                      sig_cleanup_func signal_data_cleanup_func) {
  for (int64_t i = sig_handler_stack_fill-1; i >= 0; i--) {
    sig_handler_stack_entry* e = sig_handler_stack+i;
    if (e->sig_type == sig_type || e->sig_type == SIGNAL_ALL) {
      sig_restart r = e->handler(sig_type, e->handler_userdata, msg, signal_data);
      if (signal_data_cleanup_func) signal_data_cleanup_func(signal_data);
      if (r.restart_type != SIG_RESTART_NULL) return r;
    }
  }

  return (sig_restart){ .restart_type = SIG_RESTART_NULL,
                        .restart_data = NULL,
                        .restart_data_cleanup = NULL };
}

const uint64_t _sig_push_handler(const char* sig_type, sig_handler handler, void* userdata) {
  if (sig_handler_stack_fill+1 > sig_handler_stack_alloc) {
    sig_handler_stack_alloc *= 2;
    sig_handler_stack =
      realloc(sig_handler_stack, sizeof(sig_handler_stack_entry)*sig_handler_stack_alloc);

    if (!sig_handler_stack) {
      fprintf(stderr, "Failed to realloc signal handler stack. Exiting.\n");
      exit(1);
    }
  }

  sig_handler_stack_entry e = { .sig_type = sig_type,
                                .handler = handler,
                                .handler_userdata = userdata,
                                .id = 0 };

  if (sig_handler_stack_fill > 0) e.id = sig_handler_stack[sig_handler_stack_fill-1].id+1;

  sig_handler_stack[sig_handler_stack_fill++] = e;

  return e.id;
}

void _sig_rm_handler(uint64_t id) {
  if (sig_handler_stack_fill <= 0) return;

  int64_t found = -1;
  for (int64_t i = sig_handler_stack_fill-1; i >= 0; i--) {
    if (sig_handler_stack[i].id == id) {
      found = i;
      break;
    }
  }

  // Shift found handler out
  if (found >= 0) {
    fprintf(stderr, "removing handler\n");
    for (int64_t i = found; i < sig_handler_stack_fill-1; i++) {
      sig_handler_stack[i] = sig_handler_stack[i+1];
    }

    sig_handler_stack_fill--;
  }
}

void _sig_assert_handler(const char* sig_type) {
  bool exists = false;
  for (uint64_t i = 0; i < sig_handler_stack_fill; i++) {
    if (sig_handler_stack[i].sig_type == sig_type) { exists = true; break; }
  }

  if (!exists) {
    char msg[1024];
    sprintf(msg, "Assertion failed: no signal handler for signal type %s\n", sig_type);

    SIG_SEND(SIGNAL_NO_SIG_HANDLER, msg, NULL, NULL, {});
  }
}

void _sig_assertwarn_handler(const char* sig_type) {
  bool exists = false;
  for (uint64_t i = 0; i < sig_handler_stack_fill; i++) {
    if (sig_handler_stack[i].sig_type == sig_type) { exists = true; break; }
  }

  if (!exists) {
    fprintf(stderr,
            "WARNING: No signal handler for signal of asserted type %s\n",
            sig_type);
  }
}

void _unwind_handler_free_restart(void* r) { _sig_free_restart((sig_restart*)r); }

sig_restart try_catch_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
  return (sig_restart){ .restart_type = SIG_RESTART_UNWIND,
                        .restart_data = userdata,
                        .restart_data_cleanup = NULL};
}
