#pragma once
#include <threads.h>
#include "lib/unwind.h"
#include "stdlib.h"

typedef struct {
  const char* sig_type;
  sig_handler handler;
  void*       handler_userdata;
  uint64_t    id;
} sig_handler_stack_entry;

extern thread_local sig_handler_stack_entry* sig_handler_stack;
extern thread_local int64_t sig_handler_stack_alloc;
extern thread_local int64_t sig_handler_stack_fill;

sig_restart _sig_send(const char* sig_type,
                      char* msg,
                      void* signal_data,
                      sig_cleanup_func signal_data_cleanup_func);

const uint64_t _sig_push_handler(const char* sig_type, sig_handler handler, void* userdata);
void           _sig_rm_handler (uint64_t id);
void           _sig_free_restart(sig_restart* s);
void           _sig_assert_handler(const char* sig_type);
void           _sig_assertwarn_handler(const char* sig_type);
void           _unwind_handler_free_restart(void* restart);

// Helper to free handler
[[maybe_unused]] static void sig_autopop_impl(uint64_t* p) { _sig_rm_handler(*p); }
#define sig_autopop __attribute__((__cleanup__(sig_autopop_impl)))

#define _SIG_SEND(sig_type, msg, signal_data, signal_data_cleanup_func, your_restarts, gensym) \
  { \
    sig_restart gensym = _sig_send(sig_type, msg, signal_data, signal_data_cleanup_func); \
    { \
      UNWIND_ACTION(_unwind_handler_free_restart, &gensym) \
      sig_restart restart = gensym; \
      if      (restart.restart_type == SIG_RESTART_CONTINUE) {} \
      SIG_PROVIDE_RESTART(SIG_RESTART_EXIT, { exit(*((uint32_t*) restart.restart_data)); }); \
      SIG_PROVIDE_RESTART(SIG_RESTART_UNWIND, { UNWIND(restart.restart_data); }); \
      your_restarts; \
    } \
  }

sig_restart try_catch_handler(const char* sig_type, void* userdata, char* msg, void* signal_data);

#define _TRY_CATCH(try_code, sig_type, catch_code, gensym) \
  UNWIND_RETURN_POINT(gensym, ({ \
    SIG_AUTOPOP_HANDLER(sig_type, try_catch_handler, &gensym); \
    try_code; \
  }), ({ \
    catch_code; \
  }))
