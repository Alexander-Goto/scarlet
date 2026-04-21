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

[[gnu::hot]]
core t_any McoreFNlook_other_from_end_in(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.look_other_from_end_in";

     t_any const item      = args[0];
     t_any const array     = args[1];
     u8    const item_type = item.bytes[15];
     if (array.bytes[15] == tid__error || !type_is_eq_and_common(item_type)) [[clang::unlikely]] {
          if (item_type == tid__error) {
               ref_cnt__dec(thrd_data, array);
               return item;
          }

          if (array.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, item);
               return array;
          }

          goto invalid_type_label;
     }

     switch (array.bytes[15]) {
     case tid__short_string:
          if (item_type != tid__char) [[clang::unlikely]] goto invalid_type_label;
          return short_string__look_other_from_end_in(array, item);
     case tid__short_byte_array:
          if (item_type != tid__byte) [[clang::unlikely]] goto invalid_type_label;
          return short_byte_array__look_other_from_end_in(array, item.bytes[0]);
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any result = obj__call__any_result__own(thrd_data, mtd__core_look_other_from_end_in, array, args, 2, owner);
          if (result.bytes[15] != tid__int && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array:
          if (item_type != tid__byte) [[clang::unlikely]] goto invalid_type_label;
          return long_byte_array__look_other_from_end_in__own(thrd_data, array, item.bytes[0]);
     case tid__string:
          if (item_type != tid__char) [[clang::unlikely]] goto invalid_type_label;
          return long_string__look_other_from_end_in__own(thrd_data, array, item);
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__look_other_from_end_in__own(thrd_data, array, item, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any result = custom__call__any_result__own(thrd_data, mtd__core_look_other_from_end_in, array, args, 2, owner);
          if (result.bytes[15] != tid__int && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, item);
     ref_cnt__dec(thrd_data, array);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}