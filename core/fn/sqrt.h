#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/byte/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/int/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNsqrt(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.sqrt";

     t_any num = args[0];
     switch (num.bytes[15]) {
     case tid__byte:
          num.bytes[0] = byte__sqrt(num.bytes[0]);
          return num;
     case tid__int:
          if ((i64)num.qwords[0] < 0) [[clang::unlikely]] {
               call_stack__push(thrd_data, owner);
               t_any const result = error__sqrt_from_neg(thrd_data, owner);
               call_stack__pop(thrd_data);

               return result;
          }
          num.qwords[0] = int__sqrt(num.qwords[0]);
          return num;
     case tid__float:
          return (const t_any){.structure = {.value = cast_double_to_u64(__builtin_sqrt(num.floats[0])), .type = tid__float}};
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_sqrt, tid__obj, num, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_sqrt, tid__custom, num, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] case tid__error:
          return num;
     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, num);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}