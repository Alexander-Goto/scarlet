#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNsqr(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.sqr";

     t_any const num = args[0];
     switch (num.bytes[15]) {
     case tid__byte:
          return (const t_any){.bytes = {num.bytes[0] * num.bytes[0], [15] = tid__byte}};
     case tid__int:
          return (const t_any){.structure = {.value = num.qwords[0] * num.qwords[0], .type = tid__int}};
     case tid__float: {
          double const float_num = num.floats[0];
          return (const t_any){.structure = {.value = cast_double_to_u64(float_num * float_num), .type = tid__float}};
     }
     case tid__obj: {
          ref_cnt__inc(thrd_data, num, owner);
          t_any const mul_args[2] = {num, num};

          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core___mul__, tid__obj, num, mul_args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          ref_cnt__inc(thrd_data, num, owner);
          t_any const mul_args[2] = {num, num};

          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core___mul__, tid__custom, num, mul_args, 2, owner);
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
