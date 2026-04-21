#pragma once

#include "cmp.h"

static inline t_any less__own(t_thrd_data* const thrd_data, const t_any* const args, const char* const owner) {
     t_any const left       = args[0];
     t_any const right      = args[1];
     u8    const left_type  = left.bytes[15];
     u8    const right_type = right.bytes[15];
     if (
          left_type == tid__error || right_type == tid__error ||
          !(
               left_type == right_type                                                 ||
               (left_type == tid__short_string     && right_type == tid__string)       ||
               (left_type == tid__string           && right_type == tid__short_string) ||
               (left_type == tid__short_byte_array && right_type  == tid__byte_array)  ||
               (left_type == tid__byte_array       && right_type  == tid__short_byte_array)
          )
     ) [[clang::unlikely]] {
          if (left_type == tid__error) {
               ref_cnt__dec(thrd_data, right);
               return left;
          }

          if (right_type == tid__error) {
               ref_cnt__dec(thrd_data, left);
               return right;
          }

          goto invalid_type_label;
     }

     switch (left_type) {
     case tid__short_string:
          switch (right_type) {
          case tid__short_string:
               return cmp__short_string__short_string(left, right).bytes[0] == less_id ? bool__true : bool__false;
          case tid__string: {
               u64 const right_ref_cnt = get_ref_cnt(right);
               if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);

               t_any const result = cmp__short_string__long_string(left, right).bytes[0] == less_id ? bool__true : bool__false;

               if (right_ref_cnt == 1) free((void*)right.qwords[0]);

               return result;
          }
          default:
               unreachable;
          }
     case tid__string:
          switch (right_type) {
          case tid__short_string: {
               u64 const left_ref_cnt = get_ref_cnt(left);
               if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);

               t_any const result = cmp__long_string__short_string(left, right).bytes[0] == less_id ? bool__true : bool__false;

               if (left_ref_cnt == 1) free((void*)left.qwords[0]);

               return result;
          }
          case tid__string: {
               u64 const left_ref_cnt  = get_ref_cnt(left);
               u64 const right_ref_cnt = get_ref_cnt(right);
               if (left.qwords[0] == right.qwords[0]) {
                    if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
               } else {
                    if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
                    if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
               }

               t_any const result = cmp__long_string__long_string(left, right).bytes[0] == less_id ? bool__true : bool__false;

               if (left.qwords[0] == right.qwords[0]) {
                    if (left_ref_cnt == 2) free((void*)left.qwords[0]);
               } else {
                    if (left_ref_cnt == 1) free((void*)left.qwords[0]);
                    if (right_ref_cnt == 1) free((void*)right.qwords[0]);
               }

               return result;
          }
          default:
               unreachable;
          }
          break;
     case tid__token: {
          [[gnu::aligned(32)]]
          char left_chars[32] = {};
          [[gnu::aligned(32)]]
          char right_chars[32] = {};

          u8 unneeded_len;

          token_to_ascii_chars(left, &unneeded_len, left_chars);
          token_to_ascii_chars(right, &unneeded_len, right_chars);

          return scar__memcmp(left_chars, right_chars, 32) == -1 ? bool__true : bool__false;
     }
     case tid__byte:
          return left.bytes[0] < right.bytes[0] ? bool__true : bool__false;
     case tid__char:
          return char_to_code(left.bytes) < char_to_code(right.bytes) ? bool__true : bool__false;
     case tid__int: case tid__time: return (i64)left.qwords[0] < (i64)right.qwords[0] ? bool__true : bool__false;
     case tid__float: {
          double const float_1 = left.floats[0];
          double const float_2 = right.floats[0];
          if (__builtin_isnan(float_1) || __builtin_isnan(float_2)) [[clang::unlikely]] return error__nan_cmp(thrd_data, owner);

          return float_1 < float_2 ? bool__true : bool__false;
     }
     case tid__short_byte_array: case tid__byte_array: {
          u64 left_ref_cnt  = 0;
          u64 right_ref_cnt = 0;

          if (left.qwords[0] == right.qwords[0] && left.bytes[15] == tid__byte_array && right.bytes[15] == tid__byte_array) {
               left_ref_cnt = get_ref_cnt(left);
               if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
          } else {
               if (left.bytes[15] == tid__byte_array) {
                    left_ref_cnt = get_ref_cnt(left);
                    if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
               }
               if (right.bytes[15] == tid__byte_array) {
                    right_ref_cnt = get_ref_cnt(right);
                    if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
               }
          }

          t_any const result = byte_array__cmp(left, right).bytes[0] == less_id ? bool__true : bool__false;

          if (left.qwords[0] == right.qwords[0] && left.bytes[15] == tid__byte_array && right.bytes[15] == tid__byte_array) {
               if (left_ref_cnt == 2) free((void*)left.qwords[0]);
          } else {
               if (left_ref_cnt == 1) free((void*)left.qwords[0]);
               if (right_ref_cnt == 1) free((void*)right.qwords[0]);
          }

          return result;
     }
     case tid__obj: case tid__array: case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const cmp_result = McoreFNcmp(thrd_data, args);
          call_stack__pop(thrd_data);

          switch (cmp_result.bytes[0]) {
          case less_id:
               return bool__true;
          case great_id: case equal_id:
               return bool__false;

          [[clang::unlikely]]
          case not_equal_id:
               return error__invalid_type(thrd_data, owner);
          [[clang::unlikely]]
          case nan_cmp_id:
               return error__nan_cmp(thrd_data, owner);
          default:
               unreachable;
          }
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, left);
     ref_cnt__dec(thrd_data, right);
     return error__invalid_type(thrd_data, owner);
}

core inline t_any McoreFN__less__(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.(<)";

     call_stack__push(thrd_data, owner);
     t_any const result = less__own(thrd_data, args, owner);
     call_stack__pop(thrd_data);

     return result;
}
