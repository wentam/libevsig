#pragma once
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __GENSYM(base, counter) base##_gensym_##counter
#define _GENSYM(base, counter) __GENSYM(base, counter)

// NOTE ON THE USAGE OF __COUNTER__
//
// Using __COUNTER__ technically means we're breaking the ODR (one-definition rule)
// when our macros are used inside inline header functions. This is because
// two different users of the header file may get different source code
// for that line.
//
// However, this is mostly a theoretical concern, because in pratice they will
// compile to byte-for-byte the same function as long as we limit this to just
// variable names.

#define GENSYM(base) _GENSYM(base, __COUNTER__)

typedef struct {
  bool returned;
  jmp_buf jbuf;
  int64_t unwind_to; // Point in unwind stack we should return to
} unwind_return_point;

typedef void (*on_unwind_handler)(void* userdata);

typedef struct {
  on_unwind_handler h;
  void* userdata;
} unwind_handler_stack_entry;

extern thread_local unwind_handler_stack_entry* unwind_stack;
extern thread_local uint64_t unwind_stack_alloc;
extern thread_local uint64_t unwind_stack_fill;


// IMPORTANT:
// If you set your own signal handlers for stuff like SIGTERM/SIGINT, call this in your signal
// handler. It will signal SIGUSR2 in all threads that you have called unwind_init in, which
// we have configured to call all of your unwind handlers.
//
// We set handlers in unwind_init to do this if you have not.
//
// Never set SIGUSR2 if you need this mechanism to function. We've claimed it.
//
// This is useful because - for example - if you have an unwind handler to release a lock, you can
// avoid a stale lock.
void unwind_dispatch_all();

// Can be used as a thread shutdown signal instead of exit_thread_cb
//extern _Atomic bool evsig_thread_shutdown_flag;

// Must be called before using unwind system
//
// You must also call unwind_cleanup() when done.
void unwind_init(bool threadlocal);

// Call when done using unwind system
//
// This will call evsig_thread_shutdown_signal_confirm_shutdown for you,
// so you don't need to explicitly do so.
void unwind_cleanup();

void unwind_all();

void _unwind(unwind_return_point* p);
void _unwind_action(on_unwind_handler h, void* userdata);
void unwind_run_handler(unwind_handler_stack_entry* e); // TODO only works with last one
void unwind_rm_handler(unwind_handler_stack_entry* e); // TODO only works with last one
void unwind_handler_free(void* ptr);
void unwind_handler_fclose(void* file);
void unwind_handler_print(void* str);

#define UNWIND(return_point) _unwind(return_point);

#define UNWIND_ACTION(handler, _userdata) \
  _unwind_action(handler, _userdata); \
  __attribute__((__cleanup__(unwind_run_handler))) \
  unwind_handler_stack_entry GENSYM(unwind_action) = { .h = handler, .userdata = _userdata };

// This action only runs on a signal or explicit unwind. Under normal code flow, does not
// trigger. Useful in constructors where you intend to return something you allocate,
// but need to clean up on error.
#define EXPLICIT_UNWIND_ACTION(handler, _userdata) \
  _unwind_action(handler, _userdata); \
  __attribute__((__cleanup__(unwind_rm_handler))) \
  unwind_handler_stack_entry GENSYM(unwind_action) = { .h = handler, .userdata = _userdata };

#define UNWIND_RETURN_POINT(p, code_that_might_unwind, handle_unwind_code) \
  { \
    unwind_return_point p; \
    p.returned = false; \
    p.unwind_to = unwind_stack_fill; \
    if(setjmp(p.jbuf)) p.returned = true; \
    if(p.returned) { handle_unwind_code; } else { code_that_might_unwind; }; \
  }

#define UNWIND_AUTOPOP_RETURN_POINT(p, handle_unwind_code) \
  unwind_return_point p; \
  p.returned = false; \
  p.unwind_to = unwind_stack_fill; \
  if(setjmp(p.jbuf)) p.returned = true; \
  if(p.returned) { handle_unwind_code; }

#ifdef __cplusplus
}
#endif
