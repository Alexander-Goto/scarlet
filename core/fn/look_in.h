#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/box/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/null/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"

static inline t_any look_in(t_thrd_data* const thrd_data, const t_any* const args, const char* const owner) {
     t_any const item      = args[0];
     t_any const container = args[1];
     u8    const item_type = item.bytes[15];
     if (container.bytes[15] == tid__error || !type_is_eq_and_common(item_type)) [[clang::unlikely]] {
          if (item_type == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return item;
          }

          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, item);
               return container;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string:
          if (item_type != tid__char) [[clang::unlikely]] goto invalid_type_label;
          return short_string__look_in(container, item);
     case tid__short_byte_array:
          if (item_type != tid__byte) [[clang::unlikely]] goto invalid_type_label;
          return short_byte_array__look_in(container, item.bytes[0]);
     case tid__obj:
          return obj__look_in__own(thrd_data, container, item, owner);
     case tid__table:
          return table__look_in__own(thrd_data, container, item, owner);
     case tid__set:
          return set__get_item__own(thrd_data, container, item, owner);
     case tid__byte_array:
          if (item_type != tid__byte) [[clang::unlikely]] goto invalid_type_label;
          return long_byte_array__look_in__own(thrd_data, container, item.bytes[0]);
     case tid__string:
          if (item_type != tid__char) [[clang::unlikely]] goto invalid_type_label;
          return long_string__look_in__own(thrd_data, container, item);
     case tid__array:
          return array__look_in__own(thrd_data, container, item, owner);
     case tid__custom:
          return custom__call__any_result__own(thrd_data, mtd__core_look_in, container, args, 2, owner);

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, item);
     ref_cnt__dec(thrd_data, container);
     return error__invalid_type(thrd_data, owner);
}

[[gnu::hot]]
core inline t_any McoreFNlook_in(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.look_in";

     call_stack__push(thrd_data, owner);
     t_any const result = look_in(thrd_data, args, owner);
     call_stack__pop(thrd_data);

     return result;
}