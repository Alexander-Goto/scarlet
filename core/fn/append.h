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

[[gnu::hot]]
core t_any McoreFNappend(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.append";

     t_any const container = args[0];
     t_any const item      = args[1];
     u8    const item_type = item.bytes[15];
     if (container.bytes[15] == tid__error || !type_is_common(item_type)) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, item);
               return container;
          }

          if (item_type == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return item;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string:
          if (item_type != tid__char) [[clang::unlikely]] goto invalid_type_label;
          return short_string__append(container, item);
     case tid__short_byte_array:
          if (item_type != tid__byte) [[clang::unlikely]] goto invalid_type_label;
          return concat__short_byte_array__short_byte_array(container, (const t_any){.bytes = {item.bytes[0], [14] = 1, [15] = tid__short_byte_array}});
     case tid__byte_array: {
          if (item_type != tid__byte) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = concat__long_byte_array__short_byte_array__own(thrd_data, container, (const t_any){.bytes = {item.bytes[0], [14] = 1, [15] = tid__short_byte_array}}, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          if (item_type != tid__char) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = long_string__append__own(thrd_data, container, item, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__append__own(thrd_data, container, item, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_append, tid__obj, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__set: {
          call_stack__push(thrd_data, owner);
          t_any const result = set__append__own(thrd_data, container, item, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_append, tid__custom, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, item);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}