#pragma once

#include "../common/corsar.h"
#include "../common/fn.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"
#include "../struct/byte_array/fn.h"
#include "../struct/set/fn.h"
#include "../struct/string/check.h"
#include "../struct/table/fn.h"
#include "serialize.h"

// -1 incorrect data
// -2 out of bounds
[[clang::always_inline]]
static u64 bytes__read_len(u64* const bytes_len, const u8** const bytes) {
     typedef u8  v_8_u8  __attribute__((ext_vector_type(8), aligned(1)));
     typedef u64 v_8_u64 __attribute__((ext_vector_type(8)));

     if (*bytes_len >= 8) {
          u64 const len_56_bits = __builtin_reduce_or(__builtin_convertvector(*(const v_8_u8*)*bytes & 0x7f, v_8_u64) << (const v_8_u64){0, 7, 14, 21, 28, 35, 42, 49});
          u64 const len_mask    = *(const ua_u64*)*bytes & 0x8080'8080'8080'8080ull;
          u8  const len_size    = len_mask == 0 ? 1 : (__builtin_ctzll(len_mask) + 1) / 8;
          u64 const len         = len_56_bits & (((u64)1 << len_size * 7) - 1);
          u8  const last_byte   = (*bytes)[len_size - 1];
          u64 const result      = len_mask == 0 || (len_size != 1 && last_byte == 128) ? -1 : len;
          u64 const readed_len  = result != -1 ? len_size : 0;
          *bytes               += readed_len;
          *bytes_len           -= readed_len;
          return result;
     }

     u64 len      = 0;
     u8  len_size = 0;
     u8  last_byte;
     for (u8 idx = 0; idx < 7; idx += 1) {
          if (*bytes_len == 0) [[clang::unlikely]] return -2;
          last_byte   = **bytes;
          len        |= ((u64)last_byte & 0x7f) << 7 * idx;
          len_size   += 1;
          *bytes     += 1;
          *bytes_len -= 1;

          if (last_byte >= 128) break;
     }

     len = last_byte < 128 ? -2 : len_size != 1 && last_byte == 128 ? -1 : len;
     return len;
}

[[gnu::hot, clang::noinline]]
static t_any deserialize(t_thrd_data* const thrd_data, u64* const bytes_len, const u8** const bytes, const char* const owner) {
     typedef u8 v_32_u8 __attribute__((ext_vector_type(32)));

     [[gnu::aligned(alignof(v_32_u8))]]
     char token_chars[32];

     if (*bytes_len == 0) [[clang::unlikely]] goto out_of_bounds_label;

     t_serialize_type const data_type = **bytes;

     *bytes_len -= 1;
     *bytes     += 1;

     switch (data_type) {
     case st__non_neg_num__0__ff: {
          if (*bytes_len == 0) [[clang::unlikely]] goto out_of_bounds_label;

          u8 const num = **bytes;
          *bytes_len  -= 1;
          *bytes      += 1;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__non_neg_num__100__1_00ff: {
          if (*bytes_len < 2) [[clang::unlikely]] goto out_of_bounds_label;

          u32 num = 0;
          memcpy_inline(&num, *bytes, 2);

          num        += 0x100;
          *bytes_len -= 2;
          *bytes     += 2;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__non_neg_num__1_0100__101_00ff: {
          if (*bytes_len < 3) [[clang::unlikely]] goto out_of_bounds_label;

          u32 num = 0;
          memcpy_inline(&num, *bytes, 3);
          num        += 0x1'0100ll;
          *bytes_len -= 3;
          *bytes     += 3;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__non_neg_num__101_0100__1_0101_00ff: {
          if (*bytes_len < 4) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;
          memcpy_inline(&num, *bytes, 4);
          num        += 0x101'0100ll;
          *bytes_len -= 4;
          *bytes     += 4;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__non_neg_num__1_0101_0100__101_0101_00ff: {
          if (*bytes_len < 5) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;
          memcpy_inline(&num, *bytes, 5);
          num        += 0x1'0101'0100ll;
          *bytes_len -= 5;
          *bytes     += 5;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__non_neg_num__101_0101_0100__1_0101_0101_00ff: {
          if (*bytes_len < 6) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;
          memcpy_inline(&num, *bytes, 6);
          num        += 0x101'0101'0100ll;
          *bytes_len -= 6;
          *bytes     += 6;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__non_neg_num__1_0101_0101_0100__101_0101_0101_00ff: {
          if (*bytes_len < 7) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;
          memcpy_inline(&num, *bytes, 7);
          num        += 0x1'0101'0101'0100ll;
          *bytes_len -= 7;
          *bytes     += 7;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__non_neg_num__101_0101_0101_0100__7fff_ffff_ffff_ffff: {
          if (*bytes_len < 8) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;
          memcpy_inline(&num, *bytes, 8);
          if (num > 0x7efe'fefe'fefe'feffull) [[clang::unlikely]] goto incorrect_data_label;

          num        += 0x101'0101'0101'0100ll;
          *bytes_len -= 8;
          *bytes     += 8;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__neg_num__1__100: {
          if (*bytes_len == 0) [[clang::unlikely]] goto out_of_bounds_label;

          u16 num     = **bytes;
          num         = (u16)-1 - num;
          *bytes_len -= 1;
          *bytes     += 1;
          return (const t_any){.structure = {.value = (i16)num, .type = tid__int}};
     }
     case st__neg_num__101__1_0100: {
          if (*bytes_len < 2) [[clang::unlikely]] goto out_of_bounds_label;

          u32 num = 0;
          memcpy_inline(&num, *bytes, 2);

          num         = (u32)-0x101 - num;
          *bytes_len -= 2;
          *bytes     += 2;
          return (const t_any){.structure = {.value = (i32)num, .type = tid__int}};
     }
     case st__neg_num__1_0101__101_0100: {
          if (*bytes_len < 3) [[clang::unlikely]] goto out_of_bounds_label;

          u32 num = 0;
          memcpy_inline(&num, *bytes, 3);

          num         = (u32)-0x1'0101ll - num;
          *bytes_len -= 3;
          *bytes     += 3;
          return (const t_any){.structure = {.value = (i32)num, .type = tid__int}};
     }
     case st__neg_num__101_0101__1_0101_0100: {
          if (*bytes_len < 4) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;

          memcpy_inline(&num, *bytes, 4);
          num         = (u64)-0x101'0101ll - num;
          *bytes_len -= 4;
          *bytes     += 4;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__neg_num__1_0101_0101__101_0101_0100: {
          if (*bytes_len < 5) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;
          memcpy_inline(&num, *bytes, 5);

          num         = (u64)-0x1'0101'0101ll - num;
          *bytes_len -= 5;
          *bytes     += 5;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__neg_num__101_0101_0101__1_0101_0101_0100: {
          if (*bytes_len < 6) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;
          memcpy_inline(&num, *bytes, 6);

          num         = (u64)-0x101'0101'0101ll - num;
          *bytes_len -= 6;
          *bytes     += 6;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__neg_num__1_0101_0101_0101__101_0101_0101_0100: {
          if (*bytes_len < 7) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;
          memcpy_inline(&num, *bytes, 7);

          num         = (u64)-0x1'0101'0101'0101ll - num;
          *bytes_len -= 7;
          *bytes     += 7;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__neg_num__101_0101_0101_0101__8000_0000_0000_0000: {
          if (*bytes_len < 8) [[clang::unlikely]] goto out_of_bounds_label;

          u64 num = 0;
          memcpy_inline(&num, *bytes, 8);
          if (num > 0x7efe'fefe'fefe'feffull) [[clang::unlikely]] goto incorrect_data_label;

          num         = (u64)-0x101'0101'0101'0101ll - num;
          *bytes_len -= 8;
          *bytes     += 8;
          return (const t_any){.structure = {.value = num, .type = tid__int}};
     }
     case st__string: {
          bool  is_short;
          t_any string;
          if (*bytes_len >= 16) {
               memcpy_inline(string.bytes, *bytes, 16);
               is_short = __builtin_reduce_or(string.vector == 0) != 0;
          } else {
               if (*bytes_len == 0) [[clang::unlikely]] goto out_of_bounds_label;

               is_short        = true;
               string.raw_bits = 0;
               memcpy_le_16(string.bytes, *bytes, *bytes_len);
          }

          if (is_short) {
               u8 const string_size = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(string.vector == 0, v_16_bool)));

               if (string_size >= *bytes_len) [[clang::unlikely]] goto out_of_bounds_label;

               string.raw_bits &= ((u128)1 << string_size * 8) - 1;
               if (__builtin_reduce_and(string.vector < 128) == 0)
                    if (check_ctf8_chars(string_size, string.bytes) != string_size) [[clang::unlikely]] goto incorrect_data_label;

               *bytes_len -= string_size + 1;
               *bytes     += string_size + 1;

               return string;
          }

          u64 const ctf8_size = look_byte_from_begin(*bytes, *bytes_len, 0);
          if (ctf8_size == (u64)-1) [[clang::unlikely]]
               goto out_of_bounds_label;

          string                            = long_string__new(ctf8_size);
          t_str_cvt_result const cvt_result = ctf8_chars_to_corsar_chars(ctf8_size, *bytes, ctf8_size * 3, slice_array__get_items(string));
          if (cvt_result.src_offset != ctf8_size) [[clang::unlikely]] {
               free((void*)string.qwords[0]);
               goto incorrect_data_label;
          }

          *bytes_len -= ctf8_size + 1;
          *bytes     += ctf8_size + 1;

          u64 const string_len = cvt_result.dst_offset / 3;
          if (ctf8_size != string_len)
               string.qwords[0] = (u64)realloc((u8*)string.qwords[0], cvt_result.dst_offset + 16);

          string = slice__set_metadata(string, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);
          slice_array__set_cap(string, string_len);
          slice_array__set_len(string, string_len);

          return string;
     }
     case st__token: {
          if (*bytes_len < 1) [[clang::unlikely]] goto out_of_bounds_label;

          if (*bytes_len >= 32)
               memcpy_inline(token_chars, *bytes, 32);
          else {
               memset_inline(token_chars, 0, 32);
               memcpy_le_32(token_chars, *bytes, *bytes_len);
          }

          u32 const end_char_mask = v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)token_chars > 128, v_32_bool));
          u8  const token_len     = end_char_mask == 0 ? 32 : __builtin_ctzl(end_char_mask) + 1;
          if (token_len > 25) [[clang::unlikely]] goto incorrect_data_label;

          token_chars[token_len - 1] &= 127;
          t_any const token           = token_from_ascii_chars(token_len, token_chars);
          if (token.bytes[15] == tid__null) [[clang::unlikely]] goto incorrect_data_label;

          *bytes_len -= token_len;
          *bytes     += token_len;

          return token;
     }
     case st__false:
          return bool__false;
     case st__true:
          return bool__true;
     case st__byte: {
          if (*bytes_len == 0) [[clang::unlikely]] goto out_of_bounds_label;

          t_any const byte = {.bytes = {**bytes, [15] = tid__byte}};

          *bytes_len -= 1;
          *bytes     += 1;

          return byte;
     }
     case st__char__1__100: {
          if (*bytes_len == 0) [[clang::unlikely]] goto out_of_bounds_label;

          t_any char_ = {.bytes = {[15] = tid__char}};
          char_from_code(char_.bytes, (u16)(**bytes) + 1);

          *bytes_len -= 1;
          *bytes     += 1;

          return char_;
     }
     case st__char__101__1_0100: {
          if (*bytes_len < 2) [[clang::unlikely]] goto out_of_bounds_label;

          t_any char_ = {.bytes = {[15] = tid__char}};
          u32   code  = 0;
          memcpy_inline(&code, *bytes, 2);

          code += 0x101;
          if (!corsar_code_is_correct(code)) [[clang::unlikely]] goto incorrect_data_label;

          char_from_code(char_.bytes, code);

          *bytes_len -= 2;
          *bytes     += 2;

          return char_;
     }
     case st__char__1_0101__3f_ffff: {
          if (*bytes_len < 3) [[clang::unlikely]] goto out_of_bounds_label;

          t_any char_ = {.bytes = {[15] = tid__char}};
          u32   code  = 0;
          memcpy_inline(&code, *bytes, 3);

          code += 0x1'0101ull;
          if (!corsar_code_is_correct(code)) [[clang::unlikely]] goto incorrect_data_label;

          char_from_code(char_.bytes, code);

          *bytes_len -= 3;
          *bytes     += 3;

          return char_;
     }
     case st__short_time: {
          if (*bytes_len < 4) [[clang::unlikely]] goto out_of_bounds_label;

          u32 all_seconds;
          memcpy_inline(&all_seconds, *bytes, 4);

          *bytes_len -= 4;
          *bytes     += 4;

          return (const t_any){.structure = {.value = (u64)all_seconds + 1'893'456'000ull, .type = tid__time}};
     }
     case st__full_time: {
          if (*bytes_len < 8) [[clang::unlikely]] goto out_of_bounds_label;

          t_any time = {.bytes = {[15] = tid__time}};
          memcpy_inline(time.bytes, *bytes, 8);
          if (time.qwords[0] >= 9223372036825516800ull) [[clang::unlikely]] goto incorrect_data_label;

          *bytes_len -= 8;
          *bytes     += 8;

          return time;
     }
     case st__float: {
          if (*bytes_len < 8) [[clang::unlikely]] goto out_of_bounds_label;

          t_any float_;
          memcpy_inline(float_.bytes, *bytes, 8);
          float_.bytes[15] = tid__float;

          *bytes_len -= 8;
          *bytes     += 8;

          return float_;
     }
     case st__byte_array: {
          u64 const array_len = bytes__read_len(bytes_len, bytes);
          if (array_len > array_max_len) [[clang::unlikely]] {
               if (array_len == (u64)-2) goto out_of_bounds_label;
               else                      goto incorrect_data_label;
          }

          if (*bytes_len < array_len) [[clang::unlikely]] goto out_of_bounds_label;

          t_any array;
          if (array_len < 15) {
               if (*bytes_len >= 16)
                    memcpy_inline(array.bytes, *bytes, 16);
               else memcpy_le_16(array.bytes, *bytes, array_len);

               array.raw_bits &= ((u128)1 << array_len * 8) - 1;
               array.bytes[15] = tid__short_byte_array;
               array           = short_byte_array__set_len(array, array_len);

               *bytes_len -= array_len;
               *bytes     += array_len;

               return array;
          }

          array = long_byte_array__new(array_len);
          array = slice__set_metadata(array, 0, array_len <= slice_max_len ? array_len : slice_max_len + 1);
          slice_array__set_len(array, array_len);
          memcpy(slice_array__get_items(array), *bytes, array_len);

          *bytes_len -= array_len;
          *bytes     += array_len;

          return array;
     }
     case st__obj: {
          u64 const obj_len = bytes__read_len(bytes_len, bytes);
          if (obj_len > hash_map_max_len) [[clang::unlikely]] {
               if (obj_len == (u64)-2) goto out_of_bounds_label;
               else                    goto incorrect_data_label;
          }

          t_any obj = obj__init(thrd_data, obj_len, owner);
          for (u64 counter = 0; counter < obj_len; counter += 1) {
               if (*bytes_len < 1) [[clang::unlikely]] goto out_of_bounds_label;

               if (*bytes_len >= 32)
                    memcpy_inline(token_chars, *bytes, 32);
               else {
                    memset_inline(token_chars, 0, 32);
                    memcpy_le_32(token_chars, *bytes, *bytes_len);
               }

               u32 const end_char_mask = v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)token_chars > 128, v_32_bool));
               u8  const token_len     = end_char_mask == 0 ? 32 : __builtin_ctzl(end_char_mask) + 1;
               if (token_len > 25) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, obj);
                    goto incorrect_data_label;
               }

               token_chars[token_len - 1] &= 127;
               t_any const key             = token_from_ascii_chars(token_len, token_chars);
               if (key.bytes[15] == tid__null) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, obj);
                    goto incorrect_data_label;
               }

               *bytes_len -= token_len;
               *bytes     += token_len;

               t_any const value = deserialize(thrd_data, bytes_len, bytes, owner);
               if (value.bytes[15] == tid__error) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, obj);
                    return value;
               }

               obj = obj__builtin_unsafe_add_kv__own(thrd_data, obj, key, value, owner);
               if (obj.bytes[15] != tid__obj) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, obj);
                    goto incorrect_data_label;
               }
          }

          return obj;
     }
     case st__table: {
          u64 const table_len = bytes__read_len(bytes_len, bytes);
          if (table_len > hash_map_max_len) [[clang::unlikely]] {
               if (table_len == (u64)-2) goto out_of_bounds_label;
               else                      goto incorrect_data_label;
          }

          t_any table = table__init(thrd_data, table_len, owner);
          for (u64 counter = 0; counter < table_len; counter += 1) {
               t_any const key = deserialize(thrd_data, bytes_len, bytes, owner);
               if (key.bytes[15] == tid__error) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, table);
                    return key;
               }

               t_any const value = deserialize(thrd_data, bytes_len, bytes, owner);
               if (value.bytes[15] == tid__error) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, table);
                    ref_cnt__dec(thrd_data, key);
                    return value;
               }

               table = table__builtin_add_kv__own(thrd_data, table, key, value, owner);
               if (table.bytes[15] != tid__table) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, table);
                    goto incorrect_data_label;
               }
          }

          return table;
     }
     case st__set: {
          u64 const set_len = bytes__read_len(bytes_len, bytes);
          if (set_len > hash_map_max_len) [[clang::unlikely]] {
               if (set_len == (u64)-2) goto out_of_bounds_label;
               else                    goto incorrect_data_label;
          }

          t_any set = set__init(thrd_data, set_len, owner);
          for (u64 counter = 0; counter < set_len; counter += 1) {
               t_any const item = deserialize(thrd_data, bytes_len, bytes, owner);
               if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, set);
                    return item;
               }

               set = set__builtin_add_item__own(thrd_data, set, item, owner);
               if (set.bytes[15] != tid__set) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, set);
                    goto incorrect_data_label;
               }
          }

          return set;
     }
     case st__array: {
          u64 const array_len = bytes__read_len(bytes_len, bytes);
          if (array_len > array_max_len) [[clang::unlikely]] {
               if (array_len == (u64)-2) goto out_of_bounds_label;
               else                      goto incorrect_data_label;
          }

          if (array_len == 0) return empty_array;

          t_any        array = array__init(thrd_data, array_len, owner);
          t_any* const items = slice_array__get_items(array);
          for (u64 idx = 0; idx < array_len; idx += 1) {
               items[idx] = deserialize(thrd_data, bytes_len, bytes, owner);
               if (items[idx].bytes[15] == tid__error) [[clang::unlikely]] {
                    t_any const error = items[idx];
                    for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, items[free_idx]), free_idx -= 1);
                    free((void*)array.qwords[0]);
                    return error;
               }
          }

          array = slice__set_metadata(array, 0, array_len <= slice_max_len ? array_len : slice_max_len + 1);
          slice_array__set_len(array, array_len);

          return array;
     }
     case st__fix_obj: {
          if (*bytes_len < 3) [[clang::unlikely]] goto out_of_bounds_label;

          u8 const fix_idx_offset = **bytes;
          u8 const fix_idx_size   = (*bytes)[1];

          if (fix_idx_offset >= 120 || (u16)fix_idx_offset + (u16)fix_idx_size > 120 || fix_idx_size > __builtin_popcountll(hash_map_max_chunks - 1)) [[clang::unlikely]] goto incorrect_data_label;

          *bytes_len -= 2;
          *bytes     += 2;

          u64 const obj_len = bytes__read_len(bytes_len, bytes);
          if (obj_len > hash_map_max_chunks || obj_len > ((u64)1 << fix_idx_size) || obj_len == 0) [[clang::unlikely]] {
               if (obj_len == (u64)-2) goto out_of_bounds_label;
               else                    goto incorrect_data_label;
          }

          t_any        obj   = obj__init_fix(fix_idx_offset, fix_idx_size);
          t_any* const items = obj__get_fields(obj);

          for (u64 counter = 0; counter < obj_len; counter += 1) {
               if (*bytes_len < 1) [[clang::unlikely]] goto out_of_bounds_label;

               if (*bytes_len >= 32)
                    memcpy_inline(token_chars, *bytes, 32);
               else {
                    memset_inline(token_chars, 0, 32);
                    memcpy_le_32(token_chars, *bytes, *bytes_len);
               }

               u32 const end_char_mask = v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)token_chars >= 128, v_32_bool));
               u8        token_len     = end_char_mask == 0 ? 32 : __builtin_ctzl(end_char_mask) + 1;
               if (token_len > 26 || (token_len == 26 && (u8)token_chars[token_len - 1] != 128)) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, obj);
                    goto incorrect_data_label;
               }

               u8 const is_deleted = (u8)token_chars[token_len - 1] == 128;
               if (is_deleted)
                    token_len -= 1;
               else token_chars[token_len - 1] &= 127;

               t_any const key = token_from_ascii_chars(token_len, token_chars);
               if (key.bytes[15] == tid__null) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, obj);
                    goto incorrect_data_label;
               }

               *bytes_len -= token_len + is_deleted;
               *bytes     += token_len + is_deleted;

               u8     const idx     = (u64)(key.raw_bits >> fix_idx_offset) & (((u64)1 << fix_idx_size) - 1);
               t_any* const key_ptr = &items[idx * 2];
               if (key_ptr->bytes[15] != tid__null) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, obj);
                    goto incorrect_data_label;
               }

               if (is_deleted) {
                    key_ptr[0]           = key;
                    key_ptr[0].bytes[15] = tid__holder;
                    key_ptr[1].bytes[15] = tid__holder;
               } else {
                    t_any const value = deserialize(thrd_data, bytes_len, bytes, owner);
                    if (value.bytes[15] == tid__error) [[clang::unlikely]] {
                         ref_cnt__dec(thrd_data, obj);
                         return value;
                    }

                    obj        = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
                    key_ptr[0] = key;
                    key_ptr[1] = value;
               }
          }

          return obj;
     }

     [[clang::unlikely]]
     default:
          goto incorrect_data_label;
     }

     out_of_bounds_label:
     return error__out_of_bounds(thrd_data, owner);

     incorrect_data_label:
     return error__deserialization_from_incorrect_data(thrd_data, owner);
}

core t_any McoreFNdeserialize(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.deserialize";

     const u8*   bytes;
     u64         bytes_len;
     t_any const bytes_arg = args[0];
     switch (bytes_arg.bytes[15]) {
     case tid__short_byte_array:
          bytes_len = short_byte_array__get_len(bytes_arg);
          bytes     = bytes_arg.bytes;
          break;
     case tid__byte_array: {
          u32 const slice_offset = slice__get_offset(bytes_arg);
          u32 const slice_len    = slice__get_len(bytes_arg);
          bytes_len              = slice_len <= slice_max_len ? slice_len : slice_array__get_len(bytes_arg);
          bytes                  = &((const u8*)slice_array__get_items(bytes_arg))[slice_offset];
          break;
     }
     [[clang::unlikely]]
     case tid__error:
          return bytes_arg;
     [[clang::unlikely]]
     default:
          goto invalid_type_label;
     }

     call_stack__push(thrd_data, owner);

     u64 const start_bytes_len = bytes_len;
     t_any const item = deserialize(thrd_data, &bytes_len, &bytes, owner);
     if (item.bytes[15] == tid__error) [[clang::unlikely]] {
          call_stack__pop(thrd_data);

          ref_cnt__dec(thrd_data, bytes_arg);
          return item;
     }

     t_any const remain_bytes = bytes_arg.bytes[15] == tid__byte_array ?
          long_byte_array__drop__own(thrd_data, bytes_arg, start_bytes_len - bytes_len, owner) :
          short_byte_array__drop(thrd_data, bytes_arg, start_bytes_len - bytes_len, owner);
     t_any const box = box__new(thrd_data, 2, owner);

     call_stack__pop(thrd_data);

     t_any* const box_items = box__get_items(box);
     box_items[0]           = item;
     box_items[1]           = remain_bytes;
     return box;

     invalid_type_label:
     ref_cnt__dec(thrd_data, bytes_arg);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}