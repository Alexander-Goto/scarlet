#pragma once

#include "../../common/bool_vectors.h"
#include "../../common/const.h"
#include "../../common/corsar.h"
#include "../../common/fn.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../struct/array/basic.h"
#include "../../struct/bool/basic.h"
#include "../../struct/box/basic.h"
#include "../../struct/breaker/basic.h"
#include "../../struct/byte_array/basic.h"
#include "../../struct/char/basic.h"
#include "../../struct/common/fn_struct.h"
#include "../../struct/common/slice.h"
#include "../../struct/token/basic.h"
#include "basic.h"
#include "cvt.h"

static inline t_any concat__short_str__short_str(t_any left, t_any const right) {
     assert(left.bytes[15] == tid__short_string);
     assert(right.bytes[15] == tid__short_string);

     u8 const left_size  = short_string__get_size(left);
     u8 const right_size = short_string__get_size(right);
     if (left_size + right_size < 16) {
          left.raw_bits |= right.raw_bits << left_size * 8;
          return left;
     }

     u8 const left_len = short_string__get_len(left);
     u8 const new_len  = left_len + short_string__get_len(right);
     t_any    string   = long_string__new(new_len);
     string            = slice__set_metadata(string, 0, new_len);
     slice_array__set_len(string, new_len);

     u8* const chars = slice_array__get_items(string);
     ctf8_str_ze_lt16_to_corsar_chars(left.bytes, chars);
     ctf8_str_ze_lt16_to_corsar_chars(right.bytes, &chars[left_len * 3]);
     return string;
}

static inline t_any concat__short_str__long_str__own(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner) {
     assert(left.bytes[15] == tid__short_string);
     assume(right.bytes[15] == tid__string);

     u64 const left_len = short_string__get_len(left);
     if (left_len == 0) return right;

     u32 const right_slice_len    = slice__get_len(right);
     u32 const right_slice_offset = slice__get_offset(right);
     u64 const right_array_len    = slice_array__get_len(right);
     u64 const right_ref_cnt      = get_ref_cnt(right);
     u64 const right_len          = right_slice_len <= slice_max_len ? right_slice_len : right_array_len;

     assert(right_len <= slice_max_len || right_slice_offset == 0);

     u64 const new_len = left_len + right_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     t_any string;
     if (right_ref_cnt == 1) {
          u64 const right_cap = slice_array__get_cap(right);
          string              = right;
          slice_array__set_len(string, new_len);
          string              = slice__set_metadata(string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (right_cap < new_len) {
               string.qwords[0]  = (u64)realloc((u8*)string.qwords[0], new_cap * 3 + 16);
               slice_array__set_cap(string, new_cap);
          }

          u8* const chars = slice_array__get_items(string);
          if (right_slice_offset != left_len) memmove(&chars[left_len * 3], &chars[right_slice_offset * 3], right_len * 3);
          ctf8_str_ze_lt16_to_corsar_chars(left.bytes, chars);
     } else {
          if (right_ref_cnt != 0) set_ref_cnt(right, right_ref_cnt - 1);

          string              = long_string__new(new_len);
          slice_array__set_len(string, new_len);
          string              = slice__set_metadata(string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          u8* const new_chars = slice_array__get_items(string);
          ctf8_str_ze_lt16_to_corsar_chars(left.bytes, new_chars);
          memcpy(&new_chars[left_len * 3], &((u8*)slice_array__get_items(right))[right_slice_offset * 3], right_len * 3);
     }

     return string;
}

static inline t_any concat__long_str__short_str__own(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner) {
     assume(left.bytes[15] == tid__string);
     assert(right.bytes[15] == tid__short_string);

     u64 const right_len = short_string__get_len(right);
     if (right_len == 0) return left;

     u32 const left_slice_len    = slice__get_len(left);
     u32 const left_slice_offset = slice__get_offset(left);
     u64 const left_array_len    = slice_array__get_len(left);
     u64 const left_ref_cnt      = get_ref_cnt(left);
     u64 const left_len          = left_slice_len <= slice_max_len ? left_slice_len : left_array_len;

     assert(left_len <= slice_max_len || left_slice_offset == 0);

     u64 const new_len = left_len + right_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     t_any string;
     if (left_ref_cnt == 1) {
          u64 const left_cap = slice_array__get_cap(left);
          string             = left;
          slice_array__set_len(string, new_len);
          string             = slice__set_metadata(string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (left_cap < new_len) {
               string.qwords[0]  = (u64)realloc((u8*)string.qwords[0], new_cap * 3 + 16);
               slice_array__set_cap(string, new_cap);
          }

          u8* const chars   = slice_array__get_items(string);
          if (left_slice_offset != 0) memmove(chars, &chars[left_slice_offset * 3], left_len * 3);
          ctf8_str_ze_lt16_to_corsar_chars(right.bytes, &chars[left_len * 3]);
     } else {
          if (left_ref_cnt != 0) set_ref_cnt(left, left_ref_cnt - 1);

          string = long_string__new(new_len);
          slice_array__set_len(string, new_len);
          string = slice__set_metadata(string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          u8* const new_chars = slice_array__get_items(string);
          memcpy(new_chars, &((u8*)slice_array__get_items(left))[left_slice_offset * 3], left_len * 3);
          ctf8_str_ze_lt16_to_corsar_chars(right.bytes, &new_chars[left_len * 3]);
     }

     return string;
}

static inline t_any concat__long_str__long_str__own(t_thrd_data* const thrd_data, t_any left, t_any right, const char* const owner) {
     assume(left.bytes[15] == tid__string);
     assume(right.bytes[15] == tid__string);

     u32 const left_slice_offset = slice__get_offset(left);
     u32 const left_slice_len    = slice__get_len(left);
     u64 const left_ref_cnt      = get_ref_cnt(left);
     u64 const left_array_len    = slice_array__get_len(left);
     u64 const left_array_cap    = slice_array__get_cap(left);
     u8*       left_chars        = slice_array__get_items(left);
     u64 const left_len          = left_slice_len <= slice_max_len ? left_slice_len : left_array_len;

     assert(left_slice_len <= slice_max_len || left_slice_offset == 0);
     assert(left_array_cap >= left_array_len);

     u32 const right_slice_offset = slice__get_offset(right);
     u32 const right_slice_len    = slice__get_len(right);
     u64 const right_ref_cnt      = get_ref_cnt(right);
     u64 const right_array_len    = slice_array__get_len(right);
     u64 const right_array_cap    = slice_array__get_cap(right);
     u8*       right_chars        = slice_array__get_items(right);
     u64 const right_len          = right_slice_len <= slice_max_len ? right_slice_len : right_array_len;

     assert(right_slice_len <= slice_max_len || right_slice_offset == 0);
     assert(right_array_cap >= right_array_len);

     u64 const new_len = left_len + right_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     if (left_chars == right_chars) {
          if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
     } else {
          if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
          if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
     }

     int const left_is_mut  = left_ref_cnt == 1;
     int const right_is_mut = right_ref_cnt == 1;

     u8 const scenario =
          left_is_mut                                      |
          right_is_mut * 2                                 |
          (left_is_mut  && left_array_cap  >= new_len) * 4 |
          (right_is_mut && right_array_cap >= new_len) * 8 ;

     switch (scenario) {
     case 1: case 3:
          left.qwords[0] = (u64)realloc((u8*)left.qwords[0], new_cap * 3 + 16);
          slice_array__set_cap(left, new_cap);
          left_chars     = slice_array__get_items(left);
     case 5: case 7: case 15:
          slice_array__set_len(left, new_len);
          left = slice__set_metadata(left, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (left_slice_offset != 0) memmove(left_chars, &left_chars[left_slice_offset * 3], left_len * 3);
          memcpy(&left_chars[left_len * 3], &right_chars[right_slice_offset * 3], right_len * 3);

          if (right_is_mut) free((void*)right.qwords[0]);

          return left;
     case 2:
          right.qwords[0] = (u64)realloc((u8*)right.qwords[0], new_cap * 3 + 16);
          slice_array__set_cap(right, new_cap);
          right_chars     = slice_array__get_items(right);
     case 10: case 11:
          slice_array__set_len(right, new_len);
          right = slice__set_metadata(right, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (right_slice_offset != left_len) memmove(&right_chars[left_len * 3], &right_chars[right_slice_offset * 3], right_len * 3);
          memcpy(right_chars, &left_chars[left_slice_offset * 3], left_len * 3);

          if (left_is_mut) free((void*)left.qwords[0]);

          return right;
     case 0: {
          t_any string        = long_string__new(new_len);
          slice_array__set_len(string, new_len);
          string              = slice__set_metadata(string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          u8* const new_chars = slice_array__get_items(string);
          memcpy(new_chars, &left_chars[left_slice_offset * 3], left_len * 3);
          memcpy(&new_chars[left_len * 3], &right_chars[right_slice_offset * 3], right_len * 3);

          if (left_chars == right_chars) {
               if (left_ref_cnt == 2) free((void*)left.qwords[0]);
          } else {
               if (left_is_mut) free((void*)left.qwords[0]);
               if (right_is_mut) free((void*)right.qwords[0]);
          }

          return string;
     }
     default:
          unreachable;
     }
}

static inline t_any short_string__append(t_any string, t_any const char_) {
     assert(string.bytes[15] == tid__short_string);
     assert(char_.bytes[15] == tid__char);

     u32 const corsar_code = char_to_code(char_.bytes);
     u8  const string_size = short_string__get_size(string);
     u8  const char_size   = char_size_in_ctf8(corsar_code);
     if (string_size + char_size < 16) {
          corsar_code_to_ctf8_char(corsar_code, &string.bytes[string_size]);
          return string;
     }

     u8 const string_len = short_string__get_len(string);
     u8 const new_len    = string_len + 1;
     t_any    result     = long_string__new(new_len);
     result              = slice__set_metadata(result, 0, new_len);
     slice_array__set_len(result, new_len);

     u8* const chars = slice_array__get_items(result);
     ctf8_str_ze_lt16_to_corsar_chars(string.bytes, chars);
     memcpy_inline(&chars[string_len * 3], char_.bytes, 3);
     return result;
}

static inline t_any long_string__append__own(t_thrd_data* const thrd_data, t_any const string, t_any const char_, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assert(char_.bytes[15] == tid__char);

     u32 const string_slice_len    = slice__get_len(string);
     u32 const string_slice_offset = slice__get_offset(string);
     u64 const string_array_len    = slice_array__get_len(string);
     u64 const string_ref_cnt      = get_ref_cnt(string);
     u64 const string_len          = string_slice_len <= slice_max_len ? string_slice_len : string_array_len;

     assert(string_len <= slice_max_len || string_slice_offset == 0);

     u64 const new_len = string_len + 1;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     t_any result;
     if (string_ref_cnt == 1) {
          u64 const string_cap = slice_array__get_cap(string);
          result               = string;
          slice_array__set_len(result, new_len);
          result = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (string_cap < new_len) {
               result.qwords[0] = (u64)realloc((u8*)result.qwords[0], new_cap * 3 + 16);
               slice_array__set_cap(result, new_cap);
          }

          u8* const chars = slice_array__get_items(result);
          if (string_slice_offset != 0) memmove(chars, &chars[string_slice_offset * 3], string_len * 3);
          memcpy_inline(&chars[string_len * 3], char_.bytes, 3);
     } else {
          if (string_ref_cnt != 0) set_ref_cnt(string, string_ref_cnt - 1);

          result = long_string__new(new_len);
          slice_array__set_len(result, new_len);
          result              = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          u8* const new_chars = slice_array__get_items(result);
          memcpy(new_chars, &((u8*)slice_array__get_items(string))[string_slice_offset * 3], string_len * 3);
          memcpy_inline(&new_chars[string_len * 3], char_.bytes, 3);
     }

     return result;
}

static t_any short_string__each_to_each__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8       result_chars[45];
     u8 const result_len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, result_chars);
     if (result_len < 2) {
          ref_cnt__dec(thrd_data, fn);
          return string;
     }

     u8    result_size = 0;
     t_any fn_args[2]  = {{.bytes = {[15] = tid__char}}, {.bytes = {[15] = tid__char}}};
     for (u8 idx_1 = 0; idx_1 < result_len - (u8)1; idx_1 += 1) {
          memcpy_inline(fn_args[0].bytes, &result_chars[idx_1 * 3], 3);

          for (u8 idx_2 = idx_1 + 1; idx_2 < result_len; idx_2 += 1) {
               memcpy_inline(fn_args[1].bytes, &result_chars[idx_2 * 3], 3);
               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || box_items[0].bytes[15] != tid__char || box_items[1].bytes[15] != tid__char) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[0].qwords[0] = box_items[0].qwords[0];
               memcpy_inline(&result_chars[idx_2 * 3], box_items[1].bytes, 3);
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          result_size += char_size_in_ctf8(char_to_code(fn_args[0].bytes));
          memcpy_inline(&result_chars[idx_1 * 3], fn_args[0].bytes, 3);
     }
     ref_cnt__dec(thrd_data, fn);

     result_size += char_size_in_ctf8(char_to_code(&result_chars[(result_len - 1) * 3]));
     if (result_size < 16) {
          t_any result          = {};
          u8*   result_char_ptr = result.bytes;
          for (u64 idx = 0; idx < result_len; result_char_ptr = &result_char_ptr[corsar_code_to_ctf8_char(char_to_code(&result_chars[idx * 3]), result_char_ptr)], idx += 1);
          return result;
     }

     t_any result = long_string__new(result_len);
     result       = slice__set_metadata(result, 0, result_len);
     slice_array__set_len(result, result_len);
     memcpy_le_64(slice_array__get_items(result), result_chars, result_len * 3);
     return result;
}

static t_any long_string__each_to_each__own(t_thrd_data* const thrd_data, t_any string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(string);
     u32 const slice_offset = slice__get_offset(string);
     u8*       chars        = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt != 1) {
          if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);

          string = long_string__new(len);
          string = slice__set_metadata(string, 0, slice_len);
          slice_array__set_len(string, len);
          u8* const new_chars = slice_array__get_items(string);
          memcpy(new_chars, chars, len * 3);
          chars = new_chars;
     }

     t_any fn_args[2] = {{.bytes = {[15] = tid__char}}, {.bytes = {[15] = tid__char}}};
     for (u64 idx_1 = 0; idx_1 < len - 1; idx_1 += 1) {
          memcpy_inline(fn_args[0].bytes, &chars[idx_1 * 3], 3);

          for (u64 idx_2 = idx_1 + 1; idx_2 < len; idx_2 += 1) {
               memcpy_inline(fn_args[1].bytes, &chars[idx_2 * 3], 3);

               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    free((void*)string.qwords[0]);

                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || box_items[0].bytes[15] != tid__char || box_items[1].bytes[15] != tid__char) [[clang::unlikely]] {
                    free((void*)string.qwords[0]);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[0].qwords[0] = box_items[0].qwords[0];
               memcpy_inline(&chars[idx_2 * 3], box_items[1].bytes, 3);
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          memcpy_inline(&chars[idx_1 * 3], fn_args[0].bytes, 3);
     }
     ref_cnt__dec(thrd_data, fn);

     if (len < 16) {
          t_any const short_string = short_string_from_chars(chars, len);
          if (short_string.bytes[15] == tid__short_string) {
               free((void*)string.qwords[0]);
               return short_string;
          }
     }

     return string;
}

static t_any short_string__each_to_each_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8       result_chars[45];
     u8 const result_len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, result_chars);
     if (result_len < 2) {
          ref_cnt__dec(thrd_data, fn);
          return string;
     }

     u8    result_size = 0;
     t_any fn_args[4]  = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     for (u8 idx_1 = 0; idx_1 < result_len - (u8)1; idx_1 += 1) {
          fn_args[0].bytes[0] = idx_1;
          memcpy_inline(fn_args[1].bytes, &result_chars[idx_1 * 3], 3);

          for (u8 idx_2 = idx_1 + 1; idx_2 < result_len; idx_2 += 1) {
               fn_args[2].bytes[0] = idx_2;
               memcpy_inline(fn_args[3].bytes, &result_chars[idx_2 * 3], 3);
               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 4, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || box_items[0].bytes[15] != tid__char || box_items[1].bytes[15] != tid__char) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[1].qwords[0] = box_items[0].qwords[0];
               memcpy_inline(&result_chars[idx_2 * 3], box_items[1].bytes, 3);
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          result_size += char_size_in_ctf8(char_to_code(fn_args[1].bytes));
          memcpy_inline(&result_chars[idx_1 * 3], fn_args[1].bytes, 3);
     }
     ref_cnt__dec(thrd_data, fn);

     result_size += char_size_in_ctf8(char_to_code(&result_chars[(result_len - 1) * 3]));
     if (result_size < 16) {
          t_any result          = {};
          u8*   result_char_ptr = result.bytes;
          for (u64 idx = 0; idx < result_len; result_char_ptr = &result_char_ptr[corsar_code_to_ctf8_char(char_to_code(&result_chars[idx * 3]), result_char_ptr)], idx += 1);
          return result;
     }

     t_any result = long_string__new(result_len);
     result       = slice__set_metadata(result, 0, result_len);
     slice_array__set_len(result, result_len);
     memcpy_le_64(slice_array__get_items(result), result_chars, result_len * 3);
     return result;
}

static t_any long_string__each_to_each_kv__own(t_thrd_data* const thrd_data, t_any string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(string);
     u32 const slice_offset = slice__get_offset(string);
     u8*       chars        = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt != 1) {
          if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);

          string = long_string__new(len);
          string = slice__set_metadata(string, 0, slice_len);
          slice_array__set_len(string, len);
          u8* const new_chars = slice_array__get_items(string);
          memcpy(new_chars, chars, len * 3);
          chars = new_chars;
     }

     t_any fn_args[4] = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     for (u64 idx_1 = 0; idx_1 < len - 1; idx_1 += 1) {
          fn_args[0].qwords[0] = idx_1;
          memcpy_inline(fn_args[1].bytes, &chars[idx_1 * 3], 3);

          for (u64 idx_2 = idx_1 + 1; idx_2 < len; idx_2 += 1) {
               fn_args[2].qwords[0] = idx_2;
               memcpy_inline(fn_args[3].bytes, &chars[idx_2 * 3], 3);

               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 4, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    free((void*)string.qwords[0]);

                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || box_items[0].bytes[15] != tid__char || box_items[1].bytes[15] != tid__char) [[clang::unlikely]] {
                    free((void*)string.qwords[0]);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[1].qwords[0] = box_items[0].qwords[0];
               memcpy_inline(&chars[idx_2 * 3], box_items[1].bytes, 3);
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          memcpy_inline(&chars[idx_1 * 3], fn_args[1].bytes, 3);
     }
     ref_cnt__dec(thrd_data, fn);

     if (len < 16) {
          t_any const short_string = short_string_from_chars(chars, len);
          if (short_string.bytes[15] == tid__short_string) {
               free((void*)string.qwords[0]);
               return short_string;
          }
     }

     return string;
}

[[clang::always_inline]]
static u8 short_string__offset_of_idx(t_any string, u64 const idx, bool const allow_edge) {
     assert(string.bytes[15] == tid__short_string);

     u8 const size = short_string__get_size(string);

     if (__builtin_reduce_and(string.vector < 128) != 0) {
          if (idx + (allow_edge ? 0 : 1) > size || (i64)idx < 0) return 255;

          return idx;
     }

     u16       chars_star_mask = short_string__chars_start_mask(string) | (u16)1 << size;
     u8  const len             = __builtin_popcount(chars_star_mask) - 1;
     if (idx + (allow_edge ? 0 : 1) > len || (i64)idx < 0) return 255;

     u16 remain_chars   = idx;
     u16 half_chars_qty = __builtin_popcount(chars_star_mask & 0xff);
     u16 shifts         = half_chars_qty > remain_chars || remain_chars == 0 ? 0 : 8;
     remain_chars       = shifts == 0 ? remain_chars : remain_chars - half_chars_qty;
     u16 result         = shifts;
     chars_star_mask  >>= shifts;

     half_chars_qty    = __builtin_popcount(chars_star_mask & 0xf);
     shifts            = half_chars_qty > remain_chars || remain_chars == 0 ? 0 : 4;
     remain_chars      = shifts == 0 ? remain_chars : remain_chars - half_chars_qty;
     result           += shifts;
     chars_star_mask >>= shifts;

     half_chars_qty    = __builtin_popcount(chars_star_mask & 3);
     shifts            = half_chars_qty > remain_chars || remain_chars == 0 ? 0 : 2;
     remain_chars      = shifts == 0 ? remain_chars : remain_chars - half_chars_qty;
     result           += shifts;
     chars_star_mask >>= shifts;

     half_chars_qty    = chars_star_mask & 1;
     shifts            = half_chars_qty == 0 || remain_chars == 0 ? 0 : 1;
     chars_star_mask >>= shifts;
     result           += shifts + __builtin_ctz(chars_star_mask & 0xf);

     assert((chars_star_mask & 0xf) != 0);
     assert((shifts == 0 ? remain_chars : remain_chars - half_chars_qty) == 0);

     return result;
}

static inline t_any short_string__get_item(t_thrd_data* const thrd_data, t_any const string, t_any const idx, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assert(idx.bytes[15] == tid__int);

     u8 const offset = short_string__offset_of_idx(string, idx.qwords[0], false);
     if (offset == 255) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     t_any result = {.bytes = {[15] = tid__char}};
     ctf8_char_to_corsar_char(&string.bytes[offset], result.bytes);
     return result;
}

static inline t_any short_string__slice(t_thrd_data* const thrd_data, t_any string, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(idx_from <= idx_to);
     assert(string.bytes[15] == tid__short_string);

     u8 const from_offset = short_string__offset_of_idx(string, idx_from, true);
     u8 const to_offset   = short_string__offset_of_idx(string, idx_to, true);

     if (to_offset == 255) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     u64 const len = to_offset  - from_offset;
     string.raw_bits >>= from_offset * 8;
     string.raw_bits  &= ((u128)1 << len * 8) - 1;
     return string;
}

static inline t_any long_string__get_item__own(t_thrd_data* const thrd_data, t_any const string, t_any const idx, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assert(idx.bytes[15] == tid__int);

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (idx.qwords[0] >= len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     t_any result = {.bytes = {[15] = tid__char}};
     memcpy_inline(result.bytes, &((const u8*)slice_array__get_items(string))[(slice_offset + idx.qwords[0]) * 3], 3);

     if (ref_cnt == 1) free((void*)string.qwords[0]);

     return result;
}

static inline t_any long_string__slice__own(t_thrd_data* const thrd_data, t_any string, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(idx_from <= idx_to);

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (idx_to > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const new_len = idx_to - idx_from;
     if (new_len == len) return string;

     u8* const chars       = slice_array__get_items(string);
     u64 const from_offset = slice_offset + idx_from;
     if (new_len < 16) {
          t_any const short_string = short_string_from_chars(&chars[from_offset * 3], new_len);
          if (short_string.bytes[15] == tid__short_string) {
               ref_cnt__dec(thrd_data, string);
               return short_string;
          }
     }

     if (new_len <= slice_max_len && from_offset <= slice_max_offset) [[clang::likely]] {
          string = slice__set_metadata(string, from_offset, new_len);
          return string;
     }

     u64 const ref_cnt = get_ref_cnt(string);
     if (ref_cnt == 1) {
          if (from_offset != 0) memmove(chars, &chars[from_offset * 3], new_len * 3);
          slice_array__set_len(string, new_len);
          slice_array__set_cap(string, new_len);
          string           = slice__set_metadata(string, 0, slice_len);
          string.qwords[0] = (u64)realloc((u8*)string.qwords[0], new_len * 3 + 16);
          return string;
     }
     if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);

     t_any    new_string = long_string__new(new_len);
     new_string          = slice__set_metadata(new_string, 0, slice_len);
     slice_array__set_len(new_string, new_len);
     slice_array__set_cap(new_string, new_len);
     u8* const new_chars = slice_array__get_items(new_string);
     memcpy(new_chars, &chars[from_offset * 3], new_len * 3);
     return new_string;
}

static t_any string__all__own(t_thrd_data* const thrd_data, t_any const string, t_any const default_result, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8        short_string_chars[45];
     u64       len;
     const u8* chars;
     bool      need_to_free;
     if (string.bytes[15] == tid__short_string) {
          len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, short_string_chars);
          if (len == 0) {
               ref_cnt__dec(thrd_data, fn);
               return default_result;
          }

          chars        = short_string_chars;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(string);
          u32 const slice_len    = slice__get_len(string);
          chars                  = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          u64 const ref_cnt      = get_ref_cnt(string);
          u64 const array_len    = slice_array__get_len(string);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

          need_to_free = ref_cnt == 1;
     }

     t_any result = bool__true;
     t_any char_  = {.bytes = {[15] = tid__char}};
     for (u64 idx = 0; idx < len; idx += 1) {
          memcpy_inline(char_.bytes, &chars[idx * 3], 3);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 0) {
               result = bool__false;
               break;
          }
     }

     if (need_to_free) free((void*)string.qwords[0]);
     ref_cnt__dec(thrd_data, fn);

     return result;
}

static t_any string__all_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const default_result, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8        short_string_chars[45];
     u64       len;
     const u8* chars;
     bool      need_to_free;
     if (string.bytes[15] == tid__short_string) {
          len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, short_string_chars);
          if (len == 0) {
               ref_cnt__dec(thrd_data, fn);
               return default_result;
          }

          chars        = short_string_chars;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(string);
          u32 const slice_len    = slice__get_len(string);
          chars                  = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          u64 const ref_cnt      = get_ref_cnt(string);
          u64 const array_len    = slice_array__get_len(string);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

          need_to_free = ref_cnt == 1;
     }

     t_any result     = bool__true;
     t_any fn_args[2] = {{.structure = {.type = tid__int}}, {.structure = {.type = tid__char}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[0].qwords[0] = idx;
          memcpy_inline(fn_args[1].bytes, &chars[idx * 3], 3);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 0) {
               result = bool__false;
               break;
          }
     }

     if (need_to_free) free((void*)string.qwords[0]);
     ref_cnt__dec(thrd_data, fn);

     return result;
}
static t_any string__any__own(t_thrd_data* const thrd_data, t_any const string, t_any const default_result, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8        short_string_chars[45];
     u64       len;
     const u8* chars;
     bool      need_to_free;
     if (string.bytes[15] == tid__short_string) {
          len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, short_string_chars);
          if (len == 0) {
               ref_cnt__dec(thrd_data, fn);
               return default_result;
          }

          chars        = short_string_chars;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(string);
          u32 const slice_len    = slice__get_len(string);
          chars                  = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          u64 const ref_cnt      = get_ref_cnt(string);
          u64 const array_len    = slice_array__get_len(string);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

          need_to_free = ref_cnt == 1;
     }

     t_any result = bool__false;
     t_any char_  = {.bytes = {[15] = tid__char}};
     for (u64 idx = 0; idx < len; idx += 1) {
          memcpy_inline(char_.bytes, &chars[idx * 3], 3);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 1) {
               result = bool__true;
               break;
          }
     }

     if (need_to_free) free((void*)string.qwords[0]);
     ref_cnt__dec(thrd_data, fn);

     return result;
}

static t_any string__any_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const default_result, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8        short_string_chars[45];
     u64       len;
     const u8* chars;
     bool      need_to_free;
     if (string.bytes[15] == tid__short_string) {
          len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, short_string_chars);
          if (len == 0) {
               ref_cnt__dec(thrd_data, fn);
               return default_result;
          }

          chars        = short_string_chars;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(string);
          u32 const slice_len    = slice__get_len(string);
          chars                  = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          u64 const ref_cnt      = get_ref_cnt(string);
          u64 const array_len    = slice_array__get_len(string);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

          need_to_free = ref_cnt == 1;
     }

     t_any result     = bool__false;
     t_any fn_args[2] = {{.structure = {.type = tid__int}}, {.structure = {.type = tid__char}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[0].qwords[0] = idx;
          memcpy_inline(fn_args[1].bytes, &chars[idx * 3], 3);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 1) {
               result = bool__true;
               break;
          }
     }

     if (need_to_free) free((void*)string.qwords[0]);
     ref_cnt__dec(thrd_data, fn);

     return result;
}

static inline t_any long_string__is_ascii__own(t_thrd_data* const thrd_data, t_any string) {
     assume(string.bytes[15] == tid__string);

     u32       const slice_offset    = slice__get_offset(string);
     u32       const slice_len       = slice__get_len(string);
     u64       const ref_cnt         = get_ref_cnt(string);
     u64       const array_len       = slice_array__get_len(string);
     u64       const len             = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const chars           = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const chars_bytes_len = len * 3;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     bool result = true;
     u64  idx    = 0;
     {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          for (;
               result && chars_bytes_len - idx >= 64;

               result = result && __builtin_reduce_and(*(const t_vec*)&chars[idx] < (const t_vec){1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1}) != 0,
               idx += 63
          );
     }

     if (result && chars_bytes_len != idx) {
          u8 const remain_bytes_len = chars_bytes_len - idx;
          switch (sizeof(int) * 8 - __builtin_clz(remain_bytes_len - 1)) {
          case 6: {
               assume(remain_bytes_len > 32 && remain_bytes_len < 64);

               typedef u8 t_vec __attribute__((ext_vector_type(32), aligned(1)));

               result = __builtin_reduce_and(
                    (*(const t_vec*)&chars[idx] < (const t_vec){1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1}) &
                    (*(const t_vec*)&chars[chars_bytes_len - 32] < (const t_vec){1, 128 ,1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128})
               ) != 0;

               break;
          }
          case 5: {
               assume(remain_bytes_len > 17 && remain_bytes_len <= 30);

               typedef u8 t_vec __attribute__((ext_vector_type(16), aligned(1)));

               result = __builtin_reduce_and(
                    (*(const t_vec*)&chars[idx] < (const t_vec){1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1}) &
                    (*(const t_vec*)&chars[chars_bytes_len - 16] < (const t_vec){1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128})
               ) != 0;

               break;
          }
          case 4: {
               assume(remain_bytes_len > 8 && remain_bytes_len <= 15);

               typedef u8 t_vec __attribute__((ext_vector_type(8), aligned(1)));

               result = __builtin_reduce_and(
                    (*(const t_vec*)&chars[idx] < (const t_vec){1, 1, 128, 1, 1, 128, 1, 1}) &
                    (*(const t_vec*)&chars[chars_bytes_len - 8] < (const t_vec){1, 128, 1, 1, 128, 1, 1, 128})
               ) != 0;

               break;
          }
          case 3: {
               assume(remain_bytes_len == 6);

               u32 const code_1 = char_to_code(&chars[idx]);
               u32 const code_2 = char_to_code(&chars[idx + 3]);
               result = (code_1 | code_2) < 128;

               break;
          }
          case 2: {
               assume(remain_bytes_len == 3);

               result = char_to_code(&chars[idx]) < 128;

               break;
          }
          default:
               unreachable;
          }
     }

     if (ref_cnt == 1) free((void*)string.qwords[0]);

     return result ? bool__true : bool__false;
}

static inline t_any long_string__ascii_to_lower__own(t_thrd_data* const thrd_data, t_any string) {
     assume(string.bytes[15] == tid__string);

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;
     u8* const src_chars    = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     u64 const chars_size   = len * 3;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any      dst_string;
     u8*        dst_chars = src_chars;
     bool const is_mut    = ref_cnt == 1;
     u64        idx       = 0;
     if (chars_size - idx >= 64) {
          typedef char t_vec __attribute__((ext_vector_type(64), aligned(1)));

          do {
               t_vec const chars_vec              = *(const t_vec*)&src_chars[idx];
               u64         ascii_upper_chars_mask = v_64_bool_to_u64(
                    __builtin_convertvector(
                         (chars_vec > (const t_vec){-1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1}) &
                         (chars_vec < (const t_vec){1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1})
                    , v_64_bool)
               );
               ascii_upper_chars_mask &= ascii_upper_chars_mask << 1;
               ascii_upper_chars_mask &= ascii_upper_chars_mask << 1;
               ascii_upper_chars_mask &= 0x4924'9249'2492'4924ull;

               if (ascii_upper_chars_mask != 0) {
                    if (!is_mut && dst_chars == src_chars) {
                         if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                         dst_string = long_string__new(len);
                         slice_array__set_len(dst_string, len);
                         dst_string = slice__set_metadata(dst_string, 0, slice_len);
                         dst_chars  = slice_array__get_items(dst_string);
                         memcpy(dst_chars, src_chars, idx);
                    }

                    *(t_vec*)&dst_chars[idx] = chars_vec | __builtin_convertvector(u64_to_v_64_bool(ascii_upper_chars_mask), t_vec) * 32;
               } else if (dst_chars != src_chars) *(t_vec*)&dst_chars[idx] = chars_vec;

               idx += 63;
          } while (chars_size - idx > 64);
     }

     u8 const remain_bytes = chars_size - idx;
     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef char t_vec __attribute__((ext_vector_type(32), aligned(1)));

          t_vec const low_part      = *(t_vec*)&src_chars[idx];
          u32         low_part_mask = v_32_bool_to_u32(
               __builtin_convertvector(
                    (low_part > (const t_vec){-1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1}) &
                    (low_part < (const t_vec){1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1})
               , v_32_bool)
          );
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= 0x2492'4924ul;

          t_vec const high_part     = *(t_vec*)&src_chars[chars_size - 32];
          u32         high_part_mask = v_32_bool_to_u32(
               __builtin_convertvector(
                    (high_part > (const t_vec){-1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64}) &
                    (high_part < (const t_vec){1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91})
               , v_32_bool)
          );
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= 0x9249'2490ul;

          u32 mid_char_code = char_to_code(&src_chars[chars_size - 33]);

          if ((low_part_mask | high_part_mask) != 0 || (mid_char_code > 64 && mid_char_code < 91)) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               *(t_vec*)&dst_chars[idx]             = low_part | __builtin_convertvector(u32_to_v_32_bool(low_part_mask), t_vec) * 32;
               *(t_vec*)&dst_chars[chars_size - 32] = high_part | __builtin_convertvector(u32_to_v_32_bool(high_part_mask), t_vec) * 32;

               mid_char_code = mid_char_code > 64 && mid_char_code < 91 ? mid_char_code | 32 : mid_char_code;
               char_from_code(&dst_chars[chars_size - 33], mid_char_code);
          } else if (dst_chars != src_chars) memcpy_le_64(&dst_chars[idx], &src_chars[idx], remain_bytes);

          break;
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef char t_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const low_part      = *(t_vec*)&src_chars[idx];
          u16         low_part_mask = v_16_bool_to_u16(
               __builtin_convertvector(
                    (low_part > (const t_vec){-1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1}) &
                    (low_part < (const t_vec){1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1})
               , v_16_bool)
          );
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= 0x4924;

          t_vec const high_part     = *(t_vec*)&src_chars[chars_size - 16];
          u16         high_part_mask = v_16_bool_to_u16(
               __builtin_convertvector(
                    (high_part > (const t_vec){64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64, -1, -1, 64}) &
                    (high_part < (const t_vec){91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91, 1, 1, 91})
               , v_16_bool)
          );
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= 0x9248;

          if ((low_part_mask | high_part_mask) != 0) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               u32 mid_char_code = char_to_code(&src_chars[chars_size - 18]);
               mid_char_code     = mid_char_code > 64 && mid_char_code < 91 ? mid_char_code | 32 : mid_char_code;

               *(t_vec*)&dst_chars[idx]             = low_part | __builtin_convertvector(u16_to_v_16_bool(low_part_mask), t_vec) * 32;
               *(t_vec*)&dst_chars[chars_size - 16] = high_part | __builtin_convertvector(u16_to_v_16_bool(high_part_mask), t_vec) * 32;

               char_from_code(&dst_chars[chars_size - 18], mid_char_code);
          } else if (dst_chars != src_chars) memcpy_le_32(&dst_chars[idx], &src_chars[idx], remain_bytes);

          break;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef char t_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_vec const low_part      = *(t_vec*)&src_chars[idx];
          u8          low_part_mask = v_8_bool_to_u8(
               __builtin_convertvector(
                    (low_part > (const t_vec){-1, -1, 64, -1, -1, 64, -1, -1}) &
                    (low_part < (const t_vec){1, 1, 91, 1, 1, 91, 1, 1})
               , v_8_bool)
          );
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= 0x24;

          t_vec const high_part      = *(t_vec*)&src_chars[chars_size - 8];
          u8          high_part_mask = v_8_bool_to_u8(
               __builtin_convertvector(
                    (high_part > (const t_vec){-1, 64, -1, -1, 64, -1, -1, 64}) &
                    (high_part < (const t_vec){1, 91, 1, 1, 91, 1, 1, 91})
               , v_8_bool)
          );
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= 0x90;

          u32 mid_char_code = char_to_code(&src_chars[chars_size - 9]);

          if ((low_part_mask | high_part_mask) != 0 || (mid_char_code > 64 && mid_char_code < 91)) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               *(t_vec*)&dst_chars[idx]            = low_part | __builtin_convertvector(u8_to_v_8_bool(low_part_mask), t_vec) * 32;
               *(t_vec*)&dst_chars[chars_size - 8] = high_part | __builtin_convertvector(u8_to_v_8_bool(high_part_mask), t_vec) * 32;

               mid_char_code = mid_char_code > 64 && mid_char_code < 91 ? mid_char_code | 32 : mid_char_code;
               char_from_code(&dst_chars[chars_size - 9], mid_char_code);
          } else if (dst_chars != src_chars) memcpy_le_16(&dst_chars[idx], &src_chars[idx], remain_bytes);

          break;
     }
     case 3: {
          assume(remain_bytes == 6);

          u32 char_code_0 = char_to_code(&src_chars[idx]);
          u32 char_code_1 = char_to_code(&src_chars[idx + 3]);

          if ((char_code_0 > 64 && char_code_0 < 91) || (char_code_1 > 64 && char_code_1 < 91)) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               char_code_0 = char_code_0 > 64 && char_code_0 < 91 ? char_code_0 | 32 : char_code_0;
               char_code_1 = char_code_1 > 64 && char_code_1 < 91 ? char_code_1 | 32 : char_code_1;

               char_from_code(&dst_chars[idx], char_code_0);
               char_from_code(&dst_chars[idx + 3], char_code_1);
          } else if (dst_chars != src_chars) memcpy_le_16(&dst_chars[idx], &src_chars[idx], 6);

          break;
     }
     case 2: {
          assume(remain_bytes == 3);

          u32 const char_code = char_to_code(&src_chars[idx]);
          if (char_code > 64 && char_code < 91) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               char_from_code(&dst_chars[idx], char_code | 32);
          } else if (dst_chars != src_chars) memcpy_le_16(&dst_chars[idx], &src_chars[idx], 3);

          break;
     }
     case 0:
          break;
     default:
          unreachable;
     }

     return src_chars == dst_chars ? string : dst_string;
}

static inline t_any long_string__ascii_to_upper__own(t_thrd_data* const thrd_data, t_any string) {
     assume(string.bytes[15] == tid__string);

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;
     u8* const src_chars    = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     u64 const chars_size   = len * 3;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any      dst_string;
     u8*        dst_chars = src_chars;
     bool const is_mut    = ref_cnt == 1;
     u64        idx       = 0;
     if (chars_size - idx >= 64) {
          typedef char t_vec __attribute__((ext_vector_type(64), aligned(1)));

          do {
               t_vec const chars_vec              = *(const t_vec*)&src_chars[idx];
               u64         ascii_upper_chars_mask = v_64_bool_to_u64(
                    __builtin_convertvector(
                         (chars_vec > (const t_vec){-1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1}) &
                         (chars_vec < (const t_vec){1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1})
                    , v_64_bool)
               );
               ascii_upper_chars_mask &= ascii_upper_chars_mask << 1;
               ascii_upper_chars_mask &= ascii_upper_chars_mask << 1;
               ascii_upper_chars_mask &= 0x4924'9249'2492'4924ull;

               if (ascii_upper_chars_mask != 0) {
                    if (!is_mut && dst_chars == src_chars) {
                         if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                         dst_string = long_string__new(len);
                         slice_array__set_len(dst_string, len);
                         dst_string = slice__set_metadata(dst_string, 0, slice_len);
                         dst_chars  = slice_array__get_items(dst_string);
                         memcpy(dst_chars, src_chars, idx);
                    }

                    *(t_vec*)&dst_chars[idx] = chars_vec ^ __builtin_convertvector(u64_to_v_64_bool(ascii_upper_chars_mask), t_vec) * 32;
               } else if (dst_chars != src_chars) *(t_vec*)&dst_chars[idx] = chars_vec;

               idx += 63;
          } while (chars_size - idx >= 64);
     }

     u8 const remain_bytes = chars_size - idx;
     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef char t_vec __attribute__((ext_vector_type(32), aligned(1)));

          t_vec const low_part      = *(t_vec*)&src_chars[idx];
          u32         low_part_mask = v_32_bool_to_u32(
               __builtin_convertvector(
                    (low_part > (const t_vec){-1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1}) &
                    (low_part < (const t_vec){1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1})
               , v_32_bool)
          );
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= 0x2492'4924ul;

          t_vec const high_part     = *(t_vec*)&src_chars[chars_size - 32];
          u32         high_part_mask = v_32_bool_to_u32(
               __builtin_convertvector(
                    (high_part > (const t_vec){-1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96}) &
                    (high_part < (const t_vec){1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123})
               , v_32_bool)
          );
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= 0x9249'2490ul;

          u32 mid_char_code = char_to_code(&src_chars[chars_size - 33]);

          if ((low_part_mask | high_part_mask) != 0 || (mid_char_code > 96 && mid_char_code < 123)) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               *(t_vec*)&dst_chars[idx]             = low_part ^ __builtin_convertvector(u32_to_v_32_bool(low_part_mask), t_vec) * 32;
               *(t_vec*)&dst_chars[chars_size - 32] = high_part ^ __builtin_convertvector(u32_to_v_32_bool(high_part_mask), t_vec) * 32;

               mid_char_code = mid_char_code > 96 && mid_char_code < 123 ? mid_char_code ^ 32 : mid_char_code;
               char_from_code(&dst_chars[chars_size - 33], mid_char_code);
          } else if (dst_chars != src_chars) memcpy_le_64(&dst_chars[idx], &src_chars[idx], remain_bytes);

          break;
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef char t_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const low_part      = *(t_vec*)&src_chars[idx];
          u16         low_part_mask = v_16_bool_to_u16(
               __builtin_convertvector(
                    (low_part > (const t_vec){-1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1}) &
                    (low_part < (const t_vec){1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1})
               , v_16_bool)
          );
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= 0x4924;

          t_vec const high_part     = *(t_vec*)&src_chars[chars_size - 16];
          u16         high_part_mask = v_16_bool_to_u16(
               __builtin_convertvector(
                    (high_part > (const t_vec){96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96, -1, -1, 96}) &
                    (high_part < (const t_vec){123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123, 1, 1, 123})
               , v_16_bool)
          );
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= 0x9248;

          if ((low_part_mask | high_part_mask) != 0) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               u32 mid_char_code = char_to_code(&src_chars[chars_size - 18]);
               mid_char_code     = mid_char_code > 96 && mid_char_code < 123 ? mid_char_code ^ 32 : mid_char_code;

               *(t_vec*)&dst_chars[idx]             = low_part ^ __builtin_convertvector(u16_to_v_16_bool(low_part_mask), t_vec) * 32;
               *(t_vec*)&dst_chars[chars_size - 16] = high_part ^ __builtin_convertvector(u16_to_v_16_bool(high_part_mask), t_vec) * 32;

               char_from_code(&dst_chars[chars_size - 18], mid_char_code);
          } else if (dst_chars != src_chars) memcpy_le_32(&dst_chars[idx], &src_chars[idx], remain_bytes);

          break;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef char t_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_vec const low_part      = *(t_vec*)&src_chars[idx];
          u8          low_part_mask = v_8_bool_to_u8(
               __builtin_convertvector(
                    (low_part > (const t_vec){-1, -1, 96, -1, -1, 96, -1, -1}) &
                    (low_part < (const t_vec){1, 1, 123, 1, 1, 123, 1, 1})
               , v_8_bool)
          );
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= low_part_mask << 1;
          low_part_mask &= 0x24;

          t_vec const high_part      = *(t_vec*)&src_chars[chars_size - 8];
          u8          high_part_mask = v_8_bool_to_u8(
               __builtin_convertvector(
                    (high_part > (const t_vec){-1, 96, -1, -1, 96, -1, -1, 96}) &
                    (high_part < (const t_vec){1, 123, 1, 1, 123, 1, 1, 123})
               , v_8_bool)
          );
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= high_part_mask << 1;
          high_part_mask &= 0x90;

          u32 mid_char_code = char_to_code(&src_chars[chars_size - 9]);

          if ((low_part_mask | high_part_mask) != 0 || (mid_char_code > 96 && mid_char_code < 123)) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               *(t_vec*)&dst_chars[idx]            = low_part ^ __builtin_convertvector(u8_to_v_8_bool(low_part_mask), t_vec) * 32;
               *(t_vec*)&dst_chars[chars_size - 8] = high_part ^ __builtin_convertvector(u8_to_v_8_bool(high_part_mask), t_vec) * 32;

               mid_char_code = mid_char_code > 96 && mid_char_code < 123 ? mid_char_code ^ 32 : mid_char_code;
               char_from_code(&dst_chars[chars_size - 9], mid_char_code);
          } else if (dst_chars != src_chars) memcpy_le_16(&dst_chars[idx], &src_chars[idx], remain_bytes);

          break;
     }
     case 3: {
          assume(remain_bytes == 6);

          u32 char_code_0 = char_to_code(&src_chars[idx]);
          u32 char_code_1 = char_to_code(&src_chars[idx + 3]);

          if ((char_code_0 > 96 && char_code_0 < 123) || (char_code_1 > 96 && char_code_1 < 123)) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               char_code_0 = char_code_0 > 96 && char_code_0 < 123 ? char_code_0 ^ 32 : char_code_0;
               char_code_1 = char_code_1 > 96 && char_code_1 < 123 ? char_code_1 ^ 32 : char_code_1;

               char_from_code(&dst_chars[idx], char_code_0);
               char_from_code(&dst_chars[idx + 3], char_code_1);
          } else if (dst_chars != src_chars) memcpy_le_16(&dst_chars[idx], &src_chars[idx], 6);

          break;
     }
     case 2: {
          assume(remain_bytes == 3);

          u32 const char_code = char_to_code(&src_chars[idx]);
          if (char_code > 96 && char_code < 123) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               char_from_code(&dst_chars[idx], char_code ^ 32);
          } else if (dst_chars != src_chars) memcpy_le_16(&dst_chars[idx], &src_chars[idx], 3);

          break;
     }
     case 0:
          break;
     default:
          unreachable;
     }

     return src_chars == dst_chars ? string :dst_string;
}

[[clang::always_inline]]
static t_any cmp__short_string__short_string(t_any const string_1, t_any const string_2) {
     assert(string_1.bytes[15] == tid__short_string);
     assert(string_2.bytes[15] == tid__short_string);

     switch (scar__memcmp(string_1.bytes, string_2.bytes, 16)) {
     case -1: return tkn__less;
     case  0: return tkn__equal;
     case  1: return tkn__great;
     default: unreachable;
     }
}

static inline t_any cmp__short_string__long_string(t_any const string_1, t_any const string_2) {
     assert(string_1.bytes[15] == tid__short_string);
     assert(string_2.bytes[15] == tid__string);

     const u8* short_chars = string_1.bytes;
     const u8* long_chars  = &((const u8*)slice_array__get_items(string_2))[slice__get_offset(string_2) * 3];
     u32       short_corsar_char;
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(short_chars, (u8*)&short_corsar_char);
          if (char_size == 0) return tkn__less;

          switch (scar__memcmp(&short_corsar_char, long_chars, 3)) {
          case -1: return tkn__less;
          case  0: break;
          case  1: return tkn__great;
          default: unreachable;
          }

          short_chars = &short_chars[char_size];
          long_chars  = &long_chars[3];
     }
}

static inline t_any cmp__long_string__short_string(t_any const string_1, t_any const string_2) {
     assert(string_1.bytes[15] == tid__string);
     assert(string_2.bytes[15] == tid__short_string);

     const u8* long_chars  = &((const u8*)slice_array__get_items(string_1))[slice__get_offset(string_1) * 3];
     const u8* short_chars = string_2.bytes;
     u32       short_corsar_char;
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(short_chars, (u8*)&short_corsar_char);
          if (char_size == 0) return tkn__great;

          switch (scar__memcmp(long_chars, &short_corsar_char, 3)) {
          case -1: return tkn__less;
          case  0: break;
          case  1: return tkn__great;
          default: unreachable;
          }

          short_chars = &short_chars[char_size];
          long_chars  = &long_chars[3];
     }
}

[[clang::always_inline]]
static t_any cmp__long_string__long_string(t_any const string_1, t_any const string_2) {
     assert(string_1.bytes[15] == tid__string);
     assert(string_2.bytes[15] == tid__string);

     u32       const slice_1_offset = slice__get_offset(string_1);
     u32       const slice_1_len    = slice__get_len(string_1);
     const u8* const chars_1        = &((const u8*)slice_array__get_items(string_1))[slice_1_offset * 3];
     u64       const len_1          = slice_1_len <= slice_max_len ? slice_1_len : slice_array__get_len(string_1);

     u32       const slice_2_offset = slice__get_offset(string_2);
     u32       const slice_2_len    = slice__get_len(string_2);
     const u8* const chars_2        = &((const u8*)slice_array__get_items(string_2))[slice_2_offset * 3];
     u64       const len_2          = slice_2_len <= slice_max_len ? slice_2_len : slice_array__get_len(string_2);

     assert(slice_1_len <= slice_max_len || slice_1_offset == 0);
     assert(slice_2_len <= slice_max_len || slice_2_offset == 0);

     switch (scar__memcmp(chars_1, chars_2, (len_1 < len_2 ? len_1 : len_2) * 3)) {
     case -1: return tkn__less;
     case  0: return len_1 < len_2 ? tkn__less : len_2 < len_1 ? tkn__great : tkn__equal;
     case  1: return tkn__great;
     default: unreachable;
     }
}

[[clang::always_inline]]
static t_any equal__long_string__long_string(t_any const left, t_any const right) {
     assert(left.bytes[15] == tid__string);
     assert(right.bytes[15] == tid__string);

     u32 const left_slice_len = slice__get_len(left);
     u64 const left_len       = left_slice_len <= slice_max_len ? left_slice_len : slice_array__get_len(left);

     u32 const right_slice_len = slice__get_len(right);
     u64 const right_len       = right_slice_len <= slice_max_len ? right_slice_len : slice_array__get_len(right);

     if (left_len != right_len) return bool__false;

     u32       const left_slice_offset  = slice__get_offset(left);
     const u8* const left_chars         = &((const u8*)slice_array__get_items(left))[left_slice_offset * 3];
     u32       const right_slice_offset = slice__get_offset(right);
     const u8* const right_chars        = &((const u8*)slice_array__get_items(right))[right_slice_offset * 3];

     assert(left_slice_len <= slice_max_len || left_slice_offset == 0);
     assert(right_slice_len <= slice_max_len || right_slice_offset == 0);

     return scar__memcmp(left_chars, right_chars, left_len * 3) == 0 ? bool__true : bool__false;
}

static t_any copy__short_str__short_str(t_thrd_data* const thrd_data, t_any dst, u64 const idx, t_any const src, const char* const owner) {
     assert(dst.bytes[15] == tid__short_string);
     assert(src.bytes[15] == tid__short_string);

     u8 const dst_len = short_string__get_len(dst);
     u8 const src_len = short_string__get_len(src);
     if (dst_len < idx + src_len || (i64)idx < 0) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     u8 const idx_offset  = short_string__offset_of_idx(dst, idx, true);
     u8 const tail_offset = short_string__offset_of_idx(dst, idx + src_len, true);
     u8 const src_size    = short_string__get_size(src);
     u8 const dst_size    = short_string__get_size(dst);

     if (dst_size - tail_offset + idx_offset + src_size < 16) {
          dst.raw_bits =
               (dst.raw_bits & (((u128)1 << idx_offset * 8) - 1)) |
               (src.raw_bits << idx_offset * 8) |
               ((dst.raw_bits >> tail_offset * 8) << (idx_offset + src_size) * 8);

          return dst;
     }

     t_any long_string     = long_string__new(dst_len);
     long_string           = slice__set_metadata(long_string, 0, dst_len);
     slice_array__set_len(long_string, dst_len);
     u8* long_string_chars = slice_array__get_items(long_string);
     u8 offset = 0;
     for (u64 long_idx = 0; long_idx < idx; long_idx += 1)
          offset += ctf8_char_to_corsar_char(&dst.bytes[offset], &long_string_chars[long_idx * 3]);

     offset = 0;
     for (u64 long_idx = idx; long_idx < idx + src_len; long_idx += 1)
          offset += ctf8_char_to_corsar_char(&src.bytes[offset], &long_string_chars[long_idx * 3]);

     offset = tail_offset;
     for (u64 long_idx = idx + src_len; long_idx < dst_len; long_idx += 1)
          offset += ctf8_char_to_corsar_char(&dst.bytes[offset], &long_string_chars[long_idx * 3]);
     return long_string;
}

static t_any copy__short_str__long_str__own(t_thrd_data* const thrd_data, t_any const dst, u64 const idx, t_any src, const char* const owner) {
     assert(dst.bytes[15] == tid__short_string);
     assume(src.bytes[15] == tid__string);

     u32 const src_slice_offset = slice__get_offset(src);
     u32 const src_slice_len    = slice__get_len(src);
     u64 const src_ref_cnt      = get_ref_cnt(src);
     u64 const src_array_len    = slice_array__get_len(src);
     u64 const src_len          = src_slice_len <= slice_max_len ? src_slice_len : src_array_len;
     u8*       src_chars        = slice_array__get_items(src);

     assert(src_slice_len <= slice_max_len || src_slice_offset == 0);

     u64 const dst_len = short_string__get_len(dst);
     if (dst_len < idx + src_len || (i64)idx < 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, src);
          return error__out_of_bounds(thrd_data, owner);
     }

     u8 const tail_offset = short_string__offset_of_idx(dst, idx + src_len, true);
     if (src_ref_cnt == 1) {
          u64 const src_cap = slice_array__get_cap(src);
          if (src_cap < dst_len) {
               src.qwords[0] = (u64)realloc((u8*)src.qwords[0], 16 + dst_len * 3);
               src_chars     = slice_array__get_items(src);
          }
          slice_array__set_len(src, dst_len);
          src = slice__set_metadata(src, 0, dst_len);

          if (src_slice_offset != idx) memmove_le_64(&src_chars[idx * 3], &src_chars[src_slice_offset * 3], src_len * 3);
          u8 dst_offset = 0;
          for (u8 src_idx = 0; src_idx < idx; src_idx += 1)
               dst_offset += ctf8_char_to_corsar_char(&dst.bytes[dst_offset], &src_chars[src_idx * 3]);
          dst_offset = tail_offset;
          for (u8 src_idx = idx + src_len; src_idx < dst_len; src_idx += 1)
               dst_offset += ctf8_char_to_corsar_char(&dst.bytes[dst_offset], &src_chars[src_idx * 3]);

          return src;
     }
     if (src_ref_cnt != 0) set_ref_cnt(src, src_ref_cnt - 1);

     t_any result       = long_string__new(dst_len);
     result             = slice__set_metadata(result, 0, dst_len);
     slice_array__set_len(result, dst_len);
     u8*   result_chars = slice_array__get_items(result);
     u8    dst_offset   = 0;
     for (u8 result_idx = 0; result_idx < idx; result_idx += 1)
          dst_offset += ctf8_char_to_corsar_char(&dst.bytes[dst_offset], &result_chars[result_idx * 3]);
     memcpy_le_64(&result_chars[idx * 3], &src_chars[src_slice_offset * 3], src_len * 3);
     dst_offset = tail_offset;
     for (u8 result_idx = idx + src_len; result_idx < dst_len; result_idx += 1)
          dst_offset += ctf8_char_to_corsar_char(&dst.bytes[dst_offset], &result_chars[result_idx * 3]);

     return result;
}

static t_any copy__long_str__short_str__own(t_thrd_data* const thrd_data, t_any dst, u64 const idx, t_any const src, const char* const owner) {
     assume(dst.bytes[15] == tid__string);
     assert(src.bytes[15] == tid__short_string);

     u32 const dst_slice_offset = slice__get_offset(dst);
     u32 const dst_slice_len    = slice__get_len(dst);
     u64 const dst_ref_cnt      = get_ref_cnt(dst);
     u64 const dst_array_len    = slice_array__get_len(dst);
     u64 const dst_len          = dst_slice_len <= slice_max_len ? dst_slice_len : dst_array_len;
     u8*       dst_chars        = &((u8*)slice_array__get_items(dst))[dst_slice_offset * 3];

     assert(dst_slice_len <= slice_max_len || dst_slice_offset == 0);

     u64 const src_len = short_string__get_len(src);
     if (dst_len < idx + src_len || (i64)idx < 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, dst);
          return error__out_of_bounds(thrd_data, owner);
     }
     if (src_len == 0) return dst;

     if (dst_len < 16) {
          [[gnu::aligned(alignof(t_any))]]
          u8 buffer[32];
          u8 offset = 0;
          for (u8 dst_idx = 0; dst_idx < idx; dst_idx += 1) {
               if (offset > 14) goto result_not_short_label;

               offset += corsar_code_to_ctf8_char(char_to_code(&dst_chars[dst_idx * 3]), &buffer[offset]);
          }
          memcpy_inline(&buffer[offset], src.bytes, 16);
          offset += short_string__get_size(src);
          for (u8 dst_idx = idx + src_len; dst_idx < dst_len; dst_idx += 1) {
               if (offset > 14) goto result_not_short_label;

               offset += corsar_code_to_ctf8_char(char_to_code(&dst_chars[dst_idx * 3]), &buffer[offset]);
          }
          if (offset > 15) goto result_not_short_label;

          memset_inline(&buffer[offset], 0, 16);

          ref_cnt__dec(thrd_data, dst);
          return *(const t_any*)buffer;
     }

     result_not_short_label:
     if (dst_ref_cnt == 1) {
          ctf8_str_ze_lt16_to_corsar_chars(src.bytes, &dst_chars[idx * 3]);
          return dst;
     }
     if (dst_ref_cnt != 0) set_ref_cnt(dst, dst_ref_cnt - 1);

     t_any     new_string = long_string__new(dst_len);
     new_string           = slice__set_metadata(new_string, 0, dst_slice_len);
     slice_array__set_len(new_string, dst_len);
     u8* const new_chars  = slice_array__get_items(new_string);

     memcpy(new_chars, dst_chars, idx * 3);
     ctf8_str_ze_lt16_to_corsar_chars(src.bytes, &new_chars[idx * 3]);
     memcpy(&new_chars[(idx + src_len) * 3], &dst_chars[(idx + src_len) * 3], (dst_len - idx - src_len) * 3);
     return new_string;
}

static t_any copy__long_str__long_str__own(t_thrd_data* const thrd_data, t_any dst, u64 const idx, t_any src, const char* const owner) {
     assume(dst.bytes[15] == tid__string);
     assume(src.bytes[15] == tid__string);

     u32 const dst_slice_offset = slice__get_offset(dst);
     u32 const dst_slice_len    = slice__get_len(dst);
     u64 const dst_ref_cnt      = get_ref_cnt(dst);
     u64 const dst_array_len    = slice_array__get_len(dst);
     u64 const dst_len          = dst_slice_len <= slice_max_len ? dst_slice_len : dst_array_len;
     u8*       dst_chars        = slice_array__get_items(dst);

     assert(dst_slice_len <= slice_max_len || dst_slice_offset == 0);

     u32 const src_slice_offset = slice__get_offset(src);
     u32 const src_slice_len    = slice__get_len(src);
     u64 const src_ref_cnt      = get_ref_cnt(src);
     u64 const src_array_len    = slice_array__get_len(src);
     u64 const src_len          = src_slice_len <= slice_max_len ? src_slice_len : src_array_len;
     u8*       src_chars        = slice_array__get_items(src);

     assert(src_slice_len <= slice_max_len || src_slice_offset == 0);

     if (dst_len < idx + src_len || (i64)idx < 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, dst);
          ref_cnt__dec(thrd_data, src);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (dst_ref_cnt == 1) {
          if (src_ref_cnt > 1) set_ref_cnt(src, src_ref_cnt - 1);
          memcpy(&dst_chars[(dst_slice_offset + idx) * 3], &src_chars[src_slice_offset * 3], src_len * 3);
          if (src_ref_cnt == 1) free((void*)src.qwords[0]);
          return dst;
     }
     if (dst_ref_cnt != 0) set_ref_cnt(dst, dst_ref_cnt - 1);

     if (src_ref_cnt == 1 || (src_ref_cnt == 2 && dst_chars == src_chars)) {
          if (dst_chars == src_chars) {
               memmove(&dst_chars[(dst_slice_offset + idx) * 3], &dst_chars[src_slice_offset * 3], src_len * 3);
               return dst;
          }

          u64 const src_cap = slice_array__get_cap(src);
          if (src_cap < dst_len) {
               src.qwords[0] = (u64)realloc((u8*)src.qwords[0], 16 + dst_len * 3);
               src_chars     = slice_array__get_items(src);
          }
          slice_array__set_len(src, dst_len);
          src = slice__set_metadata(src, 0, dst_slice_len);

          if (src_slice_offset != idx) memmove(&src_chars[idx * 3], &src_chars[src_slice_offset * 3], src_len * 3);
          memcpy(src_chars, &dst_chars[dst_slice_offset * 3], idx * 3);
          memcpy(&src_chars[(idx + src_len) * 3], &dst_chars[(dst_slice_offset + idx + src_len) * 3], (dst_len - src_len - idx) * 3);
          return src;
     }
     if (src_ref_cnt != 0) set_ref_cnt(src, src_ref_cnt - (src_chars == dst_chars ? 2 : 1));

     t_any new_string = long_string__new(dst_len);
     new_string       = slice__set_metadata(new_string, 0, dst_slice_len);
     slice_array__set_len(new_string, dst_len);

     u8* const new_chars = slice_array__get_items(new_string);
     memcpy(new_chars, &dst_chars[dst_slice_offset * 3], idx * 3);
     memcpy(&new_chars[idx * 3], &src_chars[src_slice_offset * 3], src_len * 3);
     memcpy(&new_chars[(idx + src_len) * 3], &dst_chars[(dst_slice_offset + idx + src_len) * 3], (dst_len - src_len - idx) * 3);
     return new_string;
}

static t_any short_string__delete(t_thrd_data* const thrd_data, t_any string, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(idx_from <= idx_to);
     assert(string.bytes[15] == tid__short_string);

     u8 const from_offset = short_string__offset_of_idx(string, idx_from, true);
     u8 const to_offset   = short_string__offset_of_idx(string, idx_to, true);
     if (to_offset == 255) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     string.raw_bits = (string.raw_bits & ((u128)1 << from_offset * 8) - 1) | ((string.raw_bits >> to_offset * 8) << from_offset * 8);
     return string;
}

static t_any long_string__delete__own(t_thrd_data* const thrd_data, t_any string, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(string.bytes[15] == tid__string);

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (idx_to > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const new_len = len - (idx_to - idx_from);
     if (new_len == len) return string;

     if (new_len < 16) {
          u8* const chars        = &((u8*)slice_array__get_items(string))[slice_offset * 3];
          u8        offset       = 0;
          t_any     short_string = {};
          for (u64 string_idx = 0; string_idx < idx_from; string_idx += 1) {
               u32 const code = char_to_code(&chars[string_idx * 3]);
               if (offset + char_size_in_ctf8(code) > 15) goto not_short_label;

               offset += corsar_code_to_ctf8_char(code, &short_string.bytes[offset]);
          }

          for (u64 string_idx = idx_to; string_idx < len; string_idx += 1) {
               u32 const code = char_to_code(&chars[string_idx * 3]);
               if (offset + char_size_in_ctf8(code) > 15) goto not_short_label;

               offset += corsar_code_to_ctf8_char(code, &short_string.bytes[offset]);
          }

          ref_cnt__dec(thrd_data, string);
          return short_string;
     }
     not_short_label:

     if (idx_from == 0 || idx_to == len) {
          u64 const slice_idx_from = idx_from == 0 ? idx_to : 0;
          u64 const slice_to_from  = idx_from == 0 ? len : idx_from;
          return long_string__slice__own(thrd_data, string, slice_idx_from, slice_to_from, owner);
     }

     u64 const ref_cnt = get_ref_cnt(string);
     if (ref_cnt == 1) {
          u8* const chars = slice_array__get_items(string);
          string          = slice__set_metadata(string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          slice_array__set_len(string, new_len);
          memmove(chars, &chars[slice_offset * 3], idx_from * 3);
          memmove(&chars[idx_from * 3], &chars[(slice_offset + idx_to) * 3], (len - idx_to) * 3);
          return string;
     }
     if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);

     u8* const chars     = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     t_any new_string    = long_string__new(new_len);
     new_string          = slice__set_metadata(new_string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
     slice_array__set_len(new_string, new_len);
     u8* const new_chars = slice_array__get_items(new_string);
     memcpy(new_chars, chars, idx_from * 3);
     memcpy(&new_chars[idx_from * 3], &chars[idx_to * 3], (len - idx_to) * 3);
     return new_string;
}

[[clang::always_inline]]
static t_any short_string__drop(t_thrd_data* const thrd_data, t_any string, u64 const n, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);

     u8 const n_offset = short_string__offset_of_idx(string, n, true);
     if (n_offset == 255) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     string.raw_bits >>= n_offset * 8;
     return string;
}

static inline t_any long_string__drop__own(t_thrd_data* const thrd_data, t_any string, u64 const n, const char* const owner) {
     assume(string.bytes[15] == tid__string);

     u32 const slice_len = slice__get_len(string);
     u64 const len       = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);
     if (n > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          return error__out_of_bounds(thrd_data, owner);
     }

     return long_string__slice__own(thrd_data, string, n, len, owner);
}

static t_any short_string__drop_while__own(t_thrd_data* const thrd_data, t_any string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8    offset = 0;
     t_any char_  = {.bytes = {[15] = tid__char}};
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(&string.bytes[offset], char_.bytes);
          if (char_size == 0) break;

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) break;
          offset += char_size;
     }
     ref_cnt__dec(thrd_data, fn);
     string.raw_bits >>= offset * 8;
     return string;
}

static t_any long_string__drop_while__own(t_thrd_data* const thrd_data, t_any string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32  const slice_offset = slice__get_offset(string);
     u32  const slice_len    = slice__get_len(string);
     u8*  const chars        = slice_array__get_items(string);
     u64  const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any char_ = {.bytes = {[15] = tid__char}};
     u64   n     = 0;
     for (; n < len; n += 1) {
          memcpy_inline(char_.bytes, &chars[(slice_offset + n) * 3], 3);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, string);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) break;
     }
     ref_cnt__dec(thrd_data, fn);

     if (n == 0) return string;

     u64 const new_len  = len - n;
     u64 const n_offset = slice_offset + n;
     if (new_len < 16) {
          t_any const short_string = short_string_from_chars(&chars[n_offset * 3], new_len);
          if (short_string.bytes[15] == tid__short_string) {
               ref_cnt__dec(thrd_data, string);
               return short_string;
          }
     }

     if (new_len <= slice_max_len && n_offset <= slice_max_offset) [[clang::likely]] {
          string = slice__set_metadata(string, n_offset, new_len);
          return string;
     }

     u64 const ref_cnt = get_ref_cnt(string);
     if (ref_cnt == 1) {
          memmove(chars, &chars[n_offset * 3], new_len * 3);
          slice_array__set_len(string, new_len);
          slice_array__set_cap(string, new_len);
          string           = slice__set_metadata(string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          string.qwords[0] = (u64)realloc((u8*)string.qwords[0], new_len * 3 + 16);
          return string;
     }
     if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);

     t_any    new_string = long_string__new(new_len);
     new_string          = slice__set_metadata(new_string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
     slice_array__set_len(new_string, new_len);
     slice_array__set_cap(new_string, new_len);
     u8* const new_chars = slice_array__get_items(new_string);
     memcpy(new_chars, &chars[n_offset * 3], new_len * 3);
     return new_string;
}

static t_any short_string__filter__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any char_           = {.bytes = {[15] = tid__char}};
     u8    str_offset      = 0;
     u8    new_str_offset  = 0;
     t_any result          = {};
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(&string.bytes[str_offset], char_.bytes);
          if (char_size == 0) break;

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 1) {
               u128 const char_in_string = (string.raw_bits >> str_offset * 8) & ((u32)-1 >> (4 - char_size) * 8);
               result.raw_bits          |= char_in_string << new_str_offset * 8;
               new_str_offset += char_size;
          }
          str_offset += char_size;
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any long_string__filter__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset   = slice__get_offset(string);
     u32       const slice_len      = slice__get_len(string);
     const u8* const original_chars = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const ref_cnt        = get_ref_cnt(string);
     u64       const array_len      = slice_array__get_len(string);
     u64       const len            = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64  part_lens[64];
     part_lens[0]      = 1;
     u8   current_part = 0;
     u64  odd_is_pass;

     t_any char_ = {.bytes = {[15] = tid__char}};
     {
          memcpy_inline(char_.bytes, original_chars, 3);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, string);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          odd_is_pass = fn_result.bytes[0] ^ 1;
     }

     t_any     dst_str    = string;
     u8*       dst_chars  = slice_array__get_items(dst_str);
     const u8* src_chars  = original_chars;
     bool      dst_is_mut = ref_cnt == 1;
     u64       dst_len    = 0;
     for (u64 idx = 1; idx < len; idx += 1) {
          memcpy_inline(char_.bytes, &original_chars[idx * 3], 3);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_str.qwords[0] != string.qwords[0]) ref_cnt__dec(thrd_data, dst_str);
               ref_cnt__dec(thrd_data, string);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == ((current_part & 1) == odd_is_pass)) part_lens[current_part] += 1;
          else if (current_part != 63) {
               current_part           += 1;
               part_lens[current_part] = 1;
          } else {
               typedef u64 t_vec __attribute__((ext_vector_type(64), aligned(alignof(u64))));

               if (!dst_is_mut) {
                    u64 const droped_len = __builtin_reduce_add(
                         *(const t_vec*)part_lens &
                         (__builtin_convertvector(u64_to_v_64_bool(odd_is_pass ? 0x5555'5555'5555'5555ull : 0xaaaa'aaaa'aaaa'aaaaull), t_vec) * -1)
                    );
                    dst_str    = long_string__new(len - droped_len);
                    dst_chars  = slice_array__get_items(dst_str);
                    dst_is_mut = true;
               }

               for (u8 part_idx = 0; part_idx < 64; part_idx += 1) {
                    u64 const part_len = part_lens[part_idx];
                    if ((part_idx & 1) == odd_is_pass) {
                         memmove(&dst_chars[dst_len * 3], src_chars, part_len * 3);
                         dst_len += part_len;
                    }
                    src_chars = &src_chars[part_len * 3];
               }

               current_part = 0;
               part_lens[0] = 1;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (src_chars == original_chars) {
          u64 from = -1;
          u64 to;

          switch (current_part) {
          case 0:
               if (odd_is_pass == 0) return string;
               ref_cnt__dec(thrd_data, string);
               return (const t_any){};
          case 1:
               from = odd_is_pass ? part_lens[0] : 0;
               to   = odd_is_pass ? len : part_lens[0];
               break;
          case 2:
               if (odd_is_pass) {
                    from = part_lens[0];
                    to   = from + part_lens[1];
               }
               break;
          }

          if (from != (u64)-1) return long_string__slice__own(thrd_data, string, from, to, owner);

          typedef u64 t_vec __attribute__((ext_vector_type(8), aligned(alignof(u64))));

          if (!dst_is_mut) {
               u64         droped_len = 0;
               u8          part_idx   = 8;
               t_vec const mask       = __builtin_convertvector(u8_to_v_8_bool(odd_is_pass ? 0x55 : 0xaa), t_vec) * -1;
               for (; part_idx <= current_part; part_idx += 8)
                    droped_len += __builtin_reduce_add(*(const t_vec*)&part_lens[part_idx - 8] & mask);
               for (part_idx -= odd_is_pass ? 8 : 7; part_idx <= current_part; droped_len += part_lens[part_idx], part_idx += 2);

               dst_str   = long_string__new(len - droped_len);
               dst_chars = slice_array__get_items(dst_str);
          }
     }

     for (u8 part_idx = 0; part_idx <= current_part; part_idx += 1) {
          u64 const part_len = part_lens[part_idx];
          if ((part_idx & 1) == odd_is_pass) {
               memmove(&dst_chars[dst_len * 3], src_chars, part_len * 3);
               dst_len += part_len;
          }
          src_chars = &src_chars[part_len * 3];
     }

     if (dst_str.qwords[0] != string.qwords[0]) ref_cnt__dec(thrd_data, string);

     if (dst_len < 16) {
          t_any const short_string = short_string_from_chars(dst_chars, dst_len);
          if (short_string.bytes[15] == tid__short_string) {
               ref_cnt__dec(thrd_data, dst_str);
               return short_string;
          }
     }

     slice_array__set_len(dst_str, dst_len);
     dst_str = slice__set_metadata(dst_str, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
     return dst_str;
}

static t_any short_string__filter_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any kv[2]           = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     u8    str_offset      = 0;
     u8    new_str_offset  = 0;
     t_any result          = {};
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(&string.bytes[str_offset], kv[1].bytes);
          if (char_size == 0) break;

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 1) {
               u128 const char_in_string = (string.raw_bits >> str_offset * 8) & ((u32)-1 >> (4 - char_size) * 8);
               result.raw_bits          |= char_in_string << new_str_offset * 8;
               new_str_offset += char_size;
          }
          str_offset     += char_size;
          kv[0].bytes[0] += 1;
     }
     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any long_string__filter_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset   = slice__get_offset(string);
     u32       const slice_len      = slice__get_len(string);
     const u8* const original_chars = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const ref_cnt        = get_ref_cnt(string);
     u64       const array_len      = slice_array__get_len(string);
     u64       const len            = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64  part_lens[64];
     part_lens[0]      = 1;
     u8   current_part = 0;
     u64  odd_is_pass;

     t_any kv[2] = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     {
          memcpy_inline(kv[1].bytes, original_chars, 3);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, string);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          odd_is_pass = (fn_result.bytes[0] & 1) ^ 1;
     }

     t_any     dst_str    = string;
     u8*       dst_chars  = slice_array__get_items(dst_str);
     const u8* src_chars  = original_chars;
     bool      dst_is_mut = ref_cnt == 1;
     u64       dst_len    = 0;
     for (u64 idx = 1; idx < len; idx += 1) {
          kv[0].qwords[0] = idx;
          memcpy_inline(kv[1].bytes, &original_chars[idx * 3], 3);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_str.qwords[0] != string.qwords[0]) ref_cnt__dec(thrd_data, dst_str);
               ref_cnt__dec(thrd_data, string);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == ((current_part & 1) == odd_is_pass)) part_lens[current_part] += 1;
          else if (current_part != 63) {
               current_part           += 1;
               part_lens[current_part] = 1;
          } else {
               typedef u64 t_vec __attribute__((ext_vector_type(64), aligned(alignof(u64))));

               if (!dst_is_mut) {
                    u64 const droped_len = __builtin_reduce_add(
                         *(const t_vec*)part_lens &
                         (__builtin_convertvector(u64_to_v_64_bool(odd_is_pass ? 0x5555'5555'5555'5555ull : 0xaaaa'aaaa'aaaa'aaaaull), t_vec) * -1)
                    );
                    dst_str    = long_string__new(len - droped_len);
                    dst_chars  = slice_array__get_items(dst_str);
                    dst_is_mut = true;
               }

               for (u8 part_idx = 0; part_idx < 64; part_idx += 1) {
                    u64 const part_len = part_lens[part_idx];
                    if ((part_idx & 1) == odd_is_pass) {
                         memmove(&dst_chars[dst_len * 3], src_chars, part_len * 3);
                         dst_len += part_len;
                    }
                    src_chars = &src_chars[part_len * 3];
               }

               current_part = 0;
               part_lens[0] = 1;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (src_chars == original_chars) {
          u64 from = -1;
          u64 to;

          switch (current_part) {
          case 0:
               if (odd_is_pass == 0) return string;
               ref_cnt__dec(thrd_data, string);
               return (const t_any){};
          case 1:
               from = odd_is_pass ? part_lens[0] : 0;
               to   = odd_is_pass ? len : part_lens[0];
               break;
          case 2:
               if (odd_is_pass == 0) {
                    from = part_lens[0];
                    to   = from + part_lens[1];
               }
               break;
          }

          if (from != (u64)-1) return long_string__slice__own(thrd_data, string, from, to, owner);

          typedef u64 t_vec __attribute__((ext_vector_type(8), aligned(alignof(u64))));

          if (!dst_is_mut) {
               u64 droped_len = 0;
               u8  part_idx   = 8;
               for (; part_idx <= current_part; part_idx += 8)
                    droped_len += __builtin_reduce_add(
                         *(const t_vec*)&part_lens[part_idx - 8] &
                         (__builtin_convertvector(u8_to_v_8_bool(odd_is_pass ? 0x55 : 0xaa), t_vec) * -1)
                    );
               for (part_idx -= odd_is_pass ? 8 : 7; part_idx <= current_part; droped_len += part_lens[part_idx], part_idx += 2);

               dst_str   = long_string__new(len - droped_len);
               dst_chars = slice_array__get_items(dst_str);
          }
     }

     for (u8 part_idx = 0; part_idx <= current_part; part_idx += 1) {
          u64 const part_len = part_lens[part_idx];
          if ((part_idx & 1) == odd_is_pass) {
               memmove(&dst_chars[dst_len * 3], src_chars, part_len * 3);
               dst_len += part_len;
          }
          src_chars = &src_chars[part_len * 3];
     }

     if (dst_str.qwords[0] != string.qwords[0]) ref_cnt__dec(thrd_data, string);

     if (dst_len < 16) {
          t_any const short_string = short_string_from_chars(dst_chars, dst_len);
          if (short_string.bytes[15] == tid__short_string) {
               ref_cnt__dec(thrd_data, dst_str);
               return short_string;
          }
     }

     slice_array__set_len(dst_str, dst_len);
     dst_str = slice__set_metadata(dst_str, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
     return dst_str;
}

static t_any short_string__foldl__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8    str_offset = 0;
     t_any fn_args[2];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(&string.bytes[str_offset], fn_args[1].bytes);
          if (char_size == 0) break;

          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;

          str_offset += char_size;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_string__foldl__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1 && state.qwords[0] != string.qwords[0]) set_ref_cnt(string, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};
     for (u64 idx = 0; idx < len; idx += 1) {
          memcpy_inline(fn_args[1].bytes, &chars[idx * 3], 3);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)string.qwords[0]);
     else if (ref_cnt != 0 && state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_string__foldl1__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {[15] = tid__char}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};
     u8  str_offset = ctf8_char_to_corsar_char(string.bytes, fn_args[0].bytes);
     if (str_offset == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(&string.bytes[str_offset], fn_args[1].bytes);
          if (char_size == 0) break;

          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;

          str_offset += char_size;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_string__foldl1__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (len == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {[15] = tid__char}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};
     memcpy_inline(fn_args[0].bytes, chars, 3);
     for (u64 idx = 1; idx < len; idx += 1) {
          memcpy_inline(fn_args[1].bytes, &chars[idx * 3], 3);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)string.qwords[0]);
     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_string__foldl_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8    str_offset = 0;
     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(&string.bytes[str_offset], fn_args[2].bytes);
          if (char_size == 0) break;

          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;

          str_offset          += char_size;
          fn_args[1].bytes[0] += 1;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_string__foldl_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1 && state.qwords[0] != string.qwords[0]) set_ref_cnt(string, ref_cnt - 1);

     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[1].qwords[0] = idx;
          memcpy_inline(fn_args[2].bytes, &chars[idx * 3], 3);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)string.qwords[0]);
     else if (ref_cnt != 0 && state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_string__foldr__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8       corsar_chars[45];
     u8 const len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, corsar_chars);
     t_any    fn_args[2];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};
     for (u8 idx = (len - 1) * 3; idx != (u8)-3; idx -= 3) {
          memcpy_inline(fn_args[1].bytes, &corsar_chars[idx], 3);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_string__foldr__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1 && state.qwords[0] != string.qwords[0]) set_ref_cnt(string, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};
     for (u64 idx = (len - 1) * 3; idx != (u64)-3; idx -= 3) {
          memcpy_inline(fn_args[1].bytes, &chars[idx], 3);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)string.qwords[0]);
     else if (ref_cnt != 0 && state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_string__foldr1__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8       corsar_chars[45];
     u8 const len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, corsar_chars);
     if (len == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {[15] = tid__char}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};
     memcpy_inline(fn_args[0].bytes, &corsar_chars[(len - 1) * 3], 3);
     for (u8 idx = (len - 2) * 3; idx != (u8)-3; idx -= 3) {
          memcpy_inline(fn_args[1].bytes, &corsar_chars[idx], 3);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_string__foldr1__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (len == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {[15] = tid__char}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};
     memcpy_inline(fn_args[0].bytes, &chars[(len - 1) * 3], 3);
     for (u64 idx = (len - 2) * 3; idx != (u64)-3; idx -= 3) {
          memcpy_inline(fn_args[1].bytes, &chars[idx], 3);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)string.qwords[0]);
     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_string__foldr_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8       corsar_chars[45];
     u8 const len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, corsar_chars);
     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     for (u8 idx = len - 1; idx != (u8)-1; idx -= 1) {
          fn_args[1].bytes[0] = idx;
          memcpy_inline(fn_args[2].bytes, &corsar_chars[idx * 3], 3);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_string__foldr_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1 && state.qwords[0] != string.qwords[0]) set_ref_cnt(string, ref_cnt - 1);

     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     for (u64 idx = len - 1; idx != (u64)-1; idx -= 1) {
          fn_args[1].qwords[0] = idx;
          memcpy_inline(fn_args[2].bytes, &chars[idx * 3], 3);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)string.qwords[0]);
     else if (ref_cnt != 0 && state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static inline t_any short_string__look_from_end_in(t_any const string, t_any const looked_char) {
     assert(string.bytes[15] == tid__short_string);
     assert(looked_char.bytes[15] == tid__char);

     u8       ctf8_looked_char[4];
     u8 const looked_char_size = corsar_code_to_ctf8_char(char_to_code(looked_char.bytes), ctf8_looked_char);

     u16 looked_char_mask;
     switch (looked_char_size) {
     case 1:
          looked_char_mask = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          break;
     case 2: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[1], v_16_bool));
          looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1);
          break;
     }
     case 3: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[2], v_16_bool));
          looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2);
          break;
     }
     case 4: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[2], v_16_bool));
          u16 const chars_mask_3 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[3], v_16_bool));
          looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2) & (chars_mask_3 >> 3);
          break;
     }
     default:
          unreachable;
     }

     return looked_char_mask == 0 ?
          null :
          (const t_any){.structure = {.value =
               __builtin_popcount(short_string__chars_start_mask(string) & ((u16)1 << (sizeof(int) * 8 - 1 - __builtin_clz(looked_char_mask))) - 1),
               .type = tid__int}};
}

static inline u64 look_char_from_end(const u8* const chars, u64 const chars_len, t_any const looked_char) {
     u64 remain_bytes = chars_len * 3;
     if (remain_bytes >= 64) {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          t_vec const looked_char_vec = looked_char.vector.s2012012012012012012012012012012012012012012012012012012012012012;

          do {
               u64 looked_char_mask = v_64_bool_to_u64(__builtin_convertvector(*(const t_vec*)&chars[remain_bytes - 64] == looked_char_vec, v_64_bool));
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask     = looked_char_mask >> 1 & 0x1249'2492'4924'9249ull;
               if (looked_char_mask != 0) return (remain_bytes + sizeof(long long) * 8 - 64 - __builtin_clzll(looked_char_mask)) / 3;
               remain_bytes -= 63;
          } while (remain_bytes >= 64);
     }

     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 t_vec __attribute__((ext_vector_type(32), aligned(1)));

          t_vec const low_part         = *(t_vec*)chars;
          t_vec const high_part        = *(t_vec*)&chars[remain_bytes - 32];
          u64         looked_char_mask =
               v_32_bool_to_u32(__builtin_convertvector(low_part == looked_char.vector.s01201201201201201201201201201201, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_char.vector.s12012012012012012012012012012012, v_32_bool)) << (remain_bytes - 32);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249'2492'4924'9249ull;

          return looked_char_mask == 0 ? -1 : (sizeof(long long) * 8 - __builtin_clzll(looked_char_mask)) / 3;
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef u8 t_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const low_part         = *(t_vec*)chars;
          t_vec const high_part        = *(t_vec*)&chars[remain_bytes - 16];
          u32         looked_char_mask =
               v_16_bool_to_u16(__builtin_convertvector(low_part == looked_char.vector.s0120120120120120, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_char.vector.s2012012012012012, v_16_bool)) << (remain_bytes - 16);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x924'9249ul;

          return looked_char_mask == 0 ? -1 : (sizeof(long) * 8 - __builtin_clzl(looked_char_mask)) / 3;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef u8 t_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_vec const low_part         = *(t_vec*)chars;
          t_vec const high_part        = *(t_vec*)&chars[remain_bytes - 8];
          u16         looked_char_mask =
               v_8_bool_to_u8(__builtin_convertvector(low_part == looked_char.vector.s01201201, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_char.vector.s12012012, v_8_bool)) << (remain_bytes - 8);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249;

          return looked_char_mask == 0 ? -1 : (sizeof(int) * 8 - __builtin_clz(looked_char_mask)) / 3;
     }
     case 3: {
          assume(remain_bytes == 6);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          u32 char_1          = 0;
          memcpy_inline(&looked_char_u32, looked_char.bytes, 3);
          memcpy_inline(&char_0, chars, 3);
          memcpy_inline(&char_1, &chars[3], 3);

          switch ((char_0 == looked_char_u32) | (char_1 == looked_char_u32) * 2) {
          case 0:         return -1;
          case 1:         return 0;
          case 2: case 3: return 1;
          default:        unreachable;
          }
     }
     case 2: {
          assume(remain_bytes == 3);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          memcpy_inline(&looked_char_u32, looked_char.bytes, 3);
          memcpy_inline(&char_0, chars, 3);

          return char_0 == looked_char_u32 ? 0 : -1;
     }
     case 0:
          return -1;
     default:
          unreachable;
     }
}

static inline u64 look_2_chars_from_end(const u8* const chars, u64 const chars_len, t_any const looked_chars) {
     u64 remain_bytes = chars_len * 3;
     if (remain_bytes >= 64) {

          typedef u8  t_chars_vec __attribute__((ext_vector_type(64), aligned(1)));
          typedef u64 t_mask_vec __attribute__((ext_vector_type(2)));

          t_chars_vec const looked_first_char_vec  = looked_chars.vector.s2012012012012012012012012012012012012012012012012012012012012012;
          t_chars_vec const looked_second_char_vec = looked_chars.vector.s5345345345345345345345345345345345345345345345345345345345345345;

          do {
               t_chars_vec const chars_vec              = *(const t_chars_vec*)&chars[remain_bytes - 64];
               t_mask_vec        looked_chars_masks_vec = (const t_mask_vec){
                    v_64_bool_to_u64(__builtin_convertvector(chars_vec == looked_first_char_vec, v_64_bool)),
                    v_64_bool_to_u64(__builtin_convertvector(chars_vec == looked_second_char_vec, v_64_bool))
               };

               looked_chars_masks_vec      &= looked_chars_masks_vec >> 1;
               looked_chars_masks_vec      &= looked_chars_masks_vec >> 1;
               looked_chars_masks_vec       = looked_chars_masks_vec >> 1 & 0x1249'2492'4924'9249ull;
               looked_chars_masks_vec     >>= (const t_mask_vec){0, 3};
               u64 const looked_chars_mask  = __builtin_reduce_and(looked_chars_masks_vec);

               if (looked_chars_mask != 0) return (remain_bytes + sizeof(long long) * 8 - 64 - __builtin_clzll(looked_chars_mask)) / 3;

               remain_bytes -= 60;
          } while (remain_bytes >= 64);
     }

     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8  t_chars_vec __attribute__((ext_vector_type(32), aligned(1)));
          typedef u64 t_mask_vec __attribute__((ext_vector_type(2)));

          t_chars_vec const low_part  = *(t_chars_vec*)chars;
          t_chars_vec const high_part = *(t_chars_vec*)&chars[remain_bytes - 32];

          t_mask_vec looked_chars_masks_vec = {
               v_32_bool_to_u32(__builtin_convertvector(low_part == looked_chars.vector.s01201201201201201201201201201201, v_32_bool)) |
                    (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_chars.vector.s12012012012012012012012012012012, v_32_bool)) << (remain_bytes - 32),
               v_32_bool_to_u32(__builtin_convertvector(low_part == looked_chars.vector.s34534534534534534534534534534534, v_32_bool)) |
                    (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_chars.vector.s45345345345345345345345345345345, v_32_bool)) << (remain_bytes - 32),
          };

          looked_chars_masks_vec      &= looked_chars_masks_vec >> 1;
          looked_chars_masks_vec      &= looked_chars_masks_vec >> 1;
          looked_chars_masks_vec      &= 0x1249'2492'4924'9249ull;
          looked_chars_masks_vec     >>= (const t_mask_vec){0, 3};
          u64 const looked_chars_mask  = __builtin_reduce_and(looked_chars_masks_vec);

          return looked_chars_mask == 0 ? -1 : (sizeof(long long) * 8 - __builtin_clzll(looked_chars_mask)) / 3;
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef u8 t_chars_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_chars_vec const low_part  = *(t_chars_vec*)chars;
          t_chars_vec const high_part = *(t_chars_vec*)&chars[remain_bytes - 16];

          u64 looked_chars_masks =
               v_16_bool_to_u16(__builtin_convertvector(low_part == looked_chars.vector.s0120120120120120, v_16_bool))                                   |
                    (u64)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_chars.vector.s2012012012012012, v_16_bool)) << (remain_bytes - 16) |
               (u64)v_16_bool_to_u16(__builtin_convertvector(low_part == looked_chars.vector.s3453453453453453, v_16_bool)) << 32                        |
                    (u64)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_chars.vector.s5345345345345345, v_16_bool)) << (remain_bytes + 16);

          looked_chars_masks          &= looked_chars_masks >> 1;
          looked_chars_masks          &= looked_chars_masks >> 1;
          looked_chars_masks          &= 0x924'9249'0924'9249ull;
          u32 const looked_chars_mask  = looked_chars_masks & (looked_chars_masks >> 35);

          return looked_chars_mask == 0 ? -1 : (sizeof(long) * 8 - __builtin_clzl(looked_chars_mask)) / 3;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef u8 t_chars_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_chars_vec const low_part  = *(t_chars_vec*)chars;
          t_chars_vec const high_part = *(t_chars_vec*)&chars[remain_bytes - 8];

          u32 looked_chars_masks =
               v_8_bool_to_u8(__builtin_convertvector(low_part == looked_chars.vector.s01201201, v_8_bool))                                  |
                    (u32)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_chars.vector.s12012012, v_8_bool)) << (remain_bytes - 8) |
               (u32)v_8_bool_to_u8(__builtin_convertvector(low_part == looked_chars.vector.s34534534, v_8_bool)) << 16                       |
                    (u32)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_chars.vector.s45345345, v_8_bool)) << (remain_bytes + 8);

          looked_chars_masks          &= looked_chars_masks >> 1;
          looked_chars_masks          &= looked_chars_masks >> 1;
          looked_chars_masks          &= 0x1249'1249ul;
          u16 const looked_chars_mask  = looked_chars_masks & (looked_chars_masks >> 19);

          return looked_chars_mask == 0 ? -1 : (sizeof(int) * 8 - __builtin_clz(looked_chars_mask)) / 3;
     }
     case 3:
          assume(remain_bytes == 6);

          if (scar__memcmp(chars, looked_chars.bytes, 6) == 0)
               return 0;

          return -1;
     case 2: case 0:
          return -1;
     default:
          unreachable;
     }
}

static t_any long_string__look_from_end_in__own(t_thrd_data* const thrd_data, t_any const string, t_any const looked_char) {
     assume(string.bytes[15] == tid__string);
     assert(looked_char.bytes[15] == tid__char);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);
     u64 const looked_char_idx = look_char_from_end(chars, len, looked_char);
     if (ref_cnt == 1) free((void*)string.qwords[0]);

     return looked_char_idx == (u64)-1 ? null : (const t_any){.structure = {.value = looked_char_idx, .type = tid__int}};
}

static inline t_any short_string__look_in(t_any string, t_any const looked_char) {
     assert(string.bytes[15] == tid__short_string);
     assert(looked_char.bytes[15] == tid__char);

     u8       ctf8_looked_char[4];
     u8 const looked_char_size = corsar_code_to_ctf8_char(char_to_code(looked_char.bytes), ctf8_looked_char);

     u16 looked_char_mask;
     switch (looked_char_size) {
     case 1:
          looked_char_mask = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          break;
     case 2: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[1], v_16_bool));
          looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1);
          break;
     }
     case 3: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[2], v_16_bool));
          looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2);
          break;
     }
     case 4: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[2], v_16_bool));
          u16 const chars_mask_3 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[3], v_16_bool));
          looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2) & (chars_mask_3 >> 3);
          break;
     }
     default:
          unreachable;
     }

     return looked_char_mask == 0 ?
          null :
          (const t_any){.structure = {.value =
               __builtin_popcount(short_string__chars_start_mask(string) & ((u16)1 << __builtin_ctz(looked_char_mask)) - 1),
               .type = tid__int}};
}

static inline u64 look_char_from_begin(const u8* const chars, u64 const chars_len, t_any const looked_char) {
     u64       idx        = 0;
     u64 const chars_size = chars_len * 3;
     if (chars_size - idx >= 64) {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          t_vec const looked_char_vec = looked_char.vector.s0120120120120120120120120120120120120120120120120120120120120120;

          do {
               u64 looked_char_mask = v_64_bool_to_u64(__builtin_convertvector(*(const t_vec*)&chars[idx] == looked_char_vec, v_64_bool));
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= 0x1249'2492'4924'9249ull;
               if (looked_char_mask != 0) return (idx + __builtin_ctzll(looked_char_mask)) / 3;
               idx += 63;
          } while (chars_size - idx >= 64);
     }

     u8 const remain_bytes = chars_size - idx;
     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 t_vec __attribute__((ext_vector_type(32), aligned(1)));

          t_vec const low_part         = *(t_vec*)&chars[idx];
          t_vec const high_part        = *(t_vec*)&chars[chars_size - 32];
          u64         looked_char_mask =
               v_32_bool_to_u32(__builtin_convertvector(low_part == looked_char.vector.s01201201201201201201201201201201, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_char.vector.s12012012012012012012012012012012, v_32_bool)) << (remain_bytes - 32);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249'2492'4924'9249ull;

          return looked_char_mask == 0 ? -1 : (idx + __builtin_ctzll(looked_char_mask)) / 3;
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef u8 t_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const low_part         = *(t_vec*)&chars[idx];
          t_vec const high_part        = *(t_vec*)&chars[chars_size - 16];
          u32         looked_char_mask =
               v_16_bool_to_u16(__builtin_convertvector(low_part == looked_char.vector.s0120120120120120, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_char.vector.s2012012012012012, v_16_bool)) << (remain_bytes - 16);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x924'9249ul;

          return looked_char_mask == 0 ? -1 : (idx + __builtin_ctzl(looked_char_mask)) / 3;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef u8 t_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_vec const low_part         = *(t_vec*)&chars[idx];
          t_vec const high_part        = *(t_vec*)&chars[chars_size - 8];
          u16         looked_char_mask =
               v_8_bool_to_u8(__builtin_convertvector(low_part == looked_char.vector.s01201201, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_char.vector.s12012012, v_8_bool)) << (remain_bytes - 8);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249;

          return looked_char_mask == 0 ? -1 : (idx + __builtin_ctz(looked_char_mask)) / 3;
     }
     case 3: {
          assume(remain_bytes == 6);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          u32 char_1          = 0;
          memcpy_inline(&looked_char_u32, looked_char.bytes, 3);
          memcpy_inline(&char_0, &chars[idx], 3);
          memcpy_inline(&char_1, &chars[idx + 3], 3);

          switch ((char_0 == looked_char_u32) | (char_1 == looked_char_u32) * 2) {
          case 0:         return -1;
          case 1: case 3: return idx / 3;
          case 2:         return idx / 3 + 1;
          default:        unreachable;
          }
     }
     case 2: {
          assume(remain_bytes == 3);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          memcpy_inline(&looked_char_u32, looked_char.bytes, 3);
          memcpy_inline(&char_0, &chars[idx], 3);

          return char_0 == looked_char_u32 ? idx / 3 : -1;
     }
     case 0:
          return -1;
     default:
          unreachable;
     }
}

static inline u64 look_2_chars_from_begin(const u8* const chars, u64 const chars_len, t_any const looked_chars) {
     u64       idx        = 0;
     u64 const chars_size = chars_len * 3;
     if (chars_size - idx >= 64) {
          typedef u8  t_chars_vec __attribute__((ext_vector_type(64), aligned(1)));
          typedef u64 t_mask_vec __attribute__((ext_vector_type(2)));

          t_chars_vec const looked_first_char_vec  = looked_chars.vector.s0120120120120120120120120120120120120120120120120120120120120120;
          t_chars_vec const looked_second_char_vec = looked_chars.vector.s3453453453453453453453453453453453453453453453453453453453453453;
          do {
               t_chars_vec const chars_vec              = *(const t_chars_vec*)&chars[idx];
               t_mask_vec        looked_chars_masks_vec = (const t_mask_vec){
                    v_64_bool_to_u64(__builtin_convertvector(chars_vec == looked_first_char_vec, v_64_bool)),
                    v_64_bool_to_u64(__builtin_convertvector(chars_vec == looked_second_char_vec, v_64_bool))
               };

               looked_chars_masks_vec      &= looked_chars_masks_vec >> 1;
               looked_chars_masks_vec      &= looked_chars_masks_vec >> 1;
               looked_chars_masks_vec      &= 0x1249'2492'4924'9249ull;
               looked_chars_masks_vec     >>= (const t_mask_vec){0, 3};
               u64 const looked_chars_mask  = __builtin_reduce_and(looked_chars_masks_vec);

               if (looked_chars_mask != 0) return (idx + __builtin_ctzll(looked_chars_mask)) / 3;

               idx += 60;
          } while (chars_size - idx >= 64);
     }

     u8 const remain_bytes = chars_size - idx;
     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8  t_chars_vec __attribute__((ext_vector_type(32), aligned(1)));
          typedef u64 t_mask_vec __attribute__((ext_vector_type(2)));

          t_chars_vec const low_part  = *(t_chars_vec*)&chars[idx];
          t_chars_vec const high_part = *(t_chars_vec*)&chars[chars_size - 32];

          t_mask_vec looked_chars_masks_vec = {
               v_32_bool_to_u32(__builtin_convertvector(low_part == looked_chars.vector.s01201201201201201201201201201201, v_32_bool)) |
                    (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_chars.vector.s12012012012012012012012012012012, v_32_bool)) << (remain_bytes - 32),
               v_32_bool_to_u32(__builtin_convertvector(low_part == looked_chars.vector.s34534534534534534534534534534534, v_32_bool)) |
                    (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_chars.vector.s45345345345345345345345345345345, v_32_bool)) << (remain_bytes - 32),
          };

          looked_chars_masks_vec      &= looked_chars_masks_vec >> 1;
          looked_chars_masks_vec      &= looked_chars_masks_vec >> 1;
          looked_chars_masks_vec      &= 0x1249'2492'4924'9249ull;
          looked_chars_masks_vec     >>= (const t_mask_vec){0, 3};
          u64 const looked_chars_mask  = __builtin_reduce_and(looked_chars_masks_vec);

          return looked_chars_mask == 0 ? -1 : (idx + __builtin_ctzll(looked_chars_mask)) / 3;
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef u8 t_chars_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_chars_vec const low_part  = *(t_chars_vec*)&chars[idx];
          t_chars_vec const high_part = *(t_chars_vec*)&chars[chars_size - 16];

          u64 looked_chars_masks =
               v_16_bool_to_u16(__builtin_convertvector(low_part == looked_chars.vector.s0120120120120120, v_16_bool))                                   |
                    (u64)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_chars.vector.s2012012012012012, v_16_bool)) << (remain_bytes - 16) |
               (u64)v_16_bool_to_u16(__builtin_convertvector(low_part == looked_chars.vector.s3453453453453453, v_16_bool)) << 32                        |
                    (u64)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_chars.vector.s5345345345345345, v_16_bool)) << (remain_bytes + 16);

          looked_chars_masks          &= looked_chars_masks >> 1;
          looked_chars_masks          &= looked_chars_masks >> 1;
          looked_chars_masks          &= 0x924'9249'0924'9249ull;
          u32 const looked_chars_mask  = looked_chars_masks & (looked_chars_masks >> 35);

          return looked_chars_mask == 0 ? -1 : (idx + __builtin_ctzll(looked_chars_mask)) / 3;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef u8 t_chars_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_chars_vec const low_part  = *(t_chars_vec*)&chars[idx];
          t_chars_vec const high_part = *(t_chars_vec*)&chars[chars_size - 8];

          u32 looked_chars_masks =
               v_8_bool_to_u8(__builtin_convertvector(low_part == looked_chars.vector.s01201201, v_8_bool))                                  |
                    (u32)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_chars.vector.s12012012, v_8_bool)) << (remain_bytes - 8) |
               (u32)v_8_bool_to_u8(__builtin_convertvector(low_part == looked_chars.vector.s34534534, v_8_bool)) << 16                       |
                    (u32)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_chars.vector.s45345345, v_8_bool)) << (remain_bytes + 8);

          looked_chars_masks          &= looked_chars_masks >> 1;
          looked_chars_masks          &= looked_chars_masks >> 1;
          looked_chars_masks          &= 0x1249'1249ul;
          u16 const looked_chars_mask  = looked_chars_masks & (looked_chars_masks >> 19);

          return looked_chars_mask == 0 ? -1 : (idx + __builtin_ctzll(looked_chars_mask)) / 3;
     }
     case 3:
          assume(remain_bytes == 6);

          if (scar__memcmp(&chars[idx], looked_chars.bytes, 6) == 0)
               return idx / 3;

          return -1;
     case 2: case 0:
          return -1;
     default:
          unreachable;
     }
}

static inline t_any long_string__look_in__own(t_thrd_data* const thrd_data, t_any const string, t_any const looked_char) {
     assume(string.bytes[15] == tid__string);
     assert(looked_char.bytes[15] == tid__char);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);
     u64 const looked_char_idx = look_char_from_begin(chars, len, looked_char);
     if (ref_cnt == 1) free((void*)string.qwords[0]);

     return looked_char_idx == (u64)-1 ? null : (const t_any){.structure = {.value = looked_char_idx, .type = tid__int}};
}

static t_any short_string__look_other_from_end_in(t_any const string, t_any const not_looked_char) {
     assert(string.bytes[15] == tid__short_string);
     assert(not_looked_char.bytes[15] == tid__char);

     u8       ctf8_not_looked_char[4];
     u8 const not_looked_char_size = corsar_code_to_ctf8_char(char_to_code(not_looked_char.bytes), ctf8_not_looked_char);

     u16 not_looked_char_mask;
     switch (not_looked_char_size) {
     case 1:
          not_looked_char_mask = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[0], v_16_bool));
          break;
     case 2: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[1], v_16_bool));
          not_looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1);
          break;
     }
     case 3: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[2], v_16_bool));
          not_looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2);
          break;
     }
     case 4: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[2], v_16_bool));
          u16 const chars_mask_3 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[3], v_16_bool));
          not_looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2) & (chars_mask_3 >> 3);
          break;
     }
     default:
          unreachable;
     }

     u8  const string_size      = short_string__get_size(string);
     u16 const char_start_mask  = short_string__chars_start_mask(string);
     u16 const looked_char_mask = ~((u16)1 << string_size | not_looked_char_mask) & char_start_mask;

     return looked_char_mask == 0 ?
          null :
          (const t_any){.structure = {.value =
               __builtin_popcount(char_start_mask & ((u16)1 << (sizeof(int) * 8 - 1 - __builtin_clz(looked_char_mask))) - 1),
               .type = tid__int}};
}

static u64 look_other_char_from_end(const u8* const chars, u64 const chars_len, t_any const not_looked_char) {
     u64 remain_bytes = chars_len * 3;
     if (remain_bytes >= 64) {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          t_vec const not_looked_char_vec = not_looked_char.vector.s2012012012012012012012012012012012012012012012012012012012012012;

          do {
               u64 looked_char_mask = v_64_bool_to_u64(__builtin_convertvector(*(const t_vec*)&chars[remain_bytes - 64] == not_looked_char_vec, v_64_bool));
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask     = (looked_char_mask >> 1 & 0x1249'2492'4924'9249ull) ^ 0x1249'2492'4924'9249ull;
               if (looked_char_mask != 0) return (remain_bytes + sizeof(long long) * 8 - 64 - __builtin_clzll(looked_char_mask)) / 3;
               remain_bytes -= 63;
          } while (remain_bytes >= 64);
     }

     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 t_vec __attribute__((ext_vector_type(32), aligned(1)));

          t_vec const low_part         = *(t_vec*)chars;
          t_vec const high_part        = *(t_vec*)&chars[remain_bytes - 32];
          u64         looked_char_mask =
               v_32_bool_to_u32(__builtin_convertvector(low_part == not_looked_char.vector.s01201201201201201201201201201201, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == not_looked_char.vector.s12012012012012012012012012012012, v_32_bool)) << (remain_bytes - 32);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249'2492'4924'9249ull;
          looked_char_mask ^= 0x1249'2492'4924'9249ull;
          looked_char_mask &= ((u64)1 << remain_bytes) - 1;

          return looked_char_mask == 0 ? -1 : (sizeof(long long) * 8 - __builtin_clzll(looked_char_mask)) / 3;
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef u8 t_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const low_part         = *(t_vec*)chars;
          t_vec const high_part        = *(t_vec*)&chars[remain_bytes - 16];
          u32         looked_char_mask =
               v_16_bool_to_u16(__builtin_convertvector(low_part == not_looked_char.vector.s0120120120120120, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(high_part == not_looked_char.vector.s2012012012012012, v_16_bool)) << (remain_bytes - 16);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x924'9249ul;
          looked_char_mask ^= 0x924'9249ul;
          looked_char_mask &= ((u32)1 << remain_bytes) - 1;

          return looked_char_mask == 0 ? -1 : (sizeof(long) * 8 - __builtin_clzl(looked_char_mask)) / 3;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef u8 t_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_vec const low_part         = *(t_vec*)chars;
          t_vec const high_part        = *(t_vec*)&chars[remain_bytes - 8];
          u16         looked_char_mask =
               v_8_bool_to_u8(__builtin_convertvector(low_part == not_looked_char.vector.s01201201, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(high_part == not_looked_char.vector.s12012012, v_8_bool)) << (remain_bytes - 8);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249;
          looked_char_mask ^= 0x1249;
          looked_char_mask &= ((u16)1 << remain_bytes) - 1;

          return looked_char_mask == 0 ? -1 : (sizeof(int) * 8 - __builtin_clz(looked_char_mask)) / 3;
     }
     case 3: {
          assume(remain_bytes == 6);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          u32 char_1          = 0;
          memcpy_inline(&looked_char_u32, not_looked_char.bytes, 3);
          memcpy_inline(&char_0, chars, 3);
          memcpy_inline(&char_1, &chars[3], 3);

          switch ((char_0 != looked_char_u32) | (char_1 != looked_char_u32) * 2) {
          case 0:         return -1;
          case 1:         return 0;
          case 2: case 3: return 1;
          default:        unreachable;
          }
     }
     case 2: {
          assume(remain_bytes == 3);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          memcpy_inline(&looked_char_u32, not_looked_char.bytes, 3);
          memcpy_inline(&char_0, chars, 3);

          return char_0 != looked_char_u32 ? 0 : -1;
     }
     case 0:
          return -1;
     default:
          unreachable;
     }
}

static t_any long_string__look_other_from_end_in__own(t_thrd_data* const thrd_data, t_any const string, t_any const not_looked_char) {
     assume(string.bytes[15] == tid__string);
     assert(not_looked_char.bytes[15] == tid__char);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);
     u64 const looked_char_idx = look_other_char_from_end(chars, len, not_looked_char);
     if (ref_cnt == 1) free((void*)string.qwords[0]);

     return looked_char_idx == (u64)-1 ? null : (const t_any){.structure = {.value = looked_char_idx, .type = tid__int}};
}

static t_any short_string__look_other_in(t_any string, t_any const not_looked_char) {
     assert(string.bytes[15] == tid__short_string);
     assert(not_looked_char.bytes[15] == tid__char);

     u8       ctf8_not_looked_char[4];
     u8 const not_looked_char_size = corsar_code_to_ctf8_char(char_to_code(not_looked_char.bytes), ctf8_not_looked_char);

     u16 not_looked_char_mask;
     switch (not_looked_char_size) {
     case 1:
          not_looked_char_mask = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[0], v_16_bool));
          break;
     case 2: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[1], v_16_bool));
          not_looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1);
          break;
     }
     case 3: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[2], v_16_bool));
          not_looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2);
          break;
     }
     case 4: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[2], v_16_bool));
          u16 const chars_mask_3 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_not_looked_char[3], v_16_bool));
          not_looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2) & (chars_mask_3 >> 3);
          break;
     }
     default:
          unreachable;
     }

     u8  const string_size      = short_string__get_size(string);
     u16 const char_start_mask  = short_string__chars_start_mask(string);
     u16 const looked_char_mask = ~((u16)1 << string_size | not_looked_char_mask) & char_start_mask;

     return looked_char_mask == 0 ?
          null :
          (const t_any){.structure = {.value =
               __builtin_popcount(char_start_mask & ((u16)1 << __builtin_ctz(looked_char_mask)) - 1),
               .type = tid__int}};
}

static u64 look_other_char_from_begin(const u8* const chars, u64 const chars_len, t_any const not_looked_char) {
     u64       idx        = 0;
     u64 const chars_size = chars_len * 3;
     if (chars_size - idx >= 64) {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          t_vec const not_looked_char_vec = not_looked_char.vector.s0120120120120120120120120120120120120120120120120120120120120120;

          do {
               u64 looked_char_mask = v_64_bool_to_u64(__builtin_convertvector(*(const t_vec*)&chars[idx] == not_looked_char_vec, v_64_bool));
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= 0x1249'2492'4924'9249ull;
               looked_char_mask    ^= 0x1249'2492'4924'9249ull;
               if (looked_char_mask != 0) return (idx + __builtin_ctzll(looked_char_mask)) / 3;
               idx += 63;
          } while (chars_size - idx >= 64);
     }

     u8 const remain_bytes = chars_size - idx;
     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 t_vec __attribute__((ext_vector_type(32), aligned(1)));

          t_vec const low_part         = *(t_vec*)&chars[idx];
          t_vec const high_part        = *(t_vec*)&chars[chars_size - 32];
          u64         looked_char_mask =
               v_32_bool_to_u32(__builtin_convertvector(low_part == not_looked_char.vector.s01201201201201201201201201201201, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == not_looked_char.vector.s12012012012012012012012012012012, v_32_bool)) << (remain_bytes - 32);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249'2492'4924'9249ull;
          looked_char_mask ^= 0x1249'2492'4924'9249ull;
          looked_char_mask &= ((u64)1 << remain_bytes) - 1;

          return looked_char_mask == 0 ? -1 : (idx + __builtin_ctzll(looked_char_mask)) / 3;
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef u8 t_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const low_part         = *(t_vec*)&chars[idx];
          t_vec const high_part        = *(t_vec*)&chars[chars_size - 16];
          u32         looked_char_mask =
               v_16_bool_to_u16(__builtin_convertvector(low_part == not_looked_char.vector.s0120120120120120, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(high_part == not_looked_char.vector.s2012012012012012, v_16_bool)) << (remain_bytes - 16);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x924'9249ul;
          looked_char_mask ^= 0x924'9249ul;
          looked_char_mask &= ((u32)1 << remain_bytes) - 1;

          return looked_char_mask == 0 ? -1 : (idx + __builtin_ctzl(looked_char_mask)) / 3;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef u8 t_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_vec const low_part         = *(t_vec*)&chars[idx];
          t_vec const high_part        = *(t_vec*)&chars[chars_size - 8];
          u16         looked_char_mask =
               v_8_bool_to_u8(__builtin_convertvector(low_part == not_looked_char.vector.s01201201, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(high_part == not_looked_char.vector.s12012012, v_8_bool)) << (remain_bytes - 8);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249;
          looked_char_mask ^= 0x1249;
          looked_char_mask &= ((u16)1 << remain_bytes) - 1;

          return looked_char_mask == 0 ? -1 : (idx + __builtin_ctz(looked_char_mask)) / 3;
     }
     case 3: {
          assume(remain_bytes == 6);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          u32 char_1          = 0;
          memcpy_inline(&looked_char_u32, not_looked_char.bytes, 3);
          memcpy_inline(&char_0, &chars[idx], 3);
          memcpy_inline(&char_1, &chars[idx + 3], 3);

          switch ((char_0 != looked_char_u32) | (char_1 != looked_char_u32) * 2) {
          case 0:         return -1;
          case 1: case 3: return idx / 3;
          case 2:         return idx / 3 + 1;
          default:        unreachable;
          }
     }
     case 2: {
          assume(remain_bytes == 3);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          memcpy_inline(&looked_char_u32, not_looked_char.bytes, 3);
          memcpy_inline(&char_0, &chars[idx], 3);

          return char_0 != looked_char_u32 ? idx / 3: -1;
     }
     case 0:
          return -1;
     default:
          unreachable;
     }
}

static t_any long_string__look_other_in__own(t_thrd_data* const thrd_data, t_any const string, t_any const not_looked_char) {
     assume(string.bytes[15] == tid__string);
     assert(not_looked_char.bytes[15] == tid__char);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);
     u64 const looked_char_idx = look_other_char_from_begin(chars, len, not_looked_char);
     if (ref_cnt == 1) free((void*)string.qwords[0]);

     return looked_char_idx == (u64)-1 ? null : (const t_any){.structure = {.value = looked_char_idx, .type = tid__int}};
}

static inline t_any look_part_from_end_in__short_string__short_string(t_any const string, t_any const part) {
     assert(string.bytes[15] == tid__short_string);
     assert(part.bytes[15] == tid__short_string);

     u8 const string_len = short_string__get_len(string);
     if (part.bytes[0] == 0) return (const t_any){.structure = {.value = string_len, .type = tid__int}};

     u16 occurrence_mask = -1;
     u8  const part_size = short_string__get_size(part);
     for (u8 idx = 0; idx < part_size; idx += 1)
          occurrence_mask &= v_16_bool_to_u16(__builtin_convertvector(string.vector == part.vector[idx], v_16_bool)) >> idx;

     return occurrence_mask == 0 ?
     null :
     (const t_any){.structure = {.value =
          __builtin_popcount(short_string__chars_start_mask(string) & ((u16)1 << (sizeof(int) * 8 - 1 - __builtin_clz(occurrence_mask))) - 1),
          .type = tid__int}};
}

[[clang::noinline]]
static u64 look_string_part_from_end__rabin_karp(const u8* const string_chars, u64 const string_size, const u8* const part_chars, u64 const part_size) {
     u64 const rnd_num     = (static_rnd_num & 0x7fff'ffffull) + 16777216;
     u64       part_hash   = 0;
     u64       string_hash = 0;
     u64       power       = 1;
     memcpy_inline(&part_hash, &part_chars[part_size - 3], 3);
     memcpy_inline(&string_hash, &string_chars[string_size - 3], 3);

     u64 part_char   = 0;
     u64 string_char = 0;
     for (u64 offset = 6; offset <= part_size; offset += 3) {
          memcpy_inline(&part_char, &part_chars[part_size - offset], 3);
          memcpy_inline(&string_char, &string_chars[string_size - offset], 3);

          part_hash   = (part_hash * rnd_num + part_char) % rabin_karp_q;
          string_hash = (string_hash * rnd_num + string_char) % rabin_karp_q;
          power       = power * rnd_num % rabin_karp_q;
     }

     u64 current_char = 0;
     u64 next_char    = 0;
     for (u64 next_idx = string_size - part_size - 3 ;; next_idx -= 3) {
          if (part_hash == string_hash && scar__memcmp(&string_chars[next_idx + 3], part_chars, part_size) == 0) return (next_idx + 3) / 3;
          if (next_idx == (u64)-3) return -1;

          memcpy_inline(&current_char, &string_chars[next_idx + part_size], 3);
          memcpy_inline(&next_char, &string_chars[next_idx], 3);
          string_hash = ((string_hash - (current_char * power) % rabin_karp_q + rabin_karp_q) * rnd_num + next_char) % rabin_karp_q;
     }
}

static inline t_any look_part_from_end_in__long_string__string__own(t_thrd_data* const thrd_data, t_any const string, t_any const part) {
     assume(string.bytes[15] == tid__string);
     assume(part.bytes[15] == tid__short_string || part.bytes[15] == tid__string);

     u32       const string_slice_offset = slice__get_offset(string);
     u32       const string_slice_len    = slice__get_len(string);
     u64       const string_ref_cnt      = get_ref_cnt(string);
     u64       const string_array_len    = slice_array__get_len(string);
     const u8* const string_chars        = &((const u8*)slice_array__get_items(string))[string_slice_offset * 3];
     u64       const string_len          = string_slice_len <= slice_max_len ? string_slice_len : string_array_len;

     assert(string_slice_len <= slice_max_len || string_slice_offset == 0);

     u8        short_string_chars[45];
     const u8* part_chars;
     u64       part_len;
     u64       part_ref_cnt;
     if (part.bytes[15] == tid__short_string) {
          part_len = ctf8_str_ze_lt16_to_corsar_chars(part.bytes, short_string_chars);

          switch (part_len) {
          default:
               part_chars = short_string_chars;
               break;
          case 1: {
               u64 const char_idx = look_char_from_end(string_chars, string_len, (const t_any){.structure = {.value = *(const ua_u32*)short_string_chars, .type = tid__char}});
               ref_cnt__dec(thrd_data, string);

               return char_idx == (u64)-1 ? null : (const t_any){.structure = {.value = char_idx, .type = tid__int}};
          }
          case 0:
               ref_cnt__dec(thrd_data, string);
               return (const t_any){.structure = {.value = string_len, .type = tid__int}};
          }

          part_ref_cnt = 0;
     } else {
          u32 const part_slice_offset = slice__get_offset(part);
          u32 const part_slice_len    = slice__get_len(part);
          part_ref_cnt                = get_ref_cnt(part);
          u64 const part_array_len    = slice_array__get_len(part);
          part_chars                  = &((const u8*)slice_array__get_items(part))[part_slice_offset * 3];
          part_len                    = part_slice_len <= slice_max_len ? part_slice_len : part_array_len;

          assert(part_slice_len <= slice_max_len || part_slice_offset == 0);
     }

     if (string.qwords[0] == part.qwords[0]) {
          if (string_ref_cnt > 2) set_ref_cnt(string, string_ref_cnt - 2);
     } else {
          if (string_ref_cnt > 1) set_ref_cnt(string, string_ref_cnt - 1);
          if (part_ref_cnt > 1) set_ref_cnt(part, part_ref_cnt - 1);
     }

     t_any result;
     if (part_len > string_len)
          result = null;
     else if (part_len >= 341 && string_len >= 2730) {
          u64 const idx = look_string_part_from_end__rabin_karp(string_chars, string_len * 3, part_chars, part_len * 3);
          result = idx == (u64)-1 ? null : (const t_any){.structure = {.value = idx, .type = tid__int}};
     } else {
          u64         remain_chars       = string_len - part_len + 2;
          t_any const part_first_2_chars = {.structure = {.value = *(const ua_u64*)part_chars, .type = tid__char}};
          while (true) {
               u64 const part_in_string_idx = look_2_chars_from_end(string_chars, remain_chars, part_first_2_chars);

               if (part_in_string_idx == (u64)-1) {
                    result = null;
                    break;
               }

               if (scar__memcmp(&string_chars[part_in_string_idx * 3], part_chars, part_len * 3) == 0) {
                    result = (const t_any){.structure = {.value = part_in_string_idx, .type = tid__int}};
                    break;
               }

               remain_chars = part_in_string_idx + 1;
          }
     }

     if (string.qwords[0] == part.qwords[0]) {
          if (string_ref_cnt == 2) free((void*)string.qwords[0]);
     } else {
          if (string_ref_cnt == 1) free((void*)string.qwords[0]);
          if (part_ref_cnt == 1) free((void*)part.qwords[0]);
     }

     return result;
}

static inline t_any look_part_in__short_string__short_string(t_any const string, t_any const part) {
     assert(string.bytes[15] == tid__short_string);
     assert(part.bytes[15] == tid__short_string);

     if (part.bytes[0] == 0) return (const t_any){.structure = {.value = 0, .type = tid__int}};

     u16 occurrence_mask = -1;
     u8  const part_size = short_string__get_size(part);
     for (u8 idx = 0; idx < part_size; idx += 1)
          occurrence_mask &= v_16_bool_to_u16(__builtin_convertvector(string.vector == part.vector[idx], v_16_bool)) >> idx;
     return occurrence_mask == 0 ?
          null :
          (const t_any){.structure = {.value =
               __builtin_popcount(short_string__chars_start_mask(string) & ((u16)1 << __builtin_ctz(occurrence_mask)) - 1),
          .type = tid__int}};
}

[[clang::noinline]]
static u64 look_string_part_from_begin__rabin_karp(const u8* const string_chars, u64 const string_size, const u8* const part_chars, u64 const part_size) {
     u64 const rnd_num     = (static_rnd_num & 0x7fff'ffffull) + 16'777'216;
     u64       part_hash   = 0;
     u64       string_hash = 0;
     u64       power       = 1;
     memcpy_inline(&part_hash, part_chars, 3);
     memcpy_inline(&string_hash, string_chars, 3);

     u64 part_char   = 0;
     u64 string_char = 0;
     for (u64 idx = 3; idx < part_size; idx += 3) {
          memcpy_inline(&part_char, &part_chars[idx], 3);
          memcpy_inline(&string_char, &string_chars[idx], 3);

          part_hash   = (part_hash * rnd_num + part_char) % rabin_karp_q;
          string_hash = (string_hash * rnd_num + string_char) % rabin_karp_q;
          power       = power * rnd_num % rabin_karp_q;
     }

     u64 current_char = 0;
     u64 next_char    = 0;
     u64 const end_idx = string_size - part_size;
     for (u64 idx = 0 ;; idx += 3) {
          if (part_hash == string_hash && scar__memcmp(&string_chars[idx], part_chars, part_size) == 0) return idx / 3;
          if (idx == end_idx) return -1;

          memcpy_inline(&current_char, &string_chars[idx], 3);
          memcpy_inline(&next_char, &string_chars[idx + part_size], 3);
          string_hash = ((string_hash - (current_char * power) % rabin_karp_q + rabin_karp_q) * rnd_num + next_char) % rabin_karp_q;
     }
}

static inline t_any look_part_in__long_string__string__own(t_thrd_data* const thrd_data, t_any const string, t_any const part) {
     assume(string.bytes[15] == tid__string);
     assume(part.bytes[15] == tid__short_string || part.bytes[15] == tid__string);

     u32       const string_slice_offset = slice__get_offset(string);
     u32       const string_slice_len    = slice__get_len(string);
     u64       const string_ref_cnt      = get_ref_cnt(string);
     u64       const string_array_len    = slice_array__get_len(string);
     const u8* const string_chars        = &((const u8*)slice_array__get_items(string))[string_slice_offset * 3];
     u64       const string_len          = string_slice_len <= slice_max_len ? string_slice_len : string_array_len;

     assert(string_slice_len <= slice_max_len || string_slice_offset == 0);

     u8        short_string_chars[45];
     const u8* part_chars;
     u64       part_len;
     u64       part_ref_cnt;
     if (part.bytes[15] == tid__short_string) {
          part_len = ctf8_str_ze_lt16_to_corsar_chars(part.bytes, short_string_chars);

          switch (part_len) {
          default:
               part_chars = short_string_chars;
               break;
          case 1: {
               u64 const char_idx = look_char_from_begin(string_chars, string_len, (const t_any){.structure = {.value = *(const ua_u32*)short_string_chars, .type = tid__char}});
               ref_cnt__dec(thrd_data, string);
               return char_idx == (u64)-1 ? null : (const t_any){.structure = {.value = char_idx, .type = tid__int}};
          }
          case 0:
               ref_cnt__dec(thrd_data, string);
               return (const t_any){.structure = {.value = 0, .type = tid__int}};
          }

          part_ref_cnt = 0;
     } else {
          u32 const part_slice_offset = slice__get_offset(part);
          u32 const part_slice_len    = slice__get_len(part);
          part_ref_cnt                = get_ref_cnt(part);
          u64 const part_array_len    = slice_array__get_len(part);
          part_chars                  = &((const u8*)slice_array__get_items(part))[part_slice_offset * 3];
          part_len                    = part_slice_len <= slice_max_len ? part_slice_len : part_array_len;

          assert(part_slice_len <= slice_max_len || part_slice_offset == 0);
     }

     if (string.qwords[0] == part.qwords[0]) {
          if (string_ref_cnt > 2) set_ref_cnt(string, string_ref_cnt - 2);
     } else {
          if (string_ref_cnt > 1) set_ref_cnt(string, string_ref_cnt - 1);
          if (part_ref_cnt > 1) set_ref_cnt(part, part_ref_cnt - 1);
     }

     t_any result;
     if (part_len > string_len)
          result = null;
     else if (part_len >= 341 && string_len >= 2730) {
          u64 const idx = look_string_part_from_begin__rabin_karp(string_chars, string_len * 3, part_chars, part_len * 3);
          result = idx == (u64)-1 ? null : (const t_any){.structure = {.value = idx, .type = tid__int}};
     } else {
          u64         idx                = 0;
          u64         edge               = string_len - part_len + 2;
          t_any const part_first_2_chars = {.structure = {.value = *(const ua_u64*)part_chars, .type = tid__int}};
          while (true) {
               u64 const part_in_string_offset = look_2_chars_from_begin(&string_chars[idx * 3], edge - idx, part_first_2_chars);

               if (part_in_string_offset == (u64)-1) {
                    result = null;
                    break;
               }

               idx += part_in_string_offset;

               if (scar__memcmp(&string_chars[idx * 3], part_chars, part_len * 3) == 0) {
                    result = (const t_any){.structure = {.value = idx, .type = tid__int}};
                    break;
               }

               idx += 1;
          }
     }

     if (string.qwords[0] == part.qwords[0]) {
          if (string_ref_cnt == 2) free((void*)string.qwords[0]);
     } else {
          if (string_ref_cnt == 1) free((void*)string.qwords[0]);
          if (part_ref_cnt == 1) free((void*)part.qwords[0]);
     }

     return result;
}

static t_any short_string__map__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8       result_chars[45];
     u8 const result_len  = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, result_chars);
     u8       result_size = 0;
     t_any    char_       = {.bytes = {[15] = tid__char}};
     for (u8 idx = 0; idx < result_len; idx += 1) {
          memcpy_inline(char_.bytes, &result_chars[idx * 3], 3);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__char) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          memcpy_inline(&result_chars[idx * 3], fn_result.bytes, 3);
          result_size += char_size_in_ctf8(char_to_code(fn_result.bytes));
     }
     ref_cnt__dec(thrd_data, fn);

     if (result_size < 16) {
          t_any result          = {};
          u8*   result_char_ptr = result.bytes;
          for (u64 idx = 0; idx < result_len; result_char_ptr = &result_char_ptr[corsar_code_to_ctf8_char(char_to_code(&result_chars[idx * 3]), result_char_ptr)], idx += 1);
          return result;
     }

     t_any result = long_string__new(result_len);
     result       = slice__set_metadata(result, 0, result_len);
     slice_array__set_len(result, result_len);
     memcpy_le_64(slice_array__get_items(result), result_chars, result_len * 3);
     return result;
}

static t_any long_string__map__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(string);
     u32 const slice_offset = slice__get_offset(string);
     u8* const chars        = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any dst_string;
     u8*   dst_chars;
     if (ref_cnt == 1)
          dst_chars = chars;
     else {
          if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);

          dst_string = long_string__new(len);
          dst_string = slice__set_metadata(dst_string, 0, slice_len);
          slice_array__set_len(dst_string, len);
          dst_chars = slice_array__get_items(dst_string);
     }

     t_any char_  = {.bytes = {[15] = tid__char}};
     for (u64 idx = 0; idx < len; idx += 1) {
          memcpy_inline(char_.bytes, &chars[idx * 3], 3);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__char) [[clang::unlikely]] {
               free((void*)(ref_cnt == 1 ? string.qwords[0] : dst_string.qwords[0]));
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          memcpy_inline(&dst_chars[idx * 3], fn_result.bytes, 3);
     }

     if (len < 16) {
          t_any const short_string = short_string_from_chars(dst_chars, len);
          if (short_string.bytes[15] == tid__short_string) {
               free((void*)(ref_cnt == 1 ? string.qwords[0] : dst_string.qwords[0]));
               ref_cnt__dec(thrd_data, fn);

               return short_string;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     return ref_cnt == 1 ? string : dst_string;
}

static t_any short_string__map_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8       result_chars[45];
     u8 const result_len  = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, result_chars);
     u8       result_size = 0;
     t_any    fn_args[2]  = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     for (u8 idx = 0; idx < result_len; idx += 1) {
          fn_args[0].bytes[0] = idx;
          memcpy_inline(fn_args[1].bytes, &result_chars[idx * 3], 3);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_result.bytes[15] != tid__char) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          memcpy_inline(&result_chars[idx * 3], fn_result.bytes, 3);
          result_size += char_size_in_ctf8(char_to_code(fn_result.bytes));
     }
     ref_cnt__dec(thrd_data, fn);

     if (result_size < 16) {
          t_any result          = {};
          u8*   result_char_ptr = result.bytes;
          for (u64 idx = 0; idx < result_len; result_char_ptr = &result_char_ptr[corsar_code_to_ctf8_char(char_to_code(&result_chars[idx * 3]), result_char_ptr)], idx += 1);
          return result;
     }

     t_any result = long_string__new(result_len);
     result       = slice__set_metadata(result, 0, result_len);
     slice_array__set_len(result, result_len);
     memcpy_le_64(slice_array__get_items(result), result_chars, result_len * 3);
     return result;
}

static t_any long_string__map_kv__own(t_thrd_data* const thrd_data, t_any const string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(string);
     u32 const slice_offset = slice__get_offset(string);
     u8* const chars        = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any dst_string;
     u8*   dst_chars;
     if (ref_cnt == 1)
          dst_chars = chars;
     else {
          if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);

          dst_string = long_string__new(len);
          dst_string = slice__set_metadata(dst_string, 0, slice_len);
          slice_array__set_len(dst_string, len);
          dst_chars = slice_array__get_items(dst_string);
     }

     t_any fn_args[2] = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[0].qwords[0] = idx;
          memcpy_inline(fn_args[1].bytes, &chars[idx * 3], 3);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_result.bytes[15] != tid__char) [[clang::unlikely]] {
               free((void*)(ref_cnt == 1 ? string.qwords[0] : dst_string.qwords[0]));
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          memcpy_inline(&dst_chars[idx * 3], fn_result.bytes, 3);
     }

     if (len < 16) {
          t_any const short_string = short_string_from_chars(dst_chars, len);
          if (short_string.bytes[15] == tid__short_string) {
               free((void*)(ref_cnt == 1 ? string.qwords[0] : dst_string.qwords[0]));
               ref_cnt__dec(thrd_data, fn);

               return short_string;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     return ref_cnt == 1 ? string : dst_string;
}

static t_any short_string__map_with_state__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8       result_chars[45];
     u8 const result_len  = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, result_chars);
     u8       result_size = 0;

     t_any fn_args[2] = {state, {.bytes = {[15] = tid__char}}};
     for (u8 idx = 0; idx < result_len; idx += 1) {
          memcpy_inline(fn_args[1].bytes, &result_chars[idx * 3], 3);
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || box_items[1].bytes[15] != tid__char) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0] = box_items[0];
          memcpy_inline(&result_chars[idx * 3], box_items[1].bytes, 3);
          result_size += char_size_in_ctf8(char_to_code(box_items[1].bytes));
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any result_string;
     if (result_size < 16) {
          result_string.raw_bits = 0;
          u8* result_char_ptr    = result_string.bytes;
          for (u64 idx = 0; idx < result_len; result_char_ptr = &result_char_ptr[corsar_code_to_ctf8_char(char_to_code(&result_chars[idx * 3]), result_char_ptr)], idx += 1);
     } else {
          result_string = long_string__new(result_len);
          result_string = slice__set_metadata(result_string, 0, result_len);
          slice_array__set_len(result_string, result_len);
          memcpy_le_64(slice_array__get_items(result_string), result_chars, result_len * 3);
     }

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[0]           = result_string;
     new_box_items[1]           = fn_args[0];

     return new_box;
}

static t_any long_string__map_with_state__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(string);
     u32 const slice_offset = slice__get_offset(string);
     u8* const chars        = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any dst_string;
     u8*   dst_chars;
     if (ref_cnt == 1)
          dst_chars = chars;
     else {
          if (ref_cnt != 0 && state.qwords[0] != string.qwords[0]) set_ref_cnt(string, ref_cnt - 1);

          dst_string = long_string__new(len);
          dst_string = slice__set_metadata(dst_string, 0, slice_len);
          slice_array__set_len(dst_string, len);
          dst_chars = slice_array__get_items(dst_string);
     }

     t_any fn_args[2] = {state, {.bytes = {[15] = tid__char}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          memcpy_inline(fn_args[1].bytes, &chars[idx * 3], 3);
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               if (ref_cnt == 1)
                    free((void*)string.qwords[0]);
               else {
                    free((void*)dst_string.qwords[0]);
                    if (state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);
               }

               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || box_items[1].bytes[15] != tid__char) [[clang::unlikely]] {
               if (ref_cnt == 1)
                    free((void*)string.qwords[0]);
               else {
                    free((void*)dst_string.qwords[0]);
                    if (state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);
               }

               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0] = box_items[0];
          memcpy_inline(&dst_chars[idx * 3], box_items[1].bytes, 3);
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[1]           = fn_args[0];

     if (len < 16) {
          new_box_items[0] = short_string_from_chars(dst_chars, len);
          if (new_box_items[0].bytes[15] == tid__short_string) {
               if (ref_cnt == 1)
                    free((void*)string.qwords[0]);
               else {
                    free((void*)dst_string.qwords[0]);
                    if (state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);
               }

               ref_cnt__dec(thrd_data, fn);
               return new_box;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (ref_cnt == 1)
          new_box_items[0] = string;
     else {
          new_box_items[0] = dst_string;
          if (state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);
     }

     return new_box;
}

static t_any short_string__map_kv_with_state__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8       result_chars[45];
     u8 const result_len  = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, result_chars);
     u8       result_size = 0;

     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     for (u8 idx = 0; idx < result_len; idx += 1) {
          fn_args[1].bytes[0] = idx;
          memcpy_inline(fn_args[2].bytes, &result_chars[idx * 3], 3);
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || box_items[1].bytes[15] != tid__char) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0] = box_items[0];
          memcpy_inline(&result_chars[idx * 3], box_items[1].bytes, 3);
          result_size += char_size_in_ctf8(char_to_code(box_items[1].bytes));
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any result_string;
     if (result_size < 16) {
          result_string.raw_bits = 0;
          u8* result_char_ptr    = result_string.bytes;
          for (u64 idx = 0; idx < result_len; result_char_ptr = &result_char_ptr[corsar_code_to_ctf8_char(char_to_code(&result_chars[idx * 3]), result_char_ptr)], idx += 1);
     } else {
          result_string = long_string__new(result_len);
          result_string = slice__set_metadata(result_string, 0, result_len);
          slice_array__set_len(result_string, result_len);
          memcpy_le_64(slice_array__get_items(result_string), result_chars, result_len * 3);
     }

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[0]           = result_string;
     new_box_items[1]           = fn_args[0];

     return new_box;
}

static t_any long_string__map_kv_with_state__own(t_thrd_data* const thrd_data, t_any const string, t_any const state, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(string);
     u32 const slice_offset = slice__get_offset(string);
     u8* const chars        = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any dst_string;
     u8*   dst_chars;
     if (ref_cnt == 1)
          dst_chars = chars;
     else {
          if (ref_cnt != 0 && state.qwords[0] != string.qwords[0]) set_ref_cnt(string, ref_cnt - 1);

          dst_string = long_string__new(len);
          dst_string = slice__set_metadata(dst_string, 0, slice_len);
          slice_array__set_len(dst_string, len);
          dst_chars = slice_array__get_items(dst_string);
     }

     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__char}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[1].qwords[0] = idx;
          memcpy_inline(fn_args[2].bytes, &chars[idx * 3], 3);
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               if (ref_cnt == 1)
                    free((void*)string.qwords[0]);
               else {
                    free((void*)dst_string.qwords[0]);
                    if (state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);
               }

               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || box_items[1].bytes[15] != tid__char) [[clang::unlikely]] {
               if (ref_cnt == 1)
                    free((void*)string.qwords[0]);
               else {
                    free((void*)dst_string.qwords[0]);
                    if (state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);
               }

               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0] = box_items[0];
          memcpy_inline(&dst_chars[idx * 3], box_items[1].bytes, 3);
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[1]           = fn_args[0];

     if (len < 16) {
          new_box_items[0] = short_string_from_chars(dst_chars, len);
          if (new_box_items[0].bytes[15] == tid__short_string) {
               if (ref_cnt == 1)
                    free((void*)string.qwords[0]);
               else {
                    free((void*)dst_string.qwords[0]);
                    if (state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);
               }

               ref_cnt__dec(thrd_data, fn);
               return new_box;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (ref_cnt == 1)
          new_box_items[0] = string;
     else {
          new_box_items[0] = dst_string;
          if (state.qwords[0] == string.qwords[0]) ref_cnt__dec(thrd_data, string);
     }

     return new_box;
}

static inline t_any short_string__parse_int(t_thrd_data* const thrd_data, t_any string, u64 const base) {
     assert(string.bytes[15] == tid__short_string);
     assume(base >= 2 && base <= 36);

     typedef u64 t_vec __attribute__((ext_vector_type(8)));

     bool is_neg;
     switch (string.bytes[0]) {
     case '-':
          is_neg = true;
          string.raw_bits >>= 8;
          break;
     case '+':
          string.raw_bits >>= 8;
     default:
          is_neg = false;
          break;

     case 0:
          return tkn__invalid_str_fmt;
     }

     u8 const max_code_1 = base > 10 ? '9' : '0' + base - 1;
     u8 const max_code_2 = base > 10 ? 'A' + base - 11 : 0;
     u8 const max_code_3 = base > 10 ? 'a' + base - 11 : 0;

     if (__builtin_reduce_and(
          (
               (string.vector >= '0' & string.vector <= max_code_1) |
               (string.vector >= 'A' & string.vector <= max_code_2) |
               (string.vector >= 'a' & string.vector <= max_code_3)
          ) == (string.vector != 0)
     ) == 0) return tkn__invalid_str_fmt;

     u64 const len     = short_string__get_size(string);
     string.vector     = string.vector.sfedcba9876543210;
     string.vector    -=
          ((string.vector >= '0') & (string.vector <= '9') & '0')        |
          ((string.vector >= 'A') & (string.vector <= 'Z') & ('A' - 10)) |
          ((string.vector >= 'a') & ('a' - 10));
     string.raw_bits >>= (16 - len) * 8;
     if (__builtin_popcount(base) == 1) {
          u64 const one_char_bits = __builtin_ctz(base);
          u64 const low_part      = __builtin_reduce_or(
               __builtin_convertvector(string.vector.s01234567, t_vec) << (one_char_bits * (const t_vec){0, 1, 2, 3, 4, 5, 6, 7})
          );
          u64 const high_part     = __builtin_reduce_or(
               __builtin_convertvector(string.vector.s89abcdef, t_vec) << (one_char_bits * (const t_vec){0, 1, 2, 3, 4, 5, 6, 7})
          );

          u64 const result = low_part | high_part << one_char_bits * 8;
          if ((high_part >> (64 - 8 * one_char_bits)) != 0 || (result > 9223372036854775808ull && is_neg)) return tkn__64_bits_is_not_enough;
          return (const t_any){.structure = {.value = is_neg ? -result : result, .type = tid__int}};
     }

     u64 const base_power_2 = base * base;
     u64 const base_power_4 = base_power_2 * base_power_2;
     u64 const base_power_8 = base_power_4 * base_power_4;
     t_vec const power_vector = {
          1,
          base,
          base_power_2,
          base_power_2 * base,
          base_power_4,
          base_power_4 * base,
          base_power_4 * base_power_2,
          base_power_4 * base_power_2 * base,
     };

     u64 const low_part  = __builtin_reduce_add(__builtin_convertvector(string.vector.s01234567, t_vec) * power_vector);
     u64 const high_part = __builtin_reduce_add(__builtin_convertvector(string.vector.s89abcdef, t_vec) * power_vector);

     u64 result;
     if (
          __builtin_mul_overflow(high_part, base_power_8, &result) ||
          __builtin_add_overflow(result, low_part, &result)        ||
          (result > 9223372036854775808ull && is_neg)
     ) return tkn__64_bits_is_not_enough;

     return (const t_any){.structure = {.value = is_neg ? -result : result, .type = tid__int}};
}

static inline t_any long_string__parse_int__own(t_thrd_data* const thrd_data, t_any string, u64 const base) {
     assert(string.bytes[15] == tid__string);
     assume(base >= 2 && base <= 36);

     typedef u8  v_8_u8  __attribute__((ext_vector_type(8), aligned(1)));
     typedef u8  v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8  v_32_u8 __attribute__((ext_vector_type(32)));
     typedef u64 v_8_u64 __attribute__((ext_vector_type(8)));

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64       remain_chars = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     bool is_neg;
     switch (char_to_code(chars)) {
     case (u32)'-':
          is_neg        = true;
          remain_chars -= 1;
          chars         = &chars[3];
          break;
     case (u32)'+':
          remain_chars -= 1;
          chars         = &chars[3];
     default:
          is_neg = false;
     }

     for (; remain_chars != 0 && char_to_code(chars) == '0'; remain_chars -= 1, chars = &chars[3]);
     u8 const max_code_1 = base > 10 ? '9' : '0' + base - 1;
     u8 const max_code_2 = base > 10 ? 'A' + base - 11 : 0;
     u8 const max_code_3 = base > 10 ? 'a' + base - 11 : 0;
     v_32_u8 const max_code_1_vec = max_code_1;
     v_32_u8 const max_code_2_vec = max_code_2;
     v_32_u8 const max_code_3_vec = max_code_3;

     u64 result = 0;
     if (__builtin_popcount(base) == 1) {
          u64 const one_char_bits         = __builtin_ctz(base);
          u64 const overflow_char_mask    = ((u64)1 << one_char_bits) - 1;
          u64 const overflow_8_chars_mask = ((u64)1 << one_char_bits * 8) - 1;

          for (; remain_chars > 8; remain_chars -= 8, chars = &chars[24]) {
               v_16_u8 const low_chars_vec  = *(const v_16_u8*)chars;
               v_16_u8 const high_chars_vec = __builtin_shufflevector(*(const v_8_u8*)&chars[16], (const v_8_u8){}, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
               v_32_u8 const chars_vec = __builtin_shufflevector(low_chars_vec, high_chars_vec, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
               if (__builtin_reduce_and(
                    (
                         (chars_vec >= '0' & chars_vec <= max_code_1_vec) |
                         (chars_vec >= 'A' & chars_vec <= max_code_2_vec) |
                         (chars_vec >= 'a' & chars_vec <= max_code_3_vec)
                    ) == (__builtin_convertvector(u32_to_v_32_bool(0x92'4924ull), v_32_u8) * 255)
               ) == 0) {
                    if (ref_cnt == 1) free((void*)string.qwords[0]);
                    return tkn__invalid_str_fmt;
               }

               v_32_u8 const int_vec    = chars_vec - (((chars_vec >= '0') & (chars_vec <= '9') & '0') | ((chars_vec >= 'A') & (chars_vec <= 'Z') & ('A' - 10)) | ((chars_vec >= 'a') & ('a' - 10)));
               u64     const chars_bits = __builtin_reduce_or(
                    __builtin_convertvector(__builtin_shufflevector(int_vec, (const v_32_u8){}, 23, 20, 17, 14, 11, 8, 5, 2), v_8_u64) <<
                    (one_char_bits * (const v_8_u64){0, 1, 2, 3, 4, 5, 6, 7})
               );

               result = __builtin_rotateleft64(result, one_char_bits * 8);
               if ((result & overflow_8_chars_mask) != 0) {
                    if (ref_cnt == 1) free((void*)string.qwords[0]);
                    return tkn__64_bits_is_not_enough;
               }

               result |= chars_bits;
          }

          for (; remain_chars != 0; remain_chars -= 1, chars = &chars[3]) {
               u32 const char_code = char_to_code(chars);
               u64       int_code;
               if (char_code >= '0' && char_code <= max_code_1)      int_code = char_code - '0';
               else if (char_code >= 'A' && char_code <= max_code_2) int_code = char_code - 'A' + 10;
               else if (char_code >= 'a' && char_code <= max_code_3) int_code = char_code - 'a' + 10;
               else {
                    if (ref_cnt == 1) free((void*)string.qwords[0]);
                    return tkn__invalid_str_fmt;
               }

               result = __builtin_rotateleft64(result, one_char_bits);
               if ((result & overflow_char_mask) != 0) {
                    if (ref_cnt == 1) free((void*)string.qwords[0]);
                    return tkn__64_bits_is_not_enough;
               }
               result |= int_code;
          }

          if (result > 9223372036854775808ull && is_neg) {
               if (ref_cnt == 1) free((void*)string.qwords[0]);
               return tkn__64_bits_is_not_enough;
          }

          if (ref_cnt == 1) free((void*)string.qwords[0]);
          return (const t_any){.structure = {.value = is_neg ? -result : result, .type = tid__int}};
     }

     u64     const base_power_2 = base * base;
     u64     const base_power_4 = base_power_2 * base_power_2;
     u64     const base_power_8 = base_power_4 * base_power_4;
     v_8_u64 const power_vector = {
          1,
          base,
          base_power_2,
          base_power_2 * base,
          base_power_4,
          base_power_4 * base,
          base_power_4 * base_power_2,
          base_power_4 * base_power_2 * base,
     };

     for (; remain_chars > 8; remain_chars -= 8, chars = &chars[24]) {
          v_16_u8 const low_chars_vec  = *(const v_16_u8*)chars;
          v_16_u8 const high_chars_vec = __builtin_shufflevector(*(const v_8_u8*)&chars[16], (const v_8_u8){}, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
          v_32_u8 const chars_vec      = __builtin_shufflevector(low_chars_vec, high_chars_vec, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
          if (__builtin_reduce_and(
               (
                    (chars_vec >= '0' & chars_vec <= max_code_1_vec) |
                    (chars_vec >= 'A' & chars_vec <= max_code_2_vec) |
                    (chars_vec >= 'a' & chars_vec <= max_code_3_vec)
               ) == (__builtin_convertvector(u32_to_v_32_bool(0x92'4924ull), v_32_u8) * 255)
          ) == 0) {
               if (ref_cnt == 1) free((void*)string.qwords[0]);
               return tkn__invalid_str_fmt;
          }

          v_32_u8 const int_vec     = chars_vec - (((chars_vec >= '0') & (chars_vec <= '9') & '0') | ((chars_vec >= 'A') & (chars_vec <= 'Z') & ('A' - 10)) | ((chars_vec >= 'a') & ('a' - 10)));
          u64     const chars_value = __builtin_reduce_add(
               __builtin_convertvector(__builtin_shufflevector(int_vec, (const v_32_u8){}, 23, 20, 17, 14, 11, 8, 5, 2), v_8_u64) * power_vector
          );

          if (
               __builtin_mul_overflow(result, base_power_8, &result) ||
               __builtin_add_overflow(result, chars_value, &result)
          ) {
               if (ref_cnt == 1) free((void*)string.qwords[0]);
               return tkn__64_bits_is_not_enough;
          }
     }

     for (; remain_chars != 0; remain_chars -= 1, chars = &chars[3]) {
          u32 const char_code = char_to_code(chars);
          u64       int_code;
          if (char_code >= '0' && char_code <= max_code_1)      int_code = char_code - '0';
          else if (char_code >= 'A' && char_code <= max_code_2) int_code = char_code - 'A' + 10;
          else if (char_code >= 'a' && char_code <= max_code_3) int_code = char_code - 'a' + 10;
          else {
               if (ref_cnt == 1) free((void*)string.qwords[0]);
               return tkn__invalid_str_fmt;
          }

          if (
               __builtin_mul_overflow(result, base, &result) ||
               __builtin_add_overflow(result, int_code, &result)
          ) {
               if (ref_cnt == 1) free((void*)string.qwords[0]);
               return tkn__64_bits_is_not_enough;
          }
     }

     if (result > 9223372036854775808ull && is_neg) {
          if (ref_cnt == 1) free((void*)string.qwords[0]);
          return tkn__64_bits_is_not_enough;
     }

     if (ref_cnt == 1) free((void*)string.qwords[0]);
     return (const t_any){.structure = {.value = is_neg ? -result : result, .type = tid__int}};
}

static inline t_any prefix_of__long_string__string__own(t_thrd_data* const thrd_data, t_any const string, t_any const prefix) {
     assume(string.bytes[15] == tid__string);
     assume(prefix.bytes[15] == tid__short_string || prefix.bytes[15] == tid__string);

     u32       const string_slice_offset = slice__get_offset(string);
     u32       const string_slice_len    = slice__get_len(string);
     const u8* const string_chars        = &((const u8*)slice_array__get_items(string))[string_slice_offset * 3];
     u64       const string_ref_cnt      = get_ref_cnt(string);
     u64       const string_array_len    = slice_array__get_len(string);
     u64       const string_len          = string_slice_len <= slice_max_len ? string_slice_len : string_array_len;

     assert(string_slice_len <= slice_max_len || string_slice_offset == 0);

     u8        short_string_chars[45];
     const u8* prefix_chars;
     u64       prefix_len;
     u64       prefix_ref_cnt;
     if (prefix.bytes[15] == tid__short_string) {
          prefix_len   = ctf8_str_ze_lt16_to_corsar_chars(prefix.bytes, short_string_chars);
          prefix_chars = short_string_chars;
          prefix_ref_cnt = 0;
     } else {
          u32 const prefix_slice_offset = slice__get_offset(prefix);
          u32 const prefix_slice_len    = slice__get_len(prefix);
          prefix_chars                  = &((const u8*)slice_array__get_items(prefix))[prefix_slice_offset * 3];
          prefix_ref_cnt                = get_ref_cnt(prefix);
          u64 const prefix_array_len    = slice_array__get_len(prefix);
          prefix_len                    = prefix_slice_len <= slice_max_len ? prefix_slice_len : prefix_array_len;

          assert(prefix_slice_len <= slice_max_len || prefix_slice_offset == 0);
     }

     if (string.qwords[0] == prefix.qwords[0] && prefix.bytes[15] == tid__string) {
          if (string_ref_cnt > 2) set_ref_cnt(string, string_ref_cnt - 2);
     } else {
          if (string_ref_cnt > 1) set_ref_cnt(string, string_ref_cnt - 1);
          if (prefix_ref_cnt > 1) set_ref_cnt(prefix, prefix_ref_cnt - 1);
     }

     t_any const result = (string_len >= prefix_len && scar__memcmp(string_chars, prefix_chars, prefix_len * 3) == 0) ? bool__true : bool__false;

     if (string.qwords[0] == prefix.qwords[0] && prefix.bytes[15] == tid__string) {
          if (string_ref_cnt == 2) free((void*)string.qwords[0]);
     } else {
          if (string_ref_cnt == 1) free((void*)string.qwords[0]);
          if (prefix_ref_cnt == 1) free((void*)prefix.qwords[0]);
     }

     return result;
}

[[clang::noinline]]
static t_any string__print(t_thrd_data* const thrd_data, t_any const string, const char* const owner, FILE* const file) {
     assert(string.bytes[15] == tid__string || string.bytes[15] == tid__short_string);

     u8 buffer[1024];
     if (string.bytes[15] == tid__short_string) {
          u64 const  string_size = short_string__get_size(string);
          bool const is_ascii    = __builtin_reduce_and(string.vector < 128 & string.vector != 0x1a) != 0;
          if (is_ascii) {
               if (fwrite(string.bytes, 1, string_size, file) != string_size) [[clang::unlikely]] goto cant_print_label;
               return null;
          }

          t_str_cvt_result const cvt_result = ctf8_chars_to_sys_chars(string_size, string.bytes, 1024, buffer);
          if (cvt_result.src_offset != string_size) [[clang::unlikely]] goto fail_text_recoding_label;

          if (fwrite(buffer, 1, cvt_result.dst_offset, file) != cvt_result.dst_offset) [[clang::unlikely]] goto cant_print_label;

          return null;
     }

     u32 const slice_offset    = slice__get_offset(string);
     u32 const slice_len       = slice__get_len(string);
     u64 const string_size     = (slice_len <= slice_max_len ? slice_len : slice_array__get_len(string)) * 3;
     u8* const chars           = &((u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     for (u64 string_offset = 0; string_offset < string_size;) {
          t_str_cvt_result const cvt_result = corsar_chars_to_sys_chars(string_size - string_offset, &chars[string_offset], 1024, buffer);

          if ((i64)cvt_result.src_offset <= 0) [[clang::unlikely]] goto fail_text_recoding_label;

          if (fwrite(buffer, 1, cvt_result.dst_offset, file) != cvt_result.dst_offset) [[clang::unlikely]] goto cant_print_label;

          string_offset += cvt_result.src_offset;
     }

     return null;

     cant_print_label:
     return error__cant_print(thrd_data, owner);

     fail_text_recoding_label:
     return error__fail_text_recoding(thrd_data, owner);
}

static t_any short_string__qty(t_any const string, t_any const looked_char) {
     assert(string.bytes[15] == tid__short_string);
     assert(looked_char.bytes[15] == tid__char);

     u8       ctf8_looked_char[4];
     u8 const looked_char_size = corsar_code_to_ctf8_char(char_to_code(looked_char.bytes), ctf8_looked_char);

     u16 looked_char_mask;
     switch (looked_char_size) {
     case 1:
          looked_char_mask = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          break;
     case 2: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[1], v_16_bool));
          looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1);
          break;
     }
     case 3: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[2], v_16_bool));
          looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2);
          break;
     }
     case 4: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[2], v_16_bool));
          u16 const chars_mask_3 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_looked_char[3], v_16_bool));
          looked_char_mask       = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2) & (chars_mask_3 >> 3);
          break;
     }
     default:
          unreachable;
     }

     return (const t_any){.structure = {.value = __builtin_popcount(looked_char_mask), .type = tid__int}};
}

static inline u64 qty_char(const u8* const chars, u64 const chars_size, t_any const looked_char) {
     u64 idx    = 0;
     u64 result = 0;
     if (chars_size - idx >= 64) {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          t_vec const looked_char_vec = looked_char.vector.s0120120120120120120120120120120120120120120120120120120120120120;

          do {
               u64 looked_char_mask = v_64_bool_to_u64(__builtin_convertvector(*(const t_vec*)&chars[idx] == looked_char_vec, v_64_bool));
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= 0x1249'2492'4924'9249ull;
               result += __builtin_popcountll(looked_char_mask);
               idx    += 63;
          } while (chars_size - idx >= 64);
     }

     u8 const remain_bytes = chars_size - idx;
     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 t_vec __attribute__((ext_vector_type(32), aligned(1)));

          t_vec const low_part         = *(t_vec*)&chars[idx];
          t_vec const high_part        = *(t_vec*)&chars[chars_size - 32];
          u64         looked_char_mask =
               v_32_bool_to_u32(__builtin_convertvector(low_part == looked_char.vector.s01201201201201201201201201201201, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_char.vector.s12012012012012012012012012012012, v_32_bool)) << (remain_bytes - 32);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249'2492'4924'9249ull;

          return result + __builtin_popcountll(looked_char_mask);
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef u8 t_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const low_part         = *(t_vec*)&chars[idx];
          t_vec const high_part        = *(t_vec*)&chars[chars_size - 16];
          u32         looked_char_mask =
               v_16_bool_to_u16(__builtin_convertvector(low_part == looked_char.vector.s0120120120120120, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_char.vector.s2012012012012012, v_16_bool)) << (remain_bytes - 16);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x924'9249ul;

          return result + __builtin_popcountl(looked_char_mask);
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef u8 t_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_vec const low_part         = *(t_vec*)&chars[idx];
          t_vec const high_part        = *(t_vec*)&chars[chars_size - 8];
          u16         looked_char_mask =
               v_8_bool_to_u8(__builtin_convertvector(low_part == looked_char.vector.s01201201, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_char.vector.s12012012, v_8_bool)) << (remain_bytes - 8);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249;

          return result + __builtin_popcount(looked_char_mask);
     }
     case 3: {
          assume(remain_bytes == 6);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          u32 char_1          = 0;
          memcpy_inline(&looked_char_u32, looked_char.bytes, 3);
          memcpy_inline(&char_0, &chars[idx], 3);
          memcpy_inline(&char_1, &chars[idx + 3], 3);

          return result + (char_0 == looked_char_u32) + (char_1 == looked_char_u32);
     }
     case 2: {
          assume(remain_bytes == 3);

          u32 looked_char_u32 = 0;
          u32 char_0          = 0;
          memcpy_inline(&looked_char_u32, looked_char.bytes, 3);
          memcpy_inline(&char_0, &chars[idx], 3);

          return result + (char_0 == looked_char_u32);
     }
     case 0:
          return result;
     default:
          unreachable;
     }
}

static t_any long_string__qty__own(t_thrd_data* const thrd_data, t_any const string, t_any const looked_char) {
     assume(string.bytes[15] == tid__string);
     assert(looked_char.bytes[15] == tid__char);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     u64 const result = qty_char(chars, len * 3, looked_char);

     if (ref_cnt == 1) free((void*)string.qwords[0]);

     return (const t_any){.structure = {.value = result, .type = tid__int}};
}

static t_any short_string__repeat(t_thrd_data* const thrd_data, t_any const part, u64 const count, const char* const owner) {
     assert(part.bytes[15] == tid__short_string);

     u64 const  part_len   = short_string__get_len(part);
     u64        string_len;
     bool const overflow   = __builtin_mul_overflow(part_len, count, &string_len);
     if (string_len > array_max_len || overflow) [[clang::unlikely]]
          return error__out_of_bounds(thrd_data, owner);

     if (part_len == 0 || count == 0) return (const t_any){};

     if (string_len < 16) {
          u64 const part_size   = short_string__get_size(part);
          u64 const string_size = part_size * count;
          if (string_size < 16) {
               t_any string = part;
               for (u64 counter = 1; counter < count; counter += 1) {
                    string.raw_bits <<= part_size * 8;
                    string.qwords[0] |= part.qwords[0];
               }
               return string;
          }
     }

     t_any     string       = long_string__new(string_len);
     slice_array__set_len(string, string_len);
     string                 = slice__set_metadata(string, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);
     u8* const string_chars = slice_array__get_items(string);

     ctf8_str_ze_lt16_to_corsar_chars(part.bytes, string_chars);
     u64 const part_size_in_string = part_len * 3;
     for (
          u64 part_idx = 1;
          part_idx < count;

          memcpy_le_64(&string_chars[part_size_in_string * part_idx], &string_chars[part_size_in_string * (part_idx - 1)], part_size_in_string),
          part_idx += 1
     );

     return string;
}

static t_any long_string__repeat__own(t_thrd_data* const thrd_data, t_any const part, u64 const count, const char* const owner) {
     assert(part.bytes[15] == tid__string);

     u32 const slice_offset   = slice__get_offset(part);
     u32 const slice_len      = slice__get_len(part);
     u64 const ref_cnt        = get_ref_cnt(part);
     u64 const part_array_len = slice_array__get_len(part);
     u64 const part_len       = slice_len <= slice_max_len ? slice_len : part_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64        string_len;
     bool const overflow   = __builtin_mul_overflow(part_len, count, &string_len);

     if (string_len > array_max_len || overflow) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, part);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (count < 2) {
          if (count == 0) {
               ref_cnt__dec(thrd_data, part);
               return (const t_any){};
          }

          return part;
     }

     u64 const part_size_in_string = part_len * 3;
     t_any     string;
     if (ref_cnt == 1) {
          u64 const array_cap = slice_array__get_cap(part);
          string              = part;
          if (array_cap < string_len) {
               string.qwords[0]  = (u64)realloc((u8*)string.qwords[0], string_len * 3 + 16);
               slice_array__set_cap(string, string_len);
          }

          u8* const chars = slice_array__get_items(string);
          if (slice_offset != 0) memmove(chars, &chars[slice_offset * 3], part_size_in_string);
     } else {
          if (ref_cnt != 0) set_ref_cnt(part, ref_cnt - 1);

          string = long_string__new(string_len);
          memcpy(slice_array__get_items(string), &((const u8*)slice_array__get_items(part))[slice_offset * 3], part_size_in_string);
     }

     slice_array__set_len(string, string_len);
     string                 = slice__set_metadata(string, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);
     u8* const string_chars = slice_array__get_items(string);

     for (
          u64 part_idx = 1;
          part_idx < count;

          memcpy(&string_chars[part_size_in_string * part_idx], &string_chars[part_size_in_string * (part_idx - 1)], part_size_in_string),
          part_idx += 1
     );

     return string;
}

static inline t_any short_string__replace(t_any string, t_any const old_char, t_any const new_char) {
     assert(string.bytes[15] == tid__short_string);
     assert(old_char.bytes[15] == tid__char);
     assert(new_char.bytes[15] == tid__char);

     u8       ctf8_old_char[4];
     u8       ctf8_new_char[4];
     u8 const old_char_size = corsar_code_to_ctf8_char(char_to_code(old_char.bytes), ctf8_old_char);
     u8 const new_char_size = corsar_code_to_ctf8_char(char_to_code(new_char.bytes), ctf8_new_char);

     u16 old_char_mask;
     switch (old_char_size) {
     case 1:
          if (new_char_size == 1) {
               string.chars_vec = (string.chars_vec != ctf8_old_char[0]) & string.chars_vec | ((string.chars_vec == ctf8_old_char[0]) & ctf8_new_char[0]);
               return string;
          }

          old_char_mask = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[0], v_16_bool));
          break;
     case 2: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[1], v_16_bool));
          old_char_mask          = chars_mask_0 & (chars_mask_1 >> 1);
          break;
     }
     case 3: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[2], v_16_bool));
          old_char_mask          = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2);
          break;
     }
     case 4: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[2], v_16_bool));
          u16 const chars_mask_3 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_old_char[3], v_16_bool));
          old_char_mask          = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2) & (chars_mask_3 >> 3);
          break;
     }
     default:
          unreachable;
     }

     u8 const old_chars_qty = __builtin_popcount(old_char_mask);
     if (old_chars_qty == 0) return string;

     i8 const delta_char_size = new_char_size - old_char_size;
     i8 const old_size        = short_string__get_size(string);
     i8 const new_size        = old_size + delta_char_size * old_chars_qty;
     if (new_size < 16) {
          if (delta_char_size > 0) {
               u16 old_char_mask_copy = old_char_mask;
               u8  item_counter       = old_chars_qty - 1;
               do {
                    u8   const old_char_idx = sizeof(int) * 8 - 1 - __builtin_clz(old_char_mask_copy);
                    u128 const first_part   = string.raw_bits & ((u128)1 << (old_char_idx * 8)) - 1;
                    u128 const last_part    = string.raw_bits & (u128)-1 << (old_char_idx + old_char_size) * 8;
                    string.raw_bits         = first_part | (last_part << delta_char_size * 8);

                    u16 const char_bit  = (u16)1 << old_char_idx;
                    old_char_mask_copy ^= char_bit;
                    old_char_mask      ^= char_bit;
                    old_char_mask      |= char_bit << delta_char_size * item_counter;
                    item_counter       -= 1;
               } while (old_char_mask_copy != 0);
          }

          switch (new_char_size) {
          case 1:
               string.vector &= __builtin_convertvector(u16_to_v_16_bool((u16)~old_char_mask), v_any_u8) * 255;
               string.vector |= __builtin_convertvector(u16_to_v_16_bool(old_char_mask), v_any_u8) * ctf8_new_char[0];
               break;
          case 2:
               string.vector &= __builtin_convertvector(u16_to_v_16_bool((u16)~(old_char_mask | old_char_mask << 1)), v_any_u8) * 255;
               string.vector |=
                    (__builtin_convertvector(u16_to_v_16_bool(old_char_mask), v_any_u8) * ctf8_new_char[0]) |
                    (__builtin_convertvector(u16_to_v_16_bool(old_char_mask << 1), v_any_u8) * ctf8_new_char[1]);
               break;
          case 3:
               string.vector &= __builtin_convertvector(u16_to_v_16_bool((u16)~(old_char_mask | old_char_mask << 1 | old_char_mask << 2)), v_any_u8) * 255;
               string.vector |=
                    (__builtin_convertvector(u16_to_v_16_bool(old_char_mask), v_any_u8) * ctf8_new_char[0])      |
                    (__builtin_convertvector(u16_to_v_16_bool(old_char_mask << 1), v_any_u8) * ctf8_new_char[1]) |
                    (__builtin_convertvector(u16_to_v_16_bool(old_char_mask << 2), v_any_u8) * ctf8_new_char[2]);
               break;
          case 4:
               string.vector &= __builtin_convertvector(u16_to_v_16_bool((u16)~(old_char_mask | old_char_mask << 1 | old_char_mask << 2 | old_char_mask << 3)), v_any_u8) * 255;
               string.vector |=
                    (__builtin_convertvector(u16_to_v_16_bool(old_char_mask), v_any_u8) * ctf8_new_char[0])      |
                    (__builtin_convertvector(u16_to_v_16_bool(old_char_mask << 1), v_any_u8) * ctf8_new_char[1]) |
                    (__builtin_convertvector(u16_to_v_16_bool(old_char_mask << 2), v_any_u8) * ctf8_new_char[2]) |
                    (__builtin_convertvector(u16_to_v_16_bool(old_char_mask << 3), v_any_u8) * ctf8_new_char[3]);
               break;
          default:
               unreachable;
          }

          if (delta_char_size >= 0) return string;

          assume(old_char_size <= 4);
          assume(new_char_size < old_char_size);

          do {
               u8   const old_char_idx = sizeof(int) * 8 - 1 - __builtin_clz(old_char_mask);
               u128 const first_part   = string.raw_bits & ((u128)1 << ((old_char_idx + new_char_size) * 8)) - 1;
               u128 const last_part    = string.raw_bits & (u128)-1 << (old_char_idx + old_char_size) * 8;
               string.raw_bits         = first_part | last_part >> delta_char_size * -8;
               old_char_mask          ^= (u16)1 << old_char_idx;
          } while (old_char_mask != 0);

          return string;
     }

     u8  const len         = short_string__get_len(string);
     t_any     long_string = long_string__new(len);
     slice_array__set_len(long_string, len);
     long_string           = slice__set_metadata(long_string, 0, len);
     u8* const chars       = slice_array__get_items(long_string);

     u8 short_idx = 0;
     for (u8 long_idx = 0; long_idx < len; long_idx += 1)
          if ((old_char_mask >> short_idx & 1) == 1) {
               memcpy_inline(&chars[long_idx * 3], new_char.bytes, 3);
               short_idx += old_char_size;
          } else short_idx += ctf8_char_to_corsar_char(&string.bytes[short_idx], &chars[long_idx * 3]);

     return long_string;
}

static inline t_any long_string__replace__own(t_thrd_data* const thrd_data, t_any const string, t_any const old_char, t_any const new_char) {
     assume(string.bytes[15] == tid__string);
     assert(old_char.bytes[15] == tid__char);

     if (old_char.qwords[0] == new_char.qwords[0]) return string;

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;
     u64 const chars_size   = len * 3;
     u8* const src_chars    = &((u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any      dst_string;
     u8*        dst_chars = src_chars;
     bool const is_mut    = ref_cnt == 1;
     bool       changed   = false;
     u64        idx       = 0;
     if (chars_size - idx >= 64) {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          t_vec const old_char_vec = old_char.vector.s0120120120120120120120120120120120120120120120120120120120120120;
          t_vec const new_char_vec = new_char.vector.s0120120120120120120120120120120120120120120120120120120120120120;
          do {
               t_vec const chars_vec     = *(const t_vec*)&src_chars[idx];
               u64         old_char_mask = v_64_bool_to_u64(__builtin_convertvector(chars_vec == old_char_vec, v_64_bool));
               old_char_mask            &= old_char_mask >> 1;
               old_char_mask            &= old_char_mask >> 1;
               old_char_mask            &= 0x1249'2492'4924'9249ull;
               old_char_mask            |= old_char_mask << 1;
               old_char_mask            |= old_char_mask << 1;
               if (old_char_mask != 0) {
                    if (!is_mut && dst_chars == src_chars) {
                         if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                         changed    = true;
                         dst_string = long_string__new(len);
                         slice_array__set_len(dst_string, len);
                         dst_string = slice__set_metadata(dst_string, 0, slice_len);
                         dst_chars  = slice_array__get_items(dst_string);
                         memcpy(dst_chars, src_chars, idx);
                    }

                    *(t_vec*)&dst_chars[idx] = (chars_vec & __builtin_convertvector(u64_to_v_64_bool(~old_char_mask), t_vec) * 255) |
                         __builtin_convertvector(u64_to_v_64_bool(old_char_mask), t_vec) * new_char_vec;
               } else if (dst_chars != src_chars) *(t_vec*)&dst_chars[idx] = chars_vec;

               idx += 63;
          } while (chars_size - idx >= 64);
     }

     u8 const remain_bytes = chars_size - idx;
     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 t_vec __attribute__((ext_vector_type(32), aligned(1)));

          t_vec const low_part      = *(t_vec*)&src_chars[idx];
          u32         low_part_mask = v_32_bool_to_u32(__builtin_convertvector(low_part == old_char.vector.s01201201201201201201201201201201, v_32_bool));
          low_part_mask            &= low_part_mask >> 1;
          low_part_mask            &= low_part_mask >> 1;
          low_part_mask            &= 0x924'9249ul;
          low_part_mask            |= low_part_mask << 1;
          low_part_mask            |= low_part_mask << 1;

          t_vec const high_part      = *(t_vec*)&src_chars[chars_size - 32];
          u32         high_part_mask = v_32_bool_to_u32(__builtin_convertvector(high_part == old_char.vector.s12012012012012012012012012012012, v_32_bool));
          high_part_mask            &= high_part_mask >> 1;
          high_part_mask            &= high_part_mask >> 1;
          high_part_mask            &= 0x2492'4924ul;
          high_part_mask            |= high_part_mask << 1;
          high_part_mask            |= high_part_mask << 1;

          u32 mid_char     = 0;
          u32 old_char_u32 = 0;
          memcpy_inline(&mid_char, &src_chars[chars_size - 33], 3);
          memcpy_inline(&old_char_u32, old_char.bytes, 3);

          if ((low_part_mask | high_part_mask) != 0 || mid_char == old_char_u32) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    changed    = true;
                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               *(t_vec*)&dst_chars[idx] = (low_part & __builtin_convertvector(u32_to_v_32_bool(~low_part_mask), t_vec) * 255) |
                    __builtin_convertvector(u32_to_v_32_bool(low_part_mask), t_vec) * new_char.vector.s01201201201201201201201201201201;
               *(t_vec*)&dst_chars[chars_size - 32] = (high_part & __builtin_convertvector(u32_to_v_32_bool(~high_part_mask), t_vec) * 255) |
                    __builtin_convertvector(u32_to_v_32_bool(high_part_mask), t_vec) * new_char.vector.s12012012012012012012012012012012;

               if (mid_char == old_char_u32) memcpy_inline(&dst_chars[chars_size - 33], new_char.bytes, 3);
          } else if (dst_chars != src_chars) memmove_le_64(&dst_chars[idx], &src_chars[idx], remain_bytes);

          break;
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef u8 t_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const low_part      = *(t_vec*)&src_chars[idx];
          u16         low_part_mask = v_16_bool_to_u16(__builtin_convertvector(low_part == old_char.vector.s0120120120120120, v_16_bool));
          low_part_mask            &= low_part_mask >> 1;
          low_part_mask            &= low_part_mask >> 1;
          low_part_mask            &= 0x1249;
          low_part_mask            |= low_part_mask << 1;
          low_part_mask            |= low_part_mask << 1;

          t_vec const high_part      = *(t_vec*)&src_chars[chars_size - 16];
          u16         high_part_mask = v_16_bool_to_u16(__builtin_convertvector(high_part == old_char.vector.s2012012012012012, v_16_bool));
          high_part_mask            &= high_part_mask >> 1;
          high_part_mask            &= high_part_mask >> 1;
          high_part_mask            &= 0x2492;
          high_part_mask            |= high_part_mask << 1;
          high_part_mask            |= high_part_mask << 1;

          if ((low_part_mask | high_part_mask) != 0) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    changed    = true;
                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               u32 mid_char     = 0;
               u32 old_char_u32 = 0;
               memcpy_inline(&mid_char, &src_chars[chars_size - 18], 3);
               memcpy_inline(&old_char_u32, old_char.bytes, 3);

               *(t_vec*)&dst_chars[idx] = (low_part & __builtin_convertvector(u16_to_v_16_bool(~low_part_mask), t_vec) * 255) |
                    __builtin_convertvector(u16_to_v_16_bool(low_part_mask), t_vec) * new_char.vector.s0120120120120120;
               *(t_vec*)&dst_chars[chars_size - 16] = (high_part & __builtin_convertvector(u16_to_v_16_bool(~high_part_mask), t_vec) * 255) |
                    __builtin_convertvector(u16_to_v_16_bool(high_part_mask), t_vec) * new_char.vector.s2012012012012012;

               if (mid_char == old_char_u32) memcpy_inline(&dst_chars[chars_size - 18], new_char.bytes, 3);
          } else if (dst_chars != src_chars) memmove_le_32(&dst_chars[idx], &src_chars[idx], remain_bytes);

          break;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef u8 t_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_vec const low_part      = *(t_vec*)&src_chars[idx];
          u8          low_part_mask = v_8_bool_to_u8(__builtin_convertvector(low_part == old_char.vector.s01201201, v_8_bool));
          low_part_mask            &= low_part_mask >> 1;
          low_part_mask            &= low_part_mask >> 1;
          low_part_mask            &= 9;
          low_part_mask            |= low_part_mask << 1;
          low_part_mask            |= low_part_mask << 1;

          t_vec const high_part      = *(t_vec*)&src_chars[chars_size - 8];
          u8          high_part_mask = v_8_bool_to_u8(__builtin_convertvector(high_part == old_char.vector.s12012012, v_8_bool));
          high_part_mask            &= high_part_mask >> 1;
          high_part_mask            &= high_part_mask >> 1;
          high_part_mask            &= 0x24;
          high_part_mask            |= high_part_mask << 1;
          high_part_mask            |= high_part_mask << 1;

          u32 mid_char     = 0;
          u32 old_char_u32 = 0;
          memcpy_inline(&mid_char, &src_chars[chars_size - 9], 3);
          memcpy_inline(&old_char_u32, old_char.bytes, 3);

          if ((low_part_mask | high_part_mask) != 0) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    changed    = true;
                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               *(t_vec*)&dst_chars[idx] = (low_part & __builtin_convertvector(u8_to_v_8_bool(~low_part_mask), t_vec) * 255) |
                    __builtin_convertvector(u8_to_v_8_bool(low_part_mask), t_vec) * new_char.vector.s01201201;
               *(t_vec*)&dst_chars[chars_size - 8] = (high_part & __builtin_convertvector(u8_to_v_8_bool(~high_part_mask), t_vec) * 255) |
                    __builtin_convertvector(u8_to_v_8_bool(high_part_mask), t_vec) * new_char.vector.s12012012;

               if (mid_char == old_char_u32) memcpy_inline(&dst_chars[chars_size - 9], new_char.bytes, 3);
          } else if (dst_chars != src_chars) memmove_le_16(&dst_chars[idx], &src_chars[idx], remain_bytes);

          break;
     }
     case 3: {
          assume(remain_bytes == 6);

          u32 old_char_u32 = 0;
          u32 char_0       = 0;
          u32 char_1       = 0;
          memcpy_inline(&old_char_u32, old_char.bytes, 3);
          memcpy_inline(&char_0, &src_chars[idx], 3);
          memcpy_inline(&char_1, &src_chars[idx + 3], 3);

          u8 const variant = (char_0 == old_char_u32) | (char_1 == old_char_u32) * 2;
          if (variant != 0) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    changed    = true;
                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               switch (variant) {
               case 3:
                    memcpy_inline(&dst_chars[idx], new_char.bytes, 3);
               case 2:
                    memcpy_inline(&dst_chars[idx + 3], new_char.bytes, 3);
                    break;
               case 1:
                    memcpy_inline(&dst_chars[idx], new_char.bytes, 3);
                    break;
               default:
                    unreachable;
               }
          } else if (dst_chars != src_chars) memmove_le_16(&dst_chars[idx], &src_chars[idx], 6);

          break;
     }
     case 2: {
          assume(remain_bytes == 3);

          u32 old_char_u32 = 0;
          u32 char_0       = 0;
          memcpy_inline(&old_char_u32, old_char.bytes, 3);
          memcpy_inline(&char_0, &src_chars[idx], 3);

          if (char_0 == old_char_u32) {
               if (!is_mut && dst_chars == src_chars) {
                    if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

                    changed    = true;
                    dst_string = long_string__new(len);
                    slice_array__set_len(dst_string, len);
                    dst_string = slice__set_metadata(dst_string, 0, slice_len);
                    dst_chars  = slice_array__get_items(dst_string);
                    memcpy(dst_chars, src_chars, idx);
               }

               memcpy_inline(&dst_chars[idx], new_char.bytes, 3);
          } else if (dst_chars != src_chars) memmove_le_16(&dst_chars[idx], &src_chars[idx], 3);

          break;
     }
     case 0:
          break;
     default:
          unreachable;
     }

     if (!changed) return string;

     if (len < 16) {
          t_any const short_string = short_string_from_chars(dst_chars, len);
          if (short_string.bytes[15] == tid__short_string) {
               free((void*)(dst_chars == src_chars ? string.qwords[0] : dst_string.qwords[0]));
               return short_string;
          }
     }

     return dst_chars == src_chars ? string : dst_string;
}

static inline t_any string__replace_part__own(t_thrd_data* const thrd_data, t_any string, t_any const old_part, t_any const new_part, const char* const owner) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);
     assert(old_part.bytes[15] == tid__short_string || old_part.bytes[15] == tid__string);
     assert(new_part.bytes[15] == tid__short_string || new_part.bytes[15] == tid__string);

     if ((string.bytes[15] == tid__short_string && old_part.bytes[15] == tid__short_string && new_part.bytes[15] == tid__short_string)) {
          u8 const string_len   = short_string__get_len(string);
          u8 const old_part_len = short_string__get_len(old_part);
          u8 const new_part_len = short_string__get_len(new_part);
          if (old_part.raw_bits == new_part.raw_bits || old_part_len > string_len) return string;

          if (old_part_len == 1 && new_part_len == 1) {
               t_any old_char = {.bytes = {[15] = tid__char}};
               t_any new_char = {.bytes = {[15] = tid__char}};
               ctf8_char_to_corsar_char(old_part.bytes, old_char.bytes);
               ctf8_char_to_corsar_char(new_part.bytes, new_char.bytes);

               return short_string__replace(string, old_char, new_char);
          }

          u8 const string_size   = short_string__get_size(string);
          u8 const new_part_size = short_string__get_size(new_part);
          if (old_part_len == 0 && (string_len + 1) * new_part_size + string_size < 16) {
               t_any dst_string      = new_part;
               t_any char_as_string  = {};
               u16   chars_star_mask = short_string__chars_start_mask(string) | (u16)1 << string_size;
               for (u64 idx = 0; idx < string_len; idx += 1) {
                    u8 const first_char_size = __builtin_ctz(chars_star_mask ^ (u16)1);
                    chars_star_mask        >>= first_char_size;
                    char_as_string.raw_bits  = string.raw_bits & ((u128)1 << first_char_size * 8) - 1;
                    string.raw_bits        >>= first_char_size * 8;
                    dst_string               = concat__short_str__short_str(dst_string, char_as_string);
                    dst_string               = concat__short_str__short_str(dst_string, new_part);
               }

               return dst_string;
          }
     }

     u8 old_part_buffer[61];
     u8 new_part_buffer[61];
     t_any old_part_as_long;
     t_any new_part_as_long;
     if (old_part.bytes[15] == tid__short_string) {
          if (old_part.raw_bits == new_part.raw_bits) return string;

          u8 const len               = ctf8_str_ze_lt16_to_corsar_chars(old_part.bytes, &old_part_buffer[16]);
          old_part_as_long.qwords[0] = (u64)old_part_buffer;
          old_part_as_long.bytes[15] = tid__string;
          old_part_as_long           = slice__set_metadata(old_part_as_long, 0, len);
#ifndef NDEBUG
          set_ref_cnt(old_part_as_long, 1);
#endif
          slice_array__set_len(old_part_as_long, len);
          slice_array__set_cap(old_part_as_long, 15);
          set_ref_cnt(old_part_as_long, 0);
     } else old_part_as_long = old_part;

     if (new_part.bytes[15] == tid__short_string) {
          u8 const len               = ctf8_str_ze_lt16_to_corsar_chars(new_part.bytes, &new_part_buffer[16]);
          new_part_as_long.qwords[0] = (u64)new_part_buffer;
          new_part_as_long.bytes[15] = tid__string;
          new_part_as_long           = slice__set_metadata(new_part_as_long, 0, len);
#ifndef NDEBUG
          set_ref_cnt(new_part_as_long, 1);
#endif
          slice_array__set_len(new_part_as_long, len);
          slice_array__set_cap(new_part_as_long, 15);
          set_ref_cnt(new_part_as_long, 0);
     } else new_part_as_long = new_part;

     u32       const old_part_slice_offset = slice__get_offset(old_part_as_long);
     u32       const old_part_slice_len    = slice__get_len(old_part_as_long);
     const u8* const old_part_chars        = &((const u8*)slice_array__get_items(old_part_as_long))[old_part_slice_offset * 3];
     u64       const old_part_len          = old_part_slice_len <= slice_max_len ? old_part_slice_len : slice_array__get_len(old_part_as_long);

     assert(old_part_slice_len <= slice_max_len || old_part_slice_offset == 0);

     u32       const new_part_slice_offset = slice__get_offset(new_part_as_long);
     u32       const new_part_slice_len    = slice__get_len(new_part_as_long);
     const u8* const new_part_chars        = &((const u8*)slice_array__get_items(new_part_as_long))[new_part_slice_offset * 3];
     u64       const new_part_len          = new_part_slice_len <= slice_max_len ? new_part_slice_len : slice_array__get_len(new_part_as_long);

     assert(new_part_slice_len <= slice_max_len || new_part_slice_offset == 0);

     t_any const old_part_first_char_or_2_chars = {.structure = {.value = *(const ua_u64*)old_part_chars, .type = tid__char}};
     if (old_part_len == 1 && new_part_len == 1) {
          t_any new_part_first_char = {.bytes = {[15] = tid__char}};
          memcpy_inline(new_part_first_char.bytes, new_part_chars, 3);

          return long_string__replace__own(thrd_data, string, old_part_first_char_or_2_chars, new_part_first_char);
     }

     u8    string_buffer[61];
     t_any string_as_long;
     if (string.bytes[15] == tid__short_string) {
          u8 const len             = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, &string_buffer[16]);
          string_as_long.qwords[0] = (u64)string_buffer;
          string_as_long.bytes[15] = tid__string;
          string_as_long           = slice__set_metadata(string_as_long, 0, len);
#ifndef NDEBUG
          set_ref_cnt(string_as_long, 1);
#endif
          slice_array__set_len(string_as_long, len);
          slice_array__set_cap(string_as_long, 15);
          set_ref_cnt(string_as_long, 0);
     } else string_as_long = string;

     u32       const string_slice_offset = slice__get_offset(string_as_long);
     u32       const string_slice_len    = slice__get_len(string_as_long);
     const u8* const string_chars        = &((u8*)slice_array__get_items(string_as_long))[string_slice_offset * 3];
     u64       const string_len          = string_slice_len <= slice_max_len ? string_slice_len : slice_array__get_len(string_as_long);

     assert(string_slice_len <= slice_max_len || string_slice_offset == 0);

     if (old_part_len == 0) [[clang::unlikely]] {
          ref_cnt__add(thrd_data, new_part, string_len + 1, owner);
          u64 const dst_len     = (string_len + 1) * new_part_len + string_len;
          t_any     dst_string  = long_string__new(dst_len);
          dst_string            = concat__long_str__long_str__own(thrd_data, dst_string, new_part_as_long, owner);
          t_any     string_char = {.bytes = {[15] = tid__char}};
          for (u64 idx = 0; idx < string_len; idx += 1) {
               memcpy_inline(string_char.bytes, &string_chars[idx * 3], 3);
               dst_string = long_string__append__own(thrd_data, dst_string, string_char, owner);
               dst_string = concat__long_str__long_str__own(thrd_data, dst_string, new_part_as_long, owner);
          }

          ref_cnt__dec(thrd_data, old_part);
          ref_cnt__dec(thrd_data, new_part);
          ref_cnt__dec(thrd_data, string);

          return dst_string;

          if (dst_len < 16) {
               t_any const short_string = short_string_from_chars(slice_array__get_items(dst_string), dst_len);
               if (short_string.bytes[15] == tid__short_string) {
                    free((void*)dst_string.qwords[0]);
                    return short_string;
               }
          }

          return dst_string;
     }

     u64   dst_len;
     u64   dst_cap;
     u8*   dst_chars;
     t_any dst_string   = string_as_long;
     bool  first_search = true;

     u64 string_idx = 0;
     while (string_idx + old_part_len <= string_len) {
          u64 occurrence_offset;
          u64 const tail_len = string_len - string_idx;

          if (old_part_len == 1) {
               u64       string_idx_offset = 0;
               u64 const edge              = tail_len - old_part_len + 1;
               while (true) {
                    occurrence_offset = look_char_from_begin(&string_chars[(string_idx + string_idx_offset) * 3], edge - string_idx_offset, old_part_first_char_or_2_chars);
                    if (occurrence_offset == (u64)-1) break;

                    occurrence_offset += string_idx_offset;
                    string_idx_offset  = occurrence_offset + 1;

                    break;
               }
          } else if (old_part_len >= 341 && tail_len >= 2730)
               occurrence_offset = look_string_part_from_begin__rabin_karp(&string_chars[string_idx * 3], tail_len * 3, old_part_chars, old_part_len * 3);
          else {
               u64       string_idx_offset = 0;
               u64 const edge              = tail_len - old_part_len + 2;
               while (true) {
                    occurrence_offset = look_2_chars_from_begin(&string_chars[(string_idx + string_idx_offset) * 3], edge - string_idx_offset, old_part_first_char_or_2_chars);
                    if (occurrence_offset == (u64)-1) break;

                    occurrence_offset += string_idx_offset;
                    string_idx_offset  = occurrence_offset + 1;

                    if (scar__memcmp(&string_chars[(string_idx + occurrence_offset) * 3], old_part_chars, old_part_len * 3) == 0) break;
               }
          }

          if (occurrence_offset == -1) break;
          if (old_part_len == new_part_len) {
               ref_cnt__inc(thrd_data, new_part, owner);
               dst_string = copy__long_str__long_str__own(thrd_data, dst_string, string_idx + occurrence_offset, new_part_as_long, owner);
          } else {
               if (first_search) {
                    dst_string = long_string__new(string_len - old_part_len + new_part_len);
                    dst_cap    = slice_array__get_cap(dst_string);
                    dst_chars  = slice_array__get_items(dst_string);
                    dst_len    = 0;
               } else if (dst_len + occurrence_offset + new_part_len > dst_cap) {
                    dst_cap = dst_cap + occurrence_offset + new_part_len;
                    if (dst_cap > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

                    dst_cap              = dst_cap * 3 / 2;
                    dst_cap              = dst_cap > array_max_len ? array_max_len : dst_cap;
                    dst_string.qwords[0] = (u64)realloc((u8*)dst_string.qwords[0], dst_cap * 3 + 16);
                    dst_chars            = slice_array__get_items(dst_string);
               }

               memcpy(&dst_chars[dst_len * 3], &string_chars[string_idx * 3], occurrence_offset * 3);
               dst_len += occurrence_offset;
               memcpy(&dst_chars[dst_len * 3], new_part_chars, new_part_len * 3);
               dst_len += new_part_len;
          }

          string_idx  += occurrence_offset + old_part_len;
          first_search = false;
     }

     ref_cnt__dec(thrd_data, old_part);
     ref_cnt__dec(thrd_data, new_part);
     if (first_search) return string;

     if (old_part_len == new_part_len) {
          dst_len   = string_len;
          dst_chars = &((u8*)slice_array__get_items(dst_string))[slice__get_offset(dst_string) * 3];
     } else {
          u64 const last_part_len = string_len - string_idx;
          if (last_part_len != 0) {
               if (dst_len + last_part_len > dst_cap) {
                    dst_cap = dst_len + last_part_len;
                    if (dst_cap > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

                    dst_string.qwords[0] = (u64)realloc((u8*)dst_string.qwords[0], dst_cap * 3 + 16);
                    dst_chars            = slice_array__get_items(dst_string);
               }

               memcpy(&dst_chars[dst_len * 3], &string_chars[string_idx * 3], last_part_len * 3);
               dst_len += last_part_len;
          }

          ref_cnt__dec(thrd_data, string);
          dst_string = slice__set_metadata(dst_string, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
          slice_array__set_len(dst_string, dst_len);
          slice_array__set_cap(dst_string, dst_cap);
     }

     if (dst_len < 16) {
          t_any const short_string = short_string_from_chars(dst_chars, dst_len);
          if (short_string.bytes[15] == tid__short_string) {
               free((void*)dst_string.qwords[0]);
               return short_string;
          }
     }
     return dst_string;
}

static t_any long_string__reserve__own(t_thrd_data* const thrd_data, t_any string, u64 const new_items_qty, const char* const owner) {
     assume(string.bytes[15] == tid__string);

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const current_cap  = slice_array__get_cap(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);
     assert(array_len <= current_cap);

     u64 const minimum_cap = len + new_items_qty;
     if (minimum_cap > array_max_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (current_cap >= minimum_cap) return string;

     u64 new_cap = minimum_cap * 3 / 2;
     new_cap     = new_cap > array_max_len ? array_max_len : new_cap;

     if (ref_cnt == 1) {
          slice_array__set_cap(string, new_cap);
          string.qwords[0] = (u64)realloc((void*)string.qwords[0], new_cap * 3 + 16);
          return string;
     }

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     t_any new_string = long_string__new(new_cap);
     slice_array__set_len(new_string, len);
     new_string       = slice__set_metadata(new_string, 0, slice_len);
     memcpy(slice_array__get_items(new_string), &((const u8*)slice_array__get_items(string))[slice_offset * 3], len * 3);

     return new_string;
}

static t_any short_string__reverse(t_any const string) {
     assert(string.bytes[15] == tid__short_string);

     typedef char v_16_char __attribute__((ext_vector_type(16)));

     v_16_char const chars_mask  = string.vector != 0;
     u8        const string_size = -__builtin_reduce_add(chars_mask);

     v_16_char const chars_0_mask     = (string.vector < 192) & chars_mask;
     v_16_char const chars_not_0_mask = (chars_0_mask ^ -1) & chars_mask;
     v_16_char const chars_1_mask     = chars_not_0_mask & chars_0_mask.sf0123456789abcde;
     v_16_char const chars_2_mask     = chars_not_0_mask & chars_1_mask.sf0123456789abcde;
     v_16_char const chars_3_mask     = chars_not_0_mask & chars_2_mask.sf0123456789abcde;

     t_any result = {.chars_vec = string.chars_vec & (chars_0_mask ^ chars_1_mask.s123456789abcdef0)};

     v_16_char const len_2_chars_1_mask = chars_1_mask ^ chars_2_mask.s123456789abcdef0;
     result.chars_vec                  |= (string.chars_vec & len_2_chars_1_mask).s123456789abcdef0;
     result.chars_vec                  |= (string.chars_vec & len_2_chars_1_mask.s123456789abcdef0).sf0123456789abcde;

     v_16_char const len_3_chars_2_mask = chars_2_mask ^ chars_3_mask.s123456789abcdef0;
     result.chars_vec                  |= (string.chars_vec & len_3_chars_2_mask).s23456789abcdef01;
     result.chars_vec                  |= (string.chars_vec & len_3_chars_2_mask.s23456789abcdef01).sef0123456789abcd;
     result.chars_vec                  |= string.chars_vec & len_3_chars_2_mask.s123456789abcdef0;

     result.chars_vec |= (string.chars_vec & chars_3_mask).s3456789abcdef012;
     result.chars_vec |= (string.chars_vec & chars_3_mask.s3456789abcdef012).sdef0123456789abc;
     result.chars_vec |= (string.chars_vec & chars_3_mask.s23456789abcdef01).sf0123456789abcde;
     result.chars_vec |= (string.chars_vec & chars_3_mask.s123456789abcdef0).s123456789abcdef0;

     result.chars_vec  = result.chars_vec.sfedcba9876543210;
     result.raw_bits >>= string_size == 0 ? 0 : (16 - string_size) * 8;

     return result;
}

static t_any long_string__reverse__own(t_thrd_data* const thrd_data, t_any const string) {
     assume(string.bytes[15] == tid__string);

     typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;
     u64 const chars_size   = len * 3;
     u8* const chars        = &((u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64 idx = 0;
     [[gnu::aligned(64)]]
     u8 buffer[128];
     if (ref_cnt == 1) {
          typedef u8 t_big_vec __attribute__((ext_vector_type(128), aligned(64)));

          for (; chars_size >= 128 + idx * 2; idx += 63) {
               t_vec const part_from_begin = *(const t_vec*)&chars[idx];
               t_vec const part_from_end   = *(const t_vec*)&chars[chars_size - idx - 64];

               *(t_vec*)&chars[chars_size - idx - 64] = __builtin_shufflevector(part_from_begin, part_from_end,
                    64, 60, 61, 62, 57, 58, 59, 54, 55, 56, 51, 52, 53, 48, 49, 50, 45, 46, 47, 42, 43, 44, 39, 40, 41, 36, 37, 38, 33, 34, 35, 30, 31, 32, 27,
                    28, 29, 24, 25, 26, 21, 22, 23, 18, 19, 20, 15, 16, 17, 12, 13, 14, 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2
               );
               *(t_vec*)&chars[idx] = __builtin_shufflevector(part_from_end, part_from_begin,
                    61, 62, 63, 58, 59, 60, 55, 56, 57, 52, 53, 54, 49, 50, 51, 46, 47, 48, 43, 44, 45, 40, 41, 42, 37, 38, 39, 34, 35, 36, 31,
                    32, 33, 28, 29, 30, 25, 26, 27, 22, 23, 24, 19, 20, 21, 16, 17, 18, 13, 14, 15, 10, 11, 12, 7, 8, 9, 4, 5, 6, 1, 2, 3, 127
               );
          }

          u8 const remain_bytes = chars_size - idx * 2;
          if (remain_bytes != 0) {
               memcpy_le_128(buffer, &chars[idx], remain_bytes);
               *(t_big_vec*)buffer = __builtin_shufflevector(*(const t_big_vec*)buffer, (const t_big_vec){},
                    126, 127, 123, 124, 125, 120, 121, 122, 117, 118, 119, 114, 115, 116, 111, 112, 113, 108, 109, 110, 105, 106, 107, 102, 103, 104, 99, 100, 101,
                    96, 97, 98, 93, 94, 95, 90, 91, 92, 87, 88, 89, 84, 85, 86, 81, 82, 83, 78, 79, 80, 75, 76, 77, 72, 73, 74, 69, 70, 71, 66, 67, 68, 63, 64,
                    65, 60, 61, 62, 57, 58, 59, 54, 55, 56, 51, 52, 53, 48, 49, 50, 45, 46, 47, 42, 43, 44, 39, 40, 41, 36, 37, 38, 33, 34, 35, 30, 31, 32, 27,
                    28, 29, 24, 25, 26, 21, 22, 23, 18, 19, 20, 15, 16, 17, 12, 13, 14, 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2
               );
               memcpy_le_128(&chars[idx], &buffer[128 - remain_bytes], remain_bytes);
          }

          return string;
     }
     if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);

     t_any reversed_str       = long_string__new(len);
     reversed_str             = slice__set_metadata(reversed_str, 0, slice_len);
     slice_array__set_len(reversed_str, len);
     u8* const reversed_chars = slice_array__get_items(reversed_str);

     for (;
          chars_size - idx >= 64;

          *(t_vec*)&reversed_chars[chars_size - idx - 64] = __builtin_shufflevector(*(const t_vec*)&chars[idx], (const t_vec){}, 63, 60, 61, 62, 57, 58, 59, 54, 55, 56, 51, 52, 53, 48, 49, 50, 45, 46, 47, 42, 43, 44, 39, 40, 41, 36, 37, 38, 33, 34, 35, 30, 31, 32, 27, 28, 29, 24, 25, 26, 21, 22, 23, 18, 19, 20, 15, 16, 17, 12, 13, 14, 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2),
          idx += 63
     );

     u8 const remain_bytes = chars_size - idx;
     memcpy_le_64(buffer, &chars[idx], remain_bytes);

     typedef u8 t_vec_a64 __attribute__((ext_vector_type(64), aligned(64)));

     *(t_vec_a64*)buffer = __builtin_shufflevector(*(const t_vec_a64*)buffer, (const t_vec_a64){},
          63, 60, 61, 62, 57, 58, 59, 54, 55, 56, 51, 52, 53, 48, 49, 50, 45, 46, 47, 42, 43, 44, 39, 40, 41, 36, 37, 38, 33, 34, 35, 30, 31, 32, 27,
          28, 29, 24, 25, 26, 21, 22, 23, 18, 19, 20, 15, 16, 17, 12, 13, 14, 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2
     );
     memcpy_le_64(reversed_chars, &buffer[64 - remain_bytes], remain_bytes);

     return reversed_str;
}

static t_any short_string__self_copy(t_thrd_data* const thrd_data, t_any string, u64 const idx_from, u64 const idx_to, u64 const part_len, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);

     u8 const string_len = short_string__get_len(string);
     if (idx_from > string_len || idx_to > string_len || part_len > string_len || idx_from + part_len > string_len || idx_to + part_len > string_len) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     t_any const part = short_string__slice(thrd_data, string, idx_from, idx_from + part_len, owner);

     assert(part.bytes[15] == tid__short_string);

     string = copy__short_str__short_str(thrd_data, string, idx_to, part, owner);

     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);

     return string;
}

static t_any long_string__dup__own(t_thrd_data* const thrd_data, t_any const string) {
     assume(string.bytes[15] == tid__string);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     u64       const ref_cnt      = get_ref_cnt(string);
     u64       const array_len    = slice_array__get_len(string);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const old_chars    = &((const u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(ref_cnt != 1);

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     t_any     new_string = long_string__new(len);
     slice_array__set_len(new_string, len);
     new_string           = slice__set_metadata(new_string, 0, slice_len);
     u8* const new_chars  = slice_array__get_items(new_string);
     memcpy(new_chars, old_chars, len * 3);

     return new_string;
}

static t_any long_string__self_copy__own(t_thrd_data* const thrd_data, t_any string, u64 const idx_from, u64 const idx_to, u64 const part_len, const char* const owner) {
     assume(string.bytes[15] == tid__string);

     u32 const slice_len  = slice__get_len(string);
     u64 const string_len = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);

     if (idx_from > string_len || idx_to > string_len || part_len > string_len || idx_from + part_len > string_len || idx_to + part_len > string_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (part_len == 0 || idx_from == idx_to) return string;
     if (get_ref_cnt(string) != 1) string = long_string__dup__own(thrd_data, string);

     u8* const chars = &((u8*)slice_array__get_items(string))[slice__get_offset(string) * 3];
     memmove(&chars[idx_to * 3], &chars[idx_from * 3], part_len * 3);
     return string;
}

static t_any short_string__sort(t_any string, bool const is_asc_order) {
     assert(string.bytes[15] == tid__short_string);

     u8 const string_len  = short_string__get_len(string);
     if (string_len < 2) return string;

     typedef u32 v_16_u32 __attribute__((ext_vector_type(16)));
     typedef u64 v_8_u64 __attribute__((ext_vector_type(8)));

     [[gnu::aligned(alignof(v_16_u32))]]
     u32 chars[16];
     memset_inline(chars, is_asc_order ? -1 : 0, 64);

     u8 string_offset = 0;
     for (u8 idx = 0; idx < string_len; string_offset += ctf8_char_to_corsar_code(&string.bytes[string_offset], &chars[idx]), idx += 1);

     v_16_u32 chars_vec = *(const v_16_u32*)chars;
     v_8_u64  shifts    = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);
     // chars_vec          = chars_vec.s0132457689bacdfe;
     // chars_vec          = chars_vec.s021375648a9bfdec;
     chars_vec          = chars_vec.s031265748b9aedfc;
     shifts             = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);
     chars_vec          = chars_vec.s021375648a9bfdec;
     shifts             = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);
     // chars_vec          = chars_vec.s0123547689abdcfe;
     // chars_vec          = chars_vec.s04152637fbead9c8;
     chars_vec          = chars_vec.s05142736ebfac9d8;
     shifts             = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);
     // chars_vec          = chars_vec.s02461357fdb9eca8;
     // chars_vec          = chars_vec.s02134657b9a8fdec;
     chars_vec          = chars_vec.s042615379dbf8cae;
     shifts             = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);
     chars_vec          = chars_vec.s02134657b9a8fdec;
     shifts             = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);
     // chars_vec          = chars_vec.s0123456798badcfe;
     // chars_vec          = chars_vec.s08192a3b4c5d6e7f;
     chars_vec          = chars_vec.s09182b3a4d5c6f7e;
     shifts             = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);
     // chars_vec          = chars_vec.s02468ace13579bdf;
     // chars_vec          = chars_vec.s041526378c9daebf;
     chars_vec          = chars_vec.s082a4c6e193b5d7f;
     shifts             = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);
     // chars_vec          = chars_vec.s024613578ace9bdf;
     // chars_vec          = chars_vec.s021346578a9bcedf;
     chars_vec          = chars_vec.s042615378cae9dbf;
     shifts             = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);
     chars_vec          = chars_vec.s021346578a9bcedf;
     shifts             = __builtin_convertvector((chars_vec.s02468ace > chars_vec.s13579bdf) & 32, v_8_u64);
     chars_vec          = (const v_16_u32)((const v_8_u64)chars_vec << shifts | (const v_8_u64)chars_vec >> shifts);

     if (!is_asc_order) chars_vec = chars_vec.sfedcba9876543210;

     *(v_16_u32*)chars = chars_vec;
     string_offset     = 0;
     for (u8 idx = 0; idx < string_len; string_offset += corsar_code_to_ctf8_char(chars[idx], &string.bytes[string_offset]), idx += 1);
     return string;
}

[[clang::always_inline]]
static void chars__swap(u8* const chars, u64 const idx_1, u64 const idx_2) {
     u32 buffer;
     memcpy_inline(&buffer, &chars[idx_1 * 3], 3);
     memcpy_inline(&chars[idx_1 * 3], &chars[idx_2 * 3], 3);
     memcpy_inline(&chars[idx_2 * 3], &buffer, 3);
}

static void chars__bitonic_sort(u8* const chars, u64 const begin_idx, u64 const end_edge, bool const is_asc_order) {
     typedef u32 v_16_u32 __attribute__((ext_vector_type(16)));
     typedef u8  v_64_u8  __attribute__((ext_vector_type(64), aligned(alignof(v_16_u32))));
     typedef u64 v_8_u64  __attribute__((ext_vector_type(8)));

     v_16_u32  buffer = is_asc_order ? -1 : 0;
     u64 const len    = end_edge - begin_idx;
     memcpy_le_64(&buffer, &chars[begin_idx * 3], len * 3);
     *(v_64_u8*)&buffer = __builtin_shufflevector(*(const v_64_u8*)&buffer, (const v_64_u8){}, 2, 1, 0, 64, 5, 4, 3, 64, 8, 7, 6, 64, 11, 10, 9, 64, 14, 13, 12, 64, 17, 16, 15, 64, 20, 19, 18, 64, 23, 22, 21, 64, 26, 25, 24, 64, 29, 28, 27, 64, 32, 31, 30, 64, 35, 34, 33, 64, 38, 37, 36, 64, 41, 40, 39, 64, 44, 43, 42, 64, 47, 46, 45, 64);

     v_8_u64 shifts = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);
     // buffer         = buffer.s0132457689bacdfe;
     // buffer         = buffer.s021375648a9bfdec;
     buffer         = buffer.s031265748b9aedfc;
     shifts         = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);
     buffer         = buffer.s021375648a9bfdec;
     shifts         = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);
     // buffer         = buffer.s0123547689abdcfe;
     // buffer         = buffer.s04152637fbead9c8;
     buffer         = buffer.s05142736ebfac9d8;
     shifts         = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);
     // buffer         = buffer.s02461357fdb9eca8;
     // buffer         = buffer.s02134657b9a8fdec;
     buffer         = buffer.s042615379dbf8cae;
     shifts         = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);
     buffer         = buffer.s02134657b9a8fdec;
     shifts         = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);
     // buffer         = buffer.s0123456798badcfe;
     // buffer         = buffer.s08192a3b4c5d6e7f;
     buffer         = buffer.s09182b3a4d5c6f7e;
     shifts         = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);
     // buffer         = buffer.s02468ace13579bdf;
     // buffer         = buffer.s041526378c9daebf;
     buffer         = buffer.s082a4c6e193b5d7f;
     shifts         = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);
     // buffer         = buffer.s024613578ace9bdf;
     // buffer         = buffer.s021346578a9bcedf;
     buffer         = buffer.s042615378cae9dbf;
     shifts         = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);
     buffer         = buffer.s021346578a9bcedf;
     shifts         = __builtin_convertvector((buffer.s02468ace > buffer.s13579bdf) & 32, v_8_u64);
     buffer         = (const v_16_u32)((const v_8_u64)buffer << shifts | (const v_8_u64)buffer >> shifts);

     if (!is_asc_order) buffer = buffer.sfedcba9876543210;

     *(v_64_u8*)&buffer = __builtin_shufflevector(*(const v_64_u8*)&buffer, (const v_64_u8){}, 2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, 18, 17, 16, 22, 21, 20, 26, 25, 24, 30, 29, 28, 34, 33, 32, 38, 37, 36, 42, 41, 40, 46, 45, 44, 50, 49, 48, 54, 53, 52, 58, 57, 56, 62, 61, 60, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64);
     memcpy_le_64(&chars[begin_idx * 3], &buffer, len * 3);
}

[[clang::noinline]]
static void chars__quick_sort(u8* const chars, u64 const begin_idx, u64 const end_edge, u64 const rnd_num, bool const is_asc_order) {
     u64 left_idx   = begin_idx;
     u64 right_edge = end_edge;
     while (true) {
          u64 const range_len    = right_edge - left_idx;
          u64       big_left_idx = -1;
          u64       big_right_edge;
          u64       small_left_idx;
          u64       small_right_edge;
          u64       small_range_len;
          if (range_len > 16) {
               u64 const range_len_25_percent = range_len / 4;
               u64       rnd_offset           = range_len & rnd_num;
               rnd_offset                     = rnd_offset < range_len_25_percent ? rnd_offset + range_len_25_percent : rnd_offset > range_len_25_percent * 3 ? rnd_offset - range_len_25_percent : rnd_offset;
               chars__swap(chars, left_idx, left_idx + rnd_offset);

               u32 const current_char = char_to_code(&chars[left_idx * 3]);
               u64       currend_idx  = left_idx;
               u64       opposite_idx = right_edge - 1;
               u64       direction    = -1;
               bool      cmp_modifier = is_asc_order;
               while (true) {
                    u32 const opposite_char = char_to_code(&chars[opposite_idx * 3]);
                    if (cmp_modifier ? opposite_char <= current_char : current_char <= opposite_char) {
                         chars__swap(chars, currend_idx, opposite_idx);

                         u64 const swap_buffer = currend_idx;
                         currend_idx           = opposite_idx;
                         opposite_idx          = swap_buffer;

                         direction    = -direction;
                         cmp_modifier = !cmp_modifier;
                    }

                    i64 const remain_len = currend_idx - opposite_idx;
                    if (remain_len >= -1 && remain_len <= 1) break;

                    opposite_idx += direction;
               }

               u64 const left_range_len  = currend_idx - left_idx;
               u64 const right_range_len = right_edge - currend_idx - 1;
               if (left_range_len < right_range_len) {
                    small_left_idx   = left_idx;
                    small_right_edge = currend_idx;
                    small_range_len  = left_range_len;
                    big_left_idx     = currend_idx + 1;
                    big_right_edge   = right_edge;
               } else {
                    small_left_idx   = currend_idx + 1;
                    small_right_edge = right_edge;
                    small_range_len  = right_range_len;
                    big_left_idx     = left_idx;
                    big_right_edge   = currend_idx;
               }

               left_idx   = big_left_idx;
               right_edge = big_right_edge;
          } else {
               small_left_idx   = left_idx;
               small_right_edge = right_edge;
               small_range_len  = range_len;
          }

          if (small_range_len > 16) chars__quick_sort(chars, small_left_idx, small_right_edge, rnd_num, is_asc_order);
          else if (small_range_len > 1) chars__bitonic_sort(chars, small_left_idx, small_right_edge, is_asc_order);

          if (big_left_idx == -1) break;
     }
}

static t_any long_string__sort__own(t_thrd_data* const thrd_data, t_any string, bool const is_asc_order) {
     assume(string.bytes[15] == tid__string);

     if (get_ref_cnt(string) != 1) string = long_string__dup__own(thrd_data, string);

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u8* const chars        = &((u8*)slice_array__get_items(string))[slice_offset * 3];
     u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     chars__quick_sort(chars, 0, len, static_rnd_num, is_asc_order);
     return string;
}

static t_any chars__heap_sort_fn__to_heap(t_thrd_data* const thrd_data, u8* const chars, t_any const fn, u64 currend_idx, u64 const begin_idx, u64 const end_edge, const char* const owner) {
     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {[15] = tid__char}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};

     while (true) {
          u64       largest_idx = currend_idx;
          u64 const left_idx    = 2 * currend_idx - begin_idx + 1;
          u64 const right_idx   = left_idx + 1;
          if (right_idx < end_edge) {
               memcpy_inline(fn_args[0].bytes, &chars[largest_idx * 3], 3);
               memcpy_inline(fn_args[1].bytes, &chars[right_idx * 3], 3);
               t_any cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__less:
                    largest_idx = right_idx;
               case comptime_tkn__great: case comptime_tkn__equal:
                    break;

               [[clang::unlikely]]
               case comptime_tkn__not_equal:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case comptime_tkn__nan_cmp:
                    goto nan_cmp_label;
               [[clang::unlikely]]
               default:
                    if (cmp_result.bytes[15] != tid__token) {
                         if (cmp_result.bytes[15] == tid__error) return cmp_result;
                         ref_cnt__dec(thrd_data, cmp_result);
                         goto invalid_type_label;
                    }

                    goto ret_incorrect_value_label;
               }

               memcpy_inline(fn_args[0].bytes, &chars[largest_idx * 3], 3);
               memcpy_inline(fn_args[1].bytes, &chars[left_idx * 3], 3);
               cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__less:
                    largest_idx = left_idx;
               case comptime_tkn__great: case comptime_tkn__equal:
                    break;

               [[clang::unlikely]]
               case comptime_tkn__not_equal:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case comptime_tkn__nan_cmp:
                    goto nan_cmp_label;
               [[clang::unlikely]]
               default:
                    if (cmp_result.bytes[15] != tid__token) {
                         if (cmp_result.bytes[15] == tid__error) return cmp_result;
                         ref_cnt__dec(thrd_data, cmp_result);
                         goto invalid_type_label;
                    }

                    goto ret_incorrect_value_label;
               }
          } else if (left_idx < end_edge) {
               memcpy_inline(fn_args[0].bytes, &chars[largest_idx * 3], 3);
               memcpy_inline(fn_args[1].bytes, &chars[left_idx * 3], 3);
               t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__less:
                    largest_idx = left_idx;
               case comptime_tkn__great: case comptime_tkn__equal:
                    break;

               [[clang::unlikely]]
               case comptime_tkn__not_equal:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case comptime_tkn__nan_cmp:
                    goto nan_cmp_label;
               [[clang::unlikely]]
               default:
                    if (cmp_result.bytes[15] != tid__token) {
                         if (cmp_result.bytes[15] == tid__error) return cmp_result;
                         ref_cnt__dec(thrd_data, cmp_result);
                         goto invalid_type_label;
                    }

                    goto ret_incorrect_value_label;
               }
          }

          if (largest_idx == currend_idx) break;

          chars__swap(chars, currend_idx, largest_idx);
          currend_idx = largest_idx;
     }
     return null;

     invalid_type_label:
     return error__invalid_type(thrd_data, owner);

     cant_ord_label:
     return error__cant_ord(thrd_data, owner);

     nan_cmp_label:
     return error__nan_cmp(thrd_data, owner);

     ret_incorrect_value_label:
     return error__ret_incorrect_value(thrd_data, owner);
}

static t_any chars__heap_sort_fn(t_thrd_data* const thrd_data, u8* const chars, t_any const fn, u64 const begin_idx, u64 const end_edge, const char* const owner) {
     for (u64 currend_idx = (end_edge - begin_idx) / 2 + begin_idx - 1; (i64)currend_idx >= (i64)begin_idx; currend_idx -= 1) {
          t_any const to_heap_result = chars__heap_sort_fn__to_heap(thrd_data, chars, fn, currend_idx, begin_idx, end_edge, owner);
          if (to_heap_result.bytes[15] == tid__error) [[clang::unlikely]] return to_heap_result;
     }

     for (u64 current_edge = end_edge - 1; (i64)current_edge >= (i64)begin_idx; current_edge -= 1) {
          chars__swap(chars, begin_idx, current_edge);
          t_any const to_heap_result = chars__heap_sort_fn__to_heap(thrd_data, chars, fn, begin_idx, begin_idx, current_edge, owner);
          if (to_heap_result.bytes[15] == tid__error) [[clang::unlikely]] return to_heap_result;
     }

     return null;
}

[[clang::noinline]]
static t_any chars__quick_sort_fn(t_thrd_data* const thrd_data, u8* const chars, t_any const fn, u64 const begin_idx, u64 const end_edge, u64 const rnd_num, const char* const owner) {
     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {[15] = tid__char}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__char}};

     u64 left_idx   = begin_idx;
     u64 right_edge = end_edge;
     while (true) {
          u64 const range_len    = right_edge - left_idx;
          u64       big_left_idx = -1;
          u64       big_right_edge;
          u64       small_left_idx;
          u64       small_right_edge;
          u64       small_range_len;
          if (range_len > 16) {
               u64 const range_len_25_percent = range_len / 4;
               u64       rnd_offset           = range_len & rnd_num;
               rnd_offset                     = rnd_offset < range_len_25_percent ? rnd_offset + range_len_25_percent : rnd_offset > range_len_25_percent * 3 ? rnd_offset - range_len_25_percent : rnd_offset;
               chars__swap(chars, left_idx, left_idx + rnd_offset);

               memcpy_inline(fn_args[0].bytes, &chars[left_idx * 3], 3);
               u64      currend_idx  = left_idx;
               u64      opposite_idx = right_edge - 1;
               u64      direction    = -1;
               bool     cmp_modifier = true;
               while (true) {
                    memcpy_inline(fn_args[1].bytes, &chars[opposite_idx * 3], 3);
                    t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

                    switch (cmp_result.raw_bits) {
                    case comptime_tkn__less: case comptime_tkn__great:
                         if ((cmp_result.bytes[0] == less_id) == cmp_modifier) break;
                    case comptime_tkn__equal: {
                         chars__swap(chars, currend_idx, opposite_idx);

                         u64 const swap_buffer = currend_idx;
                         currend_idx           = opposite_idx;
                         opposite_idx          = swap_buffer;

                         direction    = -direction;
                         cmp_modifier = !cmp_modifier;
                         break;
                    }

                    [[clang::unlikely]]
                    case comptime_tkn__not_equal:
                         goto cant_ord_label;
                    [[clang::unlikely]]
                    case comptime_tkn__nan_cmp:
                         goto nan_cmp_label;
                    [[clang::unlikely]]
                    default:
                         if (cmp_result.bytes[15] != tid__token) {
                              if (cmp_result.bytes[15] == tid__error) return cmp_result;
                              ref_cnt__dec(thrd_data, cmp_result);
                              goto invalid_type_label;
                         }

                         goto ret_incorrect_value_label;
                    }

                    i64 const remain_len = currend_idx - opposite_idx;
                    if (remain_len >= -1 && remain_len <= 1) break;

                    opposite_idx += direction;
               }

               u64 const left_range_len  = currend_idx - left_idx;
               u64 const right_range_len = right_edge - currend_idx - 1;
               if (left_range_len < right_range_len) {
                    small_left_idx   = left_idx;
                    small_right_edge = currend_idx;
                    small_range_len  = left_range_len;
                    big_left_idx     = currend_idx + 1;
                    big_right_edge   = right_edge;
               } else {
                    small_left_idx   = currend_idx + 1;
                    small_right_edge = right_edge;
                    small_range_len  = right_range_len;
                    big_left_idx     = left_idx;
                    big_right_edge   = currend_idx;
               }

               left_idx   = big_left_idx;
               right_edge = big_right_edge;
          } else {
               small_left_idx   = left_idx;
               small_right_edge = right_edge;
               small_range_len  = range_len;
          }

          if (small_range_len > 16) {
               t_any const sort_result = chars__quick_sort_fn(thrd_data, chars, fn, small_left_idx, small_right_edge, rnd_num, owner);
               if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;
          } else if (small_range_len > 2) {
               t_any const sort_result = chars__heap_sort_fn(thrd_data, chars, fn, small_left_idx, small_right_edge, owner);
               if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;
          } else if (small_range_len == 2) {
               memcpy_inline(fn_args[0].bytes, &chars[small_left_idx * 3], 3);
               memcpy_inline(fn_args[1].bytes, &chars[(small_left_idx + 1) * 3], 3);
               t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__great:
                    chars__swap(chars, small_left_idx, small_left_idx + 1);
               case comptime_tkn__less: case comptime_tkn__equal:
                    break;

               [[clang::unlikely]]
               case comptime_tkn__not_equal:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case comptime_tkn__nan_cmp:
                    goto nan_cmp_label;
               [[clang::unlikely]]
               default:
                    if (cmp_result.bytes[15] != tid__token) {
                         if (cmp_result.bytes[15] == tid__error) return cmp_result;
                         ref_cnt__dec(thrd_data, cmp_result);
                         goto invalid_type_label;
                    }

                    goto ret_incorrect_value_label;
               }
          }

          if (big_left_idx == -1) break;
     }

     return null;

     invalid_type_label:
     return error__invalid_type(thrd_data, owner);

     cant_ord_label:
     return error__cant_ord(thrd_data, owner);

     nan_cmp_label:
     return error__nan_cmp(thrd_data, owner);

     ret_incorrect_value_label:
     return error__ret_incorrect_value(thrd_data, owner);
}

static t_any string__sort_fn__own(t_thrd_data* const thrd_data, t_any string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8  short_chars_buffer[45];
     u8* chars;
     u64 len;
     if (string.bytes[15] == tid__short_string) {
          len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, short_chars_buffer);

          if (len < 2) {
               ref_cnt__dec(thrd_data, fn);
               return string;
          }

          chars = short_chars_buffer;
     } else {
          if (get_ref_cnt(string) != 1) string = long_string__dup__own(thrd_data, string);

          u32 const slice_offset = slice__get_offset(string);
          u32 const slice_len    = slice__get_len(string);
          chars                  = &((u8*)slice_array__get_items(string))[slice_offset * 3];
          len                    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);

          assert(slice_len <= slice_max_len || slice_offset == 0);
     }

     t_any const sort_result = chars__quick_sort_fn(thrd_data, chars, fn, 0, len, static_rnd_num, owner);
     ref_cnt__dec(thrd_data, fn);
     if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          return sort_result;
     }

     if (string.bytes[15] == tid__short_string) {
          string = short_string_from_chars(chars, len);

          assert(string.bytes[15] == tid__short_string);
     }

     return string;
}

static t_any short_string__split(t_thrd_data* const thrd_data, t_any string, t_any const separator, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assert(separator.bytes[15] == tid__char);

     u8        ctf8_separator[4];
     u8 const  separator_size = corsar_code_to_ctf8_char(char_to_code(separator.bytes), ctf8_separator);

     u16 separator_mask;
     switch (separator_size) {
     case 1:
          separator_mask = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[0], v_16_bool));
          break;
     case 2: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[1], v_16_bool));
          separator_mask         = chars_mask_0 & (chars_mask_1 >> 1);
          break;
     }
     case 3: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[2], v_16_bool));
          separator_mask         = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2);
          break;
     }
     case 4: {
          u16 const chars_mask_0 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[0], v_16_bool));
          u16 const chars_mask_1 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[1], v_16_bool));
          u16 const chars_mask_2 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[2], v_16_bool));
          u16 const chars_mask_3 = v_16_bool_to_u16(__builtin_convertvector(string.vector == ctf8_separator[3], v_16_bool));
          separator_mask         = chars_mask_0 & (chars_mask_1 >> 1) & (chars_mask_2 >> 2) & (chars_mask_3 >> 3);
          break;
     }
     default:
          unreachable;
     }

     u8     const result_len = __builtin_popcount(separator_mask) + 1;
     t_any        result     = array__init(thrd_data, result_len, owner);
     result                  = slice__set_metadata(result, 0, result_len);
     slice_array__set_len(result, result_len);
     t_any* const parts      = slice_array__get_items(result);
     t_any        part;
     for (u8 part_idx = 0; part_idx < result_len; part_idx += 1) {
          u8 const separator_offset = separator_mask == 0 ? short_string__get_size(string) : __builtin_ctz(separator_mask);
          part.raw_bits             = string.raw_bits & (((u128)1 << separator_offset * 8) - 1);
          parts[part_idx]           = part;
          u8 const shift            = separator_mask == 0 ? 0 : separator_offset + separator_size;
          string.raw_bits         >>= shift * 8;
          separator_mask          >>= shift;
     }

     return result;
}

static inline t_any array__append__own(t_thrd_data* const thrd_data, t_any const array, t_any const new_item, const char* const owner);

static t_any long_string__split__own(t_thrd_data* const thrd_data, t_any const string, t_any const separator, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assert(separator.bytes[15] == tid__char);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any result         = array__init(thrd_data, 1, owner);
     u64   part_first_idx = 0;
     while (true) {
          u64 separator_idx = look_char_from_begin(&chars[part_first_idx * 3], len - part_first_idx, separator);
          separator_idx     = separator_idx == (u64)-1 ? len : part_first_idx + separator_idx;

          ref_cnt__inc(thrd_data, string, owner);
          t_any const part = long_string__slice__own(thrd_data, string, part_first_idx, separator_idx, nullptr);
          part_first_idx   = separator_idx + 1;

          assert(part.bytes[15] == tid__short_string || part.bytes[15] == tid__string);

          result = array__append__own(thrd_data, result, part, owner);
          if (separator_idx == len) break;
     }

     ref_cnt__dec(thrd_data, string);
     return result;
}

static t_any short_string__split_by_part__own(t_thrd_data* const thrd_data, t_any string, t_any const separator, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assert(separator.bytes[15] == tid__short_string || separator.bytes[15] == tid__string);

     if (separator.raw_bits == 0) {
          u8 const string_len = short_string__get_len(string);
          if (string_len == 0) return empty_array;

          t_any        result = array__init(thrd_data, string_len, owner);
          result              = slice__set_metadata(result, 0, string_len);
          slice_array__set_len(result, string_len);
          t_any* const parts  = slice_array__get_items(result);
          t_any        part;
          for (u8 part_idx = 0; part_idx < string_len; part_idx += 1) {
               u8 const char_size_in_bits = ctf8_char_to_corsar_code(string.bytes, (u32*)part.bytes) * 8;
               part.raw_bits              = string.raw_bits & (((u64)1 << char_size_in_bits) - 1);
               parts[part_idx]            = part;
               string.raw_bits          >>= char_size_in_bits;
          }

          return result;
     }

     t_any result = array__init(thrd_data, 1, owner);
     if (separator.bytes[15] == tid__short_string) {
          u8  const separator_size  = short_string__get_size(separator);
          u16       occurrence_mask = -1;
          for (u8 idx = 0; idx < separator_size; idx += 1)
               occurrence_mask &= v_16_bool_to_u16(__builtin_convertvector(string.vector == separator.vector[idx], v_16_bool)) >> idx;

          t_any part;
          while (occurrence_mask != 0) {
               u8 const separator_offset = __builtin_ctz(occurrence_mask);
               part.raw_bits             = string.raw_bits & (((u128)1 << separator_offset * 8) - 1);
               u8 const shift            = separator_offset + separator_size;
               string.raw_bits         >>= shift * 8;
               occurrence_mask         >>= shift;
               result                    = array__append__own(thrd_data, result, part, owner);
          }
     } else ref_cnt__dec(thrd_data, separator);

     result = array__append__own(thrd_data, result, string, owner);
     return result;
}

static t_any long_string__split_by_part__own(t_thrd_data* const thrd_data, t_any const string, t_any const separator, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(separator.bytes[15] == tid__short_string || separator.bytes[15] == tid__string);

     u32       const string_slice_offset = slice__get_offset(string);
     u32       const string_slice_len    = slice__get_len(string);
     const u8* const string_chars        = &((const u8*)slice_array__get_items(string))[string_slice_offset * 3];
     u64       const string_len          = string_slice_len <= slice_max_len ? string_slice_len : slice_array__get_len(string);

     assert(string_slice_len <= slice_max_len || string_slice_offset == 0);

     u8        short_string_chars[45];
     const u8* separator_chars;
     u64       separator_len;
     if (separator.bytes[15] == tid__short_string) {
          separator_len   = ctf8_str_ze_lt16_to_corsar_chars(separator.bytes, short_string_chars);
          separator_chars = short_string_chars;

          if (separator_len == 0) {
               t_any        result = array__init(thrd_data, string_len, owner);
               result              = slice__set_metadata(result, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);
               slice_array__set_len(result, string_len);
               t_any* const parts  = slice_array__get_items(result);
               for (u64 part_idx = 0; part_idx < string_len; part_idx += 1) {
                    parts[part_idx].raw_bits = 0;
                    corsar_code_to_ctf8_char(char_to_code(&string_chars[part_idx * 3]), parts[part_idx].bytes);
               }

               ref_cnt__dec(thrd_data, string);
               return result;
          }
     } else {
          u32 const separator_slice_offset = slice__get_offset(separator);
          u32 const separator_slice_len    = slice__get_len(separator);
          separator_chars                  = &((const u8*)slice_array__get_items(separator))[separator_slice_offset * 3];
          separator_len                    = separator_slice_len <= slice_max_len ? separator_slice_len : slice_array__get_len(separator);

          assert(separator_slice_len <= slice_max_len || separator_slice_offset == 0);
     }

     t_any       result                          = array__init(thrd_data, 1, owner);
     u64         part_first_idx                  = 0;
     u64   const edge                            = string_len - separator_len + (separator_len == 1 ? 1 : 2);
     t_any const separator_first_char_or_2_chars = {.structure = {.value = *(const ua_u64*)separator_chars, .type = tid__char}};
     while (true) {
          u64 const string_remain_len = string_len - part_first_idx;
          u64       separator_idx;
          if (separator_len > string_remain_len) separator_idx = string_len;
          else if (separator_len == 1) {
               u64 string_idx = part_first_idx;
               while (true) {
                    separator_idx = look_char_from_begin(&string_chars[string_idx * 3], edge - string_idx, separator_first_char_or_2_chars);
                    if (separator_idx == (u64)-1) {
                         separator_idx = string_len;
                         break;
                    }

                    separator_idx += string_idx;
                    break;
               }
          } else if (separator_len >= 341 && string_remain_len >= 2730) {
               separator_idx = look_string_part_from_begin__rabin_karp(&string_chars[part_first_idx * 3], string_remain_len * 3, separator_chars, separator_len * 3);
               separator_idx = separator_idx == -1 ? string_len : part_first_idx + separator_idx;
          } else {
               u64 string_idx = part_first_idx;
               while (true) {
                    separator_idx = look_2_chars_from_begin(&string_chars[string_idx * 3], edge - string_idx, separator_first_char_or_2_chars);
                    if (separator_idx == (u64)-1) {
                         separator_idx = string_len;
                         break;
                    }

                    string_idx += separator_idx;

                    if (scar__memcmp(&string_chars[string_idx * 3], separator_chars, separator_len * 3) == 0) {
                         separator_idx = string_idx;
                         break;
                    }

                    string_idx += 1;
               }
          }

          ref_cnt__inc(thrd_data, string, owner);
          t_any const part = long_string__slice__own(thrd_data, string, part_first_idx, separator_idx, nullptr);
          part_first_idx   = separator_idx + separator_len;

          assert(part.bytes[15] == tid__short_string || part.bytes[15] == tid__string);

          result = array__append__own(thrd_data, result, part, owner);
          if (separator_idx == string_len) break;
     }

     ref_cnt__dec(thrd_data, string);
     ref_cnt__dec(thrd_data, separator);
     return result;
}

static inline t_any suffix_of__long_string__string__own(t_thrd_data* const thrd_data, t_any const string, t_any const suffix) {
     assume(string.bytes[15] == tid__string);
     assume(suffix.bytes[15] == tid__short_string || suffix.bytes[15] == tid__string);

     u32       const string_slice_offset = slice__get_offset(string);
     u32       const string_slice_len    = slice__get_len(string);
     const u8* const string_chars        = &((const u8*)slice_array__get_items(string))[string_slice_offset * 3];
     u64       const string_ref_cnt      = get_ref_cnt(string);
     u64       const string_array_len    = slice_array__get_len(string);
     u64       const string_len          = string_slice_len <= slice_max_len ? string_slice_len : string_array_len;

     assert(string_slice_len <= slice_max_len || string_slice_offset == 0);

     u8        short_string_chars[45];
     const u8* suffix_chars;
     u64       suffix_len;
     u64       suffix_ref_cnt;
     if (suffix.bytes[15] == tid__short_string) {
          suffix_len     = ctf8_str_ze_lt16_to_corsar_chars(suffix.bytes, short_string_chars);
          suffix_chars   = short_string_chars;
          suffix_ref_cnt = 0;
     } else {
          u32 const suffix_slice_offset = slice__get_offset(suffix);
          u32 const suffix_slice_len    = slice__get_len(suffix);
          suffix_chars                  = &((const u8*)slice_array__get_items(suffix))[suffix_slice_offset * 3];
          suffix_ref_cnt                = get_ref_cnt(suffix);
          u64 const suffix_array_len    = slice_array__get_len(suffix);
          suffix_len                    = suffix_slice_len <= slice_max_len ? suffix_slice_len : suffix_array_len;

          assert(suffix_slice_len <= slice_max_len || suffix_slice_offset == 0);
     }

     if (string.qwords[0] == suffix.qwords[0] && suffix.bytes[15] == tid__string) {
          if (string_ref_cnt > 2) set_ref_cnt(string, string_ref_cnt - 2);
     } else {
          if (string_ref_cnt > 1) set_ref_cnt(string, string_ref_cnt - 1);
          if (suffix_ref_cnt > 1) set_ref_cnt(suffix, suffix_ref_cnt - 1);
     }

     t_any const result = string_len >= suffix_len && scar__memcmp(&string_chars[(string_len - suffix_len) * 3], suffix_chars, suffix_len * 3) == 0 ? bool__true : bool__false;

     if (string.qwords[0] == suffix.qwords[0] && suffix.bytes[15] == tid__string) {
          if (string_ref_cnt == 2) free((void*)string.qwords[0]);
     } else {
          if (string_ref_cnt == 1) free((void*)string.qwords[0]);
          if (suffix_ref_cnt == 1) free((void*)suffix.qwords[0]);
     }

     return result;
}

[[clang::always_inline]]
static t_any short_string__take(t_thrd_data* const thrd_data, t_any string, u64 const n, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);

     u8 const n_offset = short_string__offset_of_idx(string, n, true);
     if (n_offset == 255) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     string.raw_bits &= ((u128)1 << n_offset * 8) - 1;
     return string;
}

static t_any short_string__take_while__own(t_thrd_data* const thrd_data, t_any string, t_any const fn, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8    offset = 0;
     t_any char_  = {.bytes = {[15] = tid__char}};
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(&string.bytes[offset], char_.bytes);
          if (char_size == 0) break;

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) break;
          offset += char_size;
     }
     ref_cnt__dec(thrd_data, fn);
     string.raw_bits &= ((u128)1 << offset * 8) - 1;
     return string;
}

static t_any long_string__take_while__own(t_thrd_data* const thrd_data, t_any string, t_any const fn, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any char_ = {.bytes = {[15] = tid__char}};
     u64   n     = 0;
     for (; n < len; n += 1) {
          memcpy_inline(char_.bytes, &chars[n * 3], 3);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &char_, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, string);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) break;
     }
     ref_cnt__dec(thrd_data, fn);

     if (n < 16) {
          t_any const short_string = short_string_from_chars(chars, n);
          if (short_string.bytes[15] == tid__short_string) {
               ref_cnt__dec(thrd_data, string);
               return short_string;
          }
     }

     if (n <= slice_max_len) [[clang::likely]] {
          string = slice__set_metadata(string, slice_offset, n);
          return string;
     }

     assert(slice_offset == 0);

     u64 const ref_cnt = get_ref_cnt(string);
     if (ref_cnt == 1) {
          slice_array__set_len(string, n);
          slice_array__set_cap(string, n);
          string           = slice__set_metadata(string, 0, slice_max_len + 1);
          string.qwords[0] = (u64)realloc((u8*)string.qwords[0], n * 3 + 16);
          return string;
     }
     if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);

     t_any new_string    = long_string__new(n);
     new_string          = slice__set_metadata(new_string, 0, slice_max_len + 1);
     slice_array__set_len(new_string, n);
     slice_array__set_cap(new_string, n);
     u8* const new_chars = slice_array__get_items(new_string);
     memcpy(new_chars, chars, n * 3);
     return new_string;
}

static t_any string__to_array__own(t_thrd_data* const thrd_data, t_any const string) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);

     u8        short_string_chars[45];
     const u8* chars;
     u64       len;
     bool      need_to_free;
     if (string.bytes[15] == tid__short_string) {
          len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, short_string_chars);
          if (len == 0) return empty_array;
          chars        = short_string_chars;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(string);
          u32 const slice_len    = slice__get_len(string);
          chars                  = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          u64 const ref_cnt      = get_ref_cnt(string);
          u64 const array_len    = slice_array__get_len(string);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);
          need_to_free = ref_cnt == 1;
     }

     t_any        result       = array__init(thrd_data, len, nullptr);
     result                    = slice__set_metadata(result, 0, len <= slice_max_len ? len : slice_max_len + 1);
     slice_array__set_len(result, len);
     t_any* const result_items = slice_array__get_items(result);
     t_any        char_        = {.bytes = {[15] = tid__char}};
     for (u64 idx = 0; idx < len; idx += 1) {
          memcpy_inline(char_.bytes, &chars[idx * 3], 3);
          result_items[idx] = char_;
     }

     if (need_to_free) free((void*)string.qwords[0]);
     return result;
}

static t_any string__to_box__own(t_thrd_data* const thrd_data, t_any const string, const char* const owner) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);

     u8        short_string_chars[45];
     const u8* chars;
     u64       len;
     bool      need_to_free;
     if (string.bytes[15] == tid__short_string) {
          len = ctf8_str_ze_lt16_to_corsar_chars(string.bytes, short_string_chars);
          if (len == 0) return empty_box;
          chars        = short_string_chars;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(string);
          u32 const slice_len    = slice__get_len(string);
          chars                  = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          u64 const ref_cnt      = get_ref_cnt(string);
          u64 const array_len    = slice_array__get_len(string);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (len > 16) {
               ref_cnt__dec(thrd_data, string);
               return error__out_of_bounds(thrd_data, owner);
          }

          if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);
          need_to_free = ref_cnt == 1;
     }

     t_any        box       = box__new(thrd_data, len, owner);
     t_any* const box_items = box__get_items(box);
     for (u8 idx = 0; idx < len; idx += 1) {
          box_items[idx] = (const t_any){.bytes = {[15] = tid__char}};
          memcpy_inline(box_items[idx].bytes, &chars[idx * 3], 3);
     }

     if (need_to_free) free((void*)string.qwords[0]);
     return box;
}

static t_any string__to_float__own(t_thrd_data* const thrd_data, t_any string) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);

     [[gnu::aligned(16)]]
     char  buffer[332];
     char* chars;
     u64   len;
     if (string.bytes[15] == tid__short_string) {
          string.vector |= string.vector > 64 & string.vector < 91 & 32;
          switch (string.raw_bits) {
          case 0x6e616euwb: case 0x6e616e2buwb: case 0x6e616e2duwb:
               return (const t_any){.structure = {.value = cast_double_to_u64(__builtin_nan("")), .type = tid__float}};
          case 0x666e69uwb: case 0x666e692buwb:
               return (const t_any){.structure = {.value = cast_double_to_u64(__builtin_inf()), .type = tid__float}};
          case 0x666e692duwb:
               return (const t_any){.structure = {.value = cast_double_to_u64(-__builtin_inf()), .type = tid__float}};
          default: {
               if (
                    __builtin_reduce_and(
                         (string.vector >= '0' & string.vector <= '9') |
                         (string.vector == 'e')                        |
                         (string.vector == '.')                        |
                         (string.vector == '+')                        |
                         (string.vector == '-')                        |
                         (string.vector == 0)
                    ) == 0
               ) return null;

               memcpy_inline(buffer, string.bytes, 16);
               chars        = buffer;
               len          = short_string__get_size(string);
               break;
          }
          case 0:
               return null;
          }
     } else {
          u32       const slice_offset = slice__get_offset(string);
          u32       const slice_len    = slice__get_len(string);
          const u8* const string_chars = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          u64       const ref_cnt      = get_ref_cnt(string);
          u64       const array_len    = slice_array__get_len(string);
          len                          = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);
          bool const need_to_free = ref_cnt == 1;

          if (len > 326)
               chars = malloc(len + 6);
          else {
               chars = buffer;
          }

          typedef u8 t_vec __attribute__((ext_vector_type(32), aligned(1)));
          typedef u8 t_10_chars_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const num_range_from = {0, 0, '0', 0, 0, '0', 0, 0, '0', 0, 0, '0', 0, 0, '0', 0, 0, '0', 0, 0, '0', 0, 0, '0', 0, 0, '0', 0, 0, '0', 0, 0};
          t_vec const num_range_to   = {0, 0, '9', 0, 0, '9', 0, 0, '9', 0, 0, '9', 0, 0, '9', 0, 0, '9', 0, 0, '9', 0, 0, '9', 0, 0, '9', 0, 0, '9', 0, 0};
          t_vec const vec_e          = {0, 0, 'e', 0, 0, 'e', 0, 0, 'e', 0, 0, 'e', 0, 0, 'e', 0, 0, 'e', 0, 0, 'e', 0, 0, 'e', 0, 0, 'e', 0, 0, 'e', 0, 0};
          t_vec const vec_E          = {0, 0, 'E', 0, 0, 'E', 0, 0, 'E', 0, 0, 'E', 0, 0, 'E', 0, 0, 'E', 0, 0, 'E', 0, 0, 'E', 0, 0, 'E', 0, 0, 'E', 0, 0};
          t_vec const vec_dot        = {0, 0, '.', 0, 0, '.', 0, 0, '.', 0, 0, '.', 0, 0, '.', 0, 0, '.', 0, 0, '.', 0, 0, '.', 0, 0, '.', 0, 0, '.', 0, 0};
          t_vec const vec_plus       = {0, 0, '+', 0, 0, '+', 0, 0, '+', 0, 0, '+', 0, 0, '+', 0, 0, '+', 0, 0, '+', 0, 0, '+', 0, 0, '+', 0, 0, '+', 0, 0};
          t_vec const vec_minus      = {0, 0, '-', 0, 0, '-', 0, 0, '-', 0, 0, '-', 0, 0, '-', 0, 0, '-', 0, 0, '-', 0, 0, '-', 0, 0, '-', 0, 0, '-', 0, 0};

          u64 idx = 0;
          for (; idx < len - 10; idx += 10) {
               t_vec const string_chars_vec = *(const t_vec*)&string_chars[idx * 3];
               if (
                    __builtin_reduce_and(
                         (string_chars_vec >= num_range_from & string_chars_vec <= num_range_to) |
                         (string_chars_vec == vec_e)                                             |
                         (string_chars_vec == vec_E)                                             |
                         (string_chars_vec == vec_dot)                                           |
                         (string_chars_vec == vec_plus)                                          |
                         (string_chars_vec == vec_minus)
                    ) == 0
               ) {
                    if (len > 326) free(chars);
                    if (need_to_free) free((void*)string.qwords[0]);
                    return null;
               }

               *(t_10_chars_vec*)&chars[idx] = __builtin_shufflevector(string_chars_vec, (const t_vec){}, 2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 32, 32, 32, 32, 32);
          }

          for (; idx < len; idx += 1) {
               u32 const char_code = char_to_code(&string_chars[idx * 3]);
               switch (char_code) {
               case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
               case 'e':
               case 'E':
               case '.':
               case '+':
               case '-':
                    chars[idx] = char_code;
                    break;
               default:
                    if (len > 326) free(chars);
                    if (need_to_free) free((void*)string.qwords[0]);
                    return null;
               }
          }

          chars[len] = 0;
          if (need_to_free) free((void*)string.qwords[0]);
     }

     char*  str_end;
     double float_ = strtod(chars, &str_end);
     if (len > 326) free(chars);
     if (str_end != &chars[len]) return null;

     return (const t_any){.structure = {.value = cast_double_to_u64(float_), .type = tid__float}};
}

static t_any short_string__to_lower(t_any string) {
     assert(string.bytes[15] == tid__short_string);

     if (__builtin_reduce_and(string.vector < 128) != 0) {
          string.vector |= string.vector > 64 & string.vector < 91 & 32;
          return string;
     }

     u8   dst_chars[66];
     u16  dst_size      = 0;
     u8   string_offset = 0;
     u32  corsar_code;
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_code(&string.bytes[string_offset], &corsar_code);
          if (char_size == 0) break;

          string_offset          += char_size;
          u64 const dst_few_chars = char_to_lower(corsar_code);
          char_from_code(&dst_chars[dst_size], dst_few_chars & 0x3f'ffffull);
          dst_size += 3;
          if (dst_few_chars > 0x3f'ffffull) {
               char_from_code(&dst_chars[dst_size], (dst_few_chars >> 22) & 0x1f'ffffull);
               dst_size += 3;

               if (dst_few_chars > 0x7ff'ffff'ffffull) {
                    char_from_code(&dst_chars[dst_size], dst_few_chars >> 43);
                    dst_size += 3;
               }
          }
     }

     assert(dst_size < 67);

     u8 const dst_len = dst_size / 3;
     if (dst_len < 16) {
          t_any const dst = short_string_from_chars(dst_chars, dst_len);
          if (dst.bytes[15] == tid__short_string) return dst;
     }

     t_any dst = long_string__new(dst_len);
     slice_array__set_len(dst, dst_len);
     dst       = slice__set_metadata(dst, 0, dst_len);
     memcpy_le_128(slice_array__get_items(dst), dst_chars, dst_size);
     return dst;
}

static t_any long_string__to_lower__own(t_thrd_data* const thrd_data, t_any const string, const char* const owner) {
     assume(string.bytes[15] == tid__string);

     u32  const slice_offset  = slice__get_offset(string);
     u32  const slice_len     = slice__get_len(string);
     u64  const src_ref_cnt   = get_ref_cnt(string);
     u64  const src_array_len = slice_array__get_len(string);
     u64  const src_array_cap = slice_array__get_cap(string);
     u64  const src_len       = slice_len <= slice_max_len ? slice_len : src_array_len;
     u8*  const src_chars     = &((u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);
     assert(src_array_len <= src_array_cap);

     t_any dst;
     u64   dst_len;
     u64   dst_cap;
     u8*   dst_chars;
     bool  dst_is_init = false;
     for (u64 idx = 0; idx < src_len; idx += 1) {
          u32 const corsar_code = char_to_code(&src_chars[idx * 3]);
          u64 const lower_char  = char_to_lower(corsar_code);
          if (corsar_code != lower_char || dst_is_init) {
               u8  const chars_in_lower = lower_char <= 0x3f'ffffull ? 1 : lower_char > 0x7ff'ffff'ffffull ? 3 : 2;

               if (!dst_is_init) {
                    dst_is_init = true;

                    if (src_ref_cnt == 1) {
                         dst       = string;
                         dst_cap   = src_array_cap;
                         dst_chars = slice_array__get_items(string);
                         if (slice_offset != 0) memmove(dst_chars, src_chars, idx * 3);
                    } else {
                         if (src_ref_cnt > 1) set_ref_cnt(string, src_ref_cnt - 1);

                         dst_cap   = src_len - 1 + chars_in_lower;
                         dst_cap   = dst_cap <= array_max_len ? dst_cap : src_len;
                         dst       = long_string__new(dst_cap);
                         dst_chars = slice_array__get_items(dst);
                         memcpy(dst_chars, src_chars, idx * 3);
                    }
                    dst_len = idx;
               }

               u64 const new_len = dst_len + chars_in_lower;
               if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

               if (dst_cap < new_len) {
                    dst_cap       = new_len * 3 / 2;
                    dst_cap       = dst_cap <= array_max_len ? dst_cap : new_len;
                    dst.qwords[0] = (u64)realloc((u8*)dst.qwords[0], dst_cap * 3 + 16);
                    dst_chars     = slice_array__get_items(dst);
               }

               char_from_code(&dst_chars[dst_len * 3], lower_char & 0x3f'ffffull);
               dst_len += 1;
               if (chars_in_lower > 1) {
                    char_from_code(&dst_chars[dst_len * 3], (lower_char >> 22) & 0x1f'ffffull);
                    dst_len += 1;

                    if (chars_in_lower == 3) {
                         char_from_code(&dst_chars[dst_len * 3], lower_char >> 43);
                         dst_len += 1;
                    }
               }
          }
     }

     if (!dst_is_init) return string;

     dst = slice__set_metadata(dst, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
     slice_array__set_len(dst, dst_len);
     return dst;
}

static t_any short_string__to_upper(t_any string) {
     assert(string.bytes[15] == tid__short_string);

     if (__builtin_reduce_and(string.vector < 128) != 0) {
          string.vector ^= string.vector > 96 & string.vector < 123 & 32;
          return string;
     }

     u8   dst_chars[66];
     u16  dst_size      = 0;
     u8   string_offset = 0;
     u32  corsar_code;
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_code(&string.bytes[string_offset], &corsar_code);
          if (char_size == 0) break;

          string_offset          += char_size;
          u64 const dst_few_chars = char_to_upper(corsar_code);
          char_from_code(&dst_chars[dst_size], dst_few_chars & 0x3f'ffffull);
          dst_size += 3;
          if (dst_few_chars > 0x3f'ffffull) {
               char_from_code(&dst_chars[dst_size], (dst_few_chars >> 22) & 0x1f'ffffull);
               dst_size += 3;

               if (dst_few_chars > 0x7ff'ffff'ffffull) {
                    char_from_code(&dst_chars[dst_size], dst_few_chars >> 43);
                    dst_size += 3;
               }
          }
     }

     assert(dst_size < 67);

     u8 const dst_len = dst_size / 3;
     if (dst_len < 16) {
          t_any const dst = short_string_from_chars(dst_chars, dst_len);
          if (dst.bytes[15] == tid__short_string) return dst;
     }

     t_any dst = long_string__new(dst_len);
     slice_array__set_len(dst, dst_len);
     dst       = slice__set_metadata(dst, 0, dst_len);
     memcpy_le_128(slice_array__get_items(dst), dst_chars, dst_size);
     return dst;
}

static t_any long_string__to_upper__own(t_thrd_data* const thrd_data, t_any const string, const char* const owner) {
     assume(string.bytes[15] == tid__string);

     u32  const slice_offset  = slice__get_offset(string);
     u32  const slice_len     = slice__get_len(string);
     u64  const src_ref_cnt   = get_ref_cnt(string);
     u64  const src_array_len = slice_array__get_len(string);
     u64  const src_array_cap = slice_array__get_cap(string);
     u64  const src_len       = slice_len <= slice_max_len ? slice_len : src_array_len;
     u8*  const src_chars     = &((u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);
     assert(src_array_len <= src_array_cap);

     t_any dst;
     u64   dst_len;
     u64   dst_cap;
     u8*   dst_chars;
     bool  dst_is_init = false;
     for (u64 idx = 0; idx < src_len; idx += 1) {
          u32 const corsar_code = char_to_code(&src_chars[idx * 3]);
          u64 const upper_char  = char_to_upper(corsar_code);
          if (corsar_code != upper_char || dst_is_init) {
               u8  const chars_in_upper = upper_char <= 0x3f'ffffull ? 1 : upper_char > 0x7ff'ffff'ffffull ? 3 : 2;

               if (!dst_is_init) {
                    dst_is_init = true;

                    if (src_ref_cnt == 1) {
                         dst       = string;
                         dst_cap   = src_array_cap;
                         dst_chars = slice_array__get_items(string);
                         if (slice_offset != 0) memmove(dst_chars, src_chars, idx * 3);
                    } else {
                         if (src_ref_cnt > 1) set_ref_cnt(string, src_ref_cnt - 1);

                         dst_cap   = src_len - 1 + chars_in_upper;
                         dst_cap   = dst_cap <= array_max_len ? dst_cap : src_len;
                         dst       = long_string__new(dst_cap);
                         dst_chars = slice_array__get_items(dst);
                         memcpy(dst_chars, src_chars, idx * 3);
                    }
                    dst_len = idx;
               }

               u64 const new_len = dst_len + chars_in_upper;
               if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

               if (dst_cap < new_len) {
                    dst_cap       = new_len * 3 / 2;
                    dst_cap       = dst_cap <= array_max_len ? dst_cap : new_len;
                    dst.qwords[0] = (u64)realloc((u8*)dst.qwords[0], dst_cap * 3 + 16);
                    dst_chars     = slice_array__get_items(dst);
               }

               char_from_code(&dst_chars[dst_len * 3], upper_char & 0x3f'ffffull);
               dst_len += 1;
               if (chars_in_upper > 1) {
                    char_from_code(&dst_chars[dst_len * 3], (upper_char >> 22) & 0x1f'ffffull);
                    dst_len += 1;

                    if (chars_in_upper == 3) {
                         char_from_code(&dst_chars[dst_len * 3], upper_char >> 43);
                         dst_len += 1;
                    }
               }
          }
     }

     if (!dst_is_init) return string;

     dst = slice__set_metadata(dst, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
     slice_array__set_len(dst, dst_len);
     return dst;
}

[[clang::always_inline]]
static t_any short_string__trim(t_any string) {
     assert(string.bytes[15] == tid__short_string);

     u16 const not_spaces_mask = v_16_bool_to_u16(__builtin_convertvector(string.vector != ' ' & string.vector != '\t' & string.vector != '\n' & string.vector != 0, v_16_bool));
     if (not_spaces_mask == 0) return (const t_any){};

     string.raw_bits  &= ((u128)1 << (sizeof(int) * 8 - __builtin_clz(not_spaces_mask)) * 8) - 1;
     string.raw_bits >>= __builtin_ctz(not_spaces_mask) * 8;
     return string;
}

static inline t_any long_string__trim__own(t_thrd_data* const thrd_data, t_any string) {
     assume(string.bytes[15] == tid__string);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);
     u64       const chars_size   = len * 3;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64 begin;
     u64 end;

     u64 idx = 0;
     if (chars_size - idx >= 64) {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          t_vec const space_vec    = (const t_vec){0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0};
          t_vec const tab_vec      = (const t_vec){0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0};
          t_vec const new_line_vec = (const t_vec){0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0};

          do {
               t_vec const chars_vec        = *(const t_vec*)&chars[idx];
               u64         looked_char_mask = v_64_bool_to_u64(__builtin_convertvector(
                    (chars_vec == space_vec) |
                    (chars_vec == tab_vec)   |
                    (chars_vec == new_line_vec)
               , v_64_bool));
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= looked_char_mask >> 1;
               looked_char_mask    &= 0x1249'2492'4924'9249ull;
               looked_char_mask    ^= 0x1249'2492'4924'9249ull;
               if (looked_char_mask != 0) {
                    begin = (idx + __builtin_ctzll(looked_char_mask)) / 3;

                    t_vec const space_vec_from_end    = (const t_vec){' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' '};
                    t_vec const tab_vec_from_end      = (const t_vec){'\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t'};
                    t_vec const new_line_vec_from_end = (const t_vec){'\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n'};

                    u64 remain_bytes = chars_size;
                    do {
                         t_vec const chars_vec_from_end        = *(const t_vec*)&chars[remain_bytes - 64];
                         u64         looked_char_mask_from_end =  v_64_bool_to_u64(__builtin_convertvector(
                              (chars_vec_from_end == space_vec_from_end) |
                              (chars_vec_from_end == tab_vec_from_end)   |
                              (chars_vec_from_end == new_line_vec_from_end)
                         , v_64_bool));
                         looked_char_mask_from_end    &= looked_char_mask_from_end >> 1;
                         looked_char_mask_from_end    &= looked_char_mask_from_end >> 1;
                         looked_char_mask_from_end     = (looked_char_mask_from_end >> 1 & 0x1249'2492'4924'9249ull) ^ 0x1249'2492'4924'9249ull;
                         if (looked_char_mask_from_end != 0) {
                              end = (remain_bytes + sizeof(long long) * 8 - 61 - __builtin_clzll(looked_char_mask_from_end)) / 3;
                              goto slice;
                         }

                         remain_bytes -= 63;
                    } while (remain_bytes >= 64);

                    end = (idx + 2 + sizeof(long long) * 8 - __builtin_clzll(looked_char_mask)) / 3;
                    goto slice;
               }
               idx += 63;
          } while (chars_size - idx >= 64);
     }

     u8 const remain_bytes = chars_size - idx;
     switch (remain_bytes == 0 ? 0 : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 t_vec __attribute__((ext_vector_type(32), aligned(1)));

          t_vec const space_vec    = (const t_vec){0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0};
          t_vec const tab_vec      = (const t_vec){0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0};
          t_vec const new_line_vec = (const t_vec){0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0};

          t_vec const space_vec_from_end    = (const t_vec){0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' '};
          t_vec const tab_vec_from_end      = (const t_vec){0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t'};
          t_vec const new_line_vec_from_end = (const t_vec){0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n'};

          t_vec const low_part  = *(t_vec*)&chars[idx];
          t_vec const high_part = *(t_vec*)&chars[chars_size - 32];

          u64 looked_char_mask =
               v_32_bool_to_u32(__builtin_convertvector((low_part == space_vec) | (low_part == tab_vec) | (low_part == new_line_vec), v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector((high_part == space_vec_from_end) | (high_part == tab_vec_from_end) | (high_part == new_line_vec_from_end), v_32_bool)) << (remain_bytes - 32);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249'2492'4924'9249ull;
          looked_char_mask ^= 0x1249'2492'4924'9249ull;
          looked_char_mask &= ((u64)1 << remain_bytes) - 1;

          if (looked_char_mask != 0) {
               begin = (idx + __builtin_ctzll(looked_char_mask)) / 3;
               end   = (idx + 2 + sizeof(long long) * 8 - __builtin_clzll(looked_char_mask)) / 3;
               goto slice;
          }

          ref_cnt__dec(thrd_data, string);
          return (const t_any){};
     }
     case 5: {
          assume(remain_bytes > 17 && remain_bytes <= 30);

          typedef u8 t_vec __attribute__((ext_vector_type(16), aligned(1)));

          t_vec const space_vec    = (const t_vec){0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0};
          t_vec const tab_vec      = (const t_vec){0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0};
          t_vec const new_line_vec = (const t_vec){0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0};

          t_vec const space_vec_from_end    = (const t_vec){' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' ', 0, 0, ' '};
          t_vec const tab_vec_from_end      = (const t_vec){'\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t', 0, 0, '\t'};
          t_vec const new_line_vec_from_end = (const t_vec){'\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n', 0, 0, '\n'};

          t_vec const low_part  = *(t_vec*)&chars[idx];
          t_vec const high_part = *(t_vec*)&chars[chars_size - 16];

          u32 looked_char_mask =
               v_16_bool_to_u16(__builtin_convertvector((low_part == space_vec) | (low_part == tab_vec) | (low_part == new_line_vec), v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector((high_part == space_vec_from_end) | (high_part == tab_vec_from_end) | (high_part == new_line_vec_from_end), v_16_bool)) << (remain_bytes - 16);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x924'9249ul;
          looked_char_mask ^= 0x924'9249ul;
          looked_char_mask &= ((u32)1 << remain_bytes) - 1;

          if (looked_char_mask != 0) {
               begin = (idx + __builtin_ctzl(looked_char_mask)) / 3;
               end   = (idx + 2 + sizeof(long) * 8 - __builtin_clzl(looked_char_mask)) / 3;
               goto slice;
          }

          ref_cnt__dec(thrd_data, string);
          return (const t_any){};
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 15);

          typedef u8 t_vec __attribute__((ext_vector_type(8), aligned(1)));

          t_vec const space_vec    = (const t_vec){0, 0, ' ', 0, 0, ' ', 0, 0};
          t_vec const tab_vec      = (const t_vec){0, 0, '\t', 0, 0, '\t', 0, 0};
          t_vec const new_line_vec = (const t_vec){0, 0, '\n', 0, 0, '\n', 0, 0};

          t_vec const space_vec_from_end    = (const t_vec){0, ' ', 0, 0, ' ', 0, 0, ' '};
          t_vec const tab_vec_from_end      = (const t_vec){0, '\t', 0, 0, '\t', 0, 0, '\t'};
          t_vec const new_line_vec_from_end = (const t_vec){0, '\n', 0, 0, '\n', 0, 0, '\n'};

          t_vec const low_part  = *(t_vec*)&chars[idx];
          t_vec const high_part = *(t_vec*)&chars[chars_size - 8];

          u16 looked_char_mask =
               v_8_bool_to_u8(__builtin_convertvector((low_part == space_vec) | (low_part == tab_vec) | (low_part == new_line_vec), v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector((high_part == space_vec_from_end) | (high_part == tab_vec_from_end) | (high_part == new_line_vec_from_end), v_8_bool)) << (remain_bytes - 8);
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= looked_char_mask >> 1;
          looked_char_mask &= 0x1249;
          looked_char_mask ^= 0x1249;
          looked_char_mask &= ((u16)1 << remain_bytes) - 1;

          if (looked_char_mask != 0) {
               begin = (idx + __builtin_ctz(looked_char_mask)) / 3;
               end   = (idx + 2 + sizeof(int) * 8 - __builtin_clz(looked_char_mask)) / 3;
               goto slice;
          }

          ref_cnt__dec(thrd_data, string);
          return (const t_any){};
     }
     case 3: {
          assume(remain_bytes == 6);

          u32 const char_code_0 = char_to_code(&chars[idx]);
          u32 const char_code_1 = char_to_code(&chars[idx + 3]);

          ref_cnt__dec(thrd_data, string);

          t_any short_string = {};
          u8    offset       = 0;
          if (!(char_code_0 == ' ' || char_code_0 == '\t' || char_code_0 == '\n'))
               offset = corsar_code_to_ctf8_char(char_code_0, short_string.bytes);
          if (!(char_code_1 == ' ' || char_code_1 == '\t' || char_code_1 == '\n'))
               corsar_code_to_ctf8_char(char_code_1, &short_string.bytes[offset]);

          return short_string;
     }
     case 2: {
          assume(remain_bytes == 3);

          u32 const char_code = char_to_code(&chars[idx + 3]);

          ref_cnt__dec(thrd_data, string);

          t_any short_string = {};
          if (!(char_code == ' ' || char_code == '\t' || char_code == '\n'))
               corsar_code_to_ctf8_char(char_code, short_string.bytes);

          return short_string;
     }
     case 0:
          ref_cnt__dec(thrd_data, string);
          return (const t_any){};
     default:
          unreachable;
     }

     slice:
     return long_string__slice__own(thrd_data, string, begin, end, "");
}

static t_any long_string__unslice__own(t_thrd_data* const thrd_data, t_any string) {
     assume(string.bytes[15] == tid__string);

     u32 const slice_len = slice__get_len(string);
     u64 const array_len = slice_array__get_len(string);
     u64 const array_cap = slice_array__get_cap(string);
     u64 const len       = slice_len <= slice_max_len ? slice_len : array_len;
     u64 const ref_cnt   = get_ref_cnt(string);

     if ((len == array_len && array_cap == array_len) || ref_cnt == 0) return string;

     u32 const slice_offset = slice__get_offset(string);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u8* const chars = slice_array__get_items(string);
     if (ref_cnt == 1) {
          if (slice_offset != 0) memmove(chars, &chars[slice_offset * 3], len * 3);
          slice_array__set_len(string, len);
          slice_array__set_cap(string, len);
          string           = slice__set_metadata(string, 0, slice_len);
          string.qwords[0] = (u64)realloc((u8*)string.qwords[0], len * 3 + 16);
          return string;
     }
     set_ref_cnt(string, ref_cnt - 1);

     t_any     new_string = long_string__new(len);
     new_string           = slice__set_metadata(new_string, 0, slice_len);
     slice_array__set_len(new_string, len);
     u8* const new_chars  = slice_array__get_items(new_string);
     memcpy(new_chars, &chars[slice_offset * 3], len * 3);
     return new_string;
}

static void short_string__zip_init_read(const t_any* const string, t_zip_read_state* const state, t_any* key, t_any* value, u64* const result_len, t_any* const) {
     assert(string->bytes[15] == tid__short_string);

     state->char_position = string->bytes;
     *key                 = (const t_any){.structure = {.value = -1, .type = tid__int}};
     *value               = (const t_any){.bytes = {[15] = tid__char}};
     *result_len          = short_string__get_len(*string);
}

static void long_string__zip_init_read(const t_any* const string, t_zip_read_state* const state, t_any* key, t_any* value, u64* const result_len, t_any* const) {
     assume(string->bytes[15] == tid__string);

     state->char_position = &((const u8*)slice_array__get_items(*string))[slice__get_offset(*string) * 3];
     *key                 = (const t_any){.structure = {.value = -1, .type = tid__int}};
     *value               = (const t_any){.bytes = {[15] = tid__char}};
     u32 const slice_len  = slice__get_len(*string);
     *result_len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(*string);
}

static void short_string__zip_read(t_thrd_data* const, t_zip_read_state* const state, t_any* const key, t_any* const value, const char* const) {
     u32      char_code;
     u8 const char_size   = ctf8_char_to_corsar_code(state->char_position, &char_code);
     state->char_position = &state->char_position[char_size];
     key->qwords[0]      += 1;
     char_from_code(value->bytes, char_code);
}

static void long_string__zip_read(t_thrd_data* const, t_zip_read_state* const state, t_any* const key, t_any* const value, const char* const) {
     key->qwords[0] += 1;
     memcpy_inline(value->bytes, state->char_position, 3);
     state->char_position = &state->char_position[3];
}

static void short_string__zip_init_search(const t_any* const string, t_zip_search_state* const state, t_any* value, u64* const result_len, t_any* const) {
     assert(string->bytes[15] == tid__short_string);

     state->char_position = string->bytes;
     *value               = (const t_any){.bytes = {[15] = tid__char}};
     u8 const string_len  = short_string__get_len(*string);
     *result_len          = *result_len > string_len ? string_len : *result_len;
}

static void long_string__zip_init_search(const t_any* const string, t_zip_search_state* const state, t_any* value, u64* const result_len, t_any* const) {
     assume(string->bytes[15] == tid__string);

     state->char_position = &((const u8*)slice_array__get_items(*string))[slice__get_offset(*string) * 3];
     *value               = (const t_any){.bytes = {[15] = tid__char}};
     u32 const slice_len  = slice__get_len(*string);
     u64 const string_len = slice_len <= slice_max_len ? slice_len : slice_array__get_len(*string);
     *result_len          = *result_len > string_len ? string_len : *result_len;
}

static bool short_string__zip_search(t_thrd_data* const, t_zip_search_state* const state, t_any const, t_any* const value, const char* const) {
     assume(state->char_position[0] != 0);

     u32      char_code   = 0;
     u8 const char_size   = ctf8_char_to_corsar_code(state->char_position, &char_code);
     state->char_position = &state->char_position[char_size];
     char_from_code(value->bytes, char_code);
     return true;
}

static bool long_string__zip_search(t_thrd_data* const, t_zip_search_state* const state, t_any const, t_any* const value, const char* const) {
     memcpy_inline(value->bytes, state->char_position, 3);
     state->char_position = &state->char_position[3];
     return true;
}

[[clang::noinline]]
static void short_string__dump__half_own(t_thrd_data* const thrd_data, t_any* const result__own, t_any string, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);

     u8  buffer[340];
     u32 corsar_code;
     u16 buffer_size   = 19;
     u8  string_offset = 0;
     memcpy_inline(&buffer[16], (const u8[3]){0, 0, '"'}, 3);
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_code(&string.bytes[string_offset], &corsar_code);
          if (char_size == 0) break;

          string_offset += char_size;
          switch (corsar_code) {
          case '\t':
               memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '\\', 0, 0, 't'}, 6);
               buffer_size += 6;
               break;
          case '\n':
               memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '\\', 0, 0, 'n'}, 6);
               buffer_size += 6;
               break;
          case '\\':
               memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '\\', 0, 0, '\\'}, 6);
               buffer_size += 6;
               break;
          case '"':
               memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '\\', 0, 0, '"'}, 6);
               buffer_size += 6;
               break;
          default:
               if (char_is_graphic(corsar_code) && (corsar_code == ' ' || !char_is_space(corsar_code))) {
                    u32 const corsar_char = __builtin_bswap32(corsar_code) >> 8;
                    memcpy_inline(&buffer[buffer_size], &corsar_char, 3);
                    buffer_size += 3;
               } else {
                    memcpy_inline(&buffer[buffer_size], (const u8[3]){0, 0, '\\'}, 3);
                    buffer_size += 3;
                    for (u8 idx = 0; idx < 3; idx += 1) {
                         u8  const byte          = (corsar_code >> (16 - idx * 8)) & 0xff;
                         u32 const corsar_char_1 = (u32)("0123456789abcdef"[byte >> 4]) << 16;
                         u32 const corsar_char_2 = (u32)("0123456789abcdef"[byte & 0xf]) << 16;
                         memcpy_inline(&buffer[buffer_size], &corsar_char_1, 3);
                         buffer_size += 3;
                         memcpy_inline(&buffer[buffer_size], &corsar_char_2, 3);
                         buffer_size += 3;
                    }
               }
          }
     }
     memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '"', 0, 0, '\n'}, 6);
     buffer_size += 6;

     u8 const chars_len = (buffer_size - 16) / 3;
     if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
          result__own->qwords[0] = (u64)malloc(chars_len * 3 + 16);
          result__own->bytes[15] = tid__string;
          memcpy(&((u8*)result__own->qwords[0])[16], &buffer[16], buffer_size - 16);
          *result__own = slice__set_metadata(*result__own, 0, chars_len);
          set_ref_cnt(*result__own, 1);
          slice_array__set_len(*result__own, chars_len);
          slice_array__set_cap(*result__own, chars_len);
          return;
     }

     t_any added_string = (const t_any){.structure = {.value = (u64)buffer, .type = tid__string}};
     added_string       = slice__set_metadata(added_string, 0, chars_len);
#ifndef NDEBUG
     set_ref_cnt(added_string, 1);
#endif
     slice_array__set_len(added_string, chars_len);
     slice_array__set_cap(added_string, chars_len);
     set_ref_cnt(added_string, 0);
     *result__own = result__own->bytes[15] == tid__short_string ? concat__short_str__long_str__own(thrd_data, *result__own, added_string, owner) : concat__long_str__long_str__own(thrd_data, *result__own, added_string, owner);
}

[[clang::noinline]]
static void long_string__dump__half_own(t_thrd_data* const thrd_data, t_any* const result__own, t_any string, const char* const owner) {
     assert(string.bytes[15] == tid__string);

     u32       const slice_offset = slice__get_offset(string);
     u32       const slice_len    = slice__get_len(string);
     const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
     u64       const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);

     u8  buffer[340];
     u16 buffer_size = 19;
     memcpy_inline(&buffer[16], (const u8[3]){0, 0, '"'}, 3);

     for (u64 idx = 0; idx < len; idx += 1) {
          u32 const corsar_code = char_to_code(&chars[idx * 3]);
          switch (corsar_code) {
          case '\t':
               memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '\\', 0, 0, 't'}, 6);
               buffer_size += 6;
               break;
          case '\n':
               memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '\\', 0, 0, 'n'}, 6);
               buffer_size += 6;
               break;
          case '\\':
               memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '\\', 0, 0, '\\'}, 6);
               buffer_size += 6;
               break;
          case '"':
               memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '\\', 0, 0, '"'}, 6);
               buffer_size += 6;
               break;
          default:
               if (char_is_graphic(corsar_code) && (corsar_code == ' ' || !char_is_space(corsar_code))) {
                    memcpy_inline(&buffer[buffer_size], &chars[idx * 3], 3);
                    buffer_size += 3;
               } else {
                    memcpy_inline(&buffer[buffer_size], (const u8[3]){0, 0, '\\'}, 3);
                    buffer_size += 3;
                    for (u8 idx = 0; idx < 3; idx += 1) {
                         u8  const byte          = (corsar_code >> (16 - idx * 8)) & 0xff;
                         u32 const corsar_char_1 = (u32)("0123456789abcdef"[byte >> 4]) << 16;
                         u32 const corsar_char_2 = (u32)("0123456789abcdef"[byte & 0xf]) << 16;
                         memcpy_inline(&buffer[buffer_size], &corsar_char_1, 3);
                         buffer_size += 3;
                         memcpy_inline(&buffer[buffer_size], &corsar_char_2, 3);
                         buffer_size += 3;
                    }
               }
          }

          if (idx + 1 == len) {
               memcpy_inline(&buffer[buffer_size], (const u8[6]){0, 0, '"', 0, 0, '\n'}, 6);
               buffer_size += 6;
          }

          if (buffer_size > 292 || idx + 1 == len) {
               u8 const chars_len = (buffer_size - 16) / 3;
               if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
                    result__own->qwords[0] = (u64)malloc(chars_len * 3 + 16);
                    result__own->bytes[15] = tid__string;
                    memcpy(&((u8*)result__own->qwords[0])[16], &buffer[16], buffer_size - 16);
                    *result__own = slice__set_metadata(*result__own, 0, chars_len);
                    set_ref_cnt(*result__own, 1);
                    slice_array__set_len(*result__own, chars_len);
                    slice_array__set_cap(*result__own, chars_len);
               } else {
                    t_any added_string = (const t_any){.structure = {.value = (u64)buffer, .type = tid__string}};
                    added_string       = slice__set_metadata(added_string, 0, chars_len);
#ifndef NDEBUG
                    set_ref_cnt(added_string, 1);
#endif
                    slice_array__set_len(added_string, chars_len);
                    slice_array__set_cap(added_string, chars_len);
                    set_ref_cnt(added_string, 0);
                    *result__own = result__own->bytes[15] == tid__short_string ? concat__short_str__long_str__own(thrd_data, *result__own, added_string, owner) : concat__long_str__long_str__own(thrd_data, *result__own, added_string, owner);
               }

               buffer_size = 16;
          }
     }
}

static t_any str_to_ustr__own(t_thrd_data* const thrd_data, t_any const string, const char* const owner) {
     assert(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string);

     t_any result;
     if (string.bytes[15] == tid__short_string) {
          u8 buffer[48];

          u8 const string_size = short_string__get_size(string);
          t_str_cvt_result const cvt_result = ctf8_chars_to_utf8_chars(string_size, string.bytes, 48, buffer);
          if (cvt_result.src_offset != string_size) {
               assert(cvt_result.src_offset == str_cvt_err__recoding);

               return null;
          }

          if (cvt_result.dst_offset < 15) {
               memset_inline(&buffer[cvt_result.dst_offset], 0, 16);
               memcpy_inline(result.bytes, buffer, 16);
               result.bytes[15] = tid__short_byte_array;
               result           = short_byte_array__set_len(result, cvt_result.dst_offset);
               return result;
          }

          result = long_byte_array__new(cvt_result.dst_offset + 1);
          result = slice__set_metadata(result, 0, cvt_result.dst_offset);
          slice_array__set_len(result, cvt_result.dst_offset);
          memcpy_le_64(slice_array__get_items(result), buffer, cvt_result.dst_offset);
          return result;
     }

     u32       const slice_offset     = slice__get_offset(string);
     u32       const slice_len        = slice__get_len(string);
     u64       const ref_cnt          = get_ref_cnt(string);
     u64       const string_array_len = slice_array__get_len(string);
     u64       const string_len       = slice_len <= slice_max_len ? slice_len : string_array_len;
     u64       const chars_size       = string_len * 3;
     const u8* const chars            = &((const u8*)slice_array__get_items(string))[slice_offset * 3];

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     u64 result_cap   = string_len + 1;
     result           = long_byte_array__new(result_cap);
     u8* result_chars = slice_array__get_items(result);

     t_str_cvt_result current_offsets = {};
     while (true) {
          t_str_cvt_result const cvt_result = corsar_chars_to_utf8_chars(
               chars_size - current_offsets.src_offset, &chars[current_offsets.src_offset],
               result_cap - current_offsets.dst_offset, &result_chars[current_offsets.dst_offset]
          );

          if ((i64)cvt_result.src_offset <= 0) {
               assert(cvt_result.src_offset == str_cvt_err__recoding);

               if (ref_cnt == 1) free((void*)string.qwords[0]);
               return null;
          }

          current_offsets.src_offset += cvt_result.src_offset;
          current_offsets.dst_offset += cvt_result.dst_offset;
          if (current_offsets.src_offset == chars_size) {
               if (current_offsets.dst_offset < 15) {
                    t_any short_result = {.bytes = {[14] = current_offsets.dst_offset, [15] = tid__short_byte_array}};
                    memcpy_le_16(short_result.bytes, result_chars, current_offsets.dst_offset);
                    free((void*)result.qwords[0]);
                    result = short_result;
               } else {
                    if (current_offsets.dst_offset > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);
                    if (result_cap - current_offsets.dst_offset > 1) {
                         result_cap       = current_offsets.dst_offset == array_max_len ? array_max_len : current_offsets.dst_offset + 1;
                         result.qwords[0] = (u64)realloc((u8*)result.qwords[0], result_cap + 16);
                    }

                    result = slice__set_metadata(result, 0, current_offsets.dst_offset <= slice_max_len ? current_offsets.dst_offset : slice_max_len + 1);
                    slice_array__set_cap(result, result_cap);
                    slice_array__set_len(result, current_offsets.dst_offset);
               }

               if (ref_cnt == 1) free((void*)string.qwords[0]);
               return result;
          }

          if (result_cap == array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

          result_cap       = result_cap * 3 / 2;
          result_cap       = result_cap > array_max_len ? array_max_len : result_cap;
          result.qwords[0] = (u64)realloc((u8*)result.qwords[0], result_cap + 16);
          result_chars     = slice_array__get_items(result);
     }
}

static t_any string__join(
     t_thrd_data* const thrd_data,
     u64          const parts_len,
     const t_any* const parts,
     u64          const separator_len,
     const u8*          separator_chars,
     u8           const separator_short_string_size,
     u64          const result_len,
     u8           const result_short_string_size,
     u8           const step_size,
     const char*  const owner
) {
     if (parts_len == 0) return (const t_any){};

     u64 idx = 0;
     for (; parts[idx].bytes[15] == tid__null || parts[idx].bytes[15] == tid__holder; idx += step_size);

     if (parts_len == 1) {
          t_any const result = parts[idx];
          ref_cnt__inc(thrd_data, result, owner);
          return result;
     }

     u64 remain_items_len = parts_len;

     if (result_short_string_size < 16) {
          t_any result      = parts[idx];
          remain_items_len -= 1;
          for (idx += step_size; remain_items_len != 0; idx += step_size) {
               const t_any* const part_ptr = &parts[idx];
               if (part_ptr->bytes[15] == tid__null || part_ptr->bytes[15] == tid__holder) continue;

               u8 const result_size = short_string__get_size(result);
               result.raw_bits     |= *(const u128*)separator_chars << result_size * 8;
               result.raw_bits     |= part_ptr->raw_bits << (result_size + separator_short_string_size) * 8;

               remain_items_len -= 1;
          }
          return result;
     }

     u8 separator_corsar_chars[45];
     if (separator_short_string_size < 16) {
          ctf8_str_ze_lt16_to_corsar_chars(separator_chars, separator_corsar_chars);
          separator_chars = separator_corsar_chars;
     }

     t_any result = long_string__new(result_len);
     slice_array__set_len(result, result_len);
     result                 = slice__set_metadata(result, 0, result_len <= slice_max_len ? result_len : slice_max_len + 1);
     u8* const result_chars = slice_array__get_items(result);
     u64 current_result_len = 0;

     for (;; idx += step_size) {
          const t_any* const part_ptr = &parts[idx];
          switch (part_ptr->bytes[15]) {
          case tid__null: case tid__holder:
               continue;
          case tid__short_string:
               current_result_len += ctf8_str_ze_lt16_to_corsar_chars(part_ptr->bytes, &result_chars[current_result_len * 3]);
               break;
          case tid__string: {
               t_any     const part              = *part_ptr;
               u32       const part_slice_offset = slice__get_offset(part);
               u32       const part_slice_len    = slice__get_len(part);
               u64       const part_len          = part_slice_len <= slice_max_len ? part_slice_len : slice_array__get_len(part);
               const u8* const part_chars        = &((const u8*)slice_array__get_items(part))[part_slice_offset * 3];

               memcpy(&result_chars[current_result_len * 3], part_chars, part_len * 3);
               current_result_len += part_len;
               break;
          }
          default:
               unreachable;
          }

          remain_items_len -= 1;
          if (remain_items_len == 0) break;

          memcpy(&result_chars[current_result_len * 3], separator_chars, separator_len * 3);
          current_result_len += separator_len;
     }

     return result;
}

static t_any short_string__insert_to(t_thrd_data* const thrd_data, t_any string, t_any const char_, u64 const position, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assert(char_.bytes[15] == tid__char);

     u8 const len = short_string__get_len(string);
     if (position > len) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     u32 const char_corsar_code = char_to_code(char_.bytes);
     u8  const string_size      = short_string__get_size(string);
     u8  const char_size        = char_size_in_ctf8(char_corsar_code);
     if (string_size + char_size < 16) {
          u8   const position_offset = short_string__offset_of_idx(string, position, true);
          u128 const first_part_mask = ((u128)1 << position_offset * 8) - 1;
          u128 const last_part       = (string.raw_bits & ~first_part_mask) << char_size * 8;
          string.raw_bits           &= first_part_mask;
          corsar_code_to_ctf8_char(char_corsar_code, &string.bytes[position_offset]);
          string.raw_bits |= last_part;
          return string;
     }

     u8 const new_len     = len + 1;
     t_any    result      = long_string__new(new_len);
     result               = slice__set_metadata(result, 0, new_len);
     slice_array__set_len(result, new_len);
     u8*      result_char = slice_array__get_items(result);
     u8       offset      = 0;
     for (u8 idx = 0; idx < new_len; idx += 1) {
          if (idx == (u8)position) memcpy_inline(result_char, char_.bytes, 3);
          else offset += ctf8_char_to_corsar_char(&string.bytes[offset], result_char);

          result_char = &result_char[3];
     }

     return result;
}

static t_any long_string__insert_to__own(t_thrd_data* const thrd_data, t_any const string, t_any const char_, u64 const position, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assert(char_.bytes[15] == tid__char);

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (position > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const new_len = len + 1;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     t_any result;
     if (ref_cnt == 1) {
          u64 const cap = slice_array__get_cap(string);
          result        = string;
          slice_array__set_len(result, new_len);
          result        = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (cap < new_len) {
               result.qwords[0] = (u64)realloc((u8*)result.qwords[0], new_cap * 3 + 16);
               slice_array__set_cap(result, new_cap);
          }

          u8* const chars = slice_array__get_items(result);
          if (slice_offset == 0) {
               memmove(&chars[(position + 1) * 3], &chars[position * 3], (len - position) * 3);
               memcpy_inline(&chars[position * 3], char_.bytes, 3);
          } else {
               memmove(chars, &chars[slice_offset * 3], position * 3);
               memcpy_inline(&chars[position * 3], char_.bytes, 3);
               memmove(&chars[(position + 1) * 3], &chars[(slice_offset + position) * 3], (len - position) * 3);
          }
     } else {
          if (ref_cnt != 0) set_ref_cnt(string, ref_cnt - 1);
          result                    = long_string__new(new_len);
          slice_array__set_len(result, new_len);
          result                    = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          const u8* const old_chars = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          u8*       const new_chars = slice_array__get_items(result);
          memcpy(new_chars, old_chars, position * 3);
          memcpy_inline(&new_chars[position * 3], char_.bytes, 3);
          memcpy(&new_chars[(position + 1) * 3], &old_chars[position * 3], (len - position) * 3);
     }

     return result;
}

static t_any insert_part_to__short_str__short_str(t_thrd_data* const thrd_data, t_any string, t_any const part, u64 const position, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assert(part.bytes[15] == tid__short_string);

     u8 const string_len = short_string__get_len(string);
     if (position > string_len) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     u8 const string_size = short_string__get_size(string);
     u8 const part_size   = short_string__get_size(part);
     if (string_size + part_size < 16) {
          u8   const position_offset = short_string__offset_of_idx(string, position, true);
          u128 const first_part_mask = ((u128)1 << position_offset * 8) - 1;
          u128 const last_part       = (string.raw_bits & ~first_part_mask) << part_size * 8;
          string.raw_bits           &= first_part_mask;
          string.raw_bits           |= part.raw_bits << position_offset * 8;
          string.raw_bits           |= last_part;
          return string;
     }

     u8 const part_len     = short_string__get_len(part);
     u8 const new_len      = string_len + part_len;
     t_any    result       = long_string__new(new_len);
     result                = slice__set_metadata(result, 0, new_len);
     slice_array__set_len(result, new_len);
     u8*      result_chars = slice_array__get_items(result);
     u8       offset       = 0;
     for (u8 idx = 0; idx < position; idx += 1)
          offset += ctf8_char_to_corsar_char(&string.bytes[offset], &result_chars[idx * 3]);

     ctf8_str_ze_lt16_to_corsar_chars(part.bytes, &result_chars[position *  3]);

     for (u8 idx = position + part_len; idx < new_len; idx += 1)
          offset += ctf8_char_to_corsar_char(&string.bytes[offset], &result_chars[idx * 3]);

     return result;
}

static t_any insert_part_to__short_str__long_str__own(t_thrd_data* const thrd_data, t_any const string, t_any const part, u64 const position, const char* const owner) {
     assert(string.bytes[15] == tid__short_string);
     assume(part.bytes[15] == tid__string);

     u8 const string_len = short_string__get_len(string);
     if (position > string_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, part);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (string_len == 0) return part;

     u32 const slice_offset   = slice__get_offset(part);
     u32 const slice_len      = slice__get_len(part);
     u64 const part_ref_cnt   = get_ref_cnt(part);
     u64 const part_array_len = slice_array__get_len(part);
     u64 const part_len       = slice_len <= slice_max_len ? slice_len : part_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64 const new_len = string_len + part_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     u8    offset = 0;
     t_any result;
     if (part_ref_cnt == 1) {
          u64 const cap = slice_array__get_cap(part);
          result        = part;
          slice_array__set_len(result, new_len);
          result        = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (cap < new_len) {
               result.qwords[0] = (u64)realloc((u8*)result.qwords[0], new_cap * 3 + 16);
               slice_array__set_cap(result, new_cap);
          }

          u8* const result_chars = slice_array__get_items(result);

          if (slice_offset != position) memmove(&result_chars[position * 3], &result_chars[slice_offset * 3], part_len * 3);
          for (u8 idx = 0; idx < position; idx += 1)
               offset += ctf8_char_to_corsar_char(&string.bytes[offset], &result_chars[idx * 3]);
          for (u64 idx = position + part_len; idx < new_len; idx += 1)
               offset += ctf8_char_to_corsar_char(&string.bytes[offset], &result_chars[idx * 3]);
     } else {
          if (part_ref_cnt != 0) set_ref_cnt(part, part_ref_cnt - 1);

          result                       = long_string__new(new_len);
          slice_array__set_len(result, new_len);
          result                       = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          const u8* const part_chars   = &((const u8*)slice_array__get_items(part))[slice_offset * 3];
          u8*       const result_chars = slice_array__get_items(result);
          for (u8 idx = 0; idx < position; idx += 1)
               offset += ctf8_char_to_corsar_char(&string.bytes[offset], &result_chars[idx * 3]);
          memcpy(&result_chars[position * 3], part_chars, part_len * 3);
          for (u64 idx = position + part_len; idx < new_len; idx += 1)
               offset += ctf8_char_to_corsar_char(&string.bytes[offset], &result_chars[idx * 3]);
     }

     return result;
}

static t_any insert_part_to__long_str__short_str__own(t_thrd_data* const thrd_data, t_any const string, t_any const part, u64 const position, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assert(part.bytes[15] == tid__short_string);

     u32 const slice_offset     = slice__get_offset(string);
     u32 const slice_len        = slice__get_len(string);
     u64 const string_ref_cnt   = get_ref_cnt(string);
     u64 const string_array_len = slice_array__get_len(string);
     u64 const string_len       = slice_len <= slice_max_len ? slice_len : string_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (position > string_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          return error__out_of_bounds(thrd_data, owner);
     }

     u8  const part_len = short_string__get_len(part);
     if (part_len == 0) return string;

     u64 const new_len = string_len + part_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     t_any result;
     if (string_ref_cnt == 1) {
          u64 const cap = slice_array__get_cap(string);
          result        = string;
          slice_array__set_len(result, new_len);
          result        = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (cap < new_len) {
               result.qwords[0] = (u64)realloc((u8*)result.qwords[0], new_cap * 3 + 16);
               slice_array__set_cap(result, new_cap);
          }

          u8* const result_chars = slice_array__get_items(result);
          if (slice_offset != 0) memmove(result_chars, &result_chars[slice_offset * 3], position * 3);
          if (slice_offset != part_len) memmove(&result_chars[(position + part_len) * 3], &result_chars[(slice_offset + position) * 3], (string_len - position) * 3);
          ctf8_str_ze_lt16_to_corsar_chars(part.bytes, &result_chars[position * 3]);
     } else {
          if (string_ref_cnt != 0) set_ref_cnt(string, string_ref_cnt - 1);

          result                       = long_string__new(new_len);
          slice_array__set_len(result, new_len);
          result                       = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          const u8* const string_chars = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          u8*       const result_chars = slice_array__get_items(result);
          memcpy(result_chars, string_chars, position * 3);
          ctf8_str_ze_lt16_to_corsar_chars(part.bytes, &result_chars[position * 3]);
          memcpy(&result_chars[(position + part_len) * 3], &string_chars[position * 3], (string_len - position) * 3);
     }

     return result;
}

static t_any insert_part_to__long_str__long_str__own(t_thrd_data* const thrd_data, t_any string, t_any part, u64 const position, const char* const owner) {
     assume(string.bytes[15] == tid__string);
     assume(part.bytes[15] == tid__string);

     u32 const string_slice_offset = slice__get_offset(string);
     u32 const string_slice_len    = slice__get_len(string);
     u64 const string_array_len    = slice_array__get_len(string);
     u64 const string_array_cap    = slice_array__get_cap(string);
     u8*       string_chars        = slice_array__get_items(string);
     u64 const string_len          = string_slice_len <= slice_max_len ? string_slice_len : string_array_len;
     u64 const string_ref_cnt      = get_ref_cnt(string);

     assert(string_slice_len <= slice_max_len || string_slice_offset == 0);
     assert(string_array_cap >= string_array_len);

     if (position > string_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, string);
          ref_cnt__dec(thrd_data, part);
          return error__out_of_bounds(thrd_data, owner);
     }

     u32 const part_slice_offset = slice__get_offset(part);
     u32 const part_slice_len    = slice__get_len(part);
     u64 const part_array_len    = slice_array__get_len(part);
     u64 const part_array_cap    = slice_array__get_cap(part);
     u8*       part_chars        = slice_array__get_items(part);
     u64 const part_len          = part_slice_len <= slice_max_len ? part_slice_len : part_array_len;
     u64 const part_ref_cnt      = get_ref_cnt(part);

     assert(part_slice_len <= slice_max_len || part_slice_offset == 0);
     assert(part_array_cap >= part_array_len);

     u64 const new_len = string_len + part_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     if (string_chars == part_chars) {
          if (string_ref_cnt > 2) set_ref_cnt(string, string_ref_cnt - 2);
     } else {
          if (string_ref_cnt > 1) set_ref_cnt(string, string_ref_cnt - 1);
          if (part_ref_cnt > 1) set_ref_cnt(part, part_ref_cnt - 1);
     }

     int const string_is_mut  = string_ref_cnt == 1;
     int const part_is_mut = part_ref_cnt == 1;

     u8 const scenario =
          string_is_mut                                        |
          part_is_mut * 2                                      |
          (string_is_mut  && string_array_cap  >= new_len) * 4 |
          (part_is_mut && part_array_cap >= new_len) * 8       ;

     switch (scenario) {
     case 1: case 3:
          string.qwords[0] = (u64)realloc((u8*)string.qwords[0], new_cap * 3 + 16);
          slice_array__set_cap(string, new_cap);
          string_chars     = slice_array__get_items(string);
     case 5: case 7: case 15:
          slice_array__set_len(string, new_len);
          string = slice__set_metadata(string, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (string_slice_offset != 0) memmove(string_chars, &string_chars[string_slice_offset * 3], position * 3);
          if (string_slice_offset != part_len) memmove(&string_chars[(position + part_len) * 3], &string_chars[(string_slice_offset + position) * 3], (string_len - position) * 3);
          memcpy(&string_chars[position * 3], &part_chars[part_slice_offset * 3], part_len * 3);

          if (part_is_mut) free((void*)part.qwords[0]);

          return string;
     case 2:
          part.qwords[0] = (u64)realloc((u8*)part.qwords[0], new_cap * 3 + 16);
          slice_array__set_cap(part, new_cap);
          part_chars     = slice_array__get_items(part);
     case 10: case 11:
          slice_array__set_len(part, new_len);
          part = slice__set_metadata(part, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (part_slice_offset != position) memmove(&part_chars[position * 3], &part_chars[part_slice_offset * 3], part_len * 3);
          memcpy(part_chars, &string_chars[string_slice_offset * 3], position * 3);
          memcpy(&part_chars[(position + part_len) * 3], &string_chars[(string_slice_offset + position) * 3], (string_len - position) * 3);

          if (string_is_mut) free((void*)string.qwords[0]);

          return part;
     case 0: {
          t_any     result       = long_string__new(new_len);
          slice_array__set_len(result, new_len);
          result                 = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          u8* const result_chars = slice_array__get_items(result);
          memcpy(result_chars, &string_chars[string_slice_offset * 3], position * 3);
          memcpy(&result_chars[position * 3], &part_chars[part_slice_offset * 3], part_len * 3);
          memcpy(&result_chars[(position + part_len) * 3], &string_chars[(string_slice_offset + position) * 3], (string_len - position) * 3);

          if (string_chars == part_chars) {
               if (string_ref_cnt == 2) free((void*)string.qwords[0]);
          } else {
               if (string_is_mut) free((void*)string.qwords[0]);
               if (part_is_mut) free((void*)part.qwords[0]);
          }

          return result;
     }
     default:
          unreachable;
     }
}
