#pragma once

#include "../common/include.h"
#include "../common/log.h"
#include "../common/macro.h"
#include "../common/type.h"
#include "../struct/string/fn.h"

#ifdef DEBUG
[[clang::noinline]]
#endif
core t_any McoreFNlog__debug_msg(t_thrd_data* const thrd_data, const t_any* args) {
     const char* const owner = "function - core.log__debug_msg";

     t_any level = args[0];
     t_any msg   = args[1];

     if (
          level.bytes[15] != tid__int                                           ||
          !(msg.bytes[15] == tid__short_string || msg.bytes[15] == tid__string) ||
          !(level.qwords[0] >= log_lvl__minimum && level.qwords[0] <= log_lvl__maximum)
     ) [[clang::unlikely]] {
          if (level.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, msg);
               return level;
          }
          ref_cnt__dec(thrd_data, level);

          if (msg.bytes[15] == tid__error) return msg;
          ref_cnt__dec(thrd_data, msg);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (level.bytes[15] != tid__int || !(msg.bytes[15] == tid__short_string || msg.bytes[15] == tid__string))
               result = error__invalid_type(thrd_data, owner);
          else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

#ifdef DEBUG
     t_any result;
     if (level.qwords[0] < log_level)
          result.bytes[15] = tid__null;
     else {
          FILE* const out = level.qwords[0] < log_lvl__require_attention ? stdout : stderr;
          call_stack__push(thrd_data, owner);
          if (fwrite("[DEBUG][LOG]: ", 1, 7, out) == 7) [[clang::likely]] {
               result = string__print(thrd_data, msg, owner, out);
               if (result.bytes[15] == tid__null && fwrite("\n", 1, 1, out) != 1)
                    result = error__cant_print(thrd_data, owner);
          } else result = error__cant_print(thrd_data, owner);
          call_stack__pop(thrd_data);
     }

     ref_cnt__dec(thrd_data, msg);
     return result;
#else
     ref_cnt__dec(thrd_data, level);
     ref_cnt__dec(thrd_data, msg);
     return null;
#endif
}
