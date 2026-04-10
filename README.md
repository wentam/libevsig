Libevsig is an exception system, condition system, event-based programming framework, unwind mechanism, and resource cleanup mechanism for C.

While flexible, the principle purpose is error handling.

To understand the value of this Common Lisp condition style design over simple exceptions, see [here](https://youtu.be/4NO83wZVT0A?t=2832).

*NOTE: this project is experimental, do not expect a stable API or ABI*

NOTES:
* We comandeer all signals, such a SIGINT. A future update may provide more flexibility on this
front.
* Use -rdynamic at link time to get function names to work in backtraces.
* Your threads must be cancellable with pthread_cancel(). If you're having trouble with this,
you probably need to add pthread_testcancel() at good places to stop.
* If there's a bad place to stop your thread, temporarily disable cancelability
* If you have a mutex, you probably want to unlock it with UNWIND_ACTION in case the stack unwinds
in order to prevent a stale lock.

# Example

The functions main, middle, and top all have provided restarts. Depending on what fail_handler
returns, we can return to any of the those places in the stack providing a custom recovery for
each.

Also observe how if fail_handler returns SIG_RESTART_MAIN, we pretend to free a resource due
to UNWIND_ACTION even though the stack unwinds past it.

```C
#include <stdio.h>
#include <libevsig/signals.h>
#include <libevsig/sigwrap.h>
#include <libevsig/unwind.h>
#include "stdlib.h"

SIG_DEFTYPE(SIG_RESTART_MAIN);
SIG_DEFTYPE(SIG_RESTART_MIDDLE);
SIG_DEFTYPE(SIG_RESTART_TOP);

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

  SIG_AUTOPOP_RESTART(SIGNAL_FAIL, SIG_RESTART_MIDDLE, ({
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
    //
    // 'AUTOPOP' tells us that this restart will be removed once it falls
    // out of scope.
    SIG_AUTOPOP_RESTART(SIGNAL_FAIL, SIG_RESTART_MAIN, ({
      sw_fprintf(stderr, "RESTART_MAIN\n");
      exit(1);
    }));

    middle_func();
  }

  sig_cleanup();
  return 0;
}
```

# A note on local state and restarts

When defining a restart that needs to access state that may have
been mutated after the restart, the mutated state should be marked
as volatile.

This is because setjmp saves local registers. Returning to the restart
restores these registers.

Without 'volatile', the following would never increase the 'slep'
counter on subsequent restarts:

```C
  const u64 slep_start = 10000;
  const u64 slep_max   = 5000000;
  volatile u64 slep    = slep_start;

  SIG_AUTOPOP_HANDLER(SIGNAL_ALL, c->handle_tcp_errors, NULL);
  SIG_AUTOPOP_RESTART(SIGNAL_ALL, SIG_RESTART_TCP_CLIENT_RECONNECT, ({
    usleep(slep);
    slep *= 2;
    if (slep > slep_max) slep = slep_max;
  }));
```


# How it works

## The signal system

Signal handler functions are pushed onto a thread-local stack. When a signal is sent, handlers for that signal type are called, walking down the stack until one selects an approprate restart. Restarts describe a point to unwind to and code to run when we unwind to that point ("catch" but to any stack frame).

## The unwind system

TODO

# Api reference

See the headers. Start at signals.h.
