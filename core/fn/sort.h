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
core t_any McoreFNsort(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.sort";

     t_any const container    = args[0];
     t_any const is_asc_order = args[1];
     if (container.bytes[15] == tid__error || is_asc_order.bytes[15] != tid__bool) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, is_asc_order);
               return container;
          }

          if (is_asc_order.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return is_asc_order;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string:
          return short_string__sort(container, is_asc_order.bytes[0] == 1);
     case tid__short_byte_array:
          return short_byte_array__sort(container, is_asc_order.bytes[0] == 1);
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_sort, tid__obj, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array:
          return long_byte_array__sort__own(thrd_data, container, is_asc_order.bytes[0] == 1);
     case tid__string:
          return long_string__sort__own(thrd_data, container, is_asc_order.bytes[0] == 1);
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__sort__own(thrd_data, container, is_asc_order.bytes[0] == 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_sort, tid__custom, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] case tid__error:
          return container;

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, is_asc_order);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}