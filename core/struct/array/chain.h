#pragma once

#include "../../common/fn.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../fn/__concat__.h"
#include "basic.h"

static t_any array_chain_concat(t_thrd_data* const thrd_data, u64 const arrays_qty, const t_any* const arrays) {
     u64 result_len = 0;
     i64 biggest_cap = -1;
     u64 biggest_len;
     u64 biggest_cap_idx;
     u64 biggest_new_offset;
     u64 biggest_full_len;
     for (u64 idx = 0; idx < arrays_qty; idx += 1) {
          t_any const array = arrays[idx];

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
               biggest_full_len   = array_array_len;
          }

          result_len += array_len;
     }

     if (result_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", core_concat_fn_name);

     if (result_len == 0) {
          for (u64 idx = 0; idx < arrays_qty; ref_cnt__dec(thrd_data, arrays[idx]), idx += 1);
          return empty_array;
     }

     t_any  result;
     t_any* result_items;
     if (biggest_cap < (i64)result_len) {
          u64 result_cap  = result_len * 3 / 2;
          result_cap      = result_cap > array_max_len ? array_max_len : result_cap;
          result          = array__init(thrd_data, result_cap, core_concat_fn_name);
          result_items    = slice_array__get_items(result);
          biggest_cap_idx = -1;
     } else {
          result                 = arrays[biggest_cap_idx];
          u32 const slice_offset = slice__get_offset(result);
          result_items           = slice_array__get_items(result);

          for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, result_items[idx]), idx += 1);
          for (u64 idx = slice_offset + biggest_len; idx < biggest_full_len; ref_cnt__dec(thrd_data, result_items[idx]), idx += 1);

          memmove(&result_items[biggest_new_offset], &result_items[slice_offset], biggest_len * 16);
     }

     slice_array__set_len(result, result_len);
     result     = slice__set_metadata(result, 0, result_len <= slice_max_len ? result_len : slice_max_len + 1);
     u64 offset = 0;
     for (u64 array_idx = 0; array_idx < arrays_qty; array_idx += 1) {
          if (array_idx == biggest_cap_idx) {
               offset += biggest_len;
               continue;
          }

          t_any const array = arrays[array_idx];

          u32          const slice_offset    = slice__get_offset(array);
          u32          const slice_len       = slice__get_len(array);
          u64          const ref_cnt         = get_ref_cnt(array);
          u64          const array_array_len = slice_array__get_len(array);
          u64          const array_len       = slice_len <= slice_max_len ? slice_len : array_array_len;
          const t_any* const array_items     = slice_array__get_items(array);

          switch (ref_cnt) {
          case 1:
               for (u64 free_idx = 0; free_idx < slice_offset; ref_cnt__dec(thrd_data, array_items[free_idx]), free_idx += 1);
               memcpy(&result_items[offset], &array_items[slice_offset], array_len * 16);
               for (u64 free_idx = slice_offset + array_len; free_idx < array_array_len; ref_cnt__dec(thrd_data, array_items[free_idx]), free_idx += 1);
               free((void*)array.qwords[0]);
               break;
          case 0:
               memcpy(&result_items[offset], &array_items[slice_offset], array_len * 16);
               break;
          default:
               set_ref_cnt(array, ref_cnt - 1);

               for (u64 item_idx = 0; item_idx < array_len; item_idx += 1) {
                    t_any const item = array_items[slice_offset + item_idx];
                    result_items[offset + item_idx] = item;
                    ref_cnt__inc(thrd_data, item, core_concat_fn_name);
               }
          }

          offset += array_len;
     }

     return result;
}
