#include <stdio.h>
#include "lib/signals.h"
#include "lib/sigwrap.h"
#include "lib/unwind.h"
#include "stdlib.h"

SIG_DEFTYPE(SIG_RESTART_MAIN);
SIG_DEFTYPE(SIG_RESTART_MAIN2);
SIG_DEFTYPE(SIG_RESTART_MIDDLE);
SIG_DEFTYPE(SIG_RESTART_TOP);

void top_func() {
  SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
    SIG_SEND(SIGNAL_FAIL, "something bad happened", NULL, NULL);
  }), SIG_RESTART_TOP, ({
    fprintf(stderr, "RESTART_TOP\n");
    exit(1);
  }));
}

void middle_func() {
  sw_fprintf(stderr, "pretending to ALLOCATE something\n");
  UNWIND_ACTION(unwind_handler_print, "pretending to FREE something\n");

  SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
    top_func();
  }), SIG_RESTART_MIDDLE, ({
    fprintf(stderr, "RESTART_MIDDLE\n");
    exit(1);
  }));
}

const char* fail_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
  return SIG_RESTART_MAIN2;
}

int main() {
  sig_init();

  {
    SIG_AUTOPOP_HANDLER(SIGNAL_FAIL, fail_handler, NULL);

    SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
      SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
        middle_func();
      }), SIG_RESTART_MAIN2, ({
        fprintf(stderr, "RESTART_MAIN2\n");
        exit(1);
      }));
    }), SIG_RESTART_MAIN, ({
      fprintf(stderr, "RESTART_MAIN\n");
      exit(1);
    }));
  }

  sig_cleanup();
  return 0;
}
