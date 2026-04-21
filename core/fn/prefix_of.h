#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/null/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"

[[gnu::hot]]
core t_any McoreFNprefix_ofB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.prefix_of?";

     t_any const prefix         = args[0];
     t_any const container      = args[1];
     u8    const prefix_type    = prefix.bytes[15];
     u8    const container_type = container.bytes[15];
     if (
          prefix_type == tid__error || container_type == tid__error ||
          !(
               prefix_type == container_type                                                 ||
               (prefix_type == tid__short_string     && container_type == tid__string)       ||
               (prefix_type == tid__string           && container_type == tid__short_string) ||
               (prefix_type == tid__short_byte_array && container_type == tid__byte_array)   ||
               (prefix_type == tid__byte_array       && container_type == tid__short_byte_array)
          )
     ) [[clang::unlikely]] {
          if (prefix_type == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return prefix;
          }

          if (container_type == tid__error) {
               ref_cnt__dec(thrd_data, prefix);
               return container;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string: {
          if (prefix_type == tid__string) {
               ref_cnt__dec(thrd_data, prefix);
               return bool__false;
          }

          u8 const prefix_size = short_string__get_size(prefix);
          u8 const string_size = short_string__get_size(container);

          return string_size >= prefix_size && (container.raw_bits & (((u128)1 << prefix_size * 8) - 1)) == prefix.raw_bits ? bool__true : bool__false;
     }
     case tid__short_byte_array: {
          if (prefix_type == tid__byte_array) {
               ref_cnt__dec(thrd_data, prefix);
               return bool__false;
          }

          u8   const prefix_len = short_byte_array__get_len(prefix);
          u8   const array_len  = short_byte_array__get_len(container);
          u128 const mask       = ((u128)1 << prefix_len * 8) - 1;

          return array_len >= prefix_len && (container.raw_bits & mask) == (prefix.raw_bits & mask) ? bool__true : bool__false;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_prefix_of, tid__bool, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array:
          return prefix_of__long_byte_array__byte_array__own(thrd_data, container, prefix);
     case tid__string:
          return prefix_of__long_string__string__own(thrd_data, container, prefix);
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__prefix_of__own(thrd_data, container, prefix, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_prefix_of, tid__bool, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, prefix);
     ref_cnt__dec(thrd_data, container);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}