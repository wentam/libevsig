#include "lib/unwind.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib/sigwrap.h"
#include "setjmp.h"
#include "threads.h"
#include <signal.h>
#include <unistd.h>

thread_local unwind_handler_stack_entry* unwind_stack;
thread_local uint64_t unwind_stack_alloc;
thread_local uint64_t unwind_stack_fill;
thread_local int64_t  unwind_init_ref = 0;

static struct sigaction prev_sigterm;
static struct sigaction prev_sigint;
static struct sigaction prev_sigsegv;
static struct sigaction prev_sighup;
static struct sigaction prev_sigquit;
static struct sigaction prev_sigill;
static struct sigaction prev_sigpipe;
static struct sigaction prev_sigalrm;
static struct sigaction prev_sigbus;
static struct sigaction prev_sigsys;
static struct sigaction prev_sigstkflt;
static struct sigaction prev_sigabrt;
static struct sigaction prev_sigfpe;

void _runprev(struct sigaction* a, int sig) {
  if ( a->sa_handler
    && a->sa_handler != SIG_DFL
    && a->sa_handler != SIG_IGN) {
    a->sa_handler(sig);
  }
}

void _sighandle(int sig) {
  unwind_run_all_handlers();
  _runprev(&prev_sigterm, sig);
  _runprev(&prev_sigint, sig);
  _runprev(&prev_sigsegv, sig);
  _runprev(&prev_sighup, sig);
  _runprev(&prev_sigquit, sig);
  _runprev(&prev_sigill, sig);
  _runprev(&prev_sigpipe, sig);
  _runprev(&prev_sigalrm, sig);
  _runprev(&prev_sigbus, sig);
  _runprev(&prev_sigsys, sig);
  _runprev(&prev_sigstkflt, sig);
  _runprev(&prev_sigabrt, sig);
  _runprev(&prev_sigfpe, sig);
  _exit(1);
}

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

  struct sigaction sa;
  sa.sa_handler = _sighandle;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGTERM, NULL, &prev_sigterm); // Save
  sigaction(SIGINT,  NULL, &prev_sigint); // Save
  sigaction(SIGSEGV, NULL, &prev_sigsegv); // Save
  sigaction(SIGHUP, NULL, &prev_sighup); // Save
  sigaction(SIGQUIT, NULL, &prev_sigquit); // Save
  sigaction(SIGILL, NULL, &prev_sigill); // Save
  sigaction(SIGPIPE, NULL, &prev_sigpipe); // Save
  sigaction(SIGALRM, NULL, &prev_sigalrm); // Save
  sigaction(SIGBUS, NULL, &prev_sigbus); // Save
  sigaction(SIGSYS, NULL, &prev_sigsys); // Save
  sigaction(SIGSTKFLT, NULL, &prev_sigstkflt); // Save
  sigaction(SIGABRT, NULL, &prev_sigabrt); // Save
  sigaction(SIGFPE, NULL, &prev_sigfpe); // Save

  sigaction(SIGTERM, &sa, NULL); // Set
  sigaction(SIGINT,  &sa, NULL); // Set
  sigaction(SIGSEGV, &sa, NULL); // Set
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
