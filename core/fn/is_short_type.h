#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"
#include "../struct/error/fn.h"

core t_any McoreFNshort_typeB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.short_type?";

     t_any const arg = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string: case tid__short_byte_array:
          return bool__true;
     case tid__string: case tid__byte_array:
          ref_cnt__dec(thrd_data, arg);
          return bool__false;

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