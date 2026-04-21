#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNpower(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.power";

     t_any const num   = args[0];
     t_any const power = args[1];
     if (num.bytes[15] == tid__error || power.bytes[15] == tid__error || num.bytes[15] != power.bytes[15]) [[clang::unlikely]] {
          if (num.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, power);
               return num;
          }

          if (power.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, num);
               return power;
          }

          goto invalid_type_label;
     }

     switch (num.bytes[15]) {
     case tid__byte: {
          u8 result                   = 1;
          u8 num_pow_of_2_pow_of_step = num.bytes[0];
          u8 step_pow                 = power.bytes[0];
          do {
               if ((step_pow & 1) == 1) result *= num_pow_of_2_pow_of_step;

               step_pow                >>= 1;
               num_pow_of_2_pow_of_step *= num_pow_of_2_pow_of_step;
          } while (step_pow != 0);

          return (const t_any){.bytes = {result, [15] = tid__byte}};
     }
     case tid__int: {
          u64 result   = 0;
          u64 step_pow = power.qwords[0];
          if ((i64)step_pow >= 0) {
               result                       = 1;
               u64 num_pow_of_2_pow_of_step = num.qwords[0];
               do {
                    if ((step_pow & 1) == 1) result *= num_pow_of_2_pow_of_step;

                    step_pow                >>= 1;
                    num_pow_of_2_pow_of_step *= num_pow_of_2_pow_of_step;
               } while (step_pow != 0);
          }

          return (const t_any){.structure = {.value = result, .type = tid__int}};
     }
     case tid__float:
          return (const t_any){.structure = {.value = cast_double_to_u64(__builtin_pow(num.floats[0], power.floats[0])), .type = tid__float}};
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_power, tid__obj, num, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_power, tid__custom, num, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, num);
     ref_cnt__dec(thrd_data, power);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}