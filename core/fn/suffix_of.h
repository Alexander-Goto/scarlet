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
core t_any McoreFNsuffix_ofB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.suffix_of?";

     t_any       suffix         = args[0];
     t_any       container      = args[1];
     u8    const suffix_type    = suffix.bytes[15];
     u8    const container_type = container.bytes[15];
     if (
          suffix_type == tid__error || container_type == tid__error ||
          !(
               suffix_type == container_type                                                 ||
               (suffix_type == tid__short_string     && container_type == tid__string)       ||
               (suffix_type == tid__string           && container_type == tid__short_string) ||
               (suffix_type == tid__short_byte_array && container_type == tid__byte_array)   ||
               (suffix_type == tid__byte_array       && container_type == tid__short_byte_array)
          )
     ) [[clang::unlikely]] {
          if (suffix_type == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return suffix;
          }

          if (container_type == tid__error) {
               ref_cnt__dec(thrd_data, suffix);
               return container;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string: {
          if (suffix_type == tid__string) {
               ref_cnt__dec(thrd_data, suffix);
               return bool__false;
          }

          u8 const suffix_size = short_string__get_size(suffix);
          u8 const string_size = short_string__get_size(container);

          return string_size >= suffix_size && ((container.raw_bits >> (string_size - suffix_size) * 8) == suffix.raw_bits) ? bool__true : bool__false;
     }
     case tid__short_byte_array: {
          if (suffix_type == tid__byte_array) {
               ref_cnt__dec(thrd_data, suffix);
               return bool__false;
          }

          u8 const suffix_len = short_byte_array__get_len(suffix);
          u8 const array_len  = short_byte_array__get_len(container);

          memset_inline(&container.bytes[14], 0, 2);
          memset_inline(&suffix.bytes[14], 0, 2);

          return array_len >= suffix_len && (container.raw_bits >> (array_len - suffix_len) * 8) == suffix.raw_bits ? bool__true : bool__false;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_suffix_of, tid__bool, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array:
          return suffix_of__long_byte_array__byte_array__own(thrd_data, container, suffix);
     case tid__string:
          return suffix_of__long_string__string__own(thrd_data, container, suffix);
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__suffix_of__own(thrd_data, container, suffix, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_suffix_of, tid__bool, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, suffix);
     ref_cnt__dec(thrd_data, container);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}