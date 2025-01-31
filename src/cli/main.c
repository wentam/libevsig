#include <stdio.h>
#include "lib/signals.h"
#include "lib/sigwrap.h"
#include <stdint.h>
#include <stdlib.h>
#include "lib/unwind.h"

void signal_func() {
  //SIG_ASSERT_HANDLER(SIGNAL_FAIL);
  //int64_t* data = sw_malloc(sizeof(int64_t));
  //*data = 42;
  SIG_SEND(SIGNAL_FAIL, "something bad happened", NULL, NULL, 
{});
}

void middle_func() {
  sw_fprintf(stderr, "pretending to ALLOCATE something\n");
  UNWIND_ACTION(unwind_handler_print, "pretending to FREE something\n");
  signal_func();
}

sig_restart fail_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
  //sw_fprintf(stderr, "got data %ld\n", *(int64_t*)signal_data);
  sw_fprintf(stderr, "handling, selecting RESTART_EXIT\n");
  uint32_t* zero = sw_malloc(sizeof(uint32_t));
  *zero = 0;
  return (sig_restart){ .restart_type = SIG_RESTART_EXIT,
                        .restart_data = zero,
                        .restart_data_cleanup = free };
}

int main() {
  sig_init();

  {
    //SIG_AUTOPOP_HANDLER(SIGNAL_FAIL, fail_handler, NULL);
    middle_func();

    //TRY_CATCH({
    //  middle_func();
    //}, SIGNAL_FAIL, {
    //  sw_fprintf(stderr, "catch!\n");
    //});
  }

  sig_cleanup();
  return 0;
}
