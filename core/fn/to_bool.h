#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/custom/fn.h"

core t_any McoreFNto_bool(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.to_bool";

     t_any arg = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string:
          arg.vector |= arg.vector > 64 & arg.vector < 91 & 32;
          switch (arg.raw_bits) {
          case 0x65736c6166uwb: case 0x6f6euwb: case 0x66666fuwb:
               return bool__false;
          case 0x65757274uwb: case 0x736579uwb: case 0x6e6fuwb:
               return bool__true;
          default:
               return null;
          }
     case tid__bool:
          return arg;
     case tid__byte: case tid__int:
          switch (arg.qwords[0]) {
          case 0: case 1:
               arg.bytes[15] = tid__bool;
               return arg;
          default:
               return null;
          }
     case tid__token:
          switch (arg.raw_bits) {
          case 0x9a97dceb4a6252ea8c500f4b94e404fuwb: case 0x9bf8b3544f665c02d6dd5e6d4172947uwb: case 0x9ac69ea2aca586329612f099ef8824fuwb:
               return bool__false;
          case 0x9aa79ec6ee7968b08f79b7e17ab8bd3uwb: case 0x9e68e90179dd7b22eedebe8665cee2fuwb: case 0x9bd774f6df49e69acfd1ea04f3c2c1fuwb:
               return bool__true;
          default:
               return null;
          }
     case tid__obj: {
          call_stack__push(thrd_data, owner);

          t_any result = obj__call__any_result__own(thrd_data, mtd__core_to_bool, arg, args, 1, owner);
          if (result.bytes[15] != tid__bool && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);

          t_any result = custom__call__any_result__own(thrd_data, mtd__core_to_bool, arg, args, 1, owner);
          if (result.bytes[15] != tid__bool && result.bytes[15] != tid__null && result.bytes[15] != tid__error) [[clang::unlikely]] {
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