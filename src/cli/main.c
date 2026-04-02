#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "lib/signals.h"
#include "lib/sigwrap.h"
#include "lib/unwind.h"
#include "stdlib.h"

SIG_DEFTYPE(SIG_RESTART_MAIN);
SIG_DEFTYPE(SIG_RESTART_MAIN2);
SIG_DEFTYPE(SIG_RESTART_MIDDLE);
SIG_DEFTYPE(SIG_RESTART_TOP);

void unwind_handler_slow(void* ptr) {
  printf("Running slow handler\n");
  sleep(5);
  printf("done\n");
}

void* _thread(void* ud) {
  sig_init();
  {
    UNWIND_ACTION(unwind_handler_slow, NULL);
    SIG_SEND(SIGNAL_FAIL, "foo", NULL, NULL);
    //sleep(10);

    printf("hi\n");

    while(true) {
      pthread_testcancel();
      usleep(1000);
    };
  }
  sig_cleanup();

  return NULL;
}

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

    // divide by zero for SIGFPE test
    //
    //srand(time(NULL));
    //volatile int b = 0;
    //volatile int a = rand();
    //printf("%d / %d = %d\n", a, b, a/b);

  SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
    top_func();
  }), SIG_RESTART_MIDDLE, ({
    fprintf(stderr, "RESTART_MIDDLE\n");
    exit(1);
  }));
}

const char* fail_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
  if (SIG_RESTART_AVAILABLE(sig_type, SIG_RESTART_MAIN2))
    return SIG_RESTART_MAIN2;
  return SIG_RESTART_MAIN;
}

int main() {
  sig_init();

  {
    //SIG_AUTOPOP_HANDLER(SIGNAL_FAIL, fail_handler, NULL);

    pthread_t t;
    pthread_create(&t, NULL, &_thread, NULL);

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

    //sleep(20);
    while(true) {
      pthread_testcancel();
      usleep(1000);
    }
  }

  sig_cleanup();
  return 0;
}
