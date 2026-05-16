#define _GNU_SOURCE
#include <unistd.h>

#include <stdio.h>
#include <libevsig/signals.h>
#include <libevsig/sigwrap.h>
#include <libevsig/unwind.h>
#include "stdlib.h"
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

SIG_DEFTYPE(SIG_RESTART_MAIN);
SIG_DEFTYPE(SIG_RESTART_MIDDLE);
SIG_DEFTYPE(SIG_RESTART_TOP);

static void unwind_handler_sigsend(void* ptr) {
  fprintf(stderr, "hi\n");
  //SIG_SEND(SIGNAL_FAIL, "This signal was sent from an unwind handler", NULL, NULL);
  //fprintf(stderr, "hi2\n");
}

void top_func() {
  SIG_AUTOPOP_RESTART(SIGNAL_FAIL, SIG_RESTART_TOP, ({
    sw_fprintf(stderr, "RESTART_TOP\n");
    exit(1);
  }));

  SIG_SEND(SIGNAL_FAIL, "something bad happened", NULL, NULL);
}

void middle_func() {

  sw_fprintf(stderr, "pretending to ALLOCATE something\n");
  UNWIND_ACTION(unwind_handler_print, "pretending to FREE something\n");

  UNWIND_ACTION(unwind_handler_sigsend, NULL);

  SIG_AUTOPOP_RESTART(SIGNAL_FAIL, SIG_RESTART_MIDDLE, ({
    sw_fprintf(stderr, "RESTART_MIDDLE\n");
    exit(1);
  }));

  top_func();
}

void* thread(void*) {
  evsig_thread_shutdown_signal_register_thread(&evsig_global_thread_shutdown_signal, gettid());
  while(1) {
    if (evsig_thread_shutdown_p(&evsig_global_thread_shutdown_signal)) break;
    sleep(1);
    printf("bop\n");
  }
  printf("THREAD DONE\n");
  evsig_thread_shutdown_signal_confirm_shutdown(&evsig_global_thread_shutdown_signal, gettid());

  return NULL;
}

const char* fail_handler(const char* sig_type, void* userdata, const char* msg, void* signal_data) {
  return SIG_RESTART_MAIN;
}

int main() {
  sig_init(false, pthread_exit, NULL);
  {
    //SIG_AUTOPOP_HANDLER(SIGNAL_FAIL, fail_handler, NULL);

    //FILE* foo = sw_fopen("/tmp/fouhetnoaso", "r");

    pthread_t f;
    pthread_create(&f, NULL, thread, NULL);

    //sw_fclose(foo);

    // This restart runs because the handler we pushed selected SIG_RESTART_MAIN
    // upon receiving SIGNAL_FAIL, and this restart was on the restart stack
    // available to be selected.
    //
    // After running the restart, execution would continue on to repeat
    // middle_func() again if we didn't have the exit() call.
    SIG_AUTOPOP_RESTART(SIGNAL_FAIL, SIG_RESTART_MAIN, ({
      sw_fprintf(stderr, "RESTART_MAIN\n");
      exit(1);
    }));

    middle_func();
    pthread_join(f, NULL);
  }

  sig_cleanup();
  return 0;
}
