#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNlog10(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.log10";

     t_any const arg = args[0];
     switch (arg.bytes[15]) {
     case tid__float:
          return (const t_any){.structure = {.value = cast_double_to_u64(__builtin_log10(arg.floats[0])), .type = tid__float}};
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_log10, tid__obj, arg, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_log10, tid__custom, arg, args, 1, owner);
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