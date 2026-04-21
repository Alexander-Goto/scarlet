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

[[gnu::hot]]
core t_any McoreFNfoldl1(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.foldl1";

     t_any const container = args[0];
     t_any const fn        = args[1];

     if (container.bytes[15] == tid__error || !(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn)) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, fn);
               return container;
          }

          if (fn.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return fn;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_string__foldl1__own(thrd_data, container, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_byte_array__foldl1__own(thrd_data, container, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__foldl1__own(thrd_data, container, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__table: {
          call_stack__push(thrd_data, owner);
          t_any const result = table__foldl1__own(thrd_data, container, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__set: {
          call_stack__push(thrd_data, owner);
          t_any const result = set__foldl1__own(thrd_data, container, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_byte_array__foldl1__own(thrd_data, container, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_string__foldl1__own(thrd_data, container, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__foldl1__own(thrd_data, container, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__any_result__own(thrd_data, mtd__core_foldl1, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, fn);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}