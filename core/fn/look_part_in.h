#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/box/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/null/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"

static inline t_any look_part_in(t_thrd_data* const thrd_data, const t_any* const args, const char* const owner) {
     t_any const part       = args[0];
     t_any const array      = args[1];
     u8    const part_type  = part.bytes[15];
     u8    const array_type = array.bytes[15];
     if (
          part_type == tid__error || array_type == tid__error ||
          !(
               part_type == array_type                                                 ||
               (part_type == tid__short_string     && array_type == tid__string)       ||
               (part_type == tid__string           && array_type == tid__short_string) ||
               (part_type == tid__short_byte_array && array_type == tid__byte_array)   ||
               (part_type == tid__byte_array       && array_type == tid__short_byte_array)
          )
     ) [[clang::unlikely]] {
          if (part_type == tid__error) {
               ref_cnt__dec(thrd_data, array);
               return part;
          }

          if (array_type == tid__error) {
               ref_cnt__dec(thrd_data, part);
               return array;
          }

          goto invalid_type_label;
     }

     switch (array_type) {
     case tid__short_string:
          if (part_type == tid__string) {
               ref_cnt__dec(thrd_data, part);
               return null;
          }
          return look_part_in__short_string__short_string(array, part);
     case tid__short_byte_array:
          if (part_type == tid__byte_array) {
               ref_cnt__dec(thrd_data, part);
               return null;
          }
          return look_part_in__short_byte_array__short_byte_array(array, part);
     case tid__obj: {
          t_any const result = obj__call__any_result__own(thrd_data, mtd__core_look_part_in, array, args, 2, owner);
          if (result.bytes[15] != tid__int && result.bytes[15] != tid__null) [[clang::unlikely]] {
               if (result.bytes[15] == tid__error) return result;

               ref_cnt__dec(thrd_data, result);
               return error__invalid_type(thrd_data, owner);
          }

          return result;
     }
     case tid__byte_array:
          return look_part_in__long_byte_array__byte_array__own(thrd_data, array, part);
     case tid__string:
          return look_part_in__long_string__string__own(thrd_data, array, part);
     case tid__array:
          return array__look_part_in__own(thrd_data, array, part, owner);
     case tid__custom: {
          t_any const result = custom__call__any_result__own(thrd_data, mtd__core_look_part_in, array, args, 2, owner);
          if (result.bytes[15] != tid__int && result.bytes[15] != tid__null) [[clang::unlikely]] {
               if (result.bytes[15] == tid__error) return result;

               ref_cnt__dec(thrd_data, result);
               return error__invalid_type(thrd_data, owner);
          }

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, part);
     ref_cnt__dec(thrd_data, array);
     return error__invalid_type(thrd_data, owner);
}

[[gnu::hot]]
core inline t_any McoreFNlook_part_in(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.look_part_in";

     call_stack__push(thrd_data, owner);
     t_any const result = look_part_in(thrd_data, args, owner);
     call_stack__pop(thrd_data);

     return result;
}
