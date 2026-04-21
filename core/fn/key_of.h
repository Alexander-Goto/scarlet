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

core t_any McoreFNkey_ofB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.key_of?";

     t_any const key       = args[0];
     t_any const container = args[1];
     u8    const key_type  = key.bytes[15];
     if (container.bytes[15] == tid__error || key_type == tid__error) [[clang::unlikely]] {
          if (key.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return key;
          }
          ref_cnt__dec(thrd_data, key);

          return container;
     }

     t_any item;
     call_stack__push(thrd_data, owner);
     switch (container.bytes[15]) {
     case tid__obj:
          item = obj__get_item__own(thrd_data, container, key, owner);
          break;
     case tid__table:
          item = table__get_item__own(thrd_data, container, key, owner);
          break;
     case tid__set:
          item = set__get_item__own(thrd_data, container, key, owner);
          break;
     case tid__custom:
          thrd_data->fns_args[0] = container;
          thrd_data->fns_args[1] = key;

          item = custom__call__any_result__own(thrd_data, mtd__core___get_item__, container, thrd_data->fns_args, 2, owner);
          break;

     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, container);
          ref_cnt__dec(thrd_data, key);

          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}

     if (!type_is_common_or_null(item.bytes[15])) [[clang::unlikely]] {
          if (item.bytes[15] == tid__error) {
               call_stack__pop(thrd_data);
               return item;
          }

          ref_cnt__dec(thrd_data, item);

          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     call_stack__pop(thrd_data);

     t_any const result = item.bytes[15] == tid__null ? bool__false : bool__true;
     ref_cnt__dec(thrd_data, item);

     return result;
}
