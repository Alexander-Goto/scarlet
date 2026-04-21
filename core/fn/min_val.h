#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNmin_val(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.min_val";

     t_any arg  = args[0];
     switch (arg.bytes[15]) {
     case tid__token:
          return (const t_any){.raw_bits = 0x9a88ab0e5b1fa9c81ceba7fd0907b13uwb};
     case tid__byte:
          arg.bytes[0] = 0;
          return arg;
     case tid__int:
          arg.qwords[0] = -9223372036854775808wb;
          return arg;
     case tid__time:
          arg.qwords[0] = 0;
          return arg;
     case tid__float:
          arg.floats[0] = -__builtin_inf();
          return arg;
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_min_val, tid__obj, arg, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_min_val, tid__custom, arg, args, 1, owner);
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