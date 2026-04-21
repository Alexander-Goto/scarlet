#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"

core t_any McoreFNappend_new(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.append_new";

     t_any const set       = args[0];
     t_any const item      = args[1];
     u8    const item_type = item.bytes[15];
     if (set.bytes[15] == tid__error || !type_is_common(item_type)) [[clang::unlikely]] {
          if (set.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, item);
               return set;
          }

          if (item_type == tid__error) {
               ref_cnt__dec(thrd_data, set);
               return item;
          }

          goto invalid_type_label;
     }

     switch (set.bytes[15]) {
     case tid__set: {
          call_stack__push(thrd_data, owner);
          t_any const result = set__append_new__own(thrd_data, set, item, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);

          t_any result = obj__call__any_result__own(thrd_data, mtd__core_append_new, set, args, 2, owner);
          if (result.bytes[15] != tid__obj && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);

          t_any result = custom__call__any_result__own(thrd_data, mtd__core_append_new, set, args, 2, owner);
          if (result.bytes[15] != tid__custom && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
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
     ref_cnt__dec(thrd_data, set);
     ref_cnt__dec(thrd_data, item);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
