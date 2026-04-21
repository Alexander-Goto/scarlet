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

core t_any McoreFNcopy(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.copy";

     t_any const dst = args[0];
     t_any const idx = args[1];
     t_any const src = args[2];

     u8 const dst_type = dst.bytes[15];
     u8 const src_type = src.bytes[15];
     if (
          dst_type == tid__error || idx.bytes[15] != tid__int || src_type == tid__error ||
          !(
               dst_type == src_type                                                      ||
               (dst_type == tid__short_string     && src_type == tid__string)            ||
               (dst_type == tid__string           && src_type == tid__short_string )     ||
               (dst_type == tid__short_byte_array && src_type  == tid__byte_array)       ||
               (dst_type == tid__byte_array       && src_type  == tid__short_byte_array)
          )
     ) [[clang::unlikely]] {
          if (dst_type == tid__error) {
               ref_cnt__dec(thrd_data, idx);
               ref_cnt__dec(thrd_data, src);
               return dst;
          }

          if (idx.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, dst);
               ref_cnt__dec(thrd_data, src);
               return idx;
          }

          if (src_type == tid__error) {
               ref_cnt__dec(thrd_data, dst);
               ref_cnt__dec(thrd_data, idx);
               return src;
          }

          goto invalid_type_label;
     }

     switch (dst_type) {
     case tid__short_string: {
          t_any result;
          call_stack__push(thrd_data, owner);
          if (src_type == tid__short_string)
               result = copy__short_str__short_str(thrd_data, dst, idx.qwords[0], src, owner);
          else result = copy__short_str__long_str__own(thrd_data, dst, idx.qwords[0], src, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: {
          t_any result;
          call_stack__push(thrd_data, owner);
          if (src_type == tid__short_byte_array) [[clang::likely]]
               result = copy__short_byte_array__short_byte_array(thrd_data, dst, idx.qwords[0], src, owner);
          else {
               ref_cnt__dec(thrd_data, src);
               result = error__out_of_bounds(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          t_any result;
          call_stack__push(thrd_data, owner);
          if (src_type == tid__short_byte_array)
               result = copy__long_byte_array__short_byte_array__own(thrd_data, dst, idx.qwords[0], src, owner);
          else result = copy__long_byte_array__long_byte_array__own(thrd_data, dst, idx.qwords[0], src, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          t_any result;
          call_stack__push(thrd_data, owner);
          if (src_type == tid__short_string)
               result = copy__long_str__short_str__own(thrd_data, dst, idx.qwords[0], src, owner);
          else result = copy__long_str__long_str__own(thrd_data, dst, idx.qwords[0], src, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__copy__own(thrd_data, dst, idx.qwords[0], src, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_copy, tid__obj, dst, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_copy, tid__custom, dst, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, dst);
     ref_cnt__dec(thrd_data, idx);
     ref_cnt__dec(thrd_data, src);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
