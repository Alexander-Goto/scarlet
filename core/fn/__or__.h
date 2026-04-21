#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"

core t_any McoreFN__or__(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.(|)";

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
          return (const t_any){.bytes = {left.bytes[0] | right.bytes[0], [15] = tid__byte}};
     case tid__int:
          return (const t_any){.structure = {.value = left.qwords[0] | right.qwords[0], .type = tid__int}};
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core___or__, tid__obj, left, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core___or__, tid__custom, left, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, left);
     ref_cnt__dec(thrd_data, right);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
