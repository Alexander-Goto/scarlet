#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/fn.h"

core t_any McoreFNint_to_time(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.int_to_time";

     t_any sec = args[0];
     if (sec.bytes[15] != tid__int || sec.qwords[0] >= 9223372036825516800ull) [[clang::unlikely]] {
          if (sec.bytes[15] == tid__error) return sec;

          t_any result;
          call_stack__push(thrd_data, owner);
          if (sec.bytes[15] != tid__int) {
               ref_cnt__dec(thrd_data, sec);
               result = error__invalid_type(thrd_data, owner);
          } else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     sec.bytes[15] = tid__time;
     return sec;
}
