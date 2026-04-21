#pragma once

#include "../../common/fn.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../char/basic.h"
#include "../common/slice.h"
#include "../error/basic.h"
#include "../null/basic.h"
#include "cvt.h"

[[clang::always_inline]]
static u8 short_string__get_len(t_any const string) {
     assert(string.bytes[15] == tid__short_string);
     u8 const len = -__builtin_reduce_add((string.vector < 192) & (string.vector != 0));
     assume(len < 16);
     return len;
}

[[clang::always_inline]]
static u8 short_string__get_size(t_any const string) {
     assert(string.bytes[15] == tid__short_string);
     u8 const size = -__builtin_reduce_add(string.vector != 0);
     assume(size < 16);
     return size;
}

[[clang::always_inline]]
static u16 short_string__chars_start_mask(t_any const string) {
     assert(string.bytes[15] == tid__short_string);
     return v_16_bool_to_u16(__builtin_convertvector((string.vector < 192) & (string.vector != 0), v_16_bool));
}

core_string inline t_any short_string_from_chars(const u8* const chars, u8 const len) {
     t_any short_string = {};
     u8    offset       = 0;
     for (u64 idx = 0; idx < len; idx += 1) {
          u32 const code = char_to_code(&chars[idx * 3]);
          if (offset + char_size_in_ctf8(code) > 15) return null;

          offset += corsar_code_to_ctf8_char(code, &short_string.bytes[offset]);
     }

     return short_string;
}

static t_any long_string__new(u64 const cap) {
     assume(cap <= array_max_len);

     t_any string = {.structure = {.value = (u64)malloc(cap * 3 + 16), .type = tid__string}};
     set_ref_cnt(string, 1);
     slice_array__set_len(string, 0);
     slice_array__set_cap(string, cap);

     return string;
}

core_string t_any string_from_ascii(const char* const string) {
     typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));

     u64 const len = strlen(string);

     if (len < 16) {
          t_any result = {};
          memcpy_le_16(result.bytes, string, len);
          return result;
     }

     t_any result = long_string__new(len);
     result       = slice__set_metadata(result, 0, len <= slice_max_len ? len : slice_max_len + 1);
     slice_array__set_len(result, len);
     u8*   chars  = slice_array__get_items(result);
     for (u64 idx = 0; len - idx > 16; idx += 16) {
          v_16_u8 const chars_vec         = *(const v_16_u8*)&string[idx];
          *(v_32_u8*)&chars[idx * 3]      = __builtin_shufflevector(chars_vec, (const v_16_u8){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
          *(v_16_u8*)&chars[idx * 3 + 32] = __builtin_shufflevector(chars_vec, (const v_16_u8){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);
     }

     v_16_u8 const chars_vec         = *(const v_16_u8*)&string[len - 16];
     *(v_32_u8*)&chars[len * 3 - 48] = __builtin_shufflevector(chars_vec, (const v_16_u8){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
     *(v_16_u8*)&chars[len * 3 - 16] = __builtin_shufflevector(chars_vec, (const v_16_u8){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

     return result;
}

core_string t_any string_from_n_len_sysstr(t_thrd_data* const thrd_data, u64 const sysstr_len, const char* const sysstr, const char* const owner) {
     assert(sysstr_len <= array_max_len);

     if (sysstr_len <= 113) {
          t_any string = {};

          t_str_cvt_result const cvt_result = sys_chars_to_ctf8_chars(sysstr_len, (const u8*)sysstr, 15, string.bytes);
          if (cvt_result.src_offset == sysstr_len)
               return string;

          if ((i64)cvt_result.src_offset <= 0) [[clang::unlikely]] {
               switch (cvt_result.src_offset) {
               case 0: case str_cvt_err__encoding:
                    goto invalid_enc_label;
               default:
                    unreachable;
               }
          }
     }

     u64   string_cap   = sysstr_len * 2 + 7;
     string_cap         = string_cap > array_max_len ? array_max_len : string_cap;
     t_any string       = long_string__new(string_cap);
     u8*   string_chars = slice_array__get_items(string);

     t_str_cvt_result current_offsets = {};
     while (true) {
          t_str_cvt_result const cvt_result = sys_chars_to_corsar_chars(
               sysstr_len - current_offsets.src_offset, (const u8*)&sysstr[current_offsets.src_offset],
               string_cap * 3 - current_offsets.dst_offset, &string_chars[current_offsets.dst_offset]
          );

          if ((i64)cvt_result.src_offset <= 0) [[clang::unlikely]] {
               free((void*)string.qwords[0]);

               switch (cvt_result.src_offset) {
               case 0: case str_cvt_err__encoding:
                    goto invalid_enc_label;
               default:
                    unreachable;
               }
          }

          current_offsets.src_offset += cvt_result.src_offset;
          current_offsets.dst_offset += cvt_result.dst_offset;
          if (current_offsets.src_offset == sysstr_len) {
               u64 const string_len = current_offsets.dst_offset / 3;
               if (sysstr_len != string_len)
                    string.qwords[0] = (u64)realloc((u8*)string.qwords[0], current_offsets.dst_offset + 16);

               string = slice__set_metadata(string, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);
               slice_array__set_cap(string, string_len);
               slice_array__set_len(string, string_len);

               return string;
          }

          if (string_cap == array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);

          string_cap       = string_cap * 3 / 2 + 7;
          string_cap       = string_cap > array_max_len ? array_max_len : string_cap;
          string.qwords[0] = (u64)realloc((u8*)string.qwords[0], string_cap * 3 + 16);
          string_chars     = slice_array__get_items(string);
     }

     invalid_enc_label:
     return error__invalid_enc(thrd_data, owner);
}

core_string t_any string_from_ze_sysstr(t_thrd_data* const thrd_data, const char* const sysstr, const char* const owner) {
     return string_from_n_len_sysstr(thrd_data, strlen(sysstr), sysstr, owner);
}