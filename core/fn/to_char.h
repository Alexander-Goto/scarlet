#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

core t_any McoreFNto_char(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.to_char";

     t_any const arg = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string: {
          if (short_string__get_len(arg) != 1) return null;

          t_any char_ = {.bytes = {[15] = tid__char}};
          ctf8_char_to_corsar_char(arg.bytes, char_.bytes);
          return char_;
     }
     case tid__byte: {
          if (arg.bytes[0] == 0) return null;

          t_any char_ = {.bytes = {[15] = tid__char}};
          char_from_code(char_.bytes, arg.bytes[0]);
          return char_;
     }
     case tid__char:
          return arg;
     case tid__int: {
          if (arg.qwords[0] > 0xffff'ffffuwb || !corsar_code_is_correct(arg.qwords[0])) return null;

          t_any char_ = {.bytes = {[15] = tid__char}};
          char_from_code(char_.bytes, arg.qwords[0]);
          return char_;
     }
     case tid__float: {
          double const float_ = arg.floats[0];
          if (!(float_ > 0.0 && float_ <= 4'294'967'296.0) || !corsar_code_is_correct(float_)) return null;

          t_any char_ = {.bytes = {[15] = tid__char}};
          char_from_code(char_.bytes, __builtin_trunc(float_));
          return char_;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);

          t_any result = obj__call__any_result__own(thrd_data, mtd__core_to_char, arg, args, 1, owner);
          if (result.bytes[15] != tid__char && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);

          t_any result = custom__call__any_result__own(thrd_data, mtd__core_to_char, arg, args, 1, owner);
          if (result.bytes[15] != tid__char && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string:
          ref_cnt__dec(thrd_data, arg);
          return null;

     [[clang::unlikely]]
     case tid__error:
          return arg;
     [[clang::unlikely]]
     default: {
          ref_cnt__dec(thrd_data, arg);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}
