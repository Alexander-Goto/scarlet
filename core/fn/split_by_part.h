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

[[gnu::hot]]
core t_any McoreFNsplit_by_part(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.split_by_part";

     t_any const container      = args[0];
     t_any const separator      = args[1];
     u8    const container_type = container.bytes[15];
     u8    const separator_type = separator.bytes[15];
     if (
          container_type == tid__error || separator_type == tid__error ||
          !(
               container_type == separator_type                                                 ||
               (container_type == tid__short_string     && separator_type == tid__string)       ||
               (container_type == tid__string           && separator_type == tid__short_string) ||
               (container_type == tid__short_byte_array && separator_type == tid__byte_array)   ||
               (container_type == tid__byte_array       && separator_type == tid__short_byte_array)
          )
     ) [[clang::unlikely]] {
          if (container_type == tid__error) {
               ref_cnt__dec(thrd_data, separator);
               return container;
          }

          if (separator_type == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return separator;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_string__split_by_part__own(thrd_data, container, separator, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_byte_array__split_by_part__own(thrd_data, container, separator, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_split_by_part, tid__array, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_byte_array__split_by_part__own(thrd_data, container, separator, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_string__split_by_part__own(thrd_data, container, separator, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__split_by_part__own(thrd_data, container, separator, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_split_by_part, tid__array, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, separator);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}