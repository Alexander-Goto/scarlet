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
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

[[gnu::hot]]
core t_any McoreFN__slice__(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.([,])";

     t_any const container = args[0];
     t_any const idx_from  = args[1];
     t_any const idx_to    = args[2];

     if (
          container.bytes[15] == tid__error     ||
          idx_from.bytes[15]  != tid__int       ||
          idx_to.bytes[15]    != tid__int       ||
          (i64)idx_from.qwords[0] < 0           ||
          (i64)idx_from.qwords[0] > (i64)idx_to.qwords[0]
     ) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, idx_from);
               ref_cnt__dec(thrd_data, idx_to);
               return container;
          }

          if (idx_from.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, idx_to);
               return idx_from;
          }

          if (idx_to.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, idx_from);
               return idx_to;
          }

          if (idx_from.bytes[15] != tid__int || idx_to.bytes[15] != tid__int) goto invalid_type_label;

          ref_cnt__dec(thrd_data, container);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_range(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     switch (container.bytes[15]) {
     case tid__short_string: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_string__slice(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = short_byte_array__slice(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__box: {
          call_stack__push(thrd_data, owner);
          t_any const result = box__slice__own(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core___slice__, tid__obj, container, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_byte_array__slice__own(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = long_string__slice__own(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__slice__own(thrd_data, container, idx_from.qwords[0], idx_to.qwords[0], owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core___slice__, tid__custom, container, args, 3, owner);
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

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
