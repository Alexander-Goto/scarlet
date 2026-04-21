#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/fn.h"
#include "../struct/string/fn.h"

core t_any McoreFNstr_to_ustr(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.str_to_ustr";

     t_any const string = args[0];
     t_any       result;
     call_stack__push(thrd_data, owner);
     if (!(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string)) [[clang::unlikely]] {
          if (string.bytes[15] == tid__error) {
               call_stack__pop(thrd_data);
               return string;
          }

          ref_cnt__dec(thrd_data, string);
          result = error__invalid_type(thrd_data, owner);
     } else result = str_to_ustr__own(thrd_data, string, owner);
     call_stack__pop(thrd_data);

     return result;
}