#pragma once

#define __GENSYM(base, counter) base##_gensym_##counter
#define _GENSYM(base, counter) __GENSYM(base, counter)


// NOTE ON THE USAGE OF __COUNTER__
//
// Using __COUNTER__ technically means we're breaking the ODR (one-definition rule)
// when our macros are used inside inline header functions. This is because
// two different users of the header file may get different source code
// for that line.
//
// However, this is mostly a theoretical concern, because in pratice they will
// compile to byte-for-byte the same function as long as we limit this to just
// variable names.

#define GENSYM(base) _GENSYM(base, __COUNTER__)

// TODO document how to use this for event-based programming.
//      mention how to not consume a signal, mention that you can push a signal handler that does
//      consume first to avoid errors when nobody consumes. Explain what will probably be something
//      like SIG_PERSISTANT_HANDLER that doesn't go away in a stack-like fashion.
//
//      Document how the user could mask the default catchall SIGNAL_ALL handler by defining their
//      own.
//
// TODO document how to define custom restarts
// TODO document best practices, such as always asserting signals have a handler unless it's
//      basically always OK to error and exit.
// TODO document how setting RESTART_NULL doesn't consume, but anything else consumes the signal
// TODO unit tests
// TODO typedef sig_type and restart_type to const char*, use that everywhere?
// TODO better names for SIG_AUTOPOP_HANDLER and SIG_PERSISTENT_HANDLER?
// TODO some way to provide multiple restarts for a piece of code without needing to nest
//      SIG_PROVIDE_RESTART inside eachother? Variable length args?

// Signal type definitions
//
// To define a custom signal in your project, use SIG_DECLTYPE in your header
// and SIG_DEFTYPE in your .c
//
// Be sure to prefix your signal name with a project-specific namespace like you
// would anything else.
//
// This arrangment allows for us to textually namespace signals between projects
// rather than deal with enum/macro assigned numbers colliding.
//
// It also allows us to assign runtime names to signal types.

#define SIG_DECLTYPE(name) extern const char name[];
#define SIG_DEFTYPE(name) const char name[] = #name;

SIG_DECLTYPE(SIGNAL_NOTHING);
SIG_DECLTYPE(SIGNAL_ALL);
SIG_DECLTYPE(SIGNAL_SUCCESS);
SIG_DECLTYPE(SIGNAL_FAIL);
SIG_DECLTYPE(SIGNAL_UNKNOWN_ERROR);
SIG_DECLTYPE(SIGNAL_EOF);
SIG_DECLTYPE(SIGNAL_INVALID_INPUT);
SIG_DECLTYPE(SIGNAL_UNSUPPORTED_RESTART);
SIG_DECLTYPE(SIGNAL_ALLOC_FAILED);
SIG_DECLTYPE(SIGNAL_NO_SIG_HANDLER);
SIG_DECLTYPE(SIGNAL_READ_ERROR);
SIG_DECLTYPE(SIGNAL_WRITE_ERROR);
SIG_DECLTYPE(SIGNAL_PERMISSION_DENIED);
SIG_DECLTYPE(SIGNAL_FILE_EXISTS);
SIG_DECLTYPE(SIGNAL_IO_ERROR);
SIG_DECLTYPE(SIGNAL_IS_DIRECTORY);
SIG_DECLTYPE(SIGNAL_TOO_MANY_OPEN_FILES);
SIG_DECLTYPE(SIGNAL_NO_SUCH_FILE_OR_DIR);
SIG_DECLTYPE(SIGNAL_NOT_ENOUGH_MEMORY);
SIG_DECLTYPE(SIGNAL_NO_SUCH_DEVICE);
SIG_DECLTYPE(SIGNAL_READ_ONLY_FS);
SIG_DECLTYPE(SIGNAL_BAD_FILE_DESCRIPTOR);
SIG_DECLTYPE(SIGNAL_INTERRUPTED);
SIG_DECLTYPE(SIGNAL_NOT_ENOUGH_SPACE);
SIG_DECLTYPE(SIGNAL_WOULD_BLOCK);
SIG_DECLTYPE(SIGNAL_FILE_TOO_BIG);
SIG_DECLTYPE(SIGNAL_BUSY);
SIG_DECLTYPE(SIGNAL_TOO_MANY_SYMLINKS);
SIG_DECLTYPE(SIGNAL_IS_NOT_DIRECTORY);
SIG_DECLTYPE(SIGNAL_UNSUPPORTED_OP);
SIG_DECLTYPE(SIGNAL_NOT_SEEKABLE);
SIG_DECLTYPE(SIGNAL_CORRUPT_DATA);
SIG_DECLTYPE(SIGNAL_UNSUPPORTED);
SIG_DECLTYPE(SIGNAL_INSUFFICIENT_RESOURCES);
SIG_DECLTYPE(SIGNAL_SOCK_ALREADY_CONNECTED);
SIG_DECLTYPE(SIGNAL_NOT_SOCKET);
SIG_DECLTYPE(SIGNAL_ADDRESS_IN_USE);
SIG_DECLTYPE(SIGNAL_ADDRESS_NOT_AVAILABLE);
SIG_DECLTYPE(SIGNAL_IN_PROGRESS);
SIG_DECLTYPE(SIGNAL_LOOP);
SIG_DECLTYPE(SIGNAL_NAME_TOO_LONG);
SIG_DECLTYPE(SIGNAL_TOO_MANY_LOCKS);
SIG_DECLTYPE(SIGNAL_OVERFLOW);
SIG_DECLTYPE(SIGNAL_NOT_FOUND);
SIG_DECLTYPE(SIGNAL_WOULD_DEADLOCK);
SIG_DECLTYPE(SIGNAL_CONNECTION_REFUSED);
SIG_DECLTYPE(SIGNAL_NETWORK_UNREACHABLE);
SIG_DECLTYPE(SIGNAL_CONNECTION_TIMED_OUT);
SIG_DECLTYPE(SIGNAL_CONNECTION_RESET);
SIG_DECLTYPE(SIGNAL_HOST_UNREACHABLE);
SIG_DECLTYPE(SIGNAL_INTERFACE_DOWN);

// Restart type definitions
//
// Works the same way as signal definitions above
SIG_DECLTYPE(SIG_RESTART_NULL);

// Call PER THREAD to init the signal system and unwind system. Else undefined behavior
void sig_init();

// Call PER THREAD when done using the signal system to clean up memory
void sig_cleanup();

typedef void (*sig_cleanup_func)(void* thing);

// Signal handler function
typedef const char* (*sig_handler)(const char* sig_type,
                                   void* userdata,
                                   char* msg,
                                   void* signal_data);

// Implementation details
#include "_signals.h"
#include "unwind.h"

// Convenience handler that selects whatever restart it's passed as userdata
const char* sig_static_handler(const char* sig_type, void* userdata, char* msg, void* signal_data);

// Send a signal
#define SIG_SEND(sig_type, msg, signal_data, signal_data_cleanup_func) \
  _SIG_SEND(sig_type, msg, signal_data, signal_data_cleanup_func, GENSYM(sigsend));

#define SIG_PROVIDE_AUTOPOP_RESTART(sig_type, restart_type, restart_action) \
  _SIG_PROVIDE_AUTOPOP_RESTART(sig_type, restart_type, { restart_action; }, GENSYM(sigaprestart), GENSYM(sigaprestartb))

#define SIG_PROVIDE_RESTART(sig_type, might_signal_code, restart_type, restart_action) \
  _SIG_PROVIDE_RESTART(sig_type, { might_signal_code; }, restart_type, { restart_action; }, GENSYM(sigprestart), GENSYM(sigprestartb))

// Define a signal handler. Handler is removed at end of scope.
//
// If the handler function is NULL, this is a safe no-op.
#define SIG_AUTOPOP_HANDLER(sig_type, handler, userdata) \
  _SIG_AUTOPOP_HANDLER(sig_type, handler, userdata, GENSYM(sighandler))

// Define a signal handler. Returns an id. Handler sticks around until removed via SIG_RM_HANDLER
#define SIG_PERSISTENT_HANDLER(sig_type, handler, userdata) \
  _sig_push_handler(sig_type, handler, userdata)

// Remove a signal handler by id returned from SIG_PERSISTENT_HANDLER
#define SIG_RM_HANDLER(id) \
  _sig_rm_handler(id)

// Assert that a handler exists for a signal, useful to ensure important signals are handled
#define SIG_ASSERT_HANDLER(sig_type) _sig_assert_handler(sig_type)

#define SIG_RESTART_AVAILABLE(sig_type, restart_type) \
  _sig_restart_available(sig_type, restart_type)
