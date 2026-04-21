#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/string/basic.h"

core inline t_any McoreFNcall_stack(t_thrd_data* const thrd_data, const t_any* const args) {
#ifndef CALL_STACK
     return null;
#else
     const char* const owner = "function - core.call_stack";

     u64 const stack_len = thrd_data->call_stack.len;

     call_stack__push(thrd_data, owner);

     t_any call_stack = array__init(thrd_data, stack_len, owner);
     for (
          u64 idx = 0;
          idx < stack_len;

          call_stack = array__append__own(thrd_data, call_stack, string_from_ascii(thrd_data->call_stack.stack[idx]), owner),
          idx       += 1
     );

     call_stack__pop(thrd_data);

     return call_stack;
#endif
}
