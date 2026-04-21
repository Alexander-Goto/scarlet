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
#include "../struct/token/fn.h"

core t_any McoreFNto_int(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.to_int";

     t_any arg = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string: {
          t_any const result = short_string__parse_int(thrd_data, arg, 10);
          return result.bytes[15] == tid__token ? null : result;
     }
     case tid__byte:
          return (const t_any){.structure = {.value = arg.bytes[0], .type = tid__int}};
     case tid__char:
          return (const t_any){.structure = {.value = char_to_code(arg.bytes), .type = tid__int}};
     case tid__int:
          return arg;
     case tid__time:
          arg.bytes[15] = tid__int;
          return arg;
     case tid__float:
          double const float_ = arg.floats[0];
          return !(float_ >= -9223372036854775808.0 && float_ < 9223372036854775808.0) ? null : (const t_any){.structure = {.value = (i64)__builtin_trunc(float_), .type = tid__int}};
     case tid__string: {
          bool  const is_neg = ((const u8*)slice_array__get_items(arg))[slice__get_offset(arg) * 3 + 2] == '-';
          t_any const result = long_string__parse_int__own(thrd_data, arg, 10);
          return result.bytes[15] == tid__token || (result.bytes[15] == tid__int && (i64)result.qwords[0] < 0 && !is_neg) ? null : result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);

          t_any result = obj__call__any_result__own(thrd_data, mtd__core_to_int, arg, args, 1, owner);
          if (result.bytes[15] != tid__int && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);

          t_any result = custom__call__any_result__own(thrd_data, mtd__core_to_int, arg, args, 1, owner);
          if (result.bytes[15] != tid__int && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
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
