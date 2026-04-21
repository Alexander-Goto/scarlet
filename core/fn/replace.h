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
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"

[[gnu::hot]]
core t_any McoreFNreplace(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.replace";

     t_any const container     = args[0];
     t_any const old_item      = args[1];
     t_any const new_item      = args[2];
     u8    const old_item_type = old_item.bytes[15];
     u8    const new_item_type = new_item.bytes[15];
     if (container.bytes[15] == tid__error || !type_is_eq_and_common(old_item_type) || !type_is_common(new_item_type)) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, old_item);
               ref_cnt__dec(thrd_data, new_item);
               return container;
          }

          if (old_item_type == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, new_item);
               return old_item;
          }

          if (new_item_type == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, old_item);
               return new_item;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string:
          if (!(old_item_type == tid__char && new_item_type == tid__char)) [[clang::unlikely]] goto invalid_type_label;
          return short_string__replace(container, old_item, new_item);
     case tid__short_byte_array:
          if (!(old_item_type == tid__byte && new_item_type == tid__byte)) [[clang::unlikely]] goto invalid_type_label;
          return short_byte_array__replace(container, old_item.bytes[0], new_item.bytes[0]);
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__replace__own(thrd_data, container, old_item, new_item, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__table: {
          call_stack__push(thrd_data, owner);
          t_any const result = table__replace__own(thrd_data, container, old_item, new_item, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array:
          if (!(old_item_type == tid__byte && new_item_type == tid__byte)) [[clang::unlikely]] goto invalid_type_label;
          return long_byte_array__replace__own(thrd_data, container, old_item.bytes[0], new_item.bytes[0]);
     case tid__string:
          if (!(old_item_type == tid__char && new_item_type == tid__char)) [[clang::unlikely]] goto invalid_type_label;
          return long_string__replace__own(thrd_data, container, old_item, new_item);
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__replace__own(thrd_data, container, old_item, new_item, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_replace, tid__custom, container, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, old_item);
     ref_cnt__dec(thrd_data, new_item);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}