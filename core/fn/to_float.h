#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

core t_any McoreFNto_float(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.to_float";

     t_any const arg = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string: case tid__string:
          return string__to_float__own(thrd_data, arg);
     case tid__byte:
          return (const t_any){.structure = {.value = cast_double_to_u64(arg.bytes[0]), .type = tid__float}};
     case tid__int:
          return (const t_any){.structure = {.value = cast_double_to_u64((double)(i64)arg.qwords[0]), .type = tid__float}};
     case tid__float:
          return arg;
     case tid__obj: {
          call_stack__push(thrd_data, owner);

          t_any result = obj__call__any_result__own(thrd_data, mtd__core_to_float, arg, args, 1, owner);
          if (result.bytes[15] != tid__float && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);

          t_any result = custom__call__any_result__own(thrd_data, mtd__core_to_float, arg, args, 1, owner);
          if (result.bytes[15] != tid__float && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]]
     case tid__error:
          return arg;
     [[clang::unlikely]]
     default: {
          ref_cnt__dec(thrd_data, arg);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}