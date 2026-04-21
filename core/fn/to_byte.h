#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"
#include "../struct/token/fn.h"

core t_any McoreFNto_byte(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.to_byte";

     t_any arg = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string:
          if (!(
               (
                    (arg.bytes[0] >= '0' && arg.bytes[0] <= '9') ||
                    (arg.bytes[0] >= 'A' && arg.bytes[0] <= 'F') ||
                    (arg.bytes[0] >= 'a' && arg.bytes[0] <= 'f')
               ) && (
                    (arg.bytes[1] >= '0' && arg.bytes[1] <= '9') ||
                    (arg.bytes[1] >= 'A' && arg.bytes[1] <= 'F') ||
                    (arg.bytes[1] >= 'a' && arg.bytes[1] <= 'f')
               ) && arg.bytes[2] == 0
          )) return null;

          return (const t_any){.bytes = {
               (arg.bytes[0] >= '0' && arg.bytes[0] <= '9') * ((u8)(arg.bytes[0] - '0') << 4)      |
               (arg.bytes[0] >= 'A' && arg.bytes[0] <= 'F') * ((u8)(arg.bytes[0] - 'A' + 10) << 4) |
               (arg.bytes[0] >= 'a' && arg.bytes[0] <= 'f') * ((u8)(arg.bytes[0] - 'a' + 10) << 4) |
               (arg.bytes[1] >= '0' && arg.bytes[1] <= '9') * (arg.bytes[1] - '0')                 |
               (arg.bytes[1] >= 'A' && arg.bytes[1] <= 'F') * (arg.bytes[1] - 'A' + 10)            |
               (arg.bytes[1] >= 'a' && arg.bytes[1] <= 'f') * (arg.bytes[1] - 'a' + 10)
          , [15] = tid__byte}};
     case tid__byte:
          return arg;
     case tid__char:
          if (char_to_code(arg.bytes) > 255) return null;

          arg.qwords[0] = arg.bytes[2];
          arg.bytes[15] = tid__byte;
          return arg;
     case tid__int:
          if (arg.qwords[0] > 255) return null;

          arg.bytes[15] = tid__byte;
          return arg;
     case tid__float:
          double const float_ = arg.floats[0];
          if (!(float_ > -1.0 && float_ < 256.0)) return null;

          arg.qwords[0] = __builtin_trunc(float_);
          arg.bytes[15] = tid__byte;
          return arg;
     case tid__short_byte_array:
          if (short_byte_array__get_len(arg) != 1) return null;

          *(u16*)&arg.bytes[14] = (u16)tid__byte << 8;
          return arg;
     case tid__obj: {
          call_stack__push(thrd_data, owner);

          t_any result = obj__call__any_result__own(thrd_data, mtd__core_to_byte, arg, args, 1, owner);
          if (result.bytes[15] != tid__byte && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);

          t_any result = custom__call__any_result__own(thrd_data, mtd__core_to_byte, arg, args, 1, owner);
          if (result.bytes[15] != tid__byte && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: case tid__byte_array:
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
