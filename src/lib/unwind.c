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

static pthread_mutex_t threadlist_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t* threadlist = NULL;
static uint64_t threadlist_count = 0;
static uint64_t threadlist_alloc = 0;

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

//static struct sigaction prev_sigterm;
//static struct sigaction prev_sigint;
//static struct sigaction prev_sigsegv;
//static struct sigaction prev_sighup;
//static struct sigaction prev_sigquit;
//static struct sigaction prev_sigill;
//static struct sigaction prev_sigpipe;
//static struct sigaction prev_sigalrm;
//static struct sigaction prev_sigbus;
//static struct sigaction prev_sigsys;
//static struct sigaction prev_sigstkflt;
//static struct sigaction prev_sigabrt;
//static struct sigaction prev_sigfpe;

void _runprev(struct sigaction* a, int sig) {
  if (!a) return;

  if (a->sa_flags & SA_SIGINFO) {
    if (a->sa_sigaction && a->sa_sigaction != (void*)SIG_IGN && a->sa_sigaction != (void*)SIG_DFL) {
      a->sa_sigaction(sig, NULL, NULL);
    }
  } else {
    if (a->sa_handler && a->sa_handler != SIG_IGN && a->sa_handler != SIG_DFL) {
      a->sa_handler(sig);
    }
  }
}

static void _sighandle_usr(int sig) {
  //unwind_run_all_handlers();
  pthread_exit(NULL);

  //struct sigaction* a = NULL;

  //switch (sig) {
  //  case SIGTERM: a = &prev_sigterm; break;
  //  case SIGINT:  a = &prev_sigint;  break;
  //  case SIGSEGV: a = &prev_sigsegv; break;
  //  case SIGHUP:  a = &prev_sighup;  break;
  //  case SIGQUIT: a = &prev_sigquit; break;
  //  case SIGILL:  a = &prev_sigill;  break;
  //  case SIGPIPE: a = &prev_sigpipe; break;
  //  case SIGALRM: a = &prev_sigalrm; break;
  //  case SIGBUS:  a = &prev_sigbus;  break;
  //  case SIGSYS:  a = &prev_sigsys;  break;
  //  case SIGSTKFLT: a = &prev_sigstkflt; break;
  //  case SIGABRT: a = &prev_sigabrt; break;
  //  case SIGFPE:  a = &prev_sigfpe; break;
  //  default: break;
  //}

  //if (a) _runprev(a, sig);

}

// For SIGTERM SIGINT etc to dispatch SIGUSR2 to our threads
static void _sighandle_dispatch(int sig) {
  unwind_dispatch_all();
  //free(threadlist);
  for (uint64_t i = 0; i < threadlist_count; i++) {
    pthread_join(threadlist[i], NULL);
  }
  pthread_exit(NULL);
  exit(1);
}

void unwind_dispatch_all() {
  pthread_mutex_lock(&threadlist_mutex); {
    for (uint64_t i = 0; i < threadlist_count; i++) {
      if (!pthread_equal(threadlist[i], pthread_self())) {
        pthread_kill(threadlist[i], SIGUSR2);
      }
    }
  } pthread_mutex_unlock(&threadlist_mutex);
}

static void _run_unwind_handlers(void* ptr) {
  unwind_run_all_handlers();
}

static void init_unwind_key() {
  pthread_key_create(&unwind_key, _run_unwind_handlers);
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

  // Append this thread to our threadlist
  pthread_mutex_lock(&threadlist_mutex); {
    if (threadlist_alloc == 0) {
      threadlist_alloc = 8;
      threadlist = malloc(sizeof(pthread_t)*threadlist_alloc);
      if (!threadlist) {
        fprintf(stderr, "Failed to allocate threadlist");
        exit(1);
      }
    }

    if (threadlist_count >= threadlist_alloc) {
      threadlist_alloc *= 2;
      threadlist = realloc(threadlist, sizeof(pthread_t)*threadlist_alloc);
      if (!threadlist) {
        fprintf(stderr, "Failed to reallocate threadlist");
        exit(1);
      }
    }

    threadlist[threadlist_count++] = pthread_self();
  } pthread_mutex_unlock(&threadlist_mutex);

  // Set up our dummy key for destructor
  pthread_once(&unwind_key_once, init_unwind_key);
  pthread_setspecific(unwind_key, (void *)1);

  struct sigaction sa_usr;
  sa_usr.sa_handler = _sighandle_usr;
  sigemptyset(&sa_usr.sa_mask);
  sa_usr.sa_flags = 0;

  struct sigaction sa;
  sa.sa_handler = _sighandle_dispatch;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  //if (sigaction(SIGTERM, NULL, &prev_sigterm) != 0) {
  //  memset(&prev_sigterm, 0, sizeof(prev_sigterm));
  //}
  //if (sigaction(SIGINT,  NULL, &prev_sigint) != 0) {
  //  memset(&prev_sigint, 0, sizeof(prev_sigint));
  //}
  //if (sigaction(SIGSEGV, NULL, &prev_sigsegv) != 0) {
  //  memset(&prev_sigsegv, 0, sizeof(prev_sigsegv));
  //}
  //if (sigaction(SIGHUP, NULL, &prev_sighup) != 0) {
  //  memset(&prev_sighup, 0, sizeof(prev_sighup));
  //}
  //if (sigaction(SIGQUIT, NULL, &prev_sigquit) != 0) {
  //  memset(&prev_sigquit, 0, sizeof(prev_sigquit));
  //}
  //if (sigaction(SIGILL, NULL, &prev_sigill) != 0) {
  //  memset(&prev_sigill, 0, sizeof(prev_sigill));
  //}
  //if (sigaction(SIGPIPE, NULL, &prev_sigpipe) != 0) {
  //  memset(&prev_sigpipe, 0, sizeof(prev_sigpipe));
  //}
  //if (sigaction(SIGALRM, NULL, &prev_sigalrm) != 0) {
  //  memset(&prev_sigalrm, 0, sizeof(prev_sigalrm));
  //}
  //if (sigaction(SIGBUS, NULL, &prev_sigbus) != 0) {
  //  memset(&prev_sigbus, 0, sizeof(prev_sigbus));
  //}
  //if (sigaction(SIGSYS, NULL, &prev_sigsys) != 0) {
  //  memset(&prev_sigsys, 0, sizeof(prev_sigsys));
  //}
  //if (sigaction(SIGSTKFLT, NULL, &prev_sigstkflt) != 0) {
  //  memset(&prev_sigstkflt, 0, sizeof(prev_sigstkflt));
  //};
  //if (sigaction(SIGABRT, NULL, &prev_sigabrt) != 0) {
  //  memset(&prev_sigabrt, 0, sizeof(prev_sigabrt));
  //}
  //if (sigaction(SIGFPE, NULL, &prev_sigfpe) != 0) {
  //  memset(&prev_sigfpe, 0, sizeof(prev_sigfpe));
  //}

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

  sigaction(SIGUSR2, &sa_usr, NULL); // Set

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
