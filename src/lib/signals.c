#define _GNU_SOURCE // Needed for dladdr()
#include "lib/signals.h"
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include "lib/sigwrap.h"
#include "stdio.h"
#include "threads.h"
#include "stdbool.h"
#include <stdint.h>
#include "lib/_signals.h"
#include "lib/unwind.h"
#include <string.h>
#include <dlfcn.h>

#define CLR_RED     "\x1b[31m"
#define CLR_GREEN   "\x1b[32m"
#define CLR_YELLOW  "\x1b[33m"
#define CLR_BLUE    "\x1b[34m"
#define CLR_MAGENTA "\x1b[35m"
#define CLR_CYAN    "\x1b[36m"
#define CLR_BOLD    "\033[1m"
#define CLR_RESET   "\x1b[0m"

thread_local sig_handler_stack_entry* sig_handler_stack;
thread_local int64_t sig_handler_stack_alloc;
thread_local int64_t sig_handler_stack_fill;

thread_local sig_restart_stack_entry* sig_restart_stack;
thread_local int64_t sig_restart_stack_alloc;
thread_local int64_t sig_restart_stack_fill;

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

SIG_DEFTYPE(SIG_RESTART_NULL);

#define MAX_FRAMES 64

typedef struct {
    void* addr;
    char func[256];
    char lib[256];
} frame_info;

static const char* catchall_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
  sw_fprintf(stderr, CLR_BOLD "\n------------------------------\n" CLR_RESET, sig_type);
  sw_fprintf(stderr,
             "Unhandled signal of type %s\n" CLR_BOLD CLR_RED "%s" CLR_RESET "\n\n",
             sig_type, msg);

  // Print backtrace
  void *_addrs[1024];
  int nptrs = backtrace(_addrs, 1024);
  //char **strings = backtrace_symbols(buffer, nptrs);

  void* start_addr = __builtin_return_address(0);

  uint64_t start = 0;

  if (start_addr) {
    for (uint64_t i = 0; i < nptrs; i++) {
      if (_addrs[i] == start_addr) {
        start = i+1;
        break;
      }
    }
  }

  void** addrs = _addrs;
  addrs += start;
  nptrs -= start;

  frame_info frames[MAX_FRAMES];
  int max_func = 0, max_lib = 0;

  for (int i = 0; i < nptrs; i++) {
    frames[i].addr = addrs[i];

    Dl_info info;
    const char* fname = "???";
    const char* libname = "???";

    if (dladdr(addrs[i], &info)) {
      if (info.dli_sname) fname = info.dli_sname;
      if (info.dli_fname) libname = info.dli_fname;
    }

    snprintf(frames[i].func, sizeof(frames[i].func), "%s", fname);
    snprintf(frames[i].lib, sizeof(frames[i].lib), "%s", libname);

    int flen = strlen(frames[i].func);
    int llen = strlen(frames[i].lib);
    if (flen > max_func) max_func = flen;
    if (llen > max_lib) max_lib = llen;
  }

  sw_fprintf(stderr, "Backtrace:\n");

  sw_fprintf(stderr, "%-2s  %-18s  %-*s  %-*s\n", "#", "Address", max_func, "Function", max_lib, "File");

  for (int i = 0; i < nptrs; i++) {
    sw_fprintf(stderr, "%-2d  %-18p  %-*s  %-*s\n",
               i, frames[i].addr,
               max_func, frames[i].func,
               max_lib, frames[i].lib
               );
  }

  //sw_fprintf(stderr, "Backtrace:\n");
  //for (int i = 0; i < nptrs; i++) {
  //  sw_fprintf(stderr, "  %s\n", strings[i]);
  //}

  //free(strings);

  sw_fprintf(stderr, "\nRaising SIGINT (this should stop debuggers/the program)\n", sig_type);
  sw_fprintf(stderr, CLR_BOLD "------------------------------\n" CLR_RESET, sig_type);

  // Run all unwind handlers because we're not actually unwinding
  unwind_run_all_handlers();

  // Raise signal so GDB/the program stops
  raise(SIGINT);
  exit(1);

  return SIG_RESTART_NULL;
}

void sig_init() {
  sig_handler_stack_alloc = 32;
  sig_handler_stack_fill  = 0;
  sig_handler_stack       = malloc(sizeof(sig_handler_stack_entry)*sig_handler_stack_alloc);
  if (!sig_handler_stack) {
    fprintf(stderr, "Failed to allocate signal handler stack. Exiting.\n");
    exit(1);
  }

  sig_restart_stack_alloc = 32;
  sig_restart_stack_fill  = 0;
  sig_restart_stack       = malloc(sizeof(sig_restart_stack_entry)*sig_restart_stack_alloc);
  if (!sig_restart_stack) {
    fprintf(stderr, "Failed to allocate restart stack. Exiting.\n");
    exit(1);
  }

  SIG_PERSISTENT_HANDLER(SIGNAL_ALL, catchall_handler, NULL);
  unwind_init();
}

void sig_cleanup() {
  free(sig_handler_stack);
  unwind_cleanup();
}

static void _run_restart(const char* sig_type, const char* restart_type) {
  for (int64_t i = sig_restart_stack_fill-1; i >=0; i--) {
    sig_restart_stack_entry* e = sig_restart_stack+i;
    if (e->restart_type == restart_type
        && (e->sig_type == sig_type || e->sig_type == SIGNAL_ALL)) {
      UNWIND(e->p);
    }
  }
}

void _sig_send(const char* sig_type,
                      char* msg,
                      void* signal_data,
                      sig_cleanup_func signal_data_cleanup_func) {

  for (int64_t i = sig_handler_stack_fill-1; i >= 0; i--) {
    sig_handler_stack_entry* e = sig_handler_stack+i;
    if (e->sig_type == sig_type || e->sig_type == SIGNAL_ALL) {
      const char* restart_type = e->handler(sig_type, e->handler_userdata, msg, signal_data);
      if (signal_data_cleanup_func) signal_data_cleanup_func(signal_data);
      if (restart_type != SIG_RESTART_NULL) {
        _run_restart(sig_type, restart_type);
        fprintf(stderr, "Failed to run restart %s, exiting...\n", restart_type);
        exit(1);
      }
    }
  }
}

uint64_t _sig_push_restart(const char* sig_type, const char* restart_type, unwind_return_point* p) {
  if (sig_restart_stack_fill+1 > sig_restart_stack_alloc) {
    sig_restart_stack_alloc *= 2;
    sig_restart_stack =
      realloc(sig_restart_stack, sizeof(sig_restart_stack_entry)*sig_restart_stack_alloc);

    if (!sig_restart_stack) {
      fprintf(stderr, "Failed to realloc restart stack. Exiting.\n");
      exit(1);
    }
  }

  sig_restart_stack_entry e = {
    .sig_type = sig_type,
    .restart_type = restart_type,
    .p = p,
    .id = 0
  };

  if (sig_restart_stack_fill > 0) e.id = sig_restart_stack[sig_restart_stack_fill-1].id+1;
  sig_restart_stack[sig_restart_stack_fill++] = e;

  return e.id;
}

void _sig_rm_restart(uint64_t id) {
  if (sig_restart_stack_fill <= 0) return;

  int64_t found = -1;
  for (int64_t i = sig_restart_stack_fill-1; i >= 0; i--) {
    if (sig_restart_stack[i].id == id) {
      found = i;
      break;
    }
  }

  if (found >= 0) {
    for (int64_t i = found; i < sig_restart_stack_fill-1; i++) {
      sig_restart_stack[i] = sig_restart_stack[i+1];
    }
    sig_restart_stack_fill--;
  }
}

bool _sig_restart_available(const char* sig_type, const char* restart_type) {
  for (int64_t i = sig_restart_stack_fill-1; i >= 0; i--) {
    if (sig_restart_stack[i].sig_type == sig_type
        && sig_restart_stack[i].restart_type == restart_type) return true;
  }
  return false;
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

    SIG_SEND(SIGNAL_NO_SIG_HANDLER, msg, NULL, NULL);
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

void _unwind_handler_sig_rm_handler(void* ptr) {
  uint64_t id = *((uint64_t*)ptr);
  _sig_rm_handler(id);
}

void _unwind_handler_sig_rm_restart(void* ptr) {
  uint64_t id = *((uint64_t*)ptr);
  _sig_rm_restart(id);
}


const char* sig_static_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
  return userdata;
}


