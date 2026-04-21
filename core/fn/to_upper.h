#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/char/basic.h"
#include "../struct/char/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

core t_any McoreFNto_upper(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.to_upper";

     t_any arg = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string:
          return short_string__to_upper(arg);
     case tid__char:
          return char__to_upper(arg);
     case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_string__to_upper__own(thrd_data, arg, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any result = obj__call__any_result__own(thrd_data, mtd__core_to_upper, arg, args, 1, owner);
          if (!type_is_common_or_error(result.bytes[15])) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any result = custom__call__any_result__own(thrd_data, mtd__core_to_upper, arg, args, 1, owner);
          if (!type_is_common_or_error(result.bytes[15])) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]]
     case tid__error:
          return arg;
     [[clang::unlikely]]
     default: {
          ref_cnt__dec(thrd_data, arg);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}