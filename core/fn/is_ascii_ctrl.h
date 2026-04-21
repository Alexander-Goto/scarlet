#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"
#include "../struct/char/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNascii_ctrlB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.ascii_ctrl?";

     t_any const char_ = args[0];
     switch (char_.bytes[15]) {
     case tid__char: {
          typedef u32 t_vec __attribute__((ext_vector_type(4)));

          u32 const corsar_code = char_to_code(char_.bytes);
          return __builtin_reduce_or(corsar_code > (const t_vec){0,9,126} & corsar_code < (const t_vec){9,32,128}) != 0 ? bool__true : bool__false;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_is_ascii_ctrl, tid__bool, char_, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_is_ascii_ctrl, tid__bool, char_, args, 1, owner);
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
