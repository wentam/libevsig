#include "lib/unwind.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib/sigwrap.h"
#include "setjmp.h"
#include "threads.h"

thread_local unwind_handler_stack_entry* unwind_stack;
thread_local uint64_t unwind_stack_alloc;
thread_local uint64_t unwind_stack_fill;
thread_local int64_t  unwind_init_ref = 0;

void unwind_init() {
  if (unwind_init_ref == 0) {
    unwind_stack_alloc = 32;
    unwind_stack_fill = 0;
    unwind_stack = malloc(sizeof(unwind_handler_stack_entry)*unwind_stack_alloc);
    if (!unwind_stack) {
      fprintf(stderr, "Failed to allocate unwind stack");
      exit(1);
    }
  }
  unwind_init_ref++;
}

void unwind_cleanup() {
  if (unwind_init_ref > 0) unwind_init_ref--;
  if (unwind_init_ref == 0) free(unwind_stack);
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
