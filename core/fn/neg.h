#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNneg(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.neg";

     t_any num = args[0];
     switch (num.bytes[15]) {
     case tid__int:
          num.qwords[0] = -num.qwords[0];
          return num;
     case tid__float:
          num.floats[0] = -num.floats[0];
          return num;
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_neg, tid__obj, num, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_neg, tid__custom, num, args, 1, owner);
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