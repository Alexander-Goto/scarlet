#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/fn.h"
#include "../struct/string/basic.h"

core t_any McoreFNstr_to_tkn(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.str_to_tkn";

     typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));

     t_any const string = args[0];
     if (!(string.bytes[15] == tid__short_string || string.bytes[15] == tid__string)) [[clang::unlikely]] {
          if (string.bytes[15] == tid__error) return string;

          ref_cnt__dec(thrd_data, string);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     char        buffer[32];
     const char* chars;
     u8          len;
     if (string.bytes[15] == tid__short_string) {
          len   = short_string__get_size(string);
          chars = (const char*)string.bytes;
     } else {
          u32       const slice_len = slice__get_len(string);
          u32       const slice_offset = slice__get_offset(string);
          u64       const ref_cnt      = get_ref_cnt(string);
          const u8* const string_chars = &((const u8*)slice_array__get_items(string))[slice_offset * 3];
          len                          = slice_len;

          if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);
          bool const need_to_free = ref_cnt == 1;

          if (slice_len > 25 || slice_len < 16) {
               if (need_to_free) free((void*)string.qwords[0]);
               return null;
          }

          u8 idx = 0;
          do {
               v_32_u8 const part  = *(const v_32_u8*)&string_chars[idx * 3];
               if (__builtin_reduce_and(part < (const v_32_u8) {
                    1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1
               }) == 0) {
                    if (need_to_free) free((void*)string.qwords[0]);
                    return null;
               }

               *(v_16_u8*)&buffer[idx] = __builtin_shufflevector(part, (const v_32_u8){}, 2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 32, 32, 32, 32, 32);

               idx += 10;
          } while (len - idx > 10);

          v_32_u8 const part  = *(const v_32_u8*)&string_chars[len * 3 - 32];
          if (__builtin_reduce_and(part < (const v_32_u8) {
               1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128, 1, 1, 128
          }) == 0) {
               if (need_to_free) free((void*)string.qwords[0]);
               return null;
          }

          *(v_16_u8*)&buffer[len - 11] = __builtin_shufflevector(part, (const v_32_u8){}, 1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 32, 32, 32, 32, 32);

          if (need_to_free) free((void*)string.qwords[0]);
          chars = buffer;
     }

     t_any const result = token_from_ascii_chars(len, chars);

     return result;
}