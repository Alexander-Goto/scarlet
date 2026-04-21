#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

core inline t_any McoreFNformat_int(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.format_int";

     t_any const num     = args[0];
     t_any const base    = args[1];
     t_any const signed_ = args[2];
     if (num.bytes[15] != tid__int || base.bytes[15] != tid__int || base.qwords[0] < 2 || base.qwords[0] > 36 || signed_.bytes[15] != tid__bool) [[clang::unlikely]] {
          if (num.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, base);
               ref_cnt__dec(thrd_data, signed_);
               return num;
          }
          ref_cnt__dec(thrd_data, num);

          if (base.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, signed_);
               return base;
          }
          ref_cnt__dec(thrd_data, base);

          if (signed_.bytes[15] == tid__error)
               return signed_;
          ref_cnt__dec(thrd_data, signed_);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (num.bytes[15] != tid__int || base.bytes[15] != tid__int || signed_.bytes[15] != tid__bool)
               result = error__invalid_type(thrd_data, owner);
          else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     return format_int(num.qwords[0], base.bytes[0], signed_.bytes[0] == 1);
}
