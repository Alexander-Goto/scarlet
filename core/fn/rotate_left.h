#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"

core t_any McoreFNrotate_left(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.rotate_left";

     t_any const arg    = args[0];
     t_any const shifts = args[1];
     if (arg.bytes[15] == tid__error || !(shifts.bytes[15] == tid__int || shifts.bytes[15] == tid__byte)) [[clang::unlikely]] {
          if (arg.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, shifts);
               return arg;
          }

          if (shifts.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, arg);
               return shifts;
          }

          goto invalid_type_label;
     }

     switch (arg.bytes[15]) {
     case tid__byte:
          if (shifts.qwords[0] > 8) [[clang::unlikely]] goto bits_shift_label;
          return (const t_any){.structure = {.value = __builtin_rotateleft8(arg.bytes[0], shifts.bytes[0]), .type = tid__byte}};
     case tid__int:
          if (shifts.qwords[0] > 64) [[clang::unlikely]] goto bits_shift_label;
          return (const t_any){.structure = {.value = __builtin_rotateleft64(arg.qwords[0], shifts.qwords[0]), .type = tid__int}};
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_rotate_left, tid__obj, arg, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_rotate_left, tid__custom, arg, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label: {
          ref_cnt__dec(thrd_data, arg);
          ref_cnt__dec(thrd_data, shifts);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     bits_shift_label:
     call_stack__push(thrd_data, owner);
     t_any const result = error__bits_shift(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}