#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/char/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNchar_to_uchar(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.char_to_uchar";

     t_any const char_ = args[0];
     switch (char_.bytes[15]) {
     case tid__char:
          u32 const unicode = corsar_code_to_unicode(char_to_code(char_.bytes));
          return unicode == 0 ? null : (const t_any){.structure = {.value = unicode, .type = tid__int}};
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any result = obj__call__any_result__own(thrd_data, mtd__core_char_to_uchar, char_, args, 1, owner);
          if (result.bytes[15] != tid__int && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any result = custom__call__any_result__own(thrd_data, mtd__core_char_to_uchar, char_, args, 1, owner);
          if (result.bytes[15] != tid__int && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]]
     case tid__error:
          return char_;
     [[clang::unlikely]]
     default: {
          ref_cnt__dec(thrd_data, char_);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}
