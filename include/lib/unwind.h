#pragma once
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

#define __GENSYM(base, counter) base##_gensym_##counter
#define _GENSYM(base, counter) __GENSYM(base, counter)
#define GENSYM(base) _GENSYM(base,__LINE__)

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

void unwind_init(); // Must be called before using unwind system
void unwind_cleanup(); // Call when done using unwind system

void _unwind(unwind_return_point* p);
void _unwind_action(on_unwind_handler h, void* userdata);
void unwind_run_handler(unwind_handler_stack_entry* e); // TODO only works with last one
void unwind_rm_handler(unwind_handler_stack_entry* e); // TODO only works with last one
void unwind_run_all_handlers();
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

