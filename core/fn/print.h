#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/box/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/char/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/null/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/obj/mtd.h"
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"
#include "../struct/time/fn.h"
#include "../struct/token/fn.h"

[[clang::noinline]]
static t_any common_print(t_thrd_data* const thrd_data, t_any const arg, const char* const owner) {
     switch (arg.bytes[15]) {
     case tid__short_string: case tid__string:
          return string__print(thrd_data, arg, owner, stdout);
     case tid__null:
          if (fwrite("null", 1, 4, stdout) != 4) [[clang::unlikely]] goto cant_print_label;
          return null;
     case tid__token:
          return token__print(thrd_data, arg, owner, stdout);
     case tid__bool:
          if (fprintf(stdout, "%s", arg.bytes[0] == 1 ? "true" : "false") < 0) [[clang::unlikely]] goto cant_print_label;
          return null;
     case tid__byte:
          if (fprintf(stdout, "%02x", (unsigned int)arg.bytes[0]) < 0) [[clang::unlikely]] goto cant_print_label;
          return null;
     case tid__char:
          return char__print(thrd_data, arg, owner, stdout);
     case tid__int:
          if (fprintf(stdout, "%"PRIi64, (i64)arg.qwords[0]) < 0) [[clang::unlikely]] goto cant_print_label;
          return null;
     case tid__time:
          return time__print(thrd_data, arg, owner, stdout);
     case tid__float: {
          typedef char t_vec __attribute__((ext_vector_type(16), aligned(1)));

          if (__builtin_isnan(arg.floats[0])) {
               if (fwrite("nan", 1, 3, stdout) != 3) [[clang::unlikely]] goto cant_print_label;
               return null;
          }

          char buffer[327];
          int  string_len = snprintf(buffer, 327, "%.15f", arg.floats[0]);
          if (string_len >= 17) [[clang::likely]] {
               int const trailing_zeros_len = 16 - __builtin_reduce_max((*(const t_vec*)&buffer[string_len-16] != '0') & (const t_vec){2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});

               buffer[string_len - 16] = '.';
               string_len             -= trailing_zeros_len;
          }

          if (fwrite(buffer, 1, string_len, stdout) != string_len) [[clang::unlikely]] goto cant_print_label;
          return null;
     }
     case tid__short_byte_array:
          return short_byte_array__print(thrd_data, arg, owner, stdout);
     case tid__box:
          return box__print(thrd_data, arg, owner);
     case tid__obj:
          return obj__print(thrd_data, arg, owner);
     case tid__table:
          return table__print(thrd_data, arg, owner);
     case tid__set:
          return set__print(thrd_data, arg, owner);
     case tid__byte_array:
          return long_byte_array__print(thrd_data, arg, owner, stdout);
     case tid__array:
          return array__print(thrd_data, arg, owner);
     case tid__custom:
          ref_cnt__inc(thrd_data, arg, owner);
          return custom__call__own(thrd_data, mtd__core_print, tid__null, arg, &arg, 1, owner);

     [[clang::unlikely]] default:
          return error__invalid_type(thrd_data, owner);
     }

     cant_print_label:
     return error__cant_print(thrd_data, owner);
}

core t_any McoreFNprint(t_thrd_data* const thrd_data, const t_any* const args) {
     t_any const arg = args[0];
     if (arg.bytes[15] == tid__error) [[clang::unlikely]] return arg;

     const char* const owner = "function - core.print";

     call_stack__push(thrd_data, owner);

     t_any result = common_print(thrd_data, arg, owner);

     assert(result.bytes[15] == tid__null || result.bytes[15] == tid__error);

     ref_cnt__dec(thrd_data, arg);

     if (result.bytes[15] == tid__null && fflush(stdout) != 0)
          result = error__cant_print(thrd_data, owner);

     call_stack__pop(thrd_data);

     return result;
}

core t_any McoreFNprintln(t_thrd_data* const thrd_data, const t_any* const args) {
     t_any const arg = args[0];
     if (arg.bytes[15] == tid__error) [[clang::unlikely]] return arg;

     const char* const owner = "function - core.println";

     call_stack__push(thrd_data, owner);

     t_any result = common_print(thrd_data, arg, owner);

     assert(result.bytes[15] == tid__null || result.bytes[15] == tid__error);

     ref_cnt__dec(thrd_data, arg);

     if (result.bytes[15] == tid__null && fwrite("\n", 1, 1, stdout) != 1)
          result = error__cant_print(thrd_data, owner);

     call_stack__pop(thrd_data);

     return result;
}
