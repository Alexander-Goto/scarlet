#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"

core t_any McoreFNqty_0_bits(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.qty_0_bits";

     t_any const arg = args[0];
     switch (arg.bytes[15]) {
     case tid__byte:
          return (const t_any){.structure = {.value = __builtin_popcount((u8)~arg.bytes[0]), .type = tid__int}};
     case tid__int:
          return (const t_any){.structure = {.value = __builtin_popcountll((u64)~arg.qwords[0]), .type = tid__int}};
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_qty_0_bits, tid__int, arg, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_qty_0_bits, tid__int, arg, args, 1, owner);
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