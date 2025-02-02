Libevsig is an exception system, condition system, event-based programming framework, unwind mechanism, and resource cleanup mechanism for C built as a single, simple abstraction. Excluding wrappers the project is approximately just 400 lines.

While flexible, the principle purpose is error handling.

To understand the value of this Common Lisp condition style design over simple exceptions, see [here](https://youtu.be/4NO83wZVT0A?t=2832).

*NOTE: this project is experimental, do not expect a stable API or ABI*

# Example

The functions main, middle, and top all have provided restarts. Depending on what fail_handler
returns, we can return to any of the those places in the stack providing a custom recovery for
each.

Also observe how if fail_handler returns SIG_RESTART_MAIN, we pretend to free a resource even
though the stack unwinds past it due to UNWIND_ACTION.

```C
#include <stdio.h>
#include "evsig/signals.h"
#include "evsig/sigwrap.h"
#include "evsig/unwind.h"
#include "stdlib.h"

SIG_DEFTYPE(SIG_RESTART_MAIN);
SIG_DEFTYPE(SIG_RESTART_MIDDLE);
SIG_DEFTYPE(SIG_RESTART_TOP);

void top_func() {
  SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
    SIG_SEND(SIGNAL_FAIL, "something bad happened", NULL, NULL);
  }), SIG_RESTART_TOP, ({
    sw_fprintf(stderr, "RESTART_TOP\n");
    exit(1);
  }));
}

void middle_func() {
  sw_fprintf(stderr, "pretending to ALLOCATE something\n");
  UNWIND_ACTION(unwind_handler_print, "pretending to FREE something\n");

  SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
    top_func();
  }), SIG_RESTART_MIDDLE, ({
    sw_fprintf(stderr, "RESTART_MIDDLE\n");
    exit(1);
  }));
}

const char* fail_handler(const char* sig_type, void* userdata, char* msg, void* signal_data) {
  return SIG_RESTART_MAIN;
}

int main() {
  sig_init();

  {
    SIG_AUTOPOP_HANDLER(SIGNAL_FAIL, fail_handler, NULL);

    SIG_PROVIDE_RESTART(SIGNAL_FAIL, ({
      middle_func();
    }), SIG_RESTART_MAIN, ({
      sw_fprintf(stderr, "RESTART_MAIN\n");
      exit(1);
    }));
  }

  sig_cleanup();
  return 0;
}
```

# How it works

## The signal system

Signal handler functions are pushed onto a thread-local stack. When a signal is sent, handlers for that signal type are called, walking down the stack until one selects an approprate restart. Restarts - of which there are default and user-defined entries - describe a point to unwind to and error handling code to run when we unwind to that point ("catch" but to any stack frame).

## The unwind system

TODO

# Api reference

See the headers. Start at signals.h.
