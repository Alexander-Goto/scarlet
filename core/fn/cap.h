#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/byte_array/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/basic.h"

core t_any McoreFNcap(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.cap";

     t_any const container = args[0];
     switch (container.bytes[15]) {
     case tid__short_string:
          return (const t_any){.structure = {.value = short_string__get_len(container), .type = tid__int}};
     case tid__short_byte_array:
          return (const t_any){.structure = {.value = short_byte_array__get_len(container), .type = tid__int}};
     case tid__byte_array: case tid__string: case tid__array: {
          u64 const cap = slice_array__get_cap(container);
          ref_cnt__dec(thrd_data, container);
          return (const t_any){.structure = {.value = cap, .type = tid__int}};
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_cap, tid__int, container, args, 1, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_cap, tid__int, container, args, 1, owner);
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
