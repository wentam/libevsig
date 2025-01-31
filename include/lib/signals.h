#pragma once

#define __GENSYM(base, counter) base##_gensym_##counter
#define _GENSYM(base, counter) __GENSYM(base, counter)
#define GENSYM(base) _GENSYM(base,__LINE__)

// TODO document how to use this for event-based programming.
//      mention how to not consume a signal, mention that you can push a signal handler that does
//      consume first to avoid errors when nobody consumes. Explain what will probably be something
//      like SIG_PERSISTANT_HANDLER that doesn't go away in a stack-like fashion.
//
//      Document how the user could mask the default catchall SIGNAL_ALL handler by defining their
//      own.
//
// TODO document try/catch style usage w/ example
// TODO document normal non-unwind usage w/ example
// TODO document default restarts
// TODO document how to define custom restarts
// TODO document best practices, such as always asserting signals have a handler unless it's
//      basically always OK to error and exit.
// TODO document how setting RESTART_NULL doesn't consume, but anything else consumes the signal
// TODO unit tests

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

// Restart type definitions
//
// Works the same way as signal definitions above
SIG_DECLTYPE(SIG_RESTART_CONTINUE);
SIG_DECLTYPE(SIG_RESTART_RETURN);
SIG_DECLTYPE(SIG_RESTART_EXIT);
SIG_DECLTYPE(SIG_RESTART_UNWIND);
SIG_DECLTYPE(SIG_RESTART_NULL);

// Call PER THREAD to init the signal system and unwind system. Else undefined behavior
void sig_init();

// Call PER THREAD when done using the signal system to clean up memory
void sig_cleanup();

typedef void (*sig_cleanup_func)(void* thing);

// A restart is a signal handler's instructions on how to continue after handling a signal.
typedef struct {
  const char*      restart_type;
  void*            restart_data;
  sig_cleanup_func restart_data_cleanup;
} sig_restart;

// Signal handler function
typedef sig_restart (*sig_handler)(const char* sig_type,
                                   void* userdata,
                                   char* msg,
                                   void* signal_data);

// Implementation details
#include "_signals.h"

// Send a signal
#define SIG_SEND(sig_type, msg, signal_data, signal_data_cleanup_func, your_restarts) \
  _SIG_SEND(sig_type, msg, signal_data, signal_data_cleanup_func, your_restarts,GENSYM(sigsend)); \

// To add your own restarts via your_restarts in SIG_SEND
#define SIG_PROVIDE_RESTART(type, action) \
  if (restart.restart_type == type) { action; }

// Define a signal handler. Handler is removed at end of scope.
#define SIG_AUTOPOP_HANDLER(sig_type, handler, userdata) \
  sig_autopop uint64_t GENSYM(sighandler) = _sig_push_handler(sig_type, handler, userdata);

// Define a signal handler. Returns an id. Handler sticks around until removed via SIG_RM_HANDLER
#define SIG_PERSISTENT_HANDLER(sig_type, handler, userdata) \
  _sig_push_handler(sig_type, handler, userdata);

// Remove a signal handler by id returned from SIG_PERSISTENT_HANDLER
#define SIG_RM_HANDLER(id) \
  _sig_rm_handler(id)

// Classic TRY_CATCH convenience macro
#define TRY_CATCH(try_code, sig_type, catch_code) \
    _TRY_CATCH(try_code, sig_type, catch_code, GENSYM(trycatch));

// Assert that a handler exists for a signal, useful to ensure important signals are handled
#define SIG_ASSERT_HANDLER(sig_type) _sig_assert_handler(sig_type)
