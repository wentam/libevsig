#include <stdio.h>
#include <libevsig/signals.h>
#include <libevsig/sigwrap.h>
#include <libevsig/unwind.h>
#include "stdlib.h"

SIG_DEFTYPE(SIG_RESTART_MAIN);
SIG_DEFTYPE(SIG_RESTART_MIDDLE);
SIG_DEFTYPE(SIG_RESTART_TOP);

void top_func() {
  SIG_PROVIDE_AUTOPOP_RESTART(SIGNAL_FAIL, SIG_RESTART_TOP, ({
    sw_fprintf(stderr, "RESTART_TOP\n");
    exit(1);
  }));

  SIG_SEND(SIGNAL_FAIL, "something bad happened", NULL, NULL);
}

void middle_func() {
  sw_fprintf(stderr, "pretending to ALLOCATE something\n");
  UNWIND_ACTION(unwind_handler_print, "pretending to FREE something\n");

  SIG_PROVIDE_AUTOPOP_RESTART(SIGNAL_FAIL, SIG_RESTART_MIDDLE, ({
    sw_fprintf(stderr, "RESTART_MIDDLE\n");
    exit(1);
  }));

  top_func();
}

const char* fail_handler(const char* sig_type, void* userdata, const char* msg, void* signal_data) {
  return SIG_RESTART_MAIN;
}

int main() {
  sig_init();

  {
    SIG_AUTOPOP_HANDLER(SIGNAL_FAIL, fail_handler, NULL);

    FILE* foo = sw_fopen("/tmp/fouhetnoaso", "r");

    sw_fclose(foo);

    // This restart runs because the handler we pushed selected SIG_RESTART_MAIN
    // upon receiving SIGNAL_FAIL, and this restart was on the restart stack
    // available to be selected.
    //
    // After running the restart, execution would continue on to repeat
    // middle_func() again if we didn't have the exit() call.
    SIG_PROVIDE_AUTOPOP_RESTART(SIGNAL_FAIL, SIG_RESTART_MAIN, ({
      sw_fprintf(stderr, "RESTART_MAIN\n");
      exit(1);
    }));

    middle_func();
  }

  sig_cleanup();
  return 0;
}
