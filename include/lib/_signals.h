#pragma once
#include <threads.h>
#include "unwind.h"
#include "stdlib.h"

typedef struct {
  const char* sig_type;
  sig_handler handler;
  void*       handler_userdata;
  uint64_t    id;
} sig_handler_stack_entry;

typedef struct {
  const char* sig_type;
  const char* restart_type;
  unwind_return_point* p;
  uint64_t id;
} sig_restart_stack_entry;

extern thread_local sig_handler_stack_entry* sig_handler_stack;
extern thread_local int64_t sig_handler_stack_alloc;
extern thread_local int64_t sig_handler_stack_fill;

extern thread_local sig_restart_stack_entry* sig_restart_stack;
extern thread_local int64_t sig_restart_stack_alloc;
extern thread_local int64_t sig_restart_stack_fill;

void _sig_send(const char* sig_type,
               char* msg,
               void* signal_data,
               sig_cleanup_func signal_data_cleanup_func);

const uint64_t _sig_push_handler(const char* sig_type, sig_handler handler, void* userdata);
void           _sig_rm_handler (uint64_t id);
void           _sig_assert_handler(const char* sig_type);
void           _sig_assertwarn_handler(const char* sig_type);
uint64_t       _sig_push_restart(const char* sig_type, const char* restart_type, unwind_return_point* p);
void           _sig_rm_restart(uint64_t id);
void           _unwind_handler_sig_rm_handler(void* id);
void           _unwind_handler_sig_rm_restart(void* id);

#define _SIG_SEND(sig_type, msg, signal_data, signal_data_cleanup_func, gensym) \
  _sig_send(sig_type, msg, signal_data, signal_data_cleanup_func);

#define _SIG_AUTOPOP_HANDLER(sig_type, handler, userdata, gensym) \
  uint64_t gensym = _sig_push_handler(sig_type, handler, userdata); \
  UNWIND_ACTION(_unwind_handler_sig_rm_handler, &gensym);


#define _SIG_PROVIDE_RESTART(sig_type, might_signal_code, restart_type, restart_action, gensym, gensymb) \
  uint64_t gensymb; \
  UNWIND_RETURN_POINT(gensym, { \
    gensymb = _sig_push_restart(sig_type, restart_type, &gensym); \
    UNWIND_ACTION(_unwind_handler_sig_rm_restart, &gensymb); \
    might_signal_code; \
  }, restart_action); \
