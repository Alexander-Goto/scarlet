#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"
#include "../struct/char/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

core t_any McoreFNasciiB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.ascii?";

     t_any const arg = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string:
          return __builtin_reduce_and(arg.vector < 128) != 0 ? bool__true : bool__false;
     case tid__char:
          return char_to_code(arg.bytes) < 128 ? bool__true : bool__false;
     case tid__string:
          return long_string__is_ascii__own(thrd_data, arg);
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_is_ascii, tid__bool, arg, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_is_ascii, tid__bool, arg, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] case tid__error:
          return arg;
     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, arg);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}
