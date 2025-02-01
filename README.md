Libevsig is an exception system, condition system, event-based programming framework, unwind mechanism, and resource cleanup mechanism for C built as a single, simple abstraction. Excluding wrappers the project is approximately just 350 lines.

While flexible, the principle purpose is error handling.

To understand the value of this Common Lisp condition style design over simple exceptions, see [here](https://youtu.be/4NO83wZVT0A?t=2832).

*NOTE: this project is experimental, do not expect a stable API or ABI*

# Examples

Common Lisp style conditions
```C
#include <stdio.h>
#include "evsig/signals.h"
#include "evsig/sigwrap.h"
#include <stdint.h>
#include <stdlib.h>
#include "evsig/unwind.h"

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

Exception-style usage (this is just a convenience wrapper around the CL-style usage)
```C
#include <stdio.h>
#include "evsig/signals.h"
#include "evsig/sigwrap.h"
#include <stdlib.h>
#include "evsig/unwind.h"

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

# How it works

## The signal system

Signal handler functions are pushed onto a thread-local stack. When a signal is sent, handlers for that signal type are called, walking down the stack until one selects an approprate restart. Restarts - of which there are default and user-defined entries - describe how to continue execution after the signal. Unwinding the stack via RESTART_UNWIND is only one option - you may also instruct any other behavior, such as retrying an operation in the same stack frame/function that sent the signal.

## The unwind system

TODO

# Api reference

See the headers. Start at signals.h.


