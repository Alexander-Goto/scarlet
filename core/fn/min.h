#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/char/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/obj/mtd.h"
#include "../struct/string/fn.h"
#include "../struct/token/basic.h"

core t_any McoreFNmin(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.min";

     t_any const arg_1      = args[0];
     t_any const arg_2      = args[1];
     u8    const arg_1_type = arg_1.bytes[15];
     u8    const arg_2_type = arg_2.bytes[15];
     if (
          arg_1_type == tid__error || arg_2_type == tid__error ||
          !(
               arg_1_type == arg_2_type                                                 ||
               (arg_1_type == tid__short_string     && arg_2_type == tid__string)       ||
               (arg_1_type == tid__string           && arg_2_type == tid__short_string) ||
               (arg_1_type == tid__short_byte_array && arg_2_type  == tid__byte_array)  ||
               (arg_1_type == tid__byte_array       && arg_2_type  == tid__short_byte_array)
          )
     ) [[clang::unlikely]] {
          if (arg_1_type == tid__error) {
               ref_cnt__dec(thrd_data, arg_2);
               return arg_1;
          }

          if (arg_2_type == tid__error) {
               ref_cnt__dec(thrd_data, arg_1);
               return arg_2;
          }

          goto invalid_type_label;
     }

     switch (arg_1_type) {
     case tid__short_string:
          switch (arg_2_type) {
          case tid__short_string:
               return cmp__short_string__short_string(arg_1, arg_2).bytes[0] == great_id ? arg_2 : arg_1;
          case tid__string:
               if (cmp__short_string__long_string(arg_1, arg_2).bytes[0] == great_id) return arg_2;
               ref_cnt__dec(thrd_data, arg_2);
               return arg_1;
          default:
               unreachable;
          }
     case tid__string:
          switch (arg_2_type) {
          case tid__short_string:
               if (cmp__long_string__short_string(arg_1, arg_2).bytes[0] == great_id) {
                    ref_cnt__dec(thrd_data, arg_1);
                    return arg_2;
               }

               return arg_1;
          case tid__string:
               if (cmp__long_string__long_string(arg_1, arg_2).bytes[0] == great_id) {
                    ref_cnt__dec(thrd_data, arg_1);
                    return arg_2;
               }
               ref_cnt__dec(thrd_data, arg_2);
               return arg_1;
          default:
               unreachable;
          }
     case tid__token: {
          [[gnu::aligned(32)]]
          char chars_1[32] = {};
          [[gnu::aligned(32)]]
          char chars_2[32] = {};

          u8 unneeded_len;

          token_to_ascii_chars(arg_1, &unneeded_len, chars_1);
          token_to_ascii_chars(arg_2, &unneeded_len, chars_2);

          return scar__memcmp(chars_1, chars_2, 32) == -1 ? arg_1 : arg_2;
     }
     case tid__byte:
          return arg_1.bytes[0] > arg_2.bytes[0] ? arg_2 : arg_1;
     case tid__char:
          return char_to_code(arg_1.bytes) > char_to_code(arg_2.bytes) ? arg_2 : arg_1;
     case tid__int: case tid__time:
          return (i64)arg_1.qwords[0] > (i64)arg_2.qwords[0] ? arg_2 : arg_1;
     case tid__float: {
          double const float_1 = arg_1.floats[0];
          double const float_2 = arg_2.floats[0];
          if (__builtin_isnan(float_1) || __builtin_isnan(float_2)) [[clang::unlikely]] goto nan_cmp_label;

          return float_1 > float_2 ? arg_2 : arg_1;
     }
     case tid__short_byte_array: case tid__byte_array:
          if (byte_array__cmp(arg_1, arg_2).bytes[0] == great_id) {
               ref_cnt__dec(thrd_data, arg_1);
               return arg_2;
          }

          ref_cnt__dec(thrd_data, arg_2);
          return arg_1;
     case tid__obj: {
          t_any cmp_result;
          call_stack__push(thrd_data, owner);
          [[clang::noinline]] cmp_result = obj__cmp(thrd_data, arg_1, arg_2, owner);
          call_stack__pop(thrd_data);

          switch (cmp_result.bytes[0]) {
          case great_id:
               ref_cnt__dec(thrd_data, arg_1);
               return arg_2;
          case less_id: case equal_id:
               ref_cnt__dec(thrd_data, arg_2);
               return arg_1;

          [[clang::unlikely]]
          case not_equal_id:
               goto invalid_type_label;
          [[clang::unlikely]]
          case nan_cmp_id:
               ref_cnt__dec(thrd_data, arg_1);
               ref_cnt__dec(thrd_data, arg_2);
               goto nan_cmp_label;
          default:
               unreachable;
          }
     }
     case tid__array: {
          t_any cmp_result;
          call_stack__push(thrd_data, owner);
          [[clang::noinline]] cmp_result = array__cmp(thrd_data, arg_1, arg_2, owner);
          call_stack__pop(thrd_data);

          switch (cmp_result.bytes[0]) {
          case great_id:
               ref_cnt__dec(thrd_data, arg_1);
               return arg_2;
          case less_id: case equal_id:
               ref_cnt__dec(thrd_data, arg_2);
               return arg_1;

          [[clang::unlikely]]
          case not_equal_id:
               goto invalid_type_label;
          [[clang::unlikely]]
          case nan_cmp_id:
               ref_cnt__dec(thrd_data, arg_1);
               ref_cnt__dec(thrd_data, arg_2);
               goto nan_cmp_label;
          default:
               unreachable;
          }
     }
     case tid__custom: {
          ref_cnt__inc(thrd_data, arg_1, owner);
          ref_cnt__inc(thrd_data, arg_2, owner);

          bool called;
          call_stack__push(thrd_data, owner);
          t_any const cmp_result = custom__try_call__any_result__own(thrd_data, mtd__core_cmp, arg_1, args, 2, owner, &called);
          call_stack__pop(thrd_data);
          if (!called) [[clang::unlikely]] goto invalid_type_label;

          switch (cmp_result.raw_bits) {
          case comptime_tkn__great:
               ref_cnt__dec(thrd_data, arg_1);
               return arg_2;
          case comptime_tkn__less: case comptime_tkn__equal:
               ref_cnt__dec(thrd_data, arg_2);
               return arg_1;

          [[clang::unlikely]]
          case comptime_tkn__not_equal:
               goto invalid_type_label;
          [[clang::unlikely]]
          case comptime_tkn__nan_cmp:
               ref_cnt__dec(thrd_data, arg_1);
               ref_cnt__dec(thrd_data, arg_2);
               goto nan_cmp_label;
          [[clang::unlikely]]
          default:
               call_stack__push(thrd_data, owner);
               fail_with_call_stack(thrd_data, "The 'core.cmp' function returned a value other than ':equal', ':not_equal', ':less', ':great', ':nan_cmp'.", owner);
          }
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label: {
          ref_cnt__dec(thrd_data, arg_1);
          ref_cnt__dec(thrd_data, arg_2);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     nan_cmp_label:
     call_stack__push(thrd_data, owner);
     t_any const result = error__nan_cmp(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
