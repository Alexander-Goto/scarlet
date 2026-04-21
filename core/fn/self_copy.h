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

core t_any McoreFNself_copy(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.self_copy";

     t_any const container = args[0];
     t_any const idx_from  = args[1];
     t_any const idx_to    = args[2];
     t_any const part_len  = args[3];

     if (container.bytes[15] == tid__error || idx_from.bytes[15] != tid__int || idx_to.bytes[15] != tid__int || part_len.bytes[15] != tid__int) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, idx_from);
               ref_cnt__dec(thrd_data, idx_to);
               ref_cnt__dec(thrd_data, part_len);
               return container;
          }

          if (idx_from.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, idx_to);
               ref_cnt__dec(thrd_data, part_len);
               return idx_from;
          }

          if (idx_to.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, idx_from);
               ref_cnt__dec(thrd_data, part_len);
               return idx_to;
          }

          if (part_len.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, idx_from);
               ref_cnt__dec(thrd_data, idx_to);
               return part_len;
          }

          goto invalid_type_label;
     }

     switch (container.bytes[15]) {
     case tid__short_string: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_string__self_copy(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], part_len.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_byte_array__self_copy(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], part_len.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_byte_array__self_copy__own(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], part_len.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_string__self_copy__own(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], part_len.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__self_copy__own(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], part_len.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_self_copy, tid__obj, container, args, 4, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_self_copy, tid__custom, container, args, 4, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, container);
     ref_cnt__dec(thrd_data, idx_from);
     ref_cnt__dec(thrd_data, idx_to);
     ref_cnt__dec(thrd_data, part_len);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
