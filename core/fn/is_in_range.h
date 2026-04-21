#pragma once

#include "cmp.h"

core t_any McoreFNin_rangeB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.in_range?";

     t_any const arg       = args[0];
     t_any       from      = args[1];
     t_any const to        = args[2];
     u8    const arg_type  = arg.bytes[15];
     u8    const from_type = from.bytes[15];
     u8    const to_type   = to.bytes[15];
     if (
          arg_type == tid__error || from_type == tid__error || to_type == tid__error ||
          !(
               (arg_type == from_type && arg_type == to_type) ||
               (
                    (arg_type  == tid__short_string || arg_type  == tid__string) &&
                    (from_type == tid__short_string || from_type == tid__string) &&
                    (to_type   == tid__short_string || to_type   == tid__string)
               ) ||
               (
                    (arg_type  == tid__short_byte_array || arg_type  == tid__byte_array) &&
                    (from_type == tid__short_byte_array || from_type == tid__byte_array) &&
                    (to_type   == tid__short_byte_array || to_type   == tid__byte_array)
               )
          )
     ) [[clang::unlikely]] {
          if (arg_type == tid__error) {
               ref_cnt__dec(thrd_data, from);
               ref_cnt__dec(thrd_data, to);
               return arg;
          }

          if (from_type == tid__error) {
               ref_cnt__dec(thrd_data, arg);
               ref_cnt__dec(thrd_data, to);
               return from;
          }

          if (to_type == tid__error) {
               ref_cnt__dec(thrd_data, arg);
               ref_cnt__dec(thrd_data, from);
               return to;
          }

          goto invalid_type_label;
     }

     switch (arg_type) {
     case tid__token: {
          [[gnu::aligned(32)]]
          char arg_chars[32] = {};
          [[gnu::aligned(32)]]
          char from_to_chars[32] = {};

          u8 unneeded_len;

          token_to_ascii_chars(arg, &unneeded_len, arg_chars);
          token_to_ascii_chars(from, &unneeded_len, from_to_chars);

          if (scar__memcmp(arg_chars, from_to_chars, 32) == -1) return bool__false;

          memset_inline(from_to_chars, 0, 32);
          token_to_ascii_chars(to, &unneeded_len, from_to_chars);

          return scar__memcmp(arg_chars, from_to_chars, 32) == -1 ? bool__true : bool__false;
     }
     case tid__byte:
          return arg.bytes[0] >= from.bytes[0] && arg.bytes[0] < to.bytes[0] ? bool__true : bool__false;
     case tid__char: {
          u32 const arg_code  = char_to_code(arg.bytes);
          u32 const from_code = char_to_code(from.bytes);
          u32 const to_code   = char_to_code(to.bytes);
          return arg_code >= from_code && arg_code < to_code ? bool__true : bool__false;
     }
     case tid__int: case tid__time:
          return (i64)arg.qwords[0] >= (i64)from.qwords[0] && (i64)arg.qwords[0] < (i64)to.qwords[0] ? bool__true : bool__false;
     case tid__float: {
          double const arg_float  = arg.floats[0];
          double const from_float = from.floats[0];
          double const to_float   = to.floats[0];
          if (__builtin_isnan(arg_float) || __builtin_isnan(from_float) || __builtin_isnan(to_float)) [[clang::unlikely]] goto nan_cmp_label;

          return arg_float >= from_float && arg_float < to_float ? bool__true : bool__false;
     }
     case tid__string: case tid__byte_array: case tid__array: case tid__obj: case tid__custom:
          call_stack__push(thrd_data, owner);
          ref_cnt__inc(thrd_data, arg, owner);
          goto skip_push_label;
     case tid__short_string: case tid__short_byte_array: {
          {
               call_stack__push(thrd_data, owner);

               skip_push_label:
               thrd_data->fns_args[0] = arg;
               thrd_data->fns_args[1] = from;

               t_any const cmp_result = McoreFNcmp(thrd_data, thrd_data->fns_args);

               switch (cmp_result.bytes[0]) {
               case great_id: case equal_id:
                    break;
               case less_id:
                    call_stack__pop(thrd_data);

                    ref_cnt__dec(thrd_data, arg);
                    ref_cnt__dec(thrd_data, to);
                    return bool__false;

               [[clang::unlikely]]
               case not_equal_id:
                    call_stack__pop(thrd_data);

                    from = null;
                    goto invalid_type_label;
               [[clang::unlikely]]
               case nan_cmp_id:
                    call_stack__pop(thrd_data);

                    ref_cnt__dec(thrd_data, arg);
                    ref_cnt__dec(thrd_data, to);
                    goto nan_cmp_label;
               default:
                    unreachable;
               }
          }

          thrd_data->fns_args[0] = arg;
          thrd_data->fns_args[1] = to;

          t_any const cmp_result = McoreFNcmp(thrd_data, thrd_data->fns_args);
          call_stack__pop(thrd_data);

          switch (cmp_result.bytes[0]) {
          case less_id:
               return bool__true;
          case great_id: case equal_id:
               return bool__false;

          [[clang::unlikely]]
          case not_equal_id: {
               call_stack__push(thrd_data, owner);
               t_any const result = error__invalid_type(thrd_data, owner);
               call_stack__pop(thrd_data);

               return result;
          }
          [[clang::unlikely]]
          case nan_cmp_id:
               goto nan_cmp_label;
          default:
               unreachable;
          }
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label: {
          ref_cnt__dec(thrd_data, arg);
          ref_cnt__dec(thrd_data, from);
          ref_cnt__dec(thrd_data, to);

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
