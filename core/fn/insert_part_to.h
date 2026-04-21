#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

core t_any McoreFNinsert_part_to(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.insert_part_to";

     t_any const part       = args[0];
     t_any const array      = args[1];
     t_any const position   = args[2];
     u8    const part_type  = part.bytes[15];
     u8    const array_type = array.bytes[15];
     if (
          part_type == tid__error || array_type == tid__error || position.bytes[15] != tid__int ||
          !(
               part_type == array_type                                                  ||
               (part_type == tid__short_string     && array_type  == tid__string)       ||
               (part_type == tid__string           && array_type  == tid__short_string) ||
               (part_type == tid__short_byte_array && array_type  == tid__byte_array)   ||
               (part_type == tid__byte_array       && array_type  == tid__short_byte_array)
          )
     ) [[clang::unlikely]] {
          if (part_type == tid__error) {
               ref_cnt__dec(thrd_data, array);
               ref_cnt__dec(thrd_data, position);
               return part;
          }

          if (array_type == tid__error) {
               ref_cnt__dec(thrd_data, part);
               ref_cnt__dec(thrd_data, position);
               return array;
          }

          if (position.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, array);
               ref_cnt__dec(thrd_data, part);
               return position;
          }

          goto invalid_type_label;
     }

     switch (array_type) {
     case tid__short_string: {
          call_stack__push(thrd_data, owner);
          t_any const result = part_type == tid__short_string ?
               insert_part_to__short_str__short_str(thrd_data, array, part, position.qwords[0], owner) :
               insert_part_to__short_str__long_str__own(thrd_data, array, part, position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = part_type == tid__short_byte_array ?
               insert_part_to__short_byte_array__short_byte_array(thrd_data, array, part, position.qwords[0], owner) :
               insert_part_to__short_byte_array__long_byte_array__own(thrd_data, array, part, position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = part_type == tid__short_byte_array ?
               insert_part_to__long_byte_array__short_byte_array__own(thrd_data, array, part, position.qwords[0], owner) :
               insert_part_to__long_byte_array__long_byte_array__own(thrd_data, array, part, position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = part_type == tid__short_string ?
               insert_part_to__long_str__short_str__own(thrd_data, array, part, position.qwords[0], owner) :
               insert_part_to__long_str__long_str__own(thrd_data, array, part, position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__insert_part_to__own(thrd_data, array, part, position.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_insert_part_to, tid__obj, array, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_insert_part_to, tid__custom, array, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, part);
     ref_cnt__dec(thrd_data, array);
     ref_cnt__dec(thrd_data, position);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
