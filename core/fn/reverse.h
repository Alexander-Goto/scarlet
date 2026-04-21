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

core t_any McoreFNreverse(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.reverse";

     t_any const container = args[0];
     switch (container.bytes[15]) {
     case tid__short_string:
          return short_string__reverse(container);
     case tid__short_byte_array:
          return short_byte_array__reverse(container);
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_reverse, tid__obj, container, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array:
          return long_byte_array__reverse__own(thrd_data, container);
     case tid__string:
          return long_string__reverse__own(thrd_data, container);
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__reverse__own(thrd_data, container, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_reverse, tid__custom, container, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] case tid__error:
          return container;

     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, container);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}