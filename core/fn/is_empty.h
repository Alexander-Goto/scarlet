#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/bool/basic.h"
#include "../struct/box/basic.h"
#include "../struct/byte_array/basic.h"
#include "../struct/common/hash_map.h"
#include "../struct/common/slice.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNemptyB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.empty?";

     t_any const container = args[0];
     switch (container.bytes[15]) {
     case tid__short_string:
          return container.bytes[0] == 0 ? bool__true : bool__false;
     case tid__short_byte_array:
          return short_byte_array__get_len(container) == 0 ? bool__true : bool__false;
     case tid__box: {
          bool const is_empty = box__get_len(container) == 0;
          ref_cnt__dec(thrd_data, container);
          return is_empty ? bool__true : bool__false;
     }
     case tid__obj: {
          {
               bool called;
               call_stack__push(thrd_data, owner);
               t_any const result = obj__try_call__own(thrd_data, mtd__core_is_empty, tid__bool, container, args, 1, owner, &called);
               call_stack__pop(thrd_data);

               if (called) return result;
          }

          bool const is_empty = obj__get_fields_len(container) == 0;
          ref_cnt__dec(thrd_data, container);
          return is_empty ? bool__true : bool__false;
     }
     case tid__table: case tid__set: {
          bool const is_empty = hash_map__get_len(container) == 0;
          ref_cnt__dec(thrd_data, container);
          return is_empty ? bool__true : bool__false;
     }
     case tid__byte_array: case tid__string: case tid__array: {
          bool const is_empty = slice__get_len(container) == 0;
          ref_cnt__dec(thrd_data, container);
          return is_empty ? bool__true : bool__false;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_is_empty, tid__bool, container, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] case tid__error:
          return container;
     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, container);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}
