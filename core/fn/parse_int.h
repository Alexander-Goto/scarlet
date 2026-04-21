#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

core t_any McoreFNparse_int(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.parse_int";

     t_any const string = args[0];
     t_any const base   = args[1];
     if (!(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string) || base.bytes[15] != tid__int || base.qwords[0] < 2 || base.qwords[0] > 36) [[clang::unlikely]] {
          if (string.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, base);
               return string;
          }
          ref_cnt__dec(thrd_data, string);

          if (base.bytes[15] == tid__error)
               return base;
          ref_cnt__dec(thrd_data, base);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (!(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string) || base.bytes[15] != tid__int)
               result = error__invalid_type(thrd_data, owner);
          else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     return string.bytes[15] == tid__short_string ? short_string__parse_int(thrd_data, string, base.qwords[0]) : long_string__parse_int__own(thrd_data, string, base.qwords[0]);
}
