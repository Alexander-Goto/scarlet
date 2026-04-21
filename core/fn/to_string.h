#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/obj/mtd.h"
#include "../struct/string/fn.h"
#include "../struct/table/basic.h"
#include "../struct/time/fn.h"
#include "../struct/token/basic.h"

[[gnu::hot, clang::noinline]]
static t_any to_string__half_own(t_thrd_data* const thrd_data, t_any* const result__own, t_any arg, const char* const owner) {
     assert(result__own->bytes[15] == tid__short_string || result__own->bytes[15] == tid__string);

     typedef u8  v_16_u8_a1    __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8  v_16_u8_a16   __attribute__((ext_vector_type(16), aligned(16)));
     typedef u8  v_16_u8_adef  __attribute__((ext_vector_type(16)));
     typedef u8  v_32_u8_a1    __attribute__((ext_vector_type(32), aligned(1)));
     typedef u8  v_32_u8_adef  __attribute__((ext_vector_type(32)));
     typedef u8  v_64_u8_a1    __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8  v_64_u8_adef  __attribute__((ext_vector_type(64)));
     typedef u32 v_8_u32_adef  __attribute__((ext_vector_type(8)));
     typedef u64 v_2_u64_adef  __attribute__((ext_vector_type(2)));
     typedef u64 v_4_u64_adef  __attribute__((ext_vector_type(4)));
     typedef u64 v_32_u64_adef __attribute__((ext_vector_type(32)));

     typedef u8 v_16_u8_achars __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8_adef))));
     typedef u8 v_32_u8_achars __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8_adef))));

     [[gnu::aligned(alignof(v_64_u8_adef))]]
     char chars[352];
     [[gnu::aligned(16)]]
     char buffer[1072];

     switch (arg.bytes[15]) {
     case tid__short_string:
          short_string_label:
          *result__own = result__own->bytes[15] == tid__short_string ? concat__short_str__short_str(*result__own, arg) : concat__long_str__short_str__own(thrd_data, *result__own, arg, owner);
          return *result__own;
     case tid__null:
          arg = (const t_any){.bytes = "null"};
          goto short_string_label;
     case tid__token: {
          memset_inline(chars, 0, 32);

          u8 len;
          token_to_ascii_chars(arg, &len, chars);

          if (len < 16) {
               memcpy_inline(arg.bytes, chars, 16);
               goto short_string_label;
          }

          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               arg = long_string__new(len);
               slice_array__set_len(arg, len);
               arg = slice__set_metadata(arg, 0, len);
          } else {
               arg.qwords[0] = (u64)buffer;
               arg.bytes[15] = tid__string;
               arg           = slice__set_metadata(arg, 0, len);
#ifndef NDEBUG
               set_ref_cnt(arg, 1);
#endif
               slice_array__set_len(arg, len);
               slice_array__set_cap(arg, len);
               set_ref_cnt(arg, 0);
          }

          u8*            const long_chars = slice_array__get_items(arg);
          v_16_u8_achars const low_part   = *(const v_16_u8_achars*)chars;
          v_16_u8_a1     const high_part  = *(const v_16_u8_a1*)&chars[len - 16];

          *(v_32_u8_a1*)long_chars      = __builtin_shufflevector(low_part, (const v_16_u8_achars){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
          *(v_16_u8_a1*)&long_chars[32] = __builtin_shufflevector(low_part, (const v_16_u8_achars){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

          *(v_32_u8_a1*)&long_chars[len * 3 - 48] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
          *(v_16_u8_a1*)&long_chars[len * 3 - 16] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               *result__own = arg;
               return *result__own;
          }

          goto long_string_label;
     }
     case tid__bool:
          arg = arg.bytes[0] == 1 ? (const t_any){.bytes = "true"} : (const t_any){.bytes = "false"};
          goto short_string_label;
     case tid__byte: {
          u8 const byte = arg.bytes[0];
          arg.raw_bits  = 0;
          arg.bytes[0]  = "0123456789abcdef"[byte >> 4];
          arg.bytes[1]  = "0123456789abcdef"[byte & 0xf];
          goto short_string_label;
     }
     case tid__char: {
          u32 const code = char_to_code(arg.bytes);
          arg.raw_bits   = 0;
          corsar_code_to_ctf8_char(code, arg.bytes);
          goto short_string_label;
     }
     case tid__int: {
          if (arg.qwords[0] == 0) {
               arg.qwords[0] = '0';
               arg.qwords[1] = 0;
               goto short_string_label;
          }

          bool const is_neg       = (i64)arg.qwords[0] < 0;
          u64  const integer      = is_neg ? -arg.qwords[0] : arg.qwords[0];
          *(v_32_u8_achars*)chars = __builtin_convertvector((const v_32_u64_adef)integer / (const v_32_u64_adef){
               10000000000000000000ull, 1000000000000000000ull, 100000000000000000ull, 10000000000000000ull, 1000000000000000ull, 100000000000000ull, 10000000000000ull,
               1000000000000ull, 100000000000ull, 10000000000ull, 1000000000ull, 100000000ull, 10000000ull, 1000000ull, 100000ull, 10000ull, 1000ull, 100ull, 10ull,
               1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 10, v_32_u8_achars) + (u8)'0';

          u8 const str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8_achars*)chars != '0', v_32_bool)));
          u8 const str_len    = 20 - str_offset;
          u8 const full_len   = str_len + is_neg;

          if (full_len < 16) {
               arg.raw_bits = is_neg ? '-' : 0;
               memcpy_le_16(&arg.bytes[is_neg], &chars[str_offset], str_len);
               goto short_string_label;
          }

          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               arg = long_string__new(full_len);
               slice_array__set_len(arg, full_len);
               arg = slice__set_metadata(arg, 0, full_len);
          } else {
               arg.qwords[0] = (u64)buffer;
               arg.bytes[15] = tid__string;
               arg           = slice__set_metadata(arg, 0, full_len);
               #ifndef NDEBUG
               set_ref_cnt(arg, 1);
               #endif
               slice_array__set_len(arg, full_len);
               slice_array__set_cap(arg, full_len);
               set_ref_cnt(arg, 0);
          }

          u8* long_chars    = slice_array__get_items(arg);
          *(u32*)long_chars = is_neg ? (u32)'-' << 16 : 0;
          long_chars        = is_neg ? &long_chars[3] : long_chars;
          if (str_len > 32) {
               v_32_u8_a1 const low_part  = *(const v_32_u8_a1*)&chars[str_offset];
               v_32_u8_a1 const high_part = *(const v_32_u8_a1*)&chars[str_offset + str_len - 32];

               *(v_64_u8_a1*)long_chars = __builtin_shufflevector(low_part, (const v_32_u8_a1){},
                    32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
                    32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
               );
               *(v_32_u8_a1*)&long_chars[64] = __builtin_shufflevector(low_part, (const v_32_u8_a1){},
                    32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
               );

               *(v_64_u8_a1*)&long_chars[str_len * 3 - 96] = __builtin_shufflevector(high_part, (const v_32_u8_a1){},
                    32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
                    32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
               );
               *(v_32_u8_a1*)&long_chars[str_len * 3 - 32] = __builtin_shufflevector(high_part, (const v_32_u8_a1){},
                    32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
               );
          } else if (str_len != 15) {
               v_16_u8_a1 const low_part  = *(const v_16_u8_a1*)&chars[str_offset];
               v_16_u8_a1 const high_part = *(const v_16_u8_a1*)&chars[str_offset + str_len - 16];

               *(v_32_u8_a1*)long_chars      = __builtin_shufflevector(low_part, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
               *(v_16_u8_a1*)&long_chars[32] = __builtin_shufflevector(low_part, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

               *(v_32_u8_a1*)&long_chars[str_len * 3 - 48] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
               *(v_16_u8_a1*)&long_chars[str_len * 3 - 16] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);
          } else {
               if (str_offset == 0) {
                    v_16_u8_achars const short_chars = *(const v_16_u8_achars*)chars;

                    *(v_32_u8_a1*)long_chars      = __builtin_shufflevector(short_chars, (const v_16_u8_achars){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
                    *(v_16_u8_a1*)&long_chars[29] = __builtin_shufflevector(short_chars, (const v_16_u8_achars){}, 9, 16, 16, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14);
               } else {
                    v_16_u8_a1 const short_chars = *(const v_16_u8_a1*)&chars[str_offset - 1];

                    *(v_32_u8_a1*)long_chars      = __builtin_shufflevector(short_chars, (const v_16_u8_a1){}, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16, 10, 16, 16);
                    *(v_16_u8_a1*)&long_chars[29] = __builtin_shufflevector(short_chars, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);
               }
          }

          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               *result__own = arg;
               return *result__own;
          }

          goto long_string_label;
     }
     case tid__short_byte_array: {
          u8 const array_len = short_byte_array__get_len(arg);
          if (array_len < 8) {
               arg.vector    = arg.vector.s0011223344556677 >> (const v_any_u8)(const v_2_u64_adef)0x0004'0004'0004'0004ull & 0xf;
               arg.vector   += (u8)'0';
               arg.vector   += (arg.vector > (u8)'9') & (u8)('a' - ':');
               arg.raw_bits &= ((u128)1 << array_len * 16) - 1;
               goto short_string_label;
          }

          t_any new_string;
          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               new_string = long_string__new(array_len * 2);
               slice_array__set_len(new_string, array_len * 2);
               new_string = slice__set_metadata(new_string, 0, array_len * 2);
          } else {
               new_string.qwords[0] = (u64)buffer;
               new_string.bytes[15] = tid__string;
               new_string           = slice__set_metadata(new_string, 0, array_len * 2);
#ifndef NDEBUG
               set_ref_cnt(new_string, 1);
#endif
               slice_array__set_len(new_string, array_len * 2);
               slice_array__set_cap(new_string, array_len * 2);
               set_ref_cnt(new_string, 0);
          }

          u8* const long_chars = slice_array__get_items(new_string);

          memset_inline(&arg.bytes[14], 0, 2);
          v_32_u8_adef chars_vec = arg.vector.s00112233445566778899aabbccddeeff >> (const v_32_u8_adef)(const v_4_u64_adef)0x0004'0004'0004'0004ull & 0xf;
          chars_vec               += (u8)'0';
          chars_vec               += (chars_vec > (u8)'9') & (u8)('a' - ':');

          v_16_u8_adef const low_part  = chars_vec.s0123456789abcdef;
          v_16_u8_a1   const high_part = *(const v_16_u8_a1*)&((const u8*)&chars_vec)[array_len * 2 - 16];

          *(v_32_u8_a1*)long_chars      = __builtin_shufflevector(low_part, (const v_16_u8_adef){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
          *(v_16_u8_a1*)&long_chars[32] = __builtin_shufflevector(low_part, (const v_16_u8_adef){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

          *(v_32_u8_a1*)&long_chars[array_len * 6 - 48] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
          *(v_16_u8_a1*)&long_chars[array_len * 6 - 16] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               *result__own = new_string;
               return *result__own;
          }

          arg = new_string;
          goto long_string_label;
     }
     case tid__time: {
          u64 year;
          u8  month;
          u8  day;
          u8  hour;
          u8  minute;
          u8  second;
          time__disasm(arg, -timezone, &year, &month, &day, &hour, &minute, &second);

          memset_inline(chars, 0, 64);
          int const len = snprintf(chars, 64, "%02"PRIu8".%02"PRIu8".%04"PRIi64" %02"PRIu8":%02"PRIu8":%02"PRIu8" %+li.%02i", day, month, year, hour, minute, second, (-timezone) / 3600, (int)((timezone < 0 ? -timezone:timezone) % 3600 / 36));

          assert(len >= 24 && len <= 64);

          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               arg = long_string__new(len);
               slice_array__set_len(arg, len);
               arg = slice__set_metadata(arg, 0, len);
          } else {
               arg.qwords[0] = (u64)buffer;
               arg.bytes[15] = tid__string;
               arg           = slice__set_metadata(arg, 0, len);
               #ifndef NDEBUG
               set_ref_cnt(arg, 1);
               #endif
               slice_array__set_len(arg, len);
               slice_array__set_cap(arg, len);
               set_ref_cnt(arg, 0);
          }

          u8* const long_chars = slice_array__get_items(arg);
          if (len > 32) {
               v_32_u8_achars const low_part  = *(const v_32_u8_achars*)chars;
               v_32_u8_a1     const high_part = *(const v_32_u8_a1*)&chars[len - 32];

               *(v_64_u8_a1*)long_chars = __builtin_shufflevector(low_part, (const v_32_u8_achars){},
                    32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
                    32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
               );
               *(v_32_u8_a1*)&long_chars[64] = __builtin_shufflevector(low_part, (const v_32_u8_achars){},
                    32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
               );

               *(v_64_u8_a1*)&long_chars[len * 3 - 96] = __builtin_shufflevector(high_part, (const v_32_u8_a1){},
                    32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
                    32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
               );
               *(v_32_u8_a1*)&long_chars[len * 3 - 32] = __builtin_shufflevector(high_part, (const v_32_u8_a1){},
                    32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
               );
          } else {
               v_16_u8_achars const low_part  = *(const v_16_u8_a1*)chars;
               v_16_u8_a1     const high_part = *(const v_16_u8_a1*)&chars[len - 16];

               *(v_32_u8_a1*)long_chars      = __builtin_shufflevector(low_part, (const v_16_u8_achars){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
               *(v_16_u8_a1*)&long_chars[32] = __builtin_shufflevector(low_part, (const v_16_u8_achars){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

               *(v_32_u8_a1*)&long_chars[len * 3 - 48] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
               *(v_16_u8_a1*)&long_chars[len * 3 - 16] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);
          }

          goto long_string_label;
     }
     case tid__float: {
          double const float_ = arg.floats[0];
          if (__builtin_isnan(float_) || __builtin_isinf(float_)) {
               arg = __builtin_isnan(float_) ? (const t_any){.bytes = "nan"} : (float_ > 0.0 ? (const t_any){.bytes = "inf"} : (const t_any){.bytes = "-inf"});
               goto short_string_label;
          }

          int chars_len = snprintf(chars, 352, "%.15f", float_);

          assert(chars_len >= 17);

          int const trailing_zeros_len = 16 - __builtin_reduce_max((*(const v_16_u8_a1*)&chars[chars_len - 16] != '0') & (const v_16_u8_a1){2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});

          chars[chars_len - 16] = '.';
          chars_len            -= trailing_zeros_len;

          if (chars_len < 16) {
               memcpy_inline(arg.bytes, chars, 16);
               arg.raw_bits &= ((u128)1 << chars_len * 8) - 1;
               goto short_string_label;
          }

          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               arg = long_string__new(chars_len);
               slice_array__set_len(arg, chars_len);
               arg = slice__set_metadata(arg, 0, chars_len);
          } else {
               arg.qwords[0] = (u64)buffer;
               arg.bytes[15] = tid__string;
               arg           = slice__set_metadata(arg, 0, chars_len);
#ifndef NDEBUG
               set_ref_cnt(arg, 1);
#endif
               slice_array__set_len(arg, chars_len);
               slice_array__set_cap(arg, chars_len);
               set_ref_cnt(arg, 0);
          }

          u8* const long_chars = slice_array__get_items(arg);
          for (u32 char_idx = 0; chars_len - char_idx > 16; char_idx += 16) {
               v_16_u8_a16 const part_chars = *(const v_16_u8_a16*)&chars[char_idx];

               *(v_32_u8_a1*)&long_chars[char_idx * 3]      = __builtin_shufflevector(part_chars, (const v_16_u8_a16){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
               *(v_16_u8_a1*)&long_chars[char_idx * 3 + 32] = __builtin_shufflevector(part_chars, (const v_16_u8_a16){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);
          }

          v_16_u8_a1 const part_chars = *(const v_16_u8_a1*)&chars[chars_len - 16];

          *(v_32_u8_a1*)&long_chars[chars_len * 3 - 48] = __builtin_shufflevector(part_chars, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
          *(v_16_u8_a1*)&long_chars[chars_len * 3 - 16] = __builtin_shufflevector(part_chars, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               *result__own = arg;
               return *result__own;
          }

          goto long_string_label;
     }
     case tid__box: case tid__array: {
          u64          len;
          const t_any* items;
          bool const   is_array = arg.bytes[15] == tid__array;
          if (is_array) {
               u32 const slice_offset = slice__get_offset(arg);
               u32 const slice_len    = slice__get_len(arg);
               len                    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(arg);
               items                  = &((const t_any*)slice_array__get_items(arg))[slice_offset];
          } else {
               len   = box__get_len(arg);
               items = box__get_items(arg);
          }

          if (len == 0) {
               arg = is_array ? (const t_any){.bytes = "[array]"} : (const t_any){.bytes = "[box]"};
               goto short_string_label;
          }

          *result__own = to_string__half_own(thrd_data, result__own, is_array ? (const t_any){.bytes = "[array "} : (const t_any){.bytes = "[box "}, owner);

          for (u64 idx = 0; idx < len; idx += 1) {
               *result__own = to_string__half_own(thrd_data, result__own, items[idx], owner);
               if (result__own->bytes[15] == tid__error) [[clang::unlikely]] return *result__own;

               *result__own = to_string__half_own(thrd_data, result__own, idx + 1 == len ? (const t_any){.bytes = "]"} : (const t_any){.bytes = ", "}, owner);
          }

          return *result__own;
     }
     case tid__obj: {
          ref_cnt__inc(thrd_data, arg, owner);

          bool        called;
          t_any const call_result = obj__try_call__any_result__own(thrd_data, mtd__core_to_string, arg, &arg, 1, owner, &called);
          if (called) {
               arg = call_result;

               switch (arg.bytes[15]) {
               case tid__short_string: goto short_string_label;
               case tid__string:       goto long_string_label;

               [[clang::unlikely]]
               case tid__error:        goto error_label;
               [[clang::unlikely]]
               default: {
                    ref_cnt__dec(thrd_data, arg);
                    goto invalid_type_label;
               }}
          }

          ref_cnt__dec(thrd_data, arg);
     }
     case tid__table: {
          bool const is_table = arg.bytes[15] == tid__table;

          u64 remain_qty = (is_table ? hash_map__get_len(arg) : obj__get_fields_len(arg)) * 2;
          if (remain_qty == 0) {
               arg = is_table ? (const t_any){.bytes = "[table]"} : (const t_any){.bytes = "[obj]"};
               goto short_string_label;
          }

          *result__own = to_string__half_own(thrd_data, result__own, is_table ? (const t_any){.bytes = "[table "} : (const t_any){.bytes = "[obj "}, owner);

          const t_any* const items  = is_table ? table__get_kvs(arg) : obj__get_fields(arg);
          bool               is_key = true;
          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) {
                    idx += 1;
                    continue;
               }

               remain_qty -= 1;

               *result__own = to_string__half_own(thrd_data, result__own, item, owner);
               if (result__own->bytes[15] == tid__error) [[clang::unlikely]] return *result__own;

               t_any str;
               if (is_key)               str = (const t_any){.bytes = " = "};
               else if (remain_qty == 0) str = (const t_any){.bytes = "]"};
               else                      str = (const t_any){.bytes = ", "};

               is_key      = !is_key;

               *result__own = to_string__half_own(thrd_data, result__own, str, owner);
          }

          return *result__own;
     }
     case tid__set: {
          u64 remain_qty = hash_map__get_len(arg);
          if (remain_qty == 0) {
               arg = (const t_any){.bytes = "[set]"};
               goto short_string_label;
          }

          *result__own = to_string__half_own(thrd_data, result__own, (const t_any){.bytes = "[set "}, owner);

          const t_any* const items = set__get_items(arg);
          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               *result__own = to_string__half_own(thrd_data, result__own, item, owner);
               if (result__own->bytes[15] == tid__error) [[clang::unlikely]] return *result__own;

               *result__own = to_string__half_own(thrd_data, result__own, remain_qty == 0 ? (const t_any){.bytes = "]"} : (const t_any){.bytes = ", "}, owner);
          }

          return *result__own;
     }
     case tid__byte_array: {
          u32       const slice_offset = slice__get_offset(arg);
          u32       const slice_len    = slice__get_len(arg);
          u64       const bytes_len    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(arg);
          const u8* const bytes        = &((const u8*)slice_array__get_items(arg))[slice_offset];

          if (result__own->bytes[0] == 0 && result__own->bytes[15] == tid__short_string) {
               u64 const string_len = bytes_len * 2;
               arg                  = long_string__new(string_len);
               slice_array__set_len(arg, string_len);
               arg                  = slice__set_metadata(arg, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);

               u8* const long_chars = slice_array__get_items(arg);
               for (u64 byte_idx = 0; bytes_len - byte_idx >= 16; byte_idx += 16) {
                    v_16_u8_a1   const part_bytes = *(const v_16_u8_a1*)&bytes[byte_idx];
                    v_32_u8_adef       part_chars = part_bytes.s00112233445566778899aabbccddeeff;
                    part_chars                  >>= (const v_32_u8_adef)(const v_8_u32_adef)0x0004'0004ul;
                    part_chars                   &= 0xf;
                    part_chars                   += (u8)'0';
                    part_chars                   += (part_chars > (u8)'9') & (u8)('a' - ':');

                    *(v_64_u8_a1*)&long_chars[byte_idx * 6] = __builtin_shufflevector(part_chars, (const v_32_u8_adef){},
                         32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
                         32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
                    );
                    *(v_32_u8_a1*)&long_chars[byte_idx * 6 + 64] = __builtin_shufflevector(part_chars, (const v_32_u8_adef){},
                         32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
                    );
               }

               v_16_u8_a1   const part_bytes = *(const v_16_u8_a1*)&bytes[(i64)(bytes_len - 16)];
               v_32_u8_adef       part_chars = part_bytes.s00112233445566778899aabbccddeeff;
               part_chars                  >>= (const v_32_u8_adef)(const v_8_u32_adef)0x0004'0004ul;
               part_chars                   &= 0xf;
               part_chars                   += (u8)'0';
               part_chars                   += (part_chars > (u8)'9') & (u8)('a' - ':');

               *(v_64_u8_a1*)&long_chars[(i64)(bytes_len * 6 - 96)] = __builtin_shufflevector(part_chars, (const v_32_u8_adef){},
                    32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
                    32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
               );
               *(v_32_u8_a1*)&long_chars[bytes_len * 6 - 32] = __builtin_shufflevector(part_chars, (const v_32_u8_adef){},
                    32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
               );

               if (bytes_len == 15)
                    slice_array__set_cap(arg, 30);

               *result__own = arg;
               return *result__own;
          }

          arg.qwords[0] = (u64)buffer;
          arg.bytes[15] = tid__string;
          arg           = slice__set_metadata(arg, 0, 352);
          #ifndef NDEBUG
          set_ref_cnt(arg, 1);
          #endif
          slice_array__set_len(arg, 352);
          slice_array__set_cap(arg, 352);
          set_ref_cnt(arg, 0);

          u32 chars_idx = 16;
          u64 byte_idx  = 0;
          for (; bytes_len - byte_idx >= 16; byte_idx += 16) {
               v_16_u8_a1   const part_bytes = *(const v_16_u8_a1*)&bytes[byte_idx];
               v_32_u8_adef       part_chars = part_bytes.s00112233445566778899aabbccddeeff;
               part_chars                  >>= (const v_32_u8_adef)(const v_8_u32_adef)0x0004'0004ul;
               part_chars                   &= 0xf;
               part_chars                   += (u8)'0';
               part_chars                   += (part_chars > (u8)'9') & (u8)('a' - ':');

               *(v_64_u8_a1*)&buffer[chars_idx] = __builtin_shufflevector(part_chars, (const v_32_u8_adef){},
                    32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
                    32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
               );
               *(v_32_u8_a1*)&buffer[chars_idx + 64] = __builtin_shufflevector(part_chars, (const v_32_u8_adef){},
                    32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
               );

               chars_idx += 96;
               if (chars_idx == 1072) {
                    chars_idx = 16;
                    *result__own = to_string__half_own(thrd_data, result__own, arg, owner);
               }
          }

          v_16_u8_a1   const part_bytes      = *(const v_16_u8_a1*)&bytes[(i64)(bytes_len - 16)];
          v_32_u8_adef       part_chars      = part_bytes.s00112233445566778899aabbccddeeff;
          part_chars                       >>= (const v_32_u8_adef)(const v_8_u32_adef)0x0004'0004ul;
          part_chars                        &= 0xf;
          part_chars                        += (u8)'0';
          part_chars                        += (part_chars > (u8)'9') & (u8)('a' - ':');
          u8           const last_bytes_len  = bytes_len - byte_idx;
          u32          const last_string_len = bytes_len == 15 ? 30 : (chars_idx - 16) / 3 + last_bytes_len * 2;
          chars_idx                          = bytes_len == 15 ? 10 : (chars_idx == 16 ? 16 : chars_idx - 96 + last_bytes_len * 6);

          *(v_64_u8_a1*)&buffer[chars_idx] = __builtin_shufflevector(part_chars, (const v_32_u8_adef){},
               32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
               32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
          );
          *(v_32_u8_a1*)&buffer[chars_idx + 64] = __builtin_shufflevector(part_chars, (const v_32_u8_adef){},
               32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
          );

          arg = slice__set_metadata(arg, 0, last_string_len);
          #ifndef NDEBUG
          set_ref_cnt(arg, 1);
          #endif
          slice_array__set_len(arg, last_string_len);
          slice_array__set_cap(arg, last_string_len);
          set_ref_cnt(arg, 0);

          goto long_string_label;
     }
     case tid__string:
          ref_cnt__inc(thrd_data, arg, owner);
          long_string_label:
          *result__own = result__own->bytes[15] == tid__short_string ? concat__short_str__long_str__own(thrd_data, *result__own, arg, owner) : concat__long_str__long_str__own(thrd_data, *result__own, arg, owner);
          return *result__own;
     case tid__custom:
          ref_cnt__inc(thrd_data, arg, owner);

          arg = custom__call__any_result__own(thrd_data, mtd__core_to_string, arg, &arg, 1, owner);
          switch (arg.bytes[15]) {
          case tid__short_string: goto short_string_label;
          case tid__string:       goto long_string_label;

          [[clang::unlikely]]
          case tid__error:        goto error_label;
          [[clang::unlikely]]
          default:{
               ref_cnt__dec(thrd_data, arg);
               goto invalid_type_label;
          }}

     [[clang::unlikely]]
     case tid__error:
          ref_cnt__inc(thrd_data, arg, owner);
          goto error_label;
     [[clang::unlikely]]
     default:
          goto invalid_type_label;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, *result__own);
     return error__invalid_type(thrd_data, owner);

     error_label:
     ref_cnt__dec(thrd_data, *result__own);
     return arg;
}

[[gnu::hot]]
core t_any McoreFNto_string(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.to_string";

     t_any const arg    = args[0];
     t_any       result = {};

     call_stack__push(thrd_data, owner);
     result = to_string__half_own(thrd_data, &result, arg, owner);
     call_stack__pop(thrd_data);

     ref_cnt__dec(thrd_data, arg);
     return result;
}