#pragma once

#include "../../common/fn.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../fn/__concat__.h"
#include "basic.h"

static t_any byte_array_chain_concat(t_thrd_data* const thrd_data, u64 const arrays_qty, const t_any* const arrays) {
     u64 result_len  = 0;
     i64 biggest_cap = -1;
     u64 biggest_len;
     u64 biggest_cap_idx;
     u64 biggest_new_offset;
     for (u64 idx = 0; idx < arrays_qty; idx += 1) {
          t_any const array = arrays[idx];
          if (array.bytes[15] == tid__short_byte_array)
               result_len += short_byte_array__get_len(array);
          else {
               u32 const slice_len       = slice__get_len(array);
               u64 const ref_cnt         = get_ref_cnt(array);
               u64 const array_array_len = slice_array__get_len(array);
               u64 const array_array_cap = slice_array__get_cap(array);
               u64 const array_len       = slice_len <= slice_max_len ? slice_len : array_array_len;

               if (ref_cnt == 1 && biggest_cap < array_array_cap) {
                    biggest_cap        = array_array_cap;
                    biggest_len        = array_len;
                    biggest_cap_idx    = idx;
                    biggest_new_offset = result_len;
               }

               result_len += array_len;
          }
     }

     if (result_len < 15) {
          t_any result = arrays[0];
          u8    offset = short_byte_array__get_len(result);
          for (u64 idx = 1; idx < arrays_qty; idx += 1) {
               t_any    array     = arrays[idx];
               u8 const array_len = short_byte_array__get_len(array);
               result.raw_bits   |= array.raw_bits << offset * 8;
               offset            += array_len;
          }

          result.bytes[15] = tid__short_byte_array;
          result           = short_byte_array__set_len(result, result_len);

          return result;
     }

     if (result_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", core_concat_fn_name);

     t_any result;
     u8*   result_bytes;
     if (biggest_cap < (i64)result_len) {
          u64 result_cap  = result_len * 3 / 2;
          result_cap      = result_cap > array_max_len ? array_max_len : result_cap;
          result          = long_byte_array__new(result_cap);
          result_bytes    = slice_array__get_items(result);
          biggest_cap_idx = -1;
     } else {
          result                 = arrays[biggest_cap_idx];
          u32 const slice_offset = slice__get_offset(result);
          result_bytes           = slice_array__get_items(result);
          memmove(&result_bytes[biggest_new_offset], &result_bytes[slice_offset], biggest_len);
     }

     slice_array__set_len(result, result_len);
     result     = slice__set_metadata(result, 0, result_len <= slice_max_len ? result_len : slice_max_len + 1);
     u64 offset = 0;
     for (u64 idx = 0; idx < arrays_qty; idx += 1) {
          if (idx == biggest_cap_idx) {
               offset += biggest_len;
               continue;
          }

          t_any const array = arrays[idx];
          if (array.bytes[15] == tid__short_byte_array) {
               u8 const array_len = short_byte_array__get_len(array);
               memcpy_le_16(&result_bytes[offset], array.bytes, array_len);
               offset += array_len;
          } else {
               u32 const slice_offset    = slice__get_offset(array);
               u32 const slice_len       = slice__get_len(array);
               u64 const ref_cnt         = get_ref_cnt(array);
               u64 const array_array_len = slice_array__get_len(array);
               u64 const array_len       = slice_len <= slice_max_len ? slice_len : array_array_len;

               if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

               memcpy(&result_bytes[offset], &((u8*)slice_array__get_items(array))[slice_offset], array_len);
               offset += array_len;

               if (ref_cnt == 1) free((void*)array.qwords[0]);
          }
     }

     return result;
}