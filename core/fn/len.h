#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/basic.h"

core t_any McoreFNlen(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.len";

     t_any const container = args[0];
     switch (container.bytes[15]) {
     case tid__short_string:
          return (const t_any){.structure = {.value = short_string__get_len(container), .type = tid__int}};
     case tid__short_byte_array:
          return (const t_any){.structure = {.value = short_byte_array__get_len(container), .type = tid__int}};
     case tid__box: {
          u8 const len = box__get_len(container);
          ref_cnt__dec(thrd_data, container);
          return (const t_any){.structure = {.value = len, .type = tid__int}};
     }
     case tid__obj: {
          bool called;

          call_stack__push(thrd_data, owner);
          t_any const len = obj__try_call__own(thrd_data, mtd__core_len, tid__int, container, args, 1, owner, &called);
          call_stack__pop(thrd_data);

          if (called) return len;

          u64 const len_u64 = obj__get_fields_len(container);
          ref_cnt__dec(thrd_data, container);
          return (const t_any){.structure = {.value = len_u64, .type = tid__int}};
     }
     case tid__table: case tid__set: {
          u64 const len = hash_map__get_len(container);
          ref_cnt__dec(thrd_data, container);
          return (const t_any){.structure = {.value = len, .type = tid__int}};
     }
     case tid__byte_array: case tid__string: case tid__array: {
          u32       const slice_len = slice__get_len(container);
          u64       const len       = slice_len <= slice_max_len ? slice_len : slice_array__get_len(container);

          ref_cnt__dec(thrd_data, container);
          return (const t_any){.structure = {.value = len, .type = tid__int}};
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_len, tid__int, container, args, 1, owner);
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