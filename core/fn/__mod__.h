#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"

core t_any McoreFN__mod__(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.(%)";

     t_any const left  = args[0];
     t_any const right = args[1];
     if (left.bytes[15] == tid__error || right.bytes[15] == tid__error || left.bytes[15] != right.bytes[15]) [[clang::unlikely]] {
          if (left.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, right);
               return left;
          }

          if (right.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, left);
               return right;
          }

          goto invalid_type_label;
     }

     switch (left.bytes[15]) {
     case tid__byte:
          if (right.bytes[0] == 0) [[clang::unlikely]] goto div_by_0_label;
          return (const t_any){.bytes = {left.bytes[0] % right.bytes[0], [15] = tid__byte}};
     case tid__int: {
          i64 const left_i64  = left.qwords[0];
          i64 const right_i64 = right.qwords[0];
          if (right_i64 == 0 || (left_i64 == -9223372036854775808wb && right_i64 == -1)) [[clang::unlikely]] {
               if (right_i64 == 0) [[clang::unlikely]] goto div_by_0_label;

               return (const t_any){.bytes = {[15] = tid__int}};
          }
          return (const t_any){.structure = {.value = left_i64 % right_i64, .type = tid__int}};
     }
     case tid__float:
          return (const t_any){.structure = {.value = cast_double_to_u64(__builtin_fmod(left.floats[0], right.floats[0])), .type = tid__float}};
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core___mod__, tid__obj, left, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core___mod__, tid__custom, left, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label: {
          ref_cnt__dec(thrd_data, left);
          ref_cnt__dec(thrd_data, right);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     div_by_0_label:
     call_stack__push(thrd_data, owner);
     t_any const result = error__div_by_0(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
