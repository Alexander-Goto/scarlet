#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"

[[clang::always_inline]]
core t_any McoreFNnot(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.not";

     t_any arg = args[0];
     if (arg.bytes[15] != tid__bool) [[clang::unlikely]] {
          if (arg.bytes[15] == tid__error) return arg;

          ref_cnt__dec(thrd_data, arg);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     arg.bytes[0] ^= 1;
     return arg;
}