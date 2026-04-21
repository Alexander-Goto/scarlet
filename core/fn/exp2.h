#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNexp2(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.exp2";

     t_any power = args[0];
     switch (power.bytes[15]) {
     case tid__byte:
          power.bytes[0] = power.bytes[0] < 8 ? 1 << power.bytes[0] : 0;
          return power;
     case tid__int:
          power.qwords[0] = power.qwords[0] < 64 ? (u64)1 << power.qwords[0] : 0;
          return power;
     case tid__float:
          return (const t_any){.structure = {.value = cast_double_to_u64(__builtin_exp2(power.floats[0])), .type = tid__float}};
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_exp2, tid__obj, power, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_exp2, tid__custom, power, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] case tid__error:
          return power;
     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, power);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}