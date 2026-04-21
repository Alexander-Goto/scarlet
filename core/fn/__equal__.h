#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/bool/basic.h"
#include "../struct/box/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/char/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/obj/mtd.h"
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"
#include "../struct/token/basic.h"

[[gnu::hot, clang::noinline]]
static t_any equal(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner) {
     if (left.bytes[15] != right.bytes[15]) return bool__false;

     switch (left.bytes[15]) {
     case tid__short_string: case tid__token: case tid__short_byte_array:
          return left.raw_bits == right.raw_bits ? bool__true : bool__false;
     case tid__string:
          return equal__long_string__long_string(left, right);
     case tid__bool: case tid__byte:
          return left.bytes[0] == right.bytes[0] ? bool__true : bool__false;
     case tid__char: case tid__int: case tid__time:
          return left.qwords[0] == right.qwords[0] ? bool__true : bool__false;
     case tid__float:
          return left.floats[0] == right.floats[0] ? bool__true : bool__false;
     case tid__box:
          unreachable;
     case tid__byte_array:
          return long_byte_array__equal(left, right);
     case tid__obj:
          return obj__equal(thrd_data, left, right, owner);
     case tid__table:
          return table__equal(thrd_data, left, right, owner);
     case tid__set:
          return set__equal(thrd_data, left, right, owner);
     case tid__array:
          return array__equal(thrd_data, left, right, owner);
     case tid__custom: {
          ref_cnt__inc(thrd_data, left, owner);
          ref_cnt__inc(thrd_data, right, owner);

          thrd_data->fns_args[0] = left;
          thrd_data->fns_args[1] = right;

          bool        called;
          t_any const result = custom__try_call__any_result__own(thrd_data, mtd__core___equal__, left, thrd_data->fns_args, 2, owner, &called);
          if (!called) {
               ref_cnt__dec(thrd_data, left);
               ref_cnt__dec(thrd_data, right);
               return bool__false;
          }

          if (result.bytes[15] != tid__bool) [[clang::unlikely]]
               fail_with_call_stack(thrd_data, "The 'core.(===)' function returned a value other than 'true', 'false'.", owner);

          return result;
     }
     default:
          return bool__false;
     }
}

static const char* const core_equal_fn_name = "function - core.(===)";

core inline t_any McoreFN__equal__(t_thrd_data* const thrd_data, const t_any* const args) {
     t_any const left  = args[0];
     t_any const right = args[1];
     if (left.bytes[15] == tid__error || right.bytes[15] == tid__error) [[clang::unlikely]] {
          if (left.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, right);
               return left;
          }
          ref_cnt__dec(thrd_data, left);

          return right;
     }

     if (left.bytes[15] != right.bytes[15]) {
          ref_cnt__dec(thrd_data, left);
          ref_cnt__dec(thrd_data, right);
          return bool__false;
     }

     switch (left.bytes[15]) {
     case tid__short_string: case tid__token: case tid__short_byte_array:
          return left.raw_bits == right.raw_bits ? bool__true : bool__false;
     case tid__string: {
          u64 const left_ref_cnt  = get_ref_cnt(left);
          u64 const right_ref_cnt = get_ref_cnt(right);
          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
          } else {
               if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
               if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
          }

          t_any const result = equal__long_string__long_string(left, right);

          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt == 2) free((void*)left.qwords[0]);
          } else {
               if (left_ref_cnt == 1) free((void*)left.qwords[0]);
               if (right_ref_cnt == 1) free((void*)right.qwords[0]);
          }

          return result;
     }
     case tid__bool: case tid__byte:
          return left.bytes[0] == right.bytes[0] ? bool__true : bool__false;
     case tid__char: case tid__int: case tid__time:
          return left.qwords[0] == right.qwords[0] ? bool__true : bool__false;
     case tid__float:
          return left.floats[0] == right.floats[0] ? bool__true : bool__false;
     case tid__box: {
          call_stack__push(thrd_data, core_equal_fn_name);
          t_any const result = box__equal__own(thrd_data, left, right, core_equal_fn_name);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
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
     case tid__obj: {
          u64 const left_ref_cnt  = get_ref_cnt(left);
          u64 const right_ref_cnt = get_ref_cnt(right);
          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
          } else {
               if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
               if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
          }

          t_any result;
          call_stack__push(thrd_data, core_equal_fn_name);
          [[clang::noinline]] result = obj__equal(thrd_data, left, right, core_equal_fn_name);
          call_stack__pop(thrd_data);

          if (left.qwords[0] == right.qwords[0]) {
               if (left_ref_cnt == 2) ref_cnt__dec__noinlined_part(thrd_data, left);
          } else {
               if (left_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, left);
               if (right_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, right);
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
          call_stack__push(thrd_data, core_equal_fn_name);
          [[clang::noinline]] result = table__equal(thrd_data, left, right, core_equal_fn_name);
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
          call_stack__push(thrd_data, core_equal_fn_name);
          [[clang::noinline]] result = set__equal(thrd_data, left, right, core_equal_fn_name);
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
          call_stack__push(thrd_data, core_equal_fn_name);
          [[clang::noinline]] result = array__equal(thrd_data, left, right, core_equal_fn_name);
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
          bool called;
          call_stack__push(thrd_data, core_equal_fn_name);
          t_any const result = custom__try_call__any_result__own(thrd_data, mtd__core___equal__, left, args, 2, core_equal_fn_name, &called);
          if (!called) {
               call_stack__pop(thrd_data);

               ref_cnt__dec(thrd_data, left);
               ref_cnt__dec(thrd_data, right);
               return bool__false;
          }

          if (result.bytes[15] != tid__bool) [[clang::unlikely]]
               fail_with_call_stack(thrd_data, "The 'core.(===)' function returned a value other than 'true', 'false'.", core_equal_fn_name);

          call_stack__pop(thrd_data);
          return result;
     }
     default:
          ref_cnt__dec(thrd_data, left);
          ref_cnt__dec(thrd_data, right);
          return bool__false;
     }
}
