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
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"

core t_any McoreFNreserve(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.reserve";

     t_any const container = args[0];
     t_any const items_qty = args[1];
     if (container.bytes[15] == tid__error || items_qty.bytes[15] != tid__int || (i64)items_qty.qwords[0] < 0) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, items_qty);
               return container;
          }

          if (items_qty.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return items_qty;
          }

          if (items_qty.bytes[15] == tid__int) {
               ref_cnt__dec(thrd_data, container);

               call_stack__push(thrd_data, owner);
               t_any const result = error__out_of_bounds(thrd_data, owner);
               call_stack__pop(thrd_data);

               return result;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string: {
          if (items_qty.qwords[0] > array_max_len || items_qty.qwords[0] + short_string__get_len(container) > array_max_len) [[clang::unlikely]] {
               call_stack__push(thrd_data, owner);
               t_any const error = error__out_of_bounds(thrd_data, owner);
               call_stack__pop(thrd_data);
               return error;
          }

          return container;
     }
     case tid__short_byte_array: {
          if (items_qty.qwords[0] > array_max_len || items_qty.qwords[0] + short_byte_array__get_len(container) > array_max_len) [[clang::unlikely]] {
               call_stack__push(thrd_data, owner);
               t_any const error = error__out_of_bounds(thrd_data, owner);
               call_stack__pop(thrd_data);
               return error;
          }

          return container;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__reserve__own(thrd_data, container, items_qty.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__table: {
          call_stack__push(thrd_data, owner);
          t_any const result = table__reserve__own(thrd_data, container, items_qty.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__set: {
          call_stack__push(thrd_data, owner);
          t_any const result = set__reserve__own(thrd_data, container, items_qty.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_byte_array__reserve__own(thrd_data, container, items_qty.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_string__reserve__own(thrd_data, container, items_qty.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__reserve__own(thrd_data, container, items_qty.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_reserve, tid__custom, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, items_qty);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}