#pragma once

#include "../common/include.h"
#include "../common/log.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/basic.h"
#include "../struct/null/basic.h"

core t_any McoreFNset_log_level(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.set_log_level";

     t_any const level = args[0];
     if (level.bytes[15] != tid__int || level.qwords[0] < log_lvl__minimum || level.qwords[0] > log_lvl__maximum) [[clang::unlikely]] {
          if (level.bytes[15] == tid__error) return level;

          t_any result;
          call_stack__push(thrd_data, owner);
          if (level.bytes[15] != tid__int) {
               ref_cnt__dec(thrd_data, level);
               result = error__invalid_type(thrd_data, owner);
          } else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     log_level = level.bytes[0];
     return null;
}