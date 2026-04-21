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
#include "../struct/string/fn.h"

core t_any McoreFNrepeat(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.repeat";

     t_any const part  = args[0];
     t_any const count = args[1];
     if (part.bytes[15] == tid__error || count.bytes[15] != tid__int || (i64)count.qwords[0] < 0) [[clang::unlikely]] {
          if (part.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, count);
               return part;
          }

          if (count.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, part);
               return count;
          }

          if (count.bytes[15] != tid__int) goto invalid_type_label;

          ref_cnt__dec(thrd_data, part);

          call_stack__push(thrd_data, owner);
          t_any const result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     switch (part.bytes[15]) {
     case tid__short_string: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_string__repeat(thrd_data, part, count.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_byte_array__repeat(thrd_data, part, count.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_repeat, tid__obj, part, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_byte_array__repeat__own(thrd_data, part, count.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_string__repeat__own(thrd_data, part, count.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__repeat__own(thrd_data, part, count.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_repeat, tid__custom, part, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, part);
     ref_cnt__dec(thrd_data, count);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
