#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNoddB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.odd?";

     t_any const num = args[0];
     switch (num.bytes[15]) {
     case tid__byte: case tid__int:
          return (num.bytes[0] & 1) == 1 ? bool__true : bool__false;
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any result = obj__call__own(thrd_data, mtd__core_is_even, tid__bool, num, args, 1, owner);
          call_stack__pop(thrd_data);

          if (result.bytes[15] == tid__error) [[clang::unlikely]] return result;

          result.bytes[0] ^= 1;
          return result;
     }
     case tid__custom:{
          call_stack__push(thrd_data, owner);
          t_any result = custom__call__own(thrd_data, mtd__core_is_even, tid__bool, num, args, 1, owner);
          call_stack__pop(thrd_data);

          if (result.bytes[15] == tid__error) [[clang::unlikely]] return result;

          result.bytes[0] ^= 1;
          return result;
     }

     [[clang::unlikely]] case tid__error:
          return num;
     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, num);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}
