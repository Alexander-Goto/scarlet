#pragma once

#include "../../common/fn.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../fn/__concat__.h"
#include "../char/basic.h"
#include "basic.h"

static t_any string_chain_concat(t_thrd_data* const thrd_data, u64 const strings_qty, const t_any* const strings) {
     u64 result_len  = 0;
     u8  short_size  = 0;
     i64 biggest_cap = -1;
     u64 biggest_len;
     u64 biggest_cap_idx;
     u64 biggest_new_offset;
     for (u64 idx = 0; idx < strings_qty; idx += 1) {
          t_any const string = strings[idx];
          if (string.bytes[15] == tid__short_string) {
               result_len += short_string__get_len(string);
               short_size += short_size < 16 ? short_string__get_size(string) : 0;
          } else {
               u32 const slice_len  = slice__get_len(string);
               u64 const ref_cnt    = get_ref_cnt(string);
               u64 const array_len  = slice_array__get_len(string);
               u64 const array_cap  = slice_array__get_cap(string);
               u64 const string_len = slice_len <= slice_max_len ? slice_len : array_len;
               short_size           = 16;

               if (ref_cnt == 1 && biggest_cap < array_cap) {
                    biggest_cap        = array_cap;
                    biggest_len        = string_len;
                    biggest_cap_idx    = idx;
                    biggest_new_offset = result_len;
               }

               result_len += string_len;
          }
     }

     if (short_size < 16) {
          t_any result = strings[0];
          u8    offset = short_string__get_size(result);
          for (u64 idx = 1; idx < strings_qty; idx += 1) {
               t_any const string = strings[idx];
               result.raw_bits   |= string.raw_bits << offset * 8;
               offset            += short_string__get_size(string);
          }

          return result;
     }

     if (result_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", core_concat_fn_name);

     t_any result;
     u8*   result_chars;
     if (biggest_cap < (i64)result_len) {
          u64 result_cap  = result_len * 3 / 2;
          result_cap      = result_cap > array_max_len ? array_max_len : result_cap;
          result          = long_string__new(result_cap);
          result_chars    = slice_array__get_items(result);
          biggest_cap_idx = -1;
     } else {
          result                 = strings[biggest_cap_idx];
          u32 const slice_offset = slice__get_offset(result);
          result_chars           = slice_array__get_items(result);
          memmove(&result_chars[biggest_new_offset * 3], &result_chars[slice_offset * 3], biggest_len * 3);
     }

     slice_array__set_len(result, result_len);
     result     = slice__set_metadata(result, 0, result_len <= slice_max_len ? result_len : slice_max_len + 1);
     u64 offset = 0;
     for (u64 idx = 0; idx < strings_qty; idx += 1) {
          if (idx == biggest_cap_idx) {
               offset += biggest_len * 3;
               continue;
          }

          t_any const string = strings[idx];
          if (string.bytes[15] == tid__short_string) {
               u8 const string_len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, &result_chars[offset]);
               offset             += string_len * 3;
          } else {
               u32 const slice_offset = slice__get_offset(string);
               u32 const slice_len    = slice__get_len(string);
               u64 const ref_cnt      = get_ref_cnt(string);
               u64 const array_len    = slice_array__get_len(string);
               u64 const string_len   = slice_len <= slice_max_len ? slice_len : array_len;

               if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

               memcpy(&result_chars[offset], &((u8*)slice_array__get_items(string))[slice_offset * 3], string_len * 3);
               offset += string_len * 3;

               if (ref_cnt == 1) free((void*)string.qwords[0]);
          }
     }

     return result;
}