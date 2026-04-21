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
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"
#include "../struct/token/fn.h"

core t_any McoreFNto_box(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.to_box";

     t_any const container = args[0];
     switch (container.bytes[15]) {
     case tid__short_string: case tid__string: {
          call_stack__push(thrd_data, owner);
          t_any const result = string__to_box__own(thrd_data, container, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__short_byte_array: case tid__byte_array: {
          call_stack__push(thrd_data, owner);
          t_any const result = byte_array__to_box__own(thrd_data, container, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__box:
          return container;
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__to_box__own(thrd_data, container, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__table: {
          call_stack__push(thrd_data, owner);
          t_any const result = table__to_box__own(thrd_data, container, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__set: {
          call_stack__push(thrd_data, owner);
          t_any const result = set__to_box__own(thrd_data, container, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__array: {
          call_stack__push(thrd_data, owner);
          t_any const result = array__to_box__own(thrd_data, container, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_to_box, tid__box, container, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]]
     case tid__error:
          return container;
     [[clang::unlikely]]
     default: {
          ref_cnt__dec(thrd_data, container);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}