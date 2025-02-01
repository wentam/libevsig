Libevsig is an exception system, condition system, event-based programming framework, unwind mechanism, and resource cleanup mechanism for C built as a single, simple abstraction. Excluding wrappers the project is approximately just 350 lines.

While flexible, the principle purpose is error handling.

*NOTE: this project is experimental, do not expect a stable API or ABI*

# Examples

Exception handling
```C
#include <stdio.h>
#include "lib/signals.h"
#include "lib/sigwrap.h"
#include <stdlib.h>
#include "lib/unwind.h"

void signal_func() {
  SIG_SEND(SIGNAL_FAIL, "something bad happened", NULL, NULL, {});
}

void middle_func() {
  sw_fprintf(stderr, "pretending to ALLOCATE something\n");
  UNWIND_ACTION(unwind_handler_print, "pretending to FREE something\n");
  signal_func();
}

int main() {
  sig_init();

  TRY_CATCH({
    middle_func();
  }, SIGNAL_FAIL, {
    sw_fprintf(stderr, "catch!\n");
  });

  sig_cleanup();
  return 0;
}
```

Common Lisp style conditions
```C
#include <stdio.h>
#include "lib/signals.h"
#include "lib/sigwrap.h"
#include <stdint.h>
#include <stdlib.h>
#include "lib/unwind.h"

void signal_func() {
  SIG_SEND(SIGNAL_FAIL, "something bad happened", NULL, NULL, {});
}

void middle_func() {
  sw_fprintf(stderr, "pretending to ALLOCATE something\n");
  UNWIND_ACTION(unwind_handler_print, "pretending to FREE something\n");
  signal_func();
}

sig_restart fail_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
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
    SIG_AUTOPOP_HANDLER(SIGNAL_FAIL, fail_handler, NULL);
    middle_func();
  }

  sig_cleanup();
  return 0;
}

```

# How it works

## The signal system

TODO

## The unwind system

TODO

# Api reference

See the headers. Start at signals.h.


