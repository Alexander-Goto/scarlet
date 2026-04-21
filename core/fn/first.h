#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/byte_array/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"

core t_any McoreFNfirst(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.first";

     t_any container = args[0];
     switch (container.bytes[15]) {
     case tid__short_string: {
          if (container.bytes[0] == 0) [[clang::unlikely]] goto out_of_bounds_label;

          ctf8_char_to_corsar_char(container.bytes, container.bytes);
          memset_inline(&container.bytes[3], 0, 12);
          container.bytes[15] = tid__char;
          return container;
     }
     case tid__short_byte_array:
          if (short_byte_array__get_len(container) == 0) [[clang::unlikely]] goto out_of_bounds_label;

          container.qwords[0] = container.bytes[0];
          container.qwords[1] = (u64)tid__byte << 56;

          return container;
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any result = obj__call__any_result__own(thrd_data, mtd__core_first, container, args, 1, owner);
          if (!type_is_common_or_error(result.bytes[15])) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: {
          u64 const ref_cnt = get_ref_cnt(container);
          if (ref_cnt > 1) set_ref_cnt(container, ref_cnt - 1);

          u32       const slice_offset = slice__get_offset(container);
          const u8* const bytes        = slice_array__get_items(container);

          t_any const result = {.bytes = {bytes[slice_offset], [15] = tid__byte}};

          if (ref_cnt == 1) free((void*)container.qwords[0]);

          return result;
     }
     case tid__string: {
          u64 const ref_cnt = get_ref_cnt(container);
          if (ref_cnt > 1) set_ref_cnt(container, ref_cnt - 1);

          u32       const slice_offset = slice__get_offset(container);
          const u8* const chars        = slice_array__get_items(container);

          t_any result = {.bytes = {[15] = tid__char}};
          memcpy_inline(result.bytes, &chars[slice_offset * 3], 3);

          if (ref_cnt == 1) free((void*)container.qwords[0]);

          return result;
     }
     case tid__array: {
          u32          const slice_offset = slice__get_offset(container);
          u32          const slice_len    = slice__get_len(container);
          u64          const ref_cnt      = get_ref_cnt(container);
          u64          const array_len    = slice_array__get_len(container);
          u64          const len          = slice_len <= slice_max_len ? slice_len : array_len;
          const t_any* const items        = slice_array__get_items(container);

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (len == 0) [[clang::unlikely]] goto out_of_bounds_label;

          if (ref_cnt > 1) set_ref_cnt(container, ref_cnt - 1);

          t_any const result = items[slice_offset];

          call_stack__push(thrd_data, owner);
          ref_cnt__inc(thrd_data, result, owner);
          call_stack__pop(thrd_data);

          if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, container);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any result = custom__call__any_result__own(thrd_data, mtd__core_first, container, args, 1, owner);
          if (!type_is_common_or_error(result.bytes[15])) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
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

     out_of_bounds_label:
     ref_cnt__dec(thrd_data, container);

     call_stack__push(thrd_data, owner);
     t_any const result = error__out_of_bounds(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}