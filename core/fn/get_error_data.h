#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/fn.h"

[[clang::noinline]]
core_error t_any McoreFNget_error_data(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.get_error_data";

     t_any const error = args[0];
     if (error.bytes[15] != tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, error);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     const t_error* const error_data = (const t_error*)error.qwords[0];
     t_any          const data       = error_data->data;

     call_stack__push(thrd_data, owner);
     ref_cnt__inc(thrd_data, data, owner);
     call_stack__pop(thrd_data);

     ref_cnt__dec(thrd_data, error);
     return data;
}
