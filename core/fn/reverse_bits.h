#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"

core t_any McoreFNreverse_bits(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.reverse_bits";

     t_any arg = args[0];
     switch (arg.bytes[15]) {
     case tid__byte:
          arg.bytes[0] = __builtin_bitreverse8(arg.bytes[0]);
          return arg;
     case tid__int:
          arg.qwords[0] = __builtin_bitreverse64(arg.qwords[0]);
          return arg;
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_reverse_bits, tid__obj, arg, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_reverse_bits, tid__custom, arg, args, 1, owner);
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