#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/breaker/basic.h"
#include "../struct/error/fn.h"

core inline t_any McoreFNbreak(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.break";

     t_any const predicate = args[0];
     t_any const result    = args[1];
     if (predicate.bytes[15] != tid__bool || result.bytes[15] == tid__error) [[clang::unlikely]] {
          if (predicate.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, result);
               return predicate;
          }
          ref_cnt__dec(thrd_data, predicate);

          if (result.bytes[15] == tid__error) return result;
          ref_cnt__dec(thrd_data, result);

          call_stack__push(thrd_data, owner);
          t_any const error = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return error;
     }

     if (predicate.bytes[0] == 0) return result;

     call_stack__push(thrd_data, owner);
     t_any const breaker = breaker__new(thrd_data, result, owner);
     call_stack__pop(thrd_data);

     return breaker;
}
