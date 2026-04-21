#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/error/basic.h"
#include "../struct/string/fn.h"

core t_any McoreFNunslice(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.unslice";

     t_any const container = args[0];
     switch (container.bytes[15]) {
     case tid__short_string: case tid__short_byte_array:
          return container;
     case tid__byte_array:
          return long_byte_array__unslice__own(thrd_data, container);
     case tid__string:
          return long_string__unslice__own(thrd_data, container);
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__unslice__own(thrd_data, container, owner);
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