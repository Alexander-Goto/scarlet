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
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"

[[gnu::hot]]
core t_any McoreFN__get_item__(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.([])";

     t_any const container = args[0];
     t_any const key       = args[1];
     u8    const key_type  = key.bytes[15];
     if (container.bytes[15] == tid__error || key_type == tid__error) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, key);
               return container;
          }
          ref_cnt__dec(thrd_data, container);

          return key;
     }

     switch (container.bytes[15]) {
     case tid__short_string: {
          if (key_type != tid__int) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = short_string__get_item(thrd_data, container, key, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__null:
          ref_cnt__dec(thrd_data, key);
          return null;
     case tid__short_byte_array: {
          if (key_type != tid__int) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = short_byte_array__get_item(thrd_data, container, key, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__box: {
          if (key_type != tid__int) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = box__get_item__own(thrd_data, container, key, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__get_item__own(thrd_data, container, key, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__table: {
          call_stack__push(thrd_data, owner);
          t_any const result = table__get_item__own(thrd_data, container, key, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__set: {
          call_stack__push(thrd_data, owner);
          t_any const result = set__get_item__own(thrd_data, container, key, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          if (key_type != tid__int) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = long_byte_array__get_item__own(thrd_data, container, key, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          if (key_type != tid__int) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = long_string__get_item__own(thrd_data, container, key, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          if (key_type != tid__int) [[clang::unlikely]] goto invalid_type_label;

          call_stack__push(thrd_data, owner);
          t_any const result = array__get_item__own(thrd_data, container, key, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any result = custom__call__any_result__own(thrd_data, mtd__core___get_item__, container, args, 2, owner);
          if (!type_is_common_or_null_or_error(result.bytes[15])) [[clang::unlikely]] {
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
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, key);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}