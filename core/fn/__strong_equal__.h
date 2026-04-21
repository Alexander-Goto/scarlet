#pragma once

#include "__equal__.h"

core inline t_any McoreFN__strong_equal__(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.(==)";

     t_any const left       = args[0];
     t_any const right      = args[1];
     u8    const left_type  = left.bytes[15];
     u8    const right_type = right.bytes[15];
     if (
          left_type == tid__error || right_type == tid__error ||
          (left_type != right_type && !(
               (left_type == tid__short_string     && right_type == tid__string)       ||
               (left_type == tid__string           && right_type == tid__short_string) ||
               (left_type == tid__short_byte_array && right_type == tid__byte_array)   ||
               (left_type == tid__byte_array       && right_type == tid__short_byte_array)
          ))
     ) [[clang::unlikely]] {
          if (left_type == tid__error) {
               ref_cnt__dec(thrd_data, right);
               return left;
          }
          ref_cnt__dec(thrd_data, left);

          if (right_type == tid__error) return right;
          ref_cnt__dec(thrd_data, right);

          goto invalid_type_label;
     }

     switch (left_type) {
     case tid__short_string: case tid__short_byte_array:
          if (left_type != right_type) {
               ref_cnt__dec(thrd_data, right);
               return bool__false;
          }
     case tid__token:
          return left.raw_bits == right.raw_bits ? bool__true : bool__false;
     case tid__bool: case tid__byte:
          return left.bytes[0] == right.bytes[0] ? bool__true : bool__false;
     case tid__char: case tid__int: case tid__time:
          return left.qwords[0] == right.qwords[0] ? bool__true : bool__false;
     case tid__float:
          return left.floats[0] == right.floats[0] ? bool__true : bool__false;
     case tid__box: {
          call_stack__push(thrd_data, owner);
          t_any const result = box__equal__own(thrd_data, left, right, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          if (left_type != right_type) {
               ref_cnt__dec(thrd_data, left);
               return bool__false;
          }

          u64 const left_ref_cnt  = get_ref_cnt(left);
          u64 const right_ref_cnt = get_ref_cnt(right);
          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
          } else {
               if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
               if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
          }

          t_any const result = long_byte_array__equal(left, right);

          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt == 2) free((void*)left.qwords[0]);
          } else {
               if (left_ref_cnt == 1) free((void*)left.qwords[0]);
               if (right_ref_cnt == 1) free((void*)right.qwords[0]);
          }

          return result;
     }
     case tid__string: {
          if (left_type != right_type) {
               ref_cnt__dec(thrd_data, left);
               return bool__false;
          }

          t_any const result = equal__long_string__long_string(left, right);
          ref_cnt__dec(thrd_data, left);
          ref_cnt__dec(thrd_data, right);
          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__any_result__own(thrd_data, mtd__core___strong_equal__, left, args, 2, owner);
          call_stack__pop(thrd_data);

          if (result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (result.bytes[15] == tid__error) return result;

               ref_cnt__dec(thrd_data, result);
               goto invalid_type_label;
          }

          return result;
     }
     case tid__table: {
          u64 const left_ref_cnt  = get_ref_cnt(left);
          u64 const right_ref_cnt = get_ref_cnt(right);
          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
          } else {
               if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
               if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
          }

          t_any result;
          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = table__equal(thrd_data, left, right, owner);
          call_stack__pop(thrd_data);

          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt == 2) ref_cnt__dec__noinlined_part(thrd_data, left);
          } else {
               if (left_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, left);
               if (right_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, right);
          }

          return result;
     }
     case tid__set: {
          u64 const left_ref_cnt  = get_ref_cnt(left);
          u64 const right_ref_cnt = get_ref_cnt(right);
          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
          } else {
               if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
               if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
          }

          t_any result;
          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = set__equal(thrd_data, left, right, owner);
          call_stack__pop(thrd_data);

          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt == 2) ref_cnt__dec__noinlined_part(thrd_data, left);
          } else {
               if (left_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, left);
               if (right_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, right);
          }

          return result;
     }
     case tid__array: {
          u64 const left_ref_cnt  = get_ref_cnt(left);
          u64 const right_ref_cnt = get_ref_cnt(right);
          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
          } else {
               if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
               if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
          }

          t_any result;
          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = array__equal(thrd_data, left, right, owner);
          call_stack__pop(thrd_data);

          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt == 2) ref_cnt__dec__noinlined_part(thrd_data, left);
          } else {
               if (left_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, left);
               if (right_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, right);
          }

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__any_result__own(thrd_data, mtd__core___strong_equal__, left, args, 2, owner);
          call_stack__pop(thrd_data);

          if (result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (result.bytes[15] == tid__error) return result;

               ref_cnt__dec(thrd_data, result);
               goto invalid_type_label;
          }

          return result;
     }
     [[clang::unlikely]]
     default:
          ref_cnt__dec(thrd_data, left);
          ref_cnt__dec(thrd_data, right);
          goto invalid_type_label;
     }

     invalid_type_label:
     call_stack__push(thrd_data, owner);
     t_any const error = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return error;
}
