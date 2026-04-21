#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/breaker/basic.h"
#include "../struct/common/fn_struct.h"

core inline t_any McoreFNloop(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.loop";

     t_any       state = args[0];
     t_any const fn    = args[1];

     if (state.bytes[15] == tid__error || !(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn)) [[clang::unlikely]] {
          if (state.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, fn);
               return state;
          }
          ref_cnt__dec(thrd_data, state);

          if (fn.bytes[15] == tid__error) return fn;
          ref_cnt__dec(thrd_data, fn);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     call_stack__push(thrd_data, owner);

     while (true) {
          state = common_fn__call__half_own(thrd_data, fn, &state, 1, owner);
          if (state.bytes[15] == tid__error) [[clang::unlikely]] {
               call_stack__pop(thrd_data);

               ref_cnt__dec(thrd_data, fn);
               return state;
          }

          if (state.bytes[15] == tid__breaker) {
               call_stack__pop(thrd_data);

               ref_cnt__dec(thrd_data, fn);
               return breaker__get_result__own(thrd_data, state);
          }
     }
}