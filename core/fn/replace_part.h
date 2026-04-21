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

[[gnu::hot]]
core t_any McoreFNreplace_part(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.replace_part";

     t_any const container      = args[0];
     t_any const old_part       = args[1];
     t_any const new_part       = args[2];
     u8    const container_type = container.bytes[15];
     u8    const old_part_type  = old_part.bytes[15];
     u8    const new_part_type  = new_part.bytes[15];
     if (
          container.bytes[15] == tid__error || old_part_type == tid__error || new_part_type == tid__error ||
          !(
               (container_type == old_part_type && container_type == new_part_type) ||
               (
                    (container_type == tid__short_string || container_type == tid__string) &&
                    (old_part_type  == tid__short_string || old_part_type  == tid__string) &&
                    (new_part_type  == tid__short_string || new_part_type  == tid__string)
               ) ||
               (
                    (container_type == tid__short_byte_array || container_type == tid__byte_array) &&
                    (old_part_type  == tid__short_byte_array || old_part_type  == tid__byte_array) &&
                    (new_part_type  == tid__short_byte_array || new_part_type  == tid__byte_array)
               )
          )
     ) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, old_part);
               ref_cnt__dec(thrd_data, new_part);
               return container;
          }

          if (old_part_type == tid__error) {
               ref_cnt__dec(thrd_data, new_part);
               ref_cnt__dec(thrd_data, container);
               return old_part;
          }

          if (new_part_type == tid__error) {
               ref_cnt__dec(thrd_data, old_part);
               ref_cnt__dec(thrd_data, container);
               return new_part;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string: case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = string__replace_part__own(thrd_data, container, old_part, new_part, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = byte_array__replace_part__own(thrd_data, container, old_part, new_part, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_replace_part, tid__obj, container, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__replace_part__own(thrd_data, container, old_part, new_part, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_replace_part, tid__custom, container, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, old_part);
     ref_cnt__dec(thrd_data, new_part);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
