//#include <stdio.h>
//#include <unistd.h>
//#include <pthread.h>
//#include "lib/signals.h"
//#include "lib/sigwrap.h"
//#include "lib/unwind.h"
//#include "stdlib.h"
//
//SIG_DEFTYPE(SIG_RESTART_MAIN);
//SIG_DEFTYPE(SIG_RESTART_MAIN2);
//SIG_DEFTYPE(SIG_RESTART_MAIN3);
//SIG_DEFTYPE(SIG_RESTART_MIDDLE);
//SIG_DEFTYPE(SIG_RESTART_TOP);
//
//void unwind_handler_slow(void* ptr) {
//  printf("Running slow handler\n");
//  sleep(5);
//  printf("done\n");
//}
//
////void* _thread(void* ud) {
////  sig_init();
////  {
////    UNWIND_ACTION(unwind_handler_slow, NULL);
////    SIG_SEND(SIGNAL_FAIL, "foo", NULL, NULL);
////    //sleep(10);
////
////    printf("hi\n");
////
////    while(true) {
////      pthread_testcancel();
////      usleep(1000);
////    };
////  }
////  sig_cleanup();
////
////  return NULL;
////}
//
//void top_func() {
//  SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
//    SIG_SEND(SIGNAL_FAIL, "something bad happened", NULL, NULL);
//  }), SIG_RESTART_TOP, ({
//    fprintf(stderr, "RESTART_TOP\n");
//    exit(1);
//  }));
//}
//
//void middle_func() {
//  sw_fprintf(stderr, "pretending to ALLOCATE something\n");
//  UNWIND_ACTION(unwind_handler_print, "pretending to FREE something\n");
//
//    // divide by zero for SIGFPE test
//    //
//    //srand(time(NULL));
//    //volatile int b = 0;
//    //volatile int a = rand();
//    //printf("%d / %d = %d\n", a, b, a/b);
//
//  SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
//    top_func();
//  }), SIG_RESTART_MIDDLE, ({
//    fprintf(stderr, "RESTART_MIDDLE\n");
//    exit(1);
//  }));
//}
//
//const char* fail_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
//  return SIG_RESTART_MAIN3;
//  if (SIG_RESTART_AVAILABLE(sig_type, SIG_RESTART_MAIN2))
//    return SIG_RESTART_MAIN2;
//  return SIG_RESTART_MAIN;
//}
//
//int main() {
//  sig_init();
//
//  {
//    SIG_AUTOPOP_HANDLER(SIGNAL_FAIL, fail_handler, NULL);
//
//    SIG_PROVIDE_AUTOPOP_RESTART(SIGNAL_ALL, SIG_RESTART_MAIN3, ({
//      fprintf(stderr, "\nRESTART_MAIN3:\n");
//    }));
//
//    fprintf(stderr, "hello\n");
//
//    //pthread_t t;
//    //pthread_create(&t, NULL, &_thread, NULL);
//
//    SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
//      SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
//        middle_func();
//      }), SIG_RESTART_MAIN2, ({
//        fprintf(stderr, "RESTART_MAIN2\n");
//        exit(1);
//      }));
//    }), SIG_RESTART_MAIN, ({
//      fprintf(stderr, "RESTART_MAIN\n");
//      exit(1);
//    }));
//
//    //sleep(20);
//    //while(true) {
//    //  pthread_testcancel();
//    //  usleep(1000);
//    //}
//  }
//
//  sig_cleanup();
//  return 0;
//}


#include <stdio.h>
#include <lib/signals.h>
#include <lib/sigwrap.h>
#include <lib/unwind.h>
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

const char* fail_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
  return SIG_RESTART_MAIN;
}

int main() {
  sig_init();

  {
    SIG_AUTOPOP_HANDLER(SIGNAL_FAIL, fail_handler, NULL);

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
