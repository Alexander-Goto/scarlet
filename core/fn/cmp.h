#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/box/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/char/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/obj/mtd.h"
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"
#include "../struct/token/basic.h"

[[gnu::hot, clang::noinline]]
static t_any cmp(t_thrd_data* const thrd_data, t_any const arg_1, t_any const arg_2, const char* const owner) {
     u8 const type_1 = arg_1.bytes[15];
     u8 const type_2 = arg_2.bytes[15];

     if (!(
          (type_1 == type_2)                                               ||
          (type_1 == tid__short_string     && type_2 == tid__string)       ||
          (type_1 == tid__string           && type_2 == tid__short_string) ||
          (type_1 == tid__short_byte_array && type_2 == tid__byte_array)   ||
          (type_1 == tid__byte_array       && type_2 == tid__short_byte_array)
     )) return tkn__not_equal;

     switch (type_1) {
     case tid__short_string:
          switch (type_2) {
          case tid__short_string: return cmp__short_string__short_string(arg_1, arg_2);
          case tid__string:       return cmp__short_string__long_string(arg_1, arg_2);
          default:                unreachable;
          }
     case tid__string:
          switch (type_2) {
          case tid__short_string: return cmp__long_string__short_string(arg_1, arg_2);
          case tid__string:       return cmp__long_string__long_string(arg_1, arg_2);
          default:                unreachable;
          }
     case tid__token: {
          [[gnu::aligned(32)]]
          char chars_1[32] = {};
          [[gnu::aligned(32)]]
          char chars_2[32] = {};

          u8 unneeded_len;

          token_to_ascii_chars(arg_1, &unneeded_len, chars_1);
          token_to_ascii_chars(arg_2, &unneeded_len, chars_2);

          switch (scar__memcmp(chars_1, chars_2, 32)) {
          case -1: return tkn__less;
          case 0:  return tkn__equal;
          case 1:  return tkn__great;
          default: unreachable;
          }
     }
     case tid__bool:
          return arg_1.bytes[0] == arg_2.bytes[0] ? tkn__equal : tkn__not_equal;
     case tid__byte: {
          u8 const byte_1 = arg_1.bytes[0];
          u8 const byte_2 = arg_2.bytes[0];
          return byte_1 < byte_2 ? tkn__less : (byte_2 < byte_1 ? tkn__great : tkn__equal);
     }
     case tid__char: {
          u32 const char_1 = char_to_code(arg_1.bytes);
          u32 const char_2 = char_to_code(arg_2.bytes);
          return char_1 < char_2 ? tkn__less : (char_2 < char_1 ? tkn__great : tkn__equal);
     }
     case tid__int: case tid__time: {
          i64 const int_1 = arg_1.qwords[0];
          i64 const int_2 = arg_2.qwords[0];
          return int_1 < int_2 ? tkn__less : (int_2 < int_1 ? tkn__great : tkn__equal);
     }
     case tid__float: {
          double const float_1 = arg_1.floats[0];
          double const float_2 = arg_2.floats[0];
          if (__builtin_isnan(float_1) || __builtin_isnan(float_2)) return tkn__nan_cmp;

          return float_1 < float_2 ? tkn__less : (float_2 < float_1 ? tkn__great : tkn__equal);
     }
     case tid__short_byte_array: case tid__byte_array:
          return byte_array__cmp(arg_1, arg_2);
     case tid__box:
          unreachable;
     case tid__obj:
          return obj__cmp(thrd_data, arg_1, arg_2, owner);
     case tid__table:
          return table__equal(thrd_data, arg_1, arg_2, owner).bytes[0] == 1 ? tkn__equal : tkn__not_equal;
     case tid__set:
          return set__equal(thrd_data, arg_1, arg_2, owner).bytes[0] == 1 ? tkn__equal : tkn__not_equal;
     case tid__array:
          return array__cmp(thrd_data, arg_1, arg_2, owner);
     case tid__custom: {
          ref_cnt__inc(thrd_data, arg_1, owner);
          ref_cnt__inc(thrd_data, arg_2, owner);

          thrd_data->fns_args[0] = arg_1;
          thrd_data->fns_args[1] = arg_2;

          bool        called;
          t_any const result = custom__try_call__any_result__own(thrd_data, mtd__core_cmp, arg_1, thrd_data->fns_args, 2, owner, &called);
          if (!called) {
               ref_cnt__dec(thrd_data, arg_1);
               ref_cnt__dec(thrd_data, arg_2);
               return tkn__not_equal;
          }

          if (
               !(
                    result.raw_bits == tkn__equal.raw_bits     ||
                    result.raw_bits == tkn__great.raw_bits     ||
                    result.raw_bits == tkn__less.raw_bits      ||
                    result.raw_bits == tkn__nan_cmp.raw_bits   ||
                    result.raw_bits == tkn__not_equal.raw_bits
               )
          ) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The 'core.cmp' function returned a value other than ':equal', ':not_equal', ':less', ':great', ':nan_cmp'.", owner);

          return result;
     }
     default:
          return tkn__not_equal;
     }
}

core t_any McoreFNcmp(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.cmp";

     t_any const arg_1 = args[0];
     t_any const arg_2 = args[1];
     if (arg_1.bytes[15] == tid__error || arg_2.bytes[15] == tid__error) [[clang::unlikely]] {
          if (arg_1.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, arg_2);
               return arg_1;
          }

          ref_cnt__dec(thrd_data, arg_1);
          return arg_2;
     }

     u8 const type_1 = arg_1.bytes[15];
     u8 const type_2 = arg_2.bytes[15];

     if (!(
          (type_1 == type_2)                                               ||
          (type_1 == tid__short_string     && type_2 == tid__string)       ||
          (type_1 == tid__string           && type_2 == tid__short_string) ||
          (type_1 == tid__short_byte_array && type_2 == tid__byte_array)   ||
          (type_1 == tid__byte_array       && type_2 == tid__short_byte_array)
     )) {
          ref_cnt__dec(thrd_data, arg_1);
          ref_cnt__dec(thrd_data, arg_2);
          return tkn__not_equal;
     }

     switch (type_1) {
     case tid__short_string:
          switch (type_2) {
          case tid__short_string:
               return cmp__short_string__short_string(arg_1, arg_2);
          case tid__string: {
               u64 const ref_cnt_2 = get_ref_cnt(arg_2);
               if (ref_cnt_2 > 1) set_ref_cnt(arg_2, ref_cnt_2 - 1);

               t_any const result = cmp__short_string__long_string(arg_1, arg_2);

               if (ref_cnt_2 == 1) free((void*)arg_2.qwords[0]);

               return result;
          }
          default:
               unreachable;
          }
     case tid__string:
          switch (type_2) {
          case tid__short_string: {
               u64 const ref_cnt_1 = get_ref_cnt(arg_1);
               if (ref_cnt_1 > 1) set_ref_cnt(arg_1, ref_cnt_1 - 1);

               t_any const result = cmp__long_string__short_string(arg_1, arg_2);

               if (ref_cnt_1 == 1) free((void*)arg_1.qwords[0]);

               return result;
          }
          case tid__string: {
               u64 const ref_cnt_1 = get_ref_cnt(arg_1);
               u64 const ref_cnt_2 = get_ref_cnt(arg_2);
               if (arg_1.qwords[0] == arg_2.qwords[0]) {
                    if (ref_cnt_1 > 2) set_ref_cnt(arg_1, ref_cnt_1 - 2);
               } else {
                    if (ref_cnt_1 > 1) set_ref_cnt(arg_1, ref_cnt_1 - 1);
                    if (ref_cnt_2 > 1) set_ref_cnt(arg_2, ref_cnt_2 - 1);
               }

               t_any const result = cmp__long_string__long_string(arg_1, arg_2);

               if (arg_1.qwords[0] == arg_2.qwords[0]) {
                    if (ref_cnt_1 == 2) free((void*)arg_1.qwords[0]);
               } else {
                    if (ref_cnt_1 == 1) free((void*)arg_1.qwords[0]);
                    if (ref_cnt_2 == 1) free((void*)arg_2.qwords[0]);
               }

               return result;
          }
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

          switch (scar__memcmp(chars_1, chars_2, 32)) {
          case -1: return tkn__less;
          case 0:  return tkn__equal;
          case 1:  return tkn__great;
          default: unreachable;
          }
     }
     case tid__bool:
          return arg_1.bytes[0] == arg_2.bytes[0] ? tkn__equal : tkn__not_equal;
     case tid__byte: {
          u8 const byte_1 = arg_1.bytes[0];
          u8 const byte_2 = arg_2.bytes[0];
          return byte_1 < byte_2 ? tkn__less : (byte_2 < byte_1 ? tkn__great : tkn__equal);
     }
     case tid__char: {
          u32 const char_1 = char_to_code(arg_1.bytes);
          u32 const char_2 = char_to_code(arg_2.bytes);
          return char_1 < char_2 ? tkn__less : (char_2 < char_1 ? tkn__great : tkn__equal);
     }
     case tid__int: case tid__time: {
          i64 const int_1 = arg_1.qwords[0];
          i64 const int_2 = arg_2.qwords[0];
          return int_1 < int_2 ? tkn__less : (int_2 < int_1 ? tkn__great : tkn__equal);
     }
     case tid__float: {
          double const float_1 = arg_1.floats[0];
          double const float_2 = arg_2.floats[0];
          if (__builtin_isnan(float_1) || __builtin_isnan(float_2)) return tkn__nan_cmp;

          return float_1 < float_2 ? tkn__less : (float_2 < float_1 ? tkn__great : tkn__equal);
     }
     case tid__short_byte_array: case tid__byte_array: {
          u64 ref_cnt_1 = 0;
          u64 ref_cnt_2 = 0;

          if (arg_1.qwords[0] == arg_2.qwords[0] && arg_1.bytes[15] == tid__byte_array && arg_2.bytes[15] == tid__byte_array) {
               ref_cnt_1 = get_ref_cnt(arg_1);
               if (ref_cnt_1 > 2) set_ref_cnt(arg_1, ref_cnt_1 - 2);
          } else {
               if (arg_1.bytes[15] == tid__byte_array) {
                    ref_cnt_1 = get_ref_cnt(arg_1);
                    if (ref_cnt_1 > 1) set_ref_cnt(arg_1, ref_cnt_1 - 1);
               }
               if (arg_2.bytes[15] == tid__byte_array) {
                    ref_cnt_2 = get_ref_cnt(arg_2);
                    if (ref_cnt_2 > 1) set_ref_cnt(arg_2, ref_cnt_2 - 1);
               }
          }

          t_any const result = byte_array__cmp(arg_1, arg_2);

          if (arg_1.qwords[0] == arg_2.qwords[0] && arg_1.bytes[15] == tid__byte_array && arg_2.bytes[15] == tid__byte_array) {
               if (ref_cnt_1 == 2) free((void*)arg_1.qwords[0]);
          } else {
               if (ref_cnt_1 == 1) free((void*)arg_1.qwords[0]);
               if (ref_cnt_2 == 1) free((void*)arg_2.qwords[0]);
          }

          return result;
     }
     case tid__box: {
          call_stack__push(thrd_data, owner);
          t_any const result = box__cmp__own(thrd_data, arg_1, arg_2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          u64 const ref_cnt_1 = get_ref_cnt(arg_1);
          u64 const ref_cnt_2 = get_ref_cnt(arg_2);
          if (arg_1.qwords[0] == arg_2.qwords[0]) {
               if (ref_cnt_1 > 2) set_ref_cnt(arg_1, ref_cnt_1 - 2);
          } else {
               if (ref_cnt_1 > 1) set_ref_cnt(arg_1, ref_cnt_1 - 1);
               if (ref_cnt_2 > 1) set_ref_cnt(arg_2, ref_cnt_2 - 1);
          }

          t_any result;
          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = obj__cmp(thrd_data, arg_1, arg_2, owner);
          call_stack__pop(thrd_data);

          if (arg_1.qwords[0] == arg_2.qwords[0]) {
               if (ref_cnt_1 == 2) ref_cnt__dec__noinlined_part(thrd_data, arg_1);
          } else {
               if (ref_cnt_1 == 1) ref_cnt__dec__noinlined_part(thrd_data, arg_1);
               if (ref_cnt_2 == 1) ref_cnt__dec__noinlined_part(thrd_data, arg_2);
          }

          return result;
     }
     case tid__table: {
          u64 const ref_cnt_1 = get_ref_cnt(arg_1);
          u64 const ref_cnt_2 = get_ref_cnt(arg_2);
          if (arg_1.qwords[0] == arg_2.qwords[0]) {
               if (ref_cnt_1 > 2) set_ref_cnt(arg_1, ref_cnt_1 - 2);
          } else {
               if (ref_cnt_1 > 1) set_ref_cnt(arg_1, ref_cnt_1 - 1);
               if (ref_cnt_2 > 1) set_ref_cnt(arg_2, ref_cnt_2 - 1);
          }

          t_any result;
          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = table__equal(thrd_data, arg_1, arg_2, owner);
          call_stack__pop(thrd_data);

          if (arg_1.qwords[0] == arg_2.qwords[0]) {
               if (ref_cnt_1 == 2) ref_cnt__dec__noinlined_part(thrd_data, arg_1);
          } else {
               if (ref_cnt_1 == 1) ref_cnt__dec__noinlined_part(thrd_data, arg_1);
               if (ref_cnt_2 == 1) ref_cnt__dec__noinlined_part(thrd_data, arg_2);
          }

          return result.bytes[0] == 1 ? tkn__equal : tkn__not_equal;
     }
     case tid__set: {
          u64 const ref_cnt_1 = get_ref_cnt(arg_1);
          u64 const ref_cnt_2 = get_ref_cnt(arg_2);
          if (arg_1.qwords[0] == arg_2.qwords[0]) {
               if (ref_cnt_1 > 2) set_ref_cnt(arg_1, ref_cnt_1 - 2);
          } else {
               if (ref_cnt_1 > 1) set_ref_cnt(arg_1, ref_cnt_1 - 1);
               if (ref_cnt_2 > 1) set_ref_cnt(arg_2, ref_cnt_2 - 1);
          }

          t_any result;
          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = set__equal(thrd_data, arg_1, arg_2, owner);
          call_stack__pop(thrd_data);

          if (arg_1.qwords[0] == arg_2.qwords[0]) {
               if (ref_cnt_1 == 2) ref_cnt__dec__noinlined_part(thrd_data, arg_1);
          } else {
               if (ref_cnt_1 == 1) ref_cnt__dec__noinlined_part(thrd_data, arg_1);
               if (ref_cnt_2 == 1) ref_cnt__dec__noinlined_part(thrd_data, arg_2);
          }

          return result.bytes[0] == 1 ? tkn__equal : tkn__not_equal;
     }
     case tid__array: {
          u64 const ref_cnt_1 = get_ref_cnt(arg_1);
          u64 const ref_cnt_2 = get_ref_cnt(arg_2);
          if (arg_1.qwords[0] == arg_2.qwords[0]) {
               if (ref_cnt_1 > 2) set_ref_cnt(arg_1, ref_cnt_1 - 2);
          } else {
               if (ref_cnt_1 > 1) set_ref_cnt(arg_1, ref_cnt_1 - 1);
               if (ref_cnt_2 > 1) set_ref_cnt(arg_2, ref_cnt_2 - 1);
          }

          t_any result;
          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = array__cmp(thrd_data, arg_1, arg_2, owner);
          call_stack__pop(thrd_data);

          if (arg_1.qwords[0] == arg_2.qwords[0]) {
               if (ref_cnt_1 == 2) ref_cnt__dec__noinlined_part(thrd_data, arg_1);
          } else {
               if (ref_cnt_1 == 1) ref_cnt__dec__noinlined_part(thrd_data, arg_1);
               if (ref_cnt_2 == 1) ref_cnt__dec__noinlined_part(thrd_data, arg_2);
          }

          return result;
     }
     case tid__custom: {
          bool called;
          call_stack__push(thrd_data, owner);
          t_any const result = custom__try_call__any_result__own(thrd_data, mtd__core_cmp, arg_1, args, 2, owner, &called);
          if (!called) {
               call_stack__pop(thrd_data);

               ref_cnt__dec(thrd_data, arg_1);
               ref_cnt__dec(thrd_data, arg_2);
               return tkn__not_equal;
          }

          if (
               !(
                    result.raw_bits == tkn__equal.raw_bits     ||
                    result.raw_bits == tkn__great.raw_bits     ||
                    result.raw_bits == tkn__less.raw_bits      ||
                    result.raw_bits == tkn__nan_cmp.raw_bits   ||
                    result.raw_bits == tkn__not_equal.raw_bits
               )
          ) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The 'core.cmp' function returned a value other than ':equal', ':not_equal', ':less', ':great', ':nan_cmp'.", owner);

          call_stack__pop(thrd_data);
          return result;
     }
     default:
          ref_cnt__dec(thrd_data, arg_1);
          ref_cnt__dec(thrd_data, arg_2);
          return tkn__not_equal;
     }
}
