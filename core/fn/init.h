#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"
#include "../struct/table/fn.h"

core t_any McoreFNinit(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.init";

     t_any const container = args[0];
     t_any const cap       = args[1];
     if (container.bytes[15] == tid__error || cap.bytes[15] != tid__int || (i64)cap.qwords[0] < 0) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, cap);
               return container;
          }

          if (cap.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               return cap;
          }

          if (cap.bytes[15] == tid__int) {
               ref_cnt__dec(thrd_data, container);

               call_stack__push(thrd_data, owner);
               t_any const result = error__out_of_bounds(thrd_data, owner);
               call_stack__pop(thrd_data);

               return result;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__obj: {
          call_stack__push(thrd_data, owner);

          bool  called;
          t_any result = obj__try_call__own(thrd_data, mtd__core_init, tid__obj, container, args, 2, owner, &called);
          if (called) {
               call_stack__pop(thrd_data);
               return result;
          }
          ref_cnt__dec(thrd_data, container);

          if (cap.qwords[0] <= hash_map_max_len) [[clang::likely]]
               result = obj__init(thrd_data, cap.qwords[0], owner);
          else result = error__out_of_bounds(thrd_data, owner);

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__table: {
          ref_cnt__dec(thrd_data, container);

          call_stack__push(thrd_data, owner);

          t_any result;
          if (cap.qwords[0] <= hash_map_max_len) [[clang::likely]]
               result = table__init(thrd_data, cap.qwords[0], owner);
          else result = error__out_of_bounds(thrd_data, owner);

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__set: {
          ref_cnt__dec(thrd_data, container);

          call_stack__push(thrd_data, owner);

          t_any result;
          if (cap.qwords[0] <= hash_map_max_len) [[clang::likely]]
               result = set__init(thrd_data, cap.qwords[0], owner);
          else result = error__out_of_bounds(thrd_data, owner);

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          ref_cnt__dec(thrd_data, container);

          call_stack__push(thrd_data, owner);

          t_any result;
          if (cap.qwords[0] <= array_max_len) [[clang::likely]]
               result = array__init(thrd_data, cap.qwords[0], owner);
          else result = error__out_of_bounds(thrd_data, owner);

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_init, tid__custom, container, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, cap);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}