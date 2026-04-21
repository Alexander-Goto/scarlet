#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/char/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNchar_lang(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.char_lang";

     t_any const char_ = args[0];
     switch (char_.bytes[15]) {
     case tid__char: {
          u32 const corsar_code = char_to_code(char_.bytes);
          u16 const lang        = char_lang(corsar_code);
          if (lang == 0) return null;

          t_any string = {};
          memcpy_inline(string.bytes, &lang, 2);
          return string;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any result = obj__call__any_result__own(thrd_data, mtd__core_char_lang, char_, args, 1, owner);
          if (result.bytes[15] != tid__short_string && result.bytes[15] != tid__string && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any result = custom__call__any_result__own(thrd_data, mtd__core_char_lang, char_, args, 1, owner);
          if (result.bytes[15] != tid__short_string && result.bytes[15] != tid__string && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] case tid__error:
          return char_;
     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, char_);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}
