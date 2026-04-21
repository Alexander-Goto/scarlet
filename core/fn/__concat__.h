#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

static const char* const core_concat_fn_name = "function - core.(++)";

[[gnu::hot]]
core t_any McoreFN__concat__(t_thrd_data* const thrd_data, const t_any* const args) {
     t_any const left       = args[0];
     t_any const right      = args[1];
     u8    const left_type  = left.bytes[15];
     u8    const right_type = right.bytes[15];
     if (
          left_type == tid__error || right_type == tid__error ||
          !(
               left_type == right_type                                                  ||
               (left_type == tid__short_string     && right_type == tid__string)         ||
               (left_type == tid__string           && right_type == tid__short_string)   ||
               (left_type == tid__short_byte_array && right_type == tid__byte_array)     ||
               (left_type == tid__byte_array       && right_type == tid__short_byte_array)
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
     case tid__short_string: {
          if (right_type == tid__short_string)
               return concat__short_str__short_str(left, right);

          call_stack__push(thrd_data, core_concat_fn_name);
          t_any const result = concat__short_str__long_str__own(thrd_data, left, right, core_concat_fn_name);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: {
          if (right_type == tid__short_byte_array)
               return concat__short_byte_array__short_byte_array(left, right);

          call_stack__push(thrd_data, core_concat_fn_name);
          t_any const result = concat__short_byte_array__long_byte_array__own(thrd_data, left, right, core_concat_fn_name);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          t_any result;
          call_stack__push(thrd_data, core_concat_fn_name);

          if (right_type == tid__short_byte_array)
               result = concat__long_byte_array__short_byte_array__own(thrd_data, left, right, core_concat_fn_name);
          else result = concat__long_byte_array__long_byte_array__own(thrd_data, left, right, core_concat_fn_name);

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          t_any result;
          call_stack__push(thrd_data, core_concat_fn_name);

          if (right_type == tid__short_string)
               result = concat__long_str__short_str__own(thrd_data, left, right, core_concat_fn_name);
          else result = concat__long_str__long_str__own(thrd_data, left, right, core_concat_fn_name);

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, core_concat_fn_name);
          t_any const result = array__concat__own(thrd_data, left, right, core_concat_fn_name);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, core_concat_fn_name);
          t_any const result = obj__call__own(thrd_data, mtd__core___concat__, tid__obj, left, args, 2, core_concat_fn_name);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, core_concat_fn_name);
          t_any const result = custom__call__own(thrd_data, mtd__core___concat__, tid__custom, left, args, 2, core_concat_fn_name);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, left);
     ref_cnt__dec(thrd_data, right);

     call_stack__push(thrd_data, core_concat_fn_name);
     t_any const result = error__invalid_type(thrd_data, core_concat_fn_name);
     call_stack__pop(thrd_data);

     return result;
}
