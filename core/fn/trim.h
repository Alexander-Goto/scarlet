#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

core t_any McoreFNtrim(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.trim";

     t_any const string = args[0];
     switch (string.bytes[15]) {
     case tid__short_string:
          return short_string__trim(string);
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_trim, tid__obj, string, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string:
          return long_string__trim__own(thrd_data, string);
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_trim, tid__custom, string, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]]
     case tid__error:
          return string;
     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, string);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}