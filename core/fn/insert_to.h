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

core t_any McoreFNinsert_to(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.insert_to";

     t_any const item      = args[0];
     t_any const container = args[1];
     t_any const position  = args[2];
     u8    const item_type = item.bytes[15];
     if (!type_is_common(item_type) || container.bytes[15] == tid__error || position.bytes[15] != tid__int) [[clang::unlikely]] {
          if (item_type == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, position);
               return item;
          }

          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, item);
               ref_cnt__dec(thrd_data, position);
               return container;
          }

          if (position.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, item);
               ref_cnt__dec(thrd_data, container);
               return position;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string: {
          if (item_type != tid__char) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = short_string__insert_to(thrd_data, container, item, position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: {
          if (item_type != tid__byte) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = short_byte_array__insert_to(thrd_data, container, item.bytes[0], position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          if (item_type != tid__byte) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = long_byte_array__insert_to__own(thrd_data, container, item.bytes[0], position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          if (item_type != tid__char) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = long_string__insert_to__own(thrd_data, container, item, position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__insert_to__own(thrd_data, container, item, position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_insert_to, tid__obj, container, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_insert_to, tid__custom, container, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, item);
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, position);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
