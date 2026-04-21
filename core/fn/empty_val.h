#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/bool/basic.h"
#include "../struct/box/basic.h"
#include "../struct/byte_array/basic.h"
#include "../struct/common/hash_map.h"
#include "../struct/common/slice.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNempty_val(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.empty_val";

     t_any const arg  = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string: case tid__string:
          ref_cnt__dec(thrd_data, arg);
          return (const t_any){};
     case tid__short_byte_array: case tid__byte_array:
          ref_cnt__dec(thrd_data, arg);
          return (const t_any){.bytes = {[15] = tid__short_byte_array}};
     case tid__obj: {
          {
               bool called;

               call_stack__push(thrd_data, owner);
               t_any const result = obj__try_call__own(thrd_data, mtd__core_empty_val, tid__obj, arg, args, 1, owner, &called);
               call_stack__pop(thrd_data);

               if (called) return result;
          }

          const t_any* const mtds = obj__get_mtds(arg);
          if (mtds->bytes[15] == tid__null) {
               ref_cnt__dec(thrd_data, arg);
               return empty_obj;
          }

          t_any const new_obj     = hash_map__set_len(obj__init(thrd_data, 1, owner), 1);
          *obj__get_mtds(new_obj) = *mtds;

          call_stack__push(thrd_data, owner);
          ref_cnt__inc(thrd_data, *mtds, owner);
          call_stack__pop(thrd_data);

          ref_cnt__dec(thrd_data, arg);
          return new_obj;
     }
     case tid__table:
          ref_cnt__dec(thrd_data, arg);
          return empty_table;
     case tid__set:
          ref_cnt__dec(thrd_data, arg);
          return empty_set;
     case tid__array:
          ref_cnt__dec(thrd_data, arg);
          return empty_array;
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_empty_val, tid__custom, arg, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] case tid__error:
          return arg;
     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, arg);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}