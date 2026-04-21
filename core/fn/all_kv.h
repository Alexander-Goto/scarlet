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
core t_any McoreFNall_kv(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.all_kv";

     t_any const container      = args[0];
     t_any const default_result = args[1];
     t_any const fn             = args[2];

     if (container.bytes[15] == tid__error || default_result.bytes[15] != tid__bool || !(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn)) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, default_result);
               ref_cnt__dec(thrd_data, fn);
               return container;
          }

          if (default_result.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, fn);
               return default_result;
          }

          if (fn.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, default_result);
               return fn;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string: case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = string__all_kv__own(thrd_data, container, default_result, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = byte_array__all_kv__own(thrd_data, container, default_result, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__all_kv__own(thrd_data, container, default_result, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__table: {
          call_stack__push(thrd_data, owner);
          t_any const result = table__all_kv__own(thrd_data, container, default_result, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__all_kv__own(thrd_data, container, default_result, fn, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_all_kv, tid__bool, container, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, default_result);
     ref_cnt__dec(thrd_data, fn);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
