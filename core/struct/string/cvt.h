#pragma once

#include "../../common/bool_vectors.h"
#include "../../common/corsar.h"
#include "../../common/const.h"
#include "../../common/fn.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/type.h"

enum : u64 {
     str_cvt_err__encoding    = (u64)-1,
     str_cvt_err__recoding    = (u64)-2,
     str_cvt_err__sys_escape  = (u64)-3,
     str_cvt_sys_is_utf8      = (u64)-4,
};

struct {
     u64 src_offset;
     u64 dst_offset;
} typedef t_str_cvt_result;

core_string t_str_cvt_result corsar_chars_to_ctf8_chars(u64 const corsar_chars_size, const u8* const corsar_chars, u64 const ctf8_chars_cap_size, u8* const ctf8_chars) {
     assert(corsar_chars_size % 3 == 0);

     typedef u8  v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8  v_32_u8    __attribute__((ext_vector_type(32)));
     typedef u32 v_4_u32    __attribute__((ext_vector_type(4)));

     u64 corsar_offset = 0;
     u64 ctf8_offset   = 0;
     while (corsar_chars_size - corsar_offset > 64 && ctf8_chars_cap_size - ctf8_offset >= 21) {
          u64 const max_corsar_offset = corsar_offset + 63;

          v_64_u8_a1 const corsar_vec = *(const v_64_u8_a1*)&corsar_chars[corsar_offset];

          u64 zero_mask  = v_64_bool_to_u64(__builtin_convertvector(corsar_vec == (u8)0, v_64_bool));
          u64 ascii_mask = v_64_bool_to_u64(__builtin_convertvector(corsar_vec <= (const v_64_u8_a1){
               0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127,
               0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0
          }, v_64_bool));

          zero_mask &= zero_mask >> 1;
          zero_mask &= zero_mask >> 1;
          zero_mask &= 0x1249'2492'4924'9249ull;

          ascii_mask &= ascii_mask >> 1;
          ascii_mask &= ascii_mask >> 1;
          ascii_mask &= 0x1249'2492'4924'9249ull;

          u64 const not_ascii_mask   = ascii_mask ^ zero_mask ^ 0x9249'2492'4924'9249ull;
          u64 const not_ascii_offset = __builtin_ctzll(not_ascii_mask);
          if (not_ascii_offset >= 42) {
               v_32_u8 const low_codes = __builtin_shufflevector(corsar_vec, (const v_64_u8_a1){},
                    2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62,
                    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
               );

               u8 const copy_len = not_ascii_offset / 3;
               memcpy_le_32(&ctf8_chars[ctf8_offset], &low_codes, copy_len);

               corsar_offset += not_ascii_offset;
               ctf8_offset   += copy_len;
          }

          while (corsar_offset < max_corsar_offset) {
               u32 corsar_code;
               memcpy_inline(&corsar_code, &corsar_chars[corsar_offset], 3);
               corsar_code = __builtin_bswap32(corsar_code) >> 8;

               if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u8  const ctf8_char_size  = char_size_in_ctf8(corsar_code);
               u64 const new_ctf8_offset = ctf8_offset + ctf8_char_size;
               if (new_ctf8_offset > ctf8_chars_cap_size)
                    return (const t_str_cvt_result){.src_offset = corsar_offset, .dst_offset = ctf8_offset};

               switch (ctf8_char_size) {
               case 1:
                    ctf8_chars[ctf8_offset] = corsar_code;
                    break;
               case 2:
                    ctf8_chars[ctf8_offset]     = corsar_code >> 6 | 0x80;
                    ctf8_chars[ctf8_offset + 1] = corsar_code & 0x3f | 0xc0;
                    break;
               case 3:
                    ctf8_chars[ctf8_offset]     = corsar_code >> 12 | 0xa0;
                    ctf8_chars[ctf8_offset + 1] = corsar_code >> 6 & 0x3f | 0xc0;
                    ctf8_chars[ctf8_offset + 2] = corsar_code & 0x3f | 0xc0;
                    break;
               case 4:
                    *(ua_u32*)&ctf8_chars[ctf8_offset] = __builtin_reduce_or(
                         (
                              (corsar_code >> (const v_4_u32){18, 12, 6, 0}) &
                              (const v_4_u32){0xf, 0x3f, 0x3f, 0x3f}         |
                              (const v_4_u32){0xb0, 0xc0, 0xc0, 0xc0}
                         ) << (const v_4_u32){0, 8, 16, 24}
                    );
                    break;
               default:
                    unreachable;
               }

               corsar_offset += 3;
               ctf8_offset    = new_ctf8_offset;
          }
     }

     while (corsar_offset < corsar_chars_size && ctf8_offset < ctf8_chars_cap_size) {
          u32 corsar_code;
          memcpy_inline(&corsar_code, &corsar_chars[corsar_offset], 3);
          corsar_code = __builtin_bswap32(corsar_code) >> 8;

          if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u8  const ctf8_char_size  = char_size_in_ctf8(corsar_code);
          u64 const new_ctf8_offset = ctf8_offset + ctf8_char_size;
          if (new_ctf8_offset > ctf8_chars_cap_size) break;

          switch (ctf8_char_size) {
          case 1:
               ctf8_chars[ctf8_offset] = corsar_code;
               break;
          case 2:
               ctf8_chars[ctf8_offset]     = corsar_code >> 6 | 0x80;
               ctf8_chars[ctf8_offset + 1] = corsar_code & 0x3f | 0xc0;
               break;
          case 3:
               ctf8_chars[ctf8_offset]     = corsar_code >> 12 | 0xa0;
               ctf8_chars[ctf8_offset + 1] = corsar_code >> 6 & 0x3f | 0xc0;
               ctf8_chars[ctf8_offset + 2] = corsar_code & 0x3f | 0xc0;
               break;
          case 4:
               *(ua_u32*)&ctf8_chars[ctf8_offset] = __builtin_reduce_or(
                    (
                         (corsar_code >> (const v_4_u32){18, 12, 6, 0}) &
                         (const v_4_u32){0xf, 0x3f, 0x3f, 0x3f}         |
                         (const v_4_u32){0xb0, 0xc0, 0xc0, 0xc0}
                    ) << (const v_4_u32){0, 8, 16, 24}
               );
               break;
          default:
               unreachable;
          }

          corsar_offset += 3;
          ctf8_offset    = new_ctf8_offset;
     }

     return (const t_str_cvt_result){.src_offset = corsar_offset, .dst_offset = ctf8_offset};
}

core_string t_str_cvt_result corsar_chars_to_sys_chars(u64 const corsar_chars_size, const u8* const corsar_chars, u64 const sys_chars_cap_size, u8* const sys_chars) {
     assert(corsar_chars_size % 3 == 0);

     typedef u8 v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8 v_32_u8    __attribute__((ext_vector_type(32)));

     mbstate_t mbstate = {};
     char      char_buffer[16];

     u64 corsar_offset = 0;
     u64 sys_offset    = 0;
     while (corsar_chars_size - corsar_offset > 64 && sys_chars_cap_size - sys_offset >= 21) {
          u64 const max_corsar_offset = corsar_offset + 63;

          v_64_u8_a1 const corsar_vec = *(const v_64_u8_a1*)&corsar_chars[corsar_offset];

          u64 zero_mask   = v_64_bool_to_u64(__builtin_convertvector(corsar_vec == (u8)0, v_64_bool));
          u64 escape_mask = v_64_bool_to_u64(__builtin_convertvector(corsar_vec == (const v_64_u8_a1){
               0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a,
               0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0, 0, 0x1a, 0
          }, v_64_bool));
          u64 ascii_mask = v_64_bool_to_u64(__builtin_convertvector(corsar_vec <= (const v_64_u8_a1){
               0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127,
               0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0
          }, v_64_bool));

          zero_mask &= zero_mask >> 1;
          zero_mask &= zero_mask >> 1;
          zero_mask &= 0x1249'2492'4924'9249ull;

          escape_mask &= escape_mask >> 1;
          escape_mask &= escape_mask >> 1;
          escape_mask &= 0x1249'2492'4924'9249ull;

          ascii_mask &= ascii_mask >> 1;
          ascii_mask &= ascii_mask >> 1;
          ascii_mask &= 0x1249'2492'4924'9249ull;

          u64 const not_ascii_mask   = ascii_mask ^ zero_mask ^ escape_mask ^ 0x9249'2492'4924'9249ull;
          u64 const not_ascii_offset = __builtin_ctzll(not_ascii_mask);
          if (not_ascii_offset >= 42) {
               v_32_u8 const low_codes = __builtin_shufflevector(corsar_vec, (const v_64_u8_a1){},
                    2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62,
                    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
               );

               u8 const copy_len = not_ascii_offset / 3;
               memcpy_le_32(&sys_chars[sys_offset], &low_codes, copy_len);

               corsar_offset += not_ascii_offset;
               sys_offset    += copy_len;
          }

          while (corsar_offset < max_corsar_offset) {
               u64 new_corsar_offset = corsar_offset + 3;

               u32 corsar_code;
               memcpy_inline(&corsar_code, &corsar_chars[corsar_offset], 3);
               corsar_code = __builtin_bswap32(corsar_code) >> 8;

               if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u32 unicode_char;
               if (corsar_code == 0x1a) {
                    if (corsar_offset + 6 > corsar_chars_size)
                         return (const t_str_cvt_result){.src_offset = corsar_offset, .dst_offset = sys_offset};

                    memcpy_inline(&corsar_code, &corsar_chars[new_corsar_offset], 3);
                    corsar_code = __builtin_bswap32(corsar_code) >> 8;
                    if (corsar_code == 0x1a) {
                         new_corsar_offset += 3;
                         unicode_char       = 0x1a;
                    } else {
                         if (corsar_offset + 21 > corsar_chars_size)
                              return (const t_str_cvt_result){.src_offset = corsar_offset, .dst_offset = sys_offset};

                         unicode_char = 0;
                         for (u8 digit_idx = 0; digit_idx < 6; digit_idx += 1) {
                              memcpy_inline(&corsar_code, &corsar_chars[corsar_offset + (digit_idx + 1) * 3], 3);
                              corsar_code = __builtin_bswap32(corsar_code) >> 8;

                              if (!((corsar_code >= '0' && corsar_code <= '9') || (corsar_code >= 'a' && corsar_code <= 'f') || (corsar_code >= 'A' && corsar_code <= 'F'))) [[clang::unlikely]]
                                   return (const t_str_cvt_result){.src_offset = str_cvt_err__sys_escape};

                              unicode_char |= (u32)(corsar_code <= '9' ? corsar_code - '0' : corsar_code - (corsar_code >= 'a' ? 'a' : 'A') + 10) << (5 - digit_idx) * 4;
                         }

                         if (unicode_char > 1'114'111ul || unicode_char <= 128) [[clang::unlikely]]
                              return (const t_str_cvt_result){.src_offset = str_cvt_err__sys_escape};

                         new_corsar_offset += 18;
                    }
               } else unicode_char = corsar_code_to_unicode(corsar_code);

               size_t sys_char_size = c32rtomb(char_buffer, unicode_char, &mbstate);
               if ((ssize_t)sys_char_size < 1) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

               u64 const new_sys_offset = sys_offset + sys_char_size;
               if (new_sys_offset > sys_chars_cap_size)
                    return (const t_str_cvt_result){.src_offset = corsar_offset, .dst_offset = sys_offset};

               memcpy_le_16(&sys_chars[sys_offset], char_buffer, sys_char_size);

               corsar_offset = new_corsar_offset;
               sys_offset    = new_sys_offset;
          }
     }

     while (corsar_offset < corsar_chars_size && sys_offset < sys_chars_cap_size) {
          u64 new_corsar_offset = corsar_offset + 3;

          u32 corsar_code;
          memcpy_inline(&corsar_code, &corsar_chars[corsar_offset], 3);
          corsar_code = __builtin_bswap32(corsar_code) >> 8;

          if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u32 unicode_char;
          if (corsar_code == 0x1a) {
               if (corsar_offset + 6 > corsar_chars_size) break;

               memcpy_inline(&corsar_code, &corsar_chars[new_corsar_offset], 3);
               corsar_code = __builtin_bswap32(corsar_code) >> 8;
               if (corsar_code == 0x1a) {
                    new_corsar_offset += 3;
                    unicode_char       = 0x1a;
               } else {
                    if (corsar_offset + 21 > corsar_chars_size) break;

                    unicode_char = 0;
                    for (u8 digit_idx = 0; digit_idx < 6; digit_idx += 1) {
                         memcpy_inline(&corsar_code, &corsar_chars[corsar_offset + (digit_idx + 1) * 3], 3);
                         corsar_code = __builtin_bswap32(corsar_code) >> 8;

                         if (!((corsar_code >= '0' && corsar_code <= '9') || (corsar_code >= 'a' && corsar_code <= 'f') || (corsar_code >= 'A' && corsar_code <= 'F'))) [[clang::unlikely]]
                              return (const t_str_cvt_result){.src_offset = str_cvt_err__sys_escape};

                         unicode_char |= (u32)(corsar_code <= '9' ? corsar_code - '0' : corsar_code - (corsar_code >= 'a' ? 'a' : 'A') + 10) << (5 - digit_idx) * 4;
                    }

                    if (unicode_char > 1'114'111ul || unicode_char <= 128) [[clang::unlikely]]
                         return (const t_str_cvt_result){.src_offset = str_cvt_err__sys_escape};

                    new_corsar_offset += 18;
               }
          } else unicode_char = corsar_code_to_unicode(corsar_code);

          size_t sys_char_size = c32rtomb(char_buffer, unicode_char, &mbstate);
          if ((ssize_t)sys_char_size < 1) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

          u64 const new_sys_offset = sys_offset + sys_char_size;
          if (new_sys_offset > sys_chars_cap_size) break;

          memcpy_le_16(&sys_chars[sys_offset], char_buffer, sys_char_size);

          corsar_offset = new_corsar_offset;
          sys_offset    = new_sys_offset;
     }

     return (const t_str_cvt_result){.src_offset = corsar_offset, .dst_offset = sys_offset};
}

core_string t_str_cvt_result corsar_chars_to_utf8_chars(u64 const corsar_chars_size, const u8* const corsar_chars, u64 const utf8_chars_cap_size, u8* const utf8_chars) {
     assert(corsar_chars_size % 3 == 0);

     typedef u8  v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8  v_32_u8    __attribute__((ext_vector_type(32)));
     typedef u32 v_4_u32    __attribute__((ext_vector_type(4)));

     u64 corsar_offset = 0;
     u64 utf8_offset   = 0;
     while (corsar_chars_size - corsar_offset > 64 && utf8_chars_cap_size - utf8_offset >= 21) {
          u64 const max_corsar_offset = corsar_offset + 63;

          v_64_u8_a1 const corsar_vec = *(const v_64_u8_a1*)&corsar_chars[corsar_offset];

          u64 zero_mask  = v_64_bool_to_u64(__builtin_convertvector(corsar_vec == (u8)0, v_64_bool));
          u64 ascii_mask = v_64_bool_to_u64(__builtin_convertvector(corsar_vec <= (const v_64_u8_a1){
               0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127,
               0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0, 0, 127, 0
          }, v_64_bool));

          zero_mask &= zero_mask >> 1;
          zero_mask &= zero_mask >> 1;
          zero_mask &= 0x1249'2492'4924'9249ull;

          ascii_mask &= ascii_mask >> 1;
          ascii_mask &= ascii_mask >> 1;
          ascii_mask &= 0x1249'2492'4924'9249ull;

          u64 const not_ascii_mask   = ascii_mask ^ zero_mask ^ 0x9249'2492'4924'9249ull;
          u64 const not_ascii_offset = __builtin_ctzll(not_ascii_mask);
          if (not_ascii_offset >= 42) {
               v_32_u8 const low_codes = __builtin_shufflevector(corsar_vec, (const v_64_u8_a1){},
                    2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62,
                    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
               );

               u8 const copy_len = not_ascii_offset / 3;
               memcpy_le_32(&utf8_chars[utf8_offset], &low_codes, copy_len);

               corsar_offset += not_ascii_offset;
               utf8_offset   += copy_len;
          }

          while (corsar_offset < max_corsar_offset) {
               u32 corsar_code;
               memcpy_inline(&corsar_code, &corsar_chars[corsar_offset], 3);
               corsar_code = __builtin_bswap32(corsar_code) >> 8;

               if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u32 const unicode_char = corsar_code_to_unicode(corsar_code);
               if (unicode_char == 0) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

               u8  const utf8_char_size  = -__builtin_reduce_add(unicode_char > (const v_4_u32){0, 0x7f, 0x7ff, 0xffff});
               u64 const new_utf8_offset = utf8_offset + utf8_char_size;
               if (new_utf8_offset > utf8_chars_cap_size)
                    return (const t_str_cvt_result){.src_offset = corsar_offset, .dst_offset = utf8_offset};

               switch (utf8_char_size) {
               case 1:
                    utf8_chars[utf8_offset] = unicode_char;
                    break;
               case 2:
                    utf8_chars[utf8_offset]     = unicode_char >> 6 | 0xc0;
                    utf8_chars[utf8_offset + 1] = unicode_char & 0x3f | 0x80;
                    break;
               case 3:
                    utf8_chars[utf8_offset]     = unicode_char >> 12 | 0xe0;
                    utf8_chars[utf8_offset + 1] = unicode_char >> 6 & 0x3f | 0x80;
                    utf8_chars[utf8_offset + 2] = unicode_char & 0x3f | 0x80;
                    break;
               case 4:
                    *(ua_u32*)&utf8_chars[utf8_offset] = __builtin_reduce_or(
                         (
                              (unicode_char >> (const v_4_u32){18, 12, 6, 0}) &
                              (const v_4_u32){7, 0x3f, 0x3f, 0x3f}            |
                              (const v_4_u32){0xf0, 0x80, 0x80, 0x80}
                         ) << (const v_4_u32){0, 8, 16, 24}
                    );
                    break;
               default:
                    unreachable;
               }

               corsar_offset += 3;
               utf8_offset    = new_utf8_offset;
          }
     }

     while (corsar_offset < corsar_chars_size && utf8_offset < utf8_chars_cap_size) {
          u32 corsar_code;
          memcpy_inline(&corsar_code, &corsar_chars[corsar_offset], 3);
          corsar_code = __builtin_bswap32(corsar_code) >> 8;

          if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u32 const unicode_char = corsar_code_to_unicode(corsar_code);
          if (unicode_char == 0) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

          u8  const utf8_char_size  = -__builtin_reduce_add(unicode_char > (const v_4_u32){0, 0x7f, 0x7ff, 0xffff});
          u64 const new_utf8_offset = utf8_offset + utf8_char_size;
          if (new_utf8_offset > utf8_chars_cap_size) break;

          switch (utf8_char_size) {
          case 1:
               utf8_chars[utf8_offset] = unicode_char;
               break;
          case 2:
               utf8_chars[utf8_offset]     = unicode_char >> 6 | 0xc0;
               utf8_chars[utf8_offset + 1] = unicode_char & 0x3f | 0x80;
               break;
          case 3:
               utf8_chars[utf8_offset]     = unicode_char >> 12 | 0xe0;
               utf8_chars[utf8_offset + 1] = unicode_char >> 6 & 0x3f | 0x80;
               utf8_chars[utf8_offset + 2] = unicode_char & 0x3f | 0x80;
               break;
          case 4:
               *(ua_u32*)&utf8_chars[utf8_offset] = __builtin_reduce_or(
                    (
                         (unicode_char >> (const v_4_u32){18, 12, 6, 0}) &
                         (const v_4_u32){7, 0x3f, 0x3f, 0x3f}            |
                         (const v_4_u32){0xf0, 0x80, 0x80, 0x80}
                    ) << (const v_4_u32){0, 8, 16, 24}
               );
               break;
          default:
               unreachable;
          }

          corsar_offset += 3;
          utf8_offset    = new_utf8_offset;
     }

     return (const t_str_cvt_result){.src_offset = corsar_offset, .dst_offset = utf8_offset};
}

core_string t_str_cvt_result ctf8_chars_to_corsar_chars(u64 const ctf8_chars_size, const u8* const ctf8_chars, u64 const corsar_chars_cap_size, u8* const corsar_chars) {
     assert(corsar_chars_cap_size % 3 == 0);

     typedef u8 v_16_u8_a1 __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8 v_32_u8    __attribute__((ext_vector_type(32)));
     typedef u8 v_16_u8    __attribute__((ext_vector_type(16)));
     typedef u8 v_4_u8     __attribute__((ext_vector_type(4)));

     u64 ctf8_offset   = 0;
     u64 corsar_offset = 0;
     while (ctf8_chars_size - ctf8_offset >= 16 && corsar_chars_cap_size - corsar_offset >= 48) {
          v_16_u8_a1 const ctf8_vec = *(const v_16_u8_a1*)&ctf8_chars[ctf8_offset];
          if (__builtin_reduce_and(ctf8_vec < (u8)128 & ctf8_vec != 0) != 0) {
               v_32_u8 const ascii_chars_low  = __builtin_shufflevector(ctf8_vec, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
               v_16_u8 const ascii_chars_high = __builtin_shufflevector(ctf8_vec, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

               memcpy_inline(&corsar_chars[corsar_offset], &ascii_chars_low, 32);
               memcpy_inline(&corsar_chars[corsar_offset + 32], &ascii_chars_high, 16);

               ctf8_offset   += 16;
               corsar_offset += 48;
          } else for (u64 const max_ctf8_offset = ctf8_offset + 16; ctf8_offset < max_ctf8_offset && corsar_offset < corsar_chars_cap_size;) {
               u32       corsar_code     = ctf8_chars[ctf8_offset];
               u8  const ctf8_char_size  = -__builtin_reduce_add((u8)corsar_code > (const v_4_u8){0, 0x7f, 0x9f, 0xaf});
               u64 const new_ctf8_offset = ctf8_offset + ctf8_char_size;
               if (new_ctf8_offset > ctf8_chars_size)
                    return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = corsar_offset};

               switch (ctf8_char_size) {
               case 0: case 1:
                    break;
               case 2: {
                    u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
                    corsar_code = byte_1 >= 0xc0 ? (corsar_code & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
                    break;
               }
               case 3: {
                    u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
                    u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
                    corsar_code = byte_1 >= 0xc0 && byte_2 >= 0xc0 ? (corsar_code & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
                    break;
               }
               case 4: {
                    u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
                    u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
                    u8 const byte_3 = ctf8_chars[ctf8_offset + 3];
                    corsar_code =
                         corsar_code < 0xc0 && byte_1 >= 0xc0 && byte_2 >= 0xc0 && byte_3 >= 0xc0
                              ? (corsar_code & 0xf) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                              : 0;
                    break;
               }
               default:
                    unreachable;
               }

               if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               corsar_code = __builtin_bswap32(corsar_code) >> 8;
               memcpy_inline(&corsar_chars[corsar_offset], &corsar_code, 3);

               ctf8_offset    = new_ctf8_offset;
               corsar_offset += 3;
          }
     }

     while (ctf8_offset < ctf8_chars_size && corsar_offset < corsar_chars_cap_size) {
          u32       corsar_code     = ctf8_chars[ctf8_offset];
          u8  const ctf8_char_size  = -__builtin_reduce_add((u8)corsar_code > (const v_4_u8){0, 0x7f, 0x9f, 0xaf});
          u64 const new_ctf8_offset = ctf8_offset + ctf8_char_size;
          if (new_ctf8_offset > ctf8_chars_size) break;

          switch (ctf8_char_size) {
          case 0: case 1:
               break;
          case 2: {
               u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
               corsar_code = byte_1 >= 0xc0 ? (corsar_code & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
               break;
          }
          case 3: {
               u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
               u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
               corsar_code = byte_1 >= 0xc0 && byte_2 >= 0xc0 ? (corsar_code & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
               break;
          }
          case 4: {
               u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
               u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
               u8 const byte_3 = ctf8_chars[ctf8_offset + 3];
               corsar_code =
                    corsar_code < 0xc0 && byte_1 >= 0xc0 && byte_2 >= 0xc0 && byte_3 >= 0xc0
                         ? (corsar_code & 0xf) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                         : 0;
               break;
          }
          default:
               unreachable;
          }

          if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          corsar_code = __builtin_bswap32(corsar_code) >> 8;
          memcpy_inline(&corsar_chars[corsar_offset], &corsar_code, 3);

          ctf8_offset    = new_ctf8_offset;
          corsar_offset += 3;
     }

     return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = corsar_offset};
}

core_string t_str_cvt_result ctf8_chars_to_sys_chars(u64 const ctf8_chars_size, const u8* const ctf8_chars, u64 const sys_chars_cap_size, u8* const sys_chars) {
     typedef u8 v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8 v_4_u8     __attribute__((ext_vector_type(4)));

     mbstate_t mbstate = {};
     char      char_buffer[16];

     u64 ctf8_offset = 0;
     u64 sys_offset  = 0;
     while (ctf8_chars_size - ctf8_offset >= 64 && sys_chars_cap_size - sys_offset >= 64) {
          u64 const max_ctf8_offset = ctf8_offset + 64;

          v_64_u8_a1 const ctf8_vec = *(const v_64_u8_a1*)&ctf8_chars[ctf8_offset];

          u64 const not_ascii_mask   = v_64_bool_to_u64(__builtin_convertvector((ctf8_vec >= (u8)128) | (ctf8_vec == (u8)0) | (ctf8_vec == (u8)0x1a), v_64_bool));
          u64 const not_ascii_offset = not_ascii_mask == 0 ? 64 : __builtin_ctzll(not_ascii_mask);
          memcpy_le_64(&sys_chars[sys_offset], &ctf8_chars[ctf8_offset], not_ascii_offset);

          ctf8_offset += not_ascii_offset;
          sys_offset  += not_ascii_offset;

          while (ctf8_offset < max_ctf8_offset) {
               u32       corsar_code     = ctf8_chars[ctf8_offset];
               u8  const ctf8_char_size  = -__builtin_reduce_add((u8)corsar_code > (const v_4_u8){0, 0x7f, 0x9f, 0xaf});
               u64       new_ctf8_offset = ctf8_offset + ctf8_char_size;
               if (new_ctf8_offset > ctf8_chars_size)
                    return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = sys_offset};

               switch (ctf8_char_size) {
               case 0: case 1:
                    break;
               case 2: {
                    u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
                    corsar_code = byte_1 >= 0xc0 ? (corsar_code & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
                    break;
               }
               case 3: {
                    u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
                    u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
                    corsar_code = byte_1 >= 0xc0 && byte_2 >= 0xc0 ? (corsar_code & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
                    break;
               }
               case 4: {
                    u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
                    u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
                    u8 const byte_3 = ctf8_chars[ctf8_offset + 3];
                    corsar_code =
                    corsar_code < 0xc0 && byte_1 >= 0xc0 && byte_2 >= 0xc0 && byte_3 >= 0xc0
                    ? (corsar_code & 0xf) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                    : 0;
                    break;
               }
               default:
                    unreachable;
               }

               if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u32 unicode_char;
               if (corsar_code == 0x1a) {
                    if (ctf8_offset + 1 == ctf8_chars_size)
                         return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = sys_offset};

                    if (ctf8_chars[ctf8_offset + 1] == 0x1a) {
                         new_ctf8_offset += 1;
                         unicode_char     = 0x1a;
                    } else {
                         if (ctf8_offset + 6 >= ctf8_chars_size)
                              return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = sys_offset};

                         unicode_char = 0;
                         for (u8 digit_idx = 0; digit_idx < 6; digit_idx += 1) {
                              u8 const digit = ctf8_chars[ctf8_offset + digit_idx + 1];
                              if (!((digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f') || (digit >= 'A' && digit <= 'F'))) [[clang::unlikely]]
                                   return (const t_str_cvt_result){.src_offset = str_cvt_err__sys_escape};

                              unicode_char |= (u32)(digit <= '9' ? digit - '0' : digit - (digit >= 'a' ? 'a' : 'A') + 10) << (5 - digit_idx) * 4;
                         }

                         if (unicode_char > 1'114'111ul || unicode_char <= 128) [[clang::unlikely]]
                              return (const t_str_cvt_result){.src_offset = str_cvt_err__sys_escape};

                         new_ctf8_offset += 6;
                    }
               } else unicode_char = corsar_code_to_unicode(corsar_code);

               size_t sys_char_size = c32rtomb(char_buffer, unicode_char, &mbstate);
               if ((ssize_t)sys_char_size < 1) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

               u64 const new_sys_offset = sys_offset + sys_char_size;
               if (new_sys_offset > sys_chars_cap_size)
                    return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = sys_offset};

               memcpy_le_16(&sys_chars[sys_offset], char_buffer, sys_char_size);

               ctf8_offset = new_ctf8_offset;
               sys_offset  = new_sys_offset;
          }
     }

     while (ctf8_offset < ctf8_chars_size && sys_offset < sys_chars_cap_size) {
          u32       corsar_code     = ctf8_chars[ctf8_offset];
          u8  const ctf8_char_size  = -__builtin_reduce_add((u8)corsar_code > (const v_4_u8){0, 0x7f, 0x9f, 0xaf});
          u64       new_ctf8_offset = ctf8_offset + ctf8_char_size;
          if (new_ctf8_offset > ctf8_chars_size) break;

          switch (ctf8_char_size) {
          case 0: case 1:
               break;
          case 2: {
               u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
               corsar_code = byte_1 >= 0xc0 ? (corsar_code & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
               break;
          }
          case 3: {
               u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
               u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
               corsar_code = byte_1 >= 0xc0 && byte_2 >= 0xc0 ? (corsar_code & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
               break;
          }
          case 4: {
               u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
               u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
               u8 const byte_3 = ctf8_chars[ctf8_offset + 3];
               corsar_code =
               corsar_code < 0xc0 && byte_1 >= 0xc0 && byte_2 >= 0xc0 && byte_3 >= 0xc0
               ? (corsar_code & 0xf) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
               : 0;
               break;
          }
          default:
               unreachable;
          }

          if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u32 unicode_char;
          if (corsar_code == 0x1a) {
               if (ctf8_offset + 1 == ctf8_chars_size) break;

               if (ctf8_chars[ctf8_offset + 1] == 0x1a) {
                    new_ctf8_offset += 1;
                    unicode_char     = 0x1a;
               } else {
                    if (ctf8_offset + 6 >= ctf8_chars_size) break;

                    unicode_char = 0;
                    for (u8 digit_idx = 0; digit_idx < 6; digit_idx += 1) {
                         u8 const digit = ctf8_chars[ctf8_offset + digit_idx + 1];
                         if (!((digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f') || (digit >= 'A' && digit <= 'F'))) [[clang::unlikely]]
                              return (const t_str_cvt_result){.src_offset = str_cvt_err__sys_escape};

                         unicode_char |= (u32)(digit <= '9' ? digit - '0' : digit - (digit >= 'a' ? 'a' : 'A') + 10) << (5 - digit_idx) * 4;
                    }

                    if (unicode_char > 1'114'111ul || unicode_char <= 128) [[clang::unlikely]]
                         return (const t_str_cvt_result){.src_offset = str_cvt_err__sys_escape};

                    new_ctf8_offset += 6;
               }
          } else unicode_char = corsar_code_to_unicode(corsar_code);

          size_t sys_char_size = c32rtomb(char_buffer, unicode_char, &mbstate);
          if ((ssize_t)sys_char_size < 1) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

          u64 const new_sys_offset = sys_offset + sys_char_size;
          if (new_sys_offset > sys_chars_cap_size) break;

          memcpy_le_16(&sys_chars[sys_offset], char_buffer, sys_char_size);

          ctf8_offset = new_ctf8_offset;
          sys_offset  = new_sys_offset;
     }

     return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = sys_offset};
}

core_string t_str_cvt_result ctf8_chars_to_utf8_chars(u64 const ctf8_chars_size, const u8* const ctf8_chars, u64 const utf8_chars_cap_size, u8* const utf8_chars) {
     typedef u8  v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8  v_4_u8     __attribute__((ext_vector_type(4)));
     typedef u32 v_4_u32    __attribute__((ext_vector_type(4)));

     u64 ctf8_offset = 0;
     u64 utf8_offset = 0;
     while (ctf8_chars_size - ctf8_offset >= 64 && utf8_chars_cap_size - utf8_offset >= 64) {
          u64 const max_ctf8_offset = ctf8_offset + 64;

          v_64_u8_a1 const ctf8_vec = *(const v_64_u8_a1*)&ctf8_chars[ctf8_offset];

          u64 const not_ascii_mask   = v_64_bool_to_u64(__builtin_convertvector((ctf8_vec >= (u8)128) | (ctf8_vec == (u8)0), v_64_bool));
          u64 const not_ascii_offset = not_ascii_mask == 0 ? 64 : __builtin_ctzll(not_ascii_mask);
          memcpy_le_64(&utf8_chars[utf8_offset], &ctf8_chars[ctf8_offset], not_ascii_offset);

          ctf8_offset += not_ascii_offset;
          utf8_offset += not_ascii_offset;

          while (ctf8_offset < max_ctf8_offset) {
               u32       corsar_code     = ctf8_chars[ctf8_offset];
               u8  const ctf8_char_size  = -__builtin_reduce_add((u8)corsar_code > (const v_4_u8){0, 0x7f, 0x9f, 0xaf});
               u64 const new_ctf8_offset = ctf8_offset + ctf8_char_size;
               if (new_ctf8_offset > ctf8_chars_size)
                    return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = utf8_offset};

               switch (ctf8_char_size) {
               case 0: case 1:
                    break;
               case 2: {
                    u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
                    corsar_code = byte_1 >= 0xc0 ? (corsar_code & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
                    break;
               }
               case 3: {
                    u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
                    u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
                    corsar_code = byte_1 >= 0xc0 && byte_2 >= 0xc0 ? (corsar_code & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
                    break;
               }
               case 4: {
                    u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
                    u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
                    u8 const byte_3 = ctf8_chars[ctf8_offset + 3];
                    corsar_code =
                    corsar_code < 0xc0 && byte_1 >= 0xc0 && byte_2 >= 0xc0 && byte_3 >= 0xc0
                    ? (corsar_code & 0xf) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                    : 0;
                    break;
               }
               default:
                    unreachable;
               }

               if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u32 const unicode_char = corsar_code_to_unicode(corsar_code);
               if (unicode_char == 0) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

               u8  const utf8_char_size  = -__builtin_reduce_add(unicode_char > (const v_4_u32){0, 0x7f, 0x7ff, 0xffff});
               u64 const new_utf8_offset = utf8_offset + utf8_char_size;
               if (new_utf8_offset > utf8_chars_cap_size)
                    return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = utf8_offset};

               switch (utf8_char_size) {
               case 1:
                    utf8_chars[utf8_offset] = unicode_char;
                    break;
               case 2:
                    utf8_chars[utf8_offset]     = unicode_char >> 6 | 0xc0;
                    utf8_chars[utf8_offset + 1] = unicode_char & 0x3f | 0x80;
                    break;
               case 3:
                    utf8_chars[utf8_offset]     = unicode_char >> 12 | 0xe0;
                    utf8_chars[utf8_offset + 1] = unicode_char >> 6 & 0x3f | 0x80;
                    utf8_chars[utf8_offset + 2] = unicode_char & 0x3f | 0x80;
                    break;
               case 4:
                    *(ua_u32*)&utf8_chars[utf8_offset] = __builtin_reduce_or(
                         (
                              (unicode_char >> (const v_4_u32){18, 12, 6, 0}) &
                              (const v_4_u32){7, 0x3f, 0x3f, 0x3f}            |
                              (const v_4_u32){0xf0, 0x80, 0x80, 0x80}
                         ) << (const v_4_u32){0, 8, 16, 24}
                    );
                    break;
               default:
                    unreachable;
               }

               ctf8_offset = new_ctf8_offset;
               utf8_offset = new_utf8_offset;
          }
     }

     while (ctf8_offset < ctf8_chars_size && utf8_offset < utf8_chars_cap_size) {
          u32       corsar_code     = ctf8_chars[ctf8_offset];
          u8  const ctf8_char_size  = -__builtin_reduce_add((u8)corsar_code > (const v_4_u8){0, 0x7f, 0x9f, 0xaf});
          u64 const new_ctf8_offset = ctf8_offset + ctf8_char_size;
          if (new_ctf8_offset > ctf8_chars_size) break;

          switch (ctf8_char_size) {
          case 0: case 1:
               break;
          case 2: {
               u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
               corsar_code = byte_1 >= 0xc0 ? (corsar_code & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
               break;
          }
          case 3: {
               u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
               u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
               corsar_code = byte_1 >= 0xc0 && byte_2 >= 0xc0 ? (corsar_code & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
               break;
          }
          case 4: {
               u8 const byte_1 = ctf8_chars[ctf8_offset + 1];
               u8 const byte_2 = ctf8_chars[ctf8_offset + 2];
               u8 const byte_3 = ctf8_chars[ctf8_offset + 3];
               corsar_code =
               corsar_code < 0xc0 && byte_1 >= 0xc0 && byte_2 >= 0xc0 && byte_3 >= 0xc0
               ? (corsar_code & 0xf) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
               : 0;
               break;
          }
          default:
               unreachable;
          }

          if (!corsar_code_is_correct(corsar_code)) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u32 const unicode_char = corsar_code_to_unicode(corsar_code);
          if (unicode_char == 0) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

          u8  const utf8_char_size  = -__builtin_reduce_add(unicode_char > (const v_4_u32){0, 0x7f, 0x7ff, 0xffff});
          u64 const new_utf8_offset = utf8_offset + utf8_char_size;
          if (new_utf8_offset > utf8_chars_cap_size) break;

          switch (utf8_char_size) {
          case 1:
               utf8_chars[utf8_offset] = unicode_char;
               break;
          case 2:
               utf8_chars[utf8_offset]     = unicode_char >> 6 | 0xc0;
               utf8_chars[utf8_offset + 1] = unicode_char & 0x3f | 0x80;
               break;
          case 3:
               utf8_chars[utf8_offset]     = unicode_char >> 12 | 0xe0;
               utf8_chars[utf8_offset + 1] = unicode_char >> 6 & 0x3f | 0x80;
               utf8_chars[utf8_offset + 2] = unicode_char & 0x3f | 0x80;
               break;
          case 4:
               *(ua_u32*)&utf8_chars[utf8_offset] = __builtin_reduce_or(
                    (
                         (unicode_char >> (const v_4_u32){18, 12, 6, 0}) &
                         (const v_4_u32){7, 0x3f, 0x3f, 0x3f}            |
                         (const v_4_u32){0xf0, 0x80, 0x80, 0x80}
                    ) << (const v_4_u32){0, 8, 16, 24}
               );
               break;
          default:
               unreachable;
          }

          ctf8_offset = new_ctf8_offset;
          utf8_offset = new_utf8_offset;
     }

     return (const t_str_cvt_result){.src_offset = ctf8_offset, .dst_offset = utf8_offset};
}

core_string t_str_cvt_result sys_chars_to_corsar_chars(u64 const sys_chars_size, const u8* const sys_chars, u64 const corsar_chars_cap_size, u8* const corsar_chars) {
     assert(corsar_chars_cap_size % 3 == 0);

     typedef u8 v_16_u8_a1 __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8 v_32_u8    __attribute__((ext_vector_type(32)));
     typedef u8 v_16_u8    __attribute__((ext_vector_type(16)));
     typedef u8 v_8_u8     __attribute__((ext_vector_type(8)));

     mbstate_t mbstate = {};

     u64 sys_offset    = 0;
     u64 corsar_offset = 0;
     while (sys_chars_size - sys_offset >= 16 && corsar_chars_cap_size - corsar_offset >= 48) {
          v_16_u8_a1 const sys_vec = *(const v_16_u8_a1*)&sys_chars[sys_offset];
          if (__builtin_reduce_and(sys_vec < (u8)128 & sys_vec != (u8)0 & sys_vec != (u8)0x1a) != 0) {
               v_32_u8 const ascii_chars_low  = __builtin_shufflevector(sys_vec, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
               v_16_u8 const ascii_chars_high = __builtin_shufflevector(sys_vec, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

               memcpy_inline(&corsar_chars[corsar_offset], &ascii_chars_low, 32);
               memcpy_inline(&corsar_chars[corsar_offset + 32], &ascii_chars_high, 16);

               sys_offset    += 16;
               corsar_offset += 48;
          } else for (u64 const max_sys_offset = sys_offset + 16; sys_offset < max_sys_offset && corsar_offset < corsar_chars_cap_size;) {
               u32 unicode_char;
               size_t const sys_char_size = mbrtoc32(&unicode_char, (const char*)&sys_chars[sys_offset], sys_chars_size, &mbstate);

               if ((ssize_t)sys_char_size < 1 && sys_char_size != (size_t)-2) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u64 const new_sys_offset = sys_offset + sys_char_size;
               if (new_sys_offset > sys_chars_size || sys_char_size == (size_t)-2)
                    return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = corsar_offset};

               assert(unicode_char <= 1'114'111ul);

               if (unicode_char == 0x1a) {
                    if (corsar_offset + 6 > corsar_chars_cap_size)
                         return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = corsar_offset};

                    corsar_chars[corsar_offset]     = 0;
                    corsar_chars[corsar_offset + 1] = 0;
                    corsar_chars[corsar_offset + 2] = 0x1a;
                    corsar_chars[corsar_offset + 3] = 0;
                    corsar_chars[corsar_offset + 4] = 0;
                    corsar_chars[corsar_offset + 5] = 0x1a;

                    sys_offset     = new_sys_offset;
                    corsar_offset += 6;
               } else {
                    if (unicode_char >= 128) {
                         if (corsar_offset + 21 > corsar_chars_cap_size)
                              return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = corsar_offset};

                         [[gnu::aligned(alignof(v_8_u8))]]
                         u64 const unicode_char_u64 = unicode_char;

                         v_8_u8 unicode_str = *(const v_8_u8*)&unicode_char_u64;
                         unicode_str        = (unicode_str.s22110033 >> (const v_8_u8){4, 0, 4, 0, 4, 0, 4, 0}) & (u8)15;
                         unicode_str        = ((unicode_str + (u8)'0') & (unicode_str < 10)) | ((unicode_str + (u8)('a' - 10)) & (unicode_str >= 10));

                         v_32_u8 const long_unicode_str = __builtin_shufflevector(unicode_str, (const v_8_u8){},
                              8, 8, 0, 8, 8, 1, 8, 8, 2, 8, 8, 3, 8, 8, 4, 8, 8, 5,
                              8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
                         );

                         corsar_chars[corsar_offset]     = 0;
                         corsar_chars[corsar_offset + 1] = 0;
                         corsar_chars[corsar_offset + 2] = 0x1a;
                         memcpy_inline(&corsar_chars[corsar_offset + 3], &long_unicode_str, 18);

                         sys_offset     = new_sys_offset;
                         corsar_offset += 21;
                    } else {
                         unicode_char = unicode_char << 16;
                         memcpy_inline(&corsar_chars[corsar_offset], &unicode_char, 3);

                         sys_offset     = new_sys_offset;
                         corsar_offset += 3;
                    }
               }
          }
     }

     while (sys_offset < sys_chars_size && corsar_offset < corsar_chars_cap_size) {
          u32 unicode_char;
          size_t const sys_char_size = mbrtoc32(&unicode_char, (const char*)&sys_chars[sys_offset], sys_chars_size, &mbstate);

          if ((ssize_t)sys_char_size < 1 && sys_char_size != (size_t)-2) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u64 const new_sys_offset = sys_offset + sys_char_size;
          if (new_sys_offset > sys_chars_size || sys_char_size == (size_t)-2) break;

          assert(unicode_char <= 1'114'111ul);

          if (unicode_char == 0x1a) {
               if (corsar_offset + 6 > corsar_chars_cap_size) break;

               corsar_chars[corsar_offset]     = 0;
               corsar_chars[corsar_offset + 1] = 0;
               corsar_chars[corsar_offset + 2] = 0x1a;
               corsar_chars[corsar_offset + 3] = 0;
               corsar_chars[corsar_offset + 4] = 0;
               corsar_chars[corsar_offset + 5] = 0x1a;

               sys_offset     = new_sys_offset;
               corsar_offset += 6;
          } else {
               if (unicode_char >= 128) {
                    if (corsar_offset + 21 > corsar_chars_cap_size) break;

                    [[gnu::aligned(alignof(v_8_u8))]]
                    u64 const unicode_char_u64 = unicode_char;

                    v_8_u8 unicode_str = *(const v_8_u8*)&unicode_char_u64;
                    unicode_str        = (unicode_str.s22110033 >> (const v_8_u8){4, 0, 4, 0, 4, 0, 4, 0}) & (u8)15;
                    unicode_str        = ((unicode_str + (u8)'0') & (unicode_str < 10)) | ((unicode_str + (u8)('a' - 10)) & (unicode_str >= 10));

                    v_32_u8 const long_unicode_str = __builtin_shufflevector(unicode_str, (const v_8_u8){},
                         8, 8, 0, 8, 8, 1, 8, 8, 2, 8, 8, 3, 8, 8, 4, 8, 8, 5,
                         8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
                    );

                    corsar_chars[corsar_offset]     = 0;
                    corsar_chars[corsar_offset + 1] = 0;
                    corsar_chars[corsar_offset + 2] = 0x1a;
                    memcpy_inline(&corsar_chars[corsar_offset + 3], &long_unicode_str, 18);

                    sys_offset     = new_sys_offset;
                    corsar_offset += 21;
               } else {
                    unicode_char = unicode_char << 16;
                    memcpy_inline(&corsar_chars[corsar_offset], &unicode_char, 3);

                    sys_offset     = new_sys_offset;
                    corsar_offset += 3;
               }
          }
     }

     return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = corsar_offset};
}

core_string t_str_cvt_result sys_chars_to_ctf8_chars(u64 const sys_chars_size, const u8* const sys_chars, u64 const ctf8_chars_cap_size, u8* const ctf8_chars) {
     typedef u8  v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8  v_8_u8     __attribute__((ext_vector_type(8)));

     mbstate_t mbstate = {};

     u64 sys_offset  = 0;
     u64 ctf8_offset = 0;
     while (sys_chars_size - sys_offset >= 64 && ctf8_chars_cap_size - ctf8_offset >= 64) {
          u64 const max_sys_offset = sys_offset + 64;

          v_64_u8_a1 const sys_vec = *(const v_64_u8_a1*)&sys_chars[sys_offset];

          u64 const not_ascii_mask   = v_64_bool_to_u64(__builtin_convertvector((sys_vec >= (u8)128) | (sys_vec == (u8)0) | (sys_vec == (u8)0x1a), v_64_bool));
          u64 const not_ascii_offset = not_ascii_mask == 0 ? 64 : __builtin_ctzll(not_ascii_mask);
          memcpy_le_64(&ctf8_chars[ctf8_offset], &sys_chars[sys_offset], not_ascii_offset);

          sys_offset  += not_ascii_offset;
          ctf8_offset += not_ascii_offset;

          while (sys_offset < max_sys_offset) {
               u32 unicode_char;
               size_t const sys_char_size = mbrtoc32(&unicode_char, (const char*)&sys_chars[sys_offset], sys_chars_size, &mbstate);

               if ((ssize_t)sys_char_size < 1 && sys_char_size != (size_t)-2) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u64 const new_sys_offset = sys_offset + sys_char_size;
               if (new_sys_offset > sys_chars_size || sys_char_size == (size_t)-2)
                    return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = ctf8_offset};

               assert(unicode_char <= 1'114'111ul);

               if (unicode_char == 0x1a) {
                    if (ctf8_offset + 1 == ctf8_chars_cap_size)
                         return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = ctf8_offset};

                    ctf8_chars[ctf8_offset]     = 0x1a;
                    ctf8_chars[ctf8_offset + 1] = 0x1a;

                    sys_offset   = new_sys_offset;
                    ctf8_offset += 2;
               } else {
                    if (unicode_char >= 128) {
                         if (ctf8_offset + 7 > ctf8_chars_cap_size)
                              return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = ctf8_offset};

                         [[gnu::aligned(alignof(v_8_u8))]]
                         u64 const unicode_char_u64 = unicode_char;

                         v_8_u8 unicode_str = *(const v_8_u8*)&unicode_char_u64;
                         unicode_str        = (unicode_str.s22110033 >> (const v_8_u8){4, 0, 4, 0, 4, 0, 4, 0}) & (u8)15;
                         unicode_str        = ((unicode_str + (u8)'0') & (unicode_str < 10)) | ((unicode_str + (u8)('a' - 10)) & (unicode_str >= 10));

                         ctf8_chars[ctf8_offset] = 0x1a;
                         memcpy_inline(&ctf8_chars[ctf8_offset + 1], &unicode_str, 6);

                         sys_offset   = new_sys_offset;
                         ctf8_offset += 7;
                    } else {
                         ctf8_chars[ctf8_offset] = unicode_char;

                         sys_offset   = new_sys_offset;
                         ctf8_offset += 1;
                    }
               }
          }
     }

     while (sys_offset < sys_chars_size && ctf8_offset < ctf8_chars_cap_size) {
          u32 unicode_char;
          size_t const sys_char_size = mbrtoc32(&unicode_char, (const char*)&sys_chars[sys_offset], sys_chars_size, &mbstate);

          if ((ssize_t)sys_char_size < 1 && sys_char_size != (size_t)-2) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u64 const new_sys_offset = sys_offset + sys_char_size;
          if (new_sys_offset > sys_chars_size || sys_char_size == (size_t)-2) break;

          assert(unicode_char <= 1'114'111ul);

          if (unicode_char == 0x1a) {
               if (ctf8_offset + 1 == ctf8_chars_cap_size) break;

               ctf8_chars[ctf8_offset]     = 0x1a;
               ctf8_chars[ctf8_offset + 1] = 0x1a;

               sys_offset   = new_sys_offset;
               ctf8_offset += 2;
          } else {
               if (unicode_char >= 128) {
                    if (ctf8_offset + 7 > ctf8_chars_cap_size) break;

                    [[gnu::aligned(alignof(v_8_u8))]]
                    u64 const unicode_char_u64 = unicode_char;

                    v_8_u8 unicode_str = *(const v_8_u8*)&unicode_char_u64;
                    unicode_str        = (unicode_str.s22110033 >> (const v_8_u8){4, 0, 4, 0, 4, 0, 4, 0}) & (u8)15;
                    unicode_str        = ((unicode_str + (u8)'0') & (unicode_str < 10)) | ((unicode_str + (u8)('a' - 10)) & (unicode_str >= 10));

                    ctf8_chars[ctf8_offset] = 0x1a;
                    memcpy_inline(&ctf8_chars[ctf8_offset + 1], &unicode_str, 6);

                    sys_offset   = new_sys_offset;
                    ctf8_offset += 7;
               } else {
                    ctf8_chars[ctf8_offset] = unicode_char;

                    sys_offset   = new_sys_offset;
                    ctf8_offset += 1;
               }
          }
     }

     return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = ctf8_offset};
}

core_string t_str_cvt_result sys_chars_to_utf8_chars(u64 const sys_chars_size, const u8* const sys_chars, u64 const utf8_chars_cap_size, u8* const utf8_chars) {
     typedef u8  v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u32 v_4_u32    __attribute__((ext_vector_type(4)));

     if (sys_enc_is_utf8)
          return (const t_str_cvt_result){.src_offset = str_cvt_sys_is_utf8};

     mbstate_t mbstate = {};

     u64 sys_offset  = 0;
     u64 utf8_offset = 0;
     while (sys_chars_size - sys_offset >= 64 && utf8_chars_cap_size - utf8_offset >= 64) {
          u64 const max_sys_offset = sys_offset + 64;

          v_64_u8_a1 const sys_vec = *(const v_64_u8_a1*)&sys_chars[sys_offset];

          u64 const not_ascii_mask   = v_64_bool_to_u64(__builtin_convertvector((sys_vec >= (u8)128) | (sys_vec == (u8)0), v_64_bool));
          u64 const not_ascii_offset = not_ascii_mask == 0 ? 64 : __builtin_ctzll(not_ascii_mask);
          memcpy_le_64(&utf8_chars[utf8_offset], &sys_chars[sys_offset], not_ascii_offset);

          sys_offset  += not_ascii_offset;
          utf8_offset += not_ascii_offset;

          while (sys_offset < max_sys_offset) {
               u32 unicode_char;
               size_t const sys_char_size = mbrtoc32(&unicode_char, (const char*)&sys_chars[sys_offset], sys_chars_size, &mbstate);

               if ((ssize_t)sys_char_size < 1 && sys_char_size != (size_t)-2) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u64 const new_sys_offset = sys_offset + sys_char_size;
               if (new_sys_offset > sys_chars_size || sys_char_size == (size_t)-2)
                    return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = utf8_offset};

               assert(unicode_char <= 1'114'111ul);

               u8  const utf8_char_size  = -__builtin_reduce_add(unicode_char > (const v_4_u32){0, 0x7f, 0x7ff, 0xffff});
               u64 const new_utf8_offset = utf8_offset + utf8_char_size;
               if (new_utf8_offset > utf8_chars_cap_size)
                    return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = utf8_offset};

               switch (utf8_char_size) {
               case 1:
                    utf8_chars[utf8_offset] = unicode_char;
                    break;
               case 2:
                    utf8_chars[utf8_offset]     = unicode_char >> 6 | 0xc0;
                    utf8_chars[utf8_offset + 1] = unicode_char & 0x3f | 0x80;
                    break;
               case 3:
                    utf8_chars[utf8_offset]     = unicode_char >> 12 | 0xe0;
                    utf8_chars[utf8_offset + 1] = unicode_char >> 6 & 0x3f | 0x80;
                    utf8_chars[utf8_offset + 2] = unicode_char & 0x3f | 0x80;
                    break;
               case 4:
                    *(ua_u32*)&utf8_chars[utf8_offset] = __builtin_reduce_or(
                         (
                              (unicode_char >> (const v_4_u32){18, 12, 6, 0}) &
                              (const v_4_u32){7, 0x3f, 0x3f, 0x3f}            |
                              (const v_4_u32){0xf0, 0x80, 0x80, 0x80}
                         ) << (const v_4_u32){0, 8, 16, 24}
                    );
                    break;
               default:
                    unreachable;
               }

               sys_offset  = new_sys_offset;
               utf8_offset = new_utf8_offset;
          }
     }

     while (sys_offset < sys_chars_size && utf8_offset < utf8_chars_cap_size) {
          u32 unicode_char;
          size_t const sys_char_size = mbrtoc32(&unicode_char, (const char*)&sys_chars[sys_offset], sys_chars_size, &mbstate);

          if ((ssize_t)sys_char_size < 1 && sys_char_size != (size_t)-2) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u64 const new_sys_offset = sys_offset + sys_char_size;
          if (new_sys_offset > sys_chars_size || sys_char_size == (size_t)-2) break;

          assert(unicode_char <= 1'114'111ul);

          u8  const utf8_char_size  = -__builtin_reduce_add(unicode_char > (const v_4_u32){0, 0x7f, 0x7ff, 0xffff});
          u64 const new_utf8_offset = utf8_offset + utf8_char_size;
          if (new_utf8_offset > utf8_chars_cap_size) break;

          switch (utf8_char_size) {
          case 1:
               utf8_chars[utf8_offset] = unicode_char;
               break;
          case 2:
               utf8_chars[utf8_offset]     = unicode_char >> 6 | 0xc0;
               utf8_chars[utf8_offset + 1] = unicode_char & 0x3f | 0x80;
               break;
          case 3:
               utf8_chars[utf8_offset]     = unicode_char >> 12 | 0xe0;
               utf8_chars[utf8_offset + 1] = unicode_char >> 6 & 0x3f | 0x80;
               utf8_chars[utf8_offset + 2] = unicode_char & 0x3f | 0x80;
               break;
          case 4:
               *(ua_u32*)&utf8_chars[utf8_offset] = __builtin_reduce_or(
                    (
                         (unicode_char >> (const v_4_u32){18, 12, 6, 0}) &
                         (const v_4_u32){7, 0x3f, 0x3f, 0x3f}            |
                         (const v_4_u32){0xf0, 0x80, 0x80, 0x80}
                    ) << (const v_4_u32){0, 8, 16, 24}
               );
               break;
          default:
               unreachable;
          }

          sys_offset  = new_sys_offset;
          utf8_offset = new_utf8_offset;
     }

     return (const t_str_cvt_result){.src_offset = sys_offset, .dst_offset = utf8_offset};
}

core_string t_str_cvt_result utf8_chars_to_corsar_chars(u64 const utf8_chars_size, const u8* const utf8_chars, u64 const corsar_chars_cap_size, u8* const corsar_chars, const t_char_codes* const lang_codes) {
     assert(corsar_chars_cap_size % 3 == 0);

     typedef u8 v_16_u8_a1 __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8 v_32_u8    __attribute__((ext_vector_type(32)));
     typedef u8 v_16_u8    __attribute__((ext_vector_type(16)));
     typedef u8 v_4_u8     __attribute__((ext_vector_type(4)));

     u64 utf8_offset   = 0;
     u64 corsar_offset = 0;
     while (utf8_chars_size - utf8_offset >= 16 && corsar_chars_cap_size - corsar_offset >= 48) {
          v_16_u8_a1 const utf8_vec = *(const v_16_u8_a1*)&utf8_chars[utf8_offset];
          if (__builtin_reduce_and(utf8_vec < (u8)128 & utf8_vec != 0) != 0) {
               v_32_u8 const ascii_chars_low  = __builtin_shufflevector(utf8_vec, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
               v_16_u8 const ascii_chars_high = __builtin_shufflevector(utf8_vec, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

               memcpy_inline(&corsar_chars[corsar_offset], &ascii_chars_low, 32);
               memcpy_inline(&corsar_chars[corsar_offset + 32], &ascii_chars_high, 16);

               utf8_offset   += 16;
               corsar_offset += 48;
          } else for (u64 const max_utf8_offset = utf8_offset + 16; utf8_offset < max_utf8_offset && corsar_offset < corsar_chars_cap_size;) {
               u32       unicode_char    = utf8_chars[utf8_offset];
               u8  const utf8_char_size  = -__builtin_reduce_add((u8)unicode_char > (const v_4_u8){0, 0x7f, 0xdf, 0xef});
               u64 const new_utf8_offset = utf8_offset + utf8_char_size;

               if (new_utf8_offset > utf8_chars_size)
                    return (const t_str_cvt_result){.src_offset = utf8_offset, .dst_offset = corsar_offset};

               switch (utf8_char_size) {
               case 0: case 1:
                    break;
               case 2: {
                    u8 const byte_1 = utf8_chars[utf8_offset + 1];
                    unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 ? (unicode_char & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
                    break;
               }
               case 3: {
                    u8 const byte_1 = utf8_chars[utf8_offset + 1];
                    u8 const byte_2 = utf8_chars[utf8_offset + 2];
                    unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 ? (unicode_char & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
                    break;
               }
               case 4: {
                    u8 const byte_1 = utf8_chars[utf8_offset + 1];
                    u8 const byte_2 = utf8_chars[utf8_offset + 2];
                    u8 const byte_3 = utf8_chars[utf8_offset + 3];
                    unicode_char =
                         (unicode_char & 0xf8) == 0xf0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 && (byte_3 & 0xc0) == 0x80
                              ? (unicode_char & 7) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                              : 0;
                    break;
               }
               default:
                    unreachable;
               }

               if (unicode_char == 0 || unicode_char > 1'114'111ul) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u32 corsar_code = unicode_to_corsar_code(unicode_char, lang_codes);
               if (corsar_code == 0) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

               corsar_code = __builtin_bswap32(corsar_code) >> 8;
               memcpy_inline(&corsar_chars[corsar_offset], &corsar_code, 3);

               utf8_offset    = new_utf8_offset;
               corsar_offset += 3;
          }
     }

     while (utf8_offset < utf8_chars_size && corsar_offset < corsar_chars_cap_size) {
          u32       unicode_char    = utf8_chars[utf8_offset];
          u8  const utf8_char_size  = -__builtin_reduce_add((u8)unicode_char > (const v_4_u8){0, 0x7f, 0xdf, 0xef});
          u64 const new_utf8_offset = utf8_offset + utf8_char_size;

          if (new_utf8_offset > utf8_chars_size) break;

          switch (utf8_char_size) {
          case 0: case 1:
               break;
          case 2: {
               u8 const byte_1 = utf8_chars[utf8_offset + 1];
               unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 ? (unicode_char & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
               break;
          }
          case 3: {
               u8 const byte_1 = utf8_chars[utf8_offset + 1];
               u8 const byte_2 = utf8_chars[utf8_offset + 2];
               unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 ? (unicode_char & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
               break;
          }
          case 4: {
               u8 const byte_1 = utf8_chars[utf8_offset + 1];
               u8 const byte_2 = utf8_chars[utf8_offset + 2];
               u8 const byte_3 = utf8_chars[utf8_offset + 3];
               unicode_char =
                    (unicode_char & 0xf8) == 0xf0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 && (byte_3 & 0xc0) == 0x80
                         ? (unicode_char & 7) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                         : 0;
               break;
          }
          default:
               unreachable;
          }

          if (unicode_char == 0 || unicode_char > 1'114'111ul) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u32 corsar_code = unicode_to_corsar_code(unicode_char, lang_codes);
          if (corsar_code == 0) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

          corsar_code = __builtin_bswap32(corsar_code) >> 8;
          memcpy_inline(&corsar_chars[corsar_offset], &corsar_code, 3);

          utf8_offset    = new_utf8_offset;
          corsar_offset += 3;
     }

     return (const t_str_cvt_result){.src_offset = utf8_offset, .dst_offset = corsar_offset};
}

core_string t_str_cvt_result utf8_chars_to_ctf8_chars(u64 const utf8_chars_size, const u8* const utf8_chars, u64 const ctf8_chars_cap_size, u8* const ctf8_chars, const t_char_codes* const lang_codes) {
     typedef u8  v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8  v_4_u8     __attribute__((ext_vector_type(4)));
     typedef u32 v_4_u32    __attribute__((ext_vector_type(4)));

     u64 utf8_offset = 0;
     u64 ctf8_offset = 0;
     while (utf8_chars_size - utf8_offset >= 64 && ctf8_chars_cap_size - ctf8_offset >= 64) {
          u64 const max_utf8_offset = utf8_offset + 64;

          v_64_u8_a1 const utf8_vec = *(const v_64_u8_a1*)&utf8_chars[utf8_offset];

          u64 const not_ascii_mask   = v_64_bool_to_u64(__builtin_convertvector((utf8_vec >= (u8)128) | (utf8_vec == (u8)0), v_64_bool));
          u64 const not_ascii_offset = not_ascii_mask == 0 ? 64 : __builtin_ctzll(not_ascii_mask);
          memcpy_le_64(&ctf8_chars[ctf8_offset], &utf8_chars[utf8_offset], not_ascii_offset);

          utf8_offset += not_ascii_offset;
          ctf8_offset += not_ascii_offset;

          while (utf8_offset < max_utf8_offset) {
               u32       unicode_char    = utf8_chars[utf8_offset];
               u8  const utf8_char_size  = -__builtin_reduce_add((u8)unicode_char > (const v_4_u8){0, 0x7f, 0xdf, 0xef});
               u64 const new_utf8_offset = utf8_offset + utf8_char_size;

               if (new_utf8_offset > utf8_chars_size)
                    return (const t_str_cvt_result){.src_offset = utf8_offset, .dst_offset = ctf8_offset};

               switch (utf8_char_size) {
               case 0: case 1:
                    break;
               case 2: {
                    u8 const byte_1 = utf8_chars[utf8_offset + 1];
                    unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 ? (unicode_char & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
                    break;
               }
               case 3: {
                    u8 const byte_1 = utf8_chars[utf8_offset + 1];
                    u8 const byte_2 = utf8_chars[utf8_offset + 2];
                    unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 ? (unicode_char & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
                    break;
               }
               case 4: {
                    u8 const byte_1 = utf8_chars[utf8_offset + 1];
                    u8 const byte_2 = utf8_chars[utf8_offset + 2];
                    u8 const byte_3 = utf8_chars[utf8_offset + 3];
                    unicode_char =
                         (unicode_char & 0xf8) == 0xf0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 && (byte_3 & 0xc0) == 0x80
                              ? (unicode_char & 7) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                              : 0;
                    break;
               }
               default:
                    unreachable;
               }

               if (unicode_char == 0 || unicode_char > 1'114'111ul) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               u32 corsar_code = unicode_to_corsar_code(unicode_char, lang_codes);
               if (corsar_code == 0) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

               u8  const ctf8_char_size  = char_size_in_ctf8(corsar_code);
               u64 const new_ctf8_offset = ctf8_offset + ctf8_char_size;
               if (new_ctf8_offset > ctf8_chars_cap_size)
                    return (const t_str_cvt_result){.src_offset = utf8_offset, .dst_offset = ctf8_offset};

               switch (ctf8_char_size) {
               case 1:
                    ctf8_chars[ctf8_offset] = corsar_code;
                    break;
               case 2:
                    ctf8_chars[ctf8_offset]     = corsar_code >> 6 | 0x80;
                    ctf8_chars[ctf8_offset + 1] = corsar_code & 0x3f | 0xc0;
                    break;
               case 3:
                    ctf8_chars[ctf8_offset]     = corsar_code >> 12 | 0xa0;
                    ctf8_chars[ctf8_offset + 1] = corsar_code >> 6 & 0x3f | 0xc0;
                    ctf8_chars[ctf8_offset + 2] = corsar_code & 0x3f | 0xc0;
                    break;
               case 4:
                    *(ua_u32*)&ctf8_chars[ctf8_offset] = __builtin_reduce_or(
                         (
                              (corsar_code >> (const v_4_u32){18, 12, 6, 0}) &
                              (const v_4_u32){0xf, 0x3f, 0x3f, 0x3f}         |
                              (const v_4_u32){0xb0, 0xc0, 0xc0, 0xc0}
                         ) << (const v_4_u32){0, 8, 16, 24}
                    );
                    break;
               default:
                    unreachable;
               }

               utf8_offset = new_utf8_offset;
               ctf8_offset = new_ctf8_offset;
          }
     }

     while (utf8_offset < utf8_chars_size && ctf8_offset < ctf8_chars_cap_size) {
          u32       unicode_char    = utf8_chars[utf8_offset];
          u8  const utf8_char_size  = -__builtin_reduce_add((u8)unicode_char > (const v_4_u8){0, 0x7f, 0xdf, 0xef});
          u64 const new_utf8_offset = utf8_offset + utf8_char_size;

          if (new_utf8_offset > utf8_chars_size) break;

          switch (utf8_char_size) {
          case 0: case 1:
               break;
          case 2: {
               u8 const byte_1 = utf8_chars[utf8_offset + 1];
               unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 ? (unicode_char & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
               break;
          }
          case 3: {
               u8 const byte_1 = utf8_chars[utf8_offset + 1];
               u8 const byte_2 = utf8_chars[utf8_offset + 2];
               unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 ? (unicode_char & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
               break;
          }
          case 4: {
               u8 const byte_1 = utf8_chars[utf8_offset + 1];
               u8 const byte_2 = utf8_chars[utf8_offset + 2];
               u8 const byte_3 = utf8_chars[utf8_offset + 3];
               unicode_char =
               (unicode_char & 0xf8) == 0xf0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 && (byte_3 & 0xc0) == 0x80
               ? (unicode_char & 7) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
               : 0;
               break;
          }
          default:
               unreachable;
          }

          if (unicode_char == 0 || unicode_char > 1'114'111ul) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          u32 corsar_code = unicode_to_corsar_code(unicode_char, lang_codes);
          if (corsar_code == 0) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

          u8  const ctf8_char_size  = char_size_in_ctf8(corsar_code);
          u64 const new_ctf8_offset = ctf8_offset + ctf8_char_size;
          if (new_ctf8_offset > ctf8_chars_cap_size) break;

          switch (ctf8_char_size) {
          case 1:
               ctf8_chars[ctf8_offset] = corsar_code;
               break;
          case 2:
               ctf8_chars[ctf8_offset]     = corsar_code >> 6 | 0x80;
               ctf8_chars[ctf8_offset + 1] = corsar_code & 0x3f | 0xc0;
               break;
          case 3:
               ctf8_chars[ctf8_offset]     = corsar_code >> 12 | 0xa0;
               ctf8_chars[ctf8_offset + 1] = corsar_code >> 6 & 0x3f | 0xc0;
               ctf8_chars[ctf8_offset + 2] = corsar_code & 0x3f | 0xc0;
               break;
          case 4:
               *(ua_u32*)&ctf8_chars[ctf8_offset] = __builtin_reduce_or(
                    (
                         (corsar_code >> (const v_4_u32){18, 12, 6, 0}) &
                         (const v_4_u32){0xf, 0x3f, 0x3f, 0x3f}         |
                         (const v_4_u32){0xb0, 0xc0, 0xc0, 0xc0}
                    ) << (const v_4_u32){0, 8, 16, 24}
               );
               break;
          default:
               unreachable;
          }

          utf8_offset = new_utf8_offset;
          ctf8_offset = new_ctf8_offset;
     }


     return (const t_str_cvt_result){.src_offset = utf8_offset, .dst_offset = ctf8_offset};
}

core_string t_str_cvt_result utf8_chars_to_sys_chars(u64 const utf8_chars_size, const u8* const utf8_chars, u64 const sys_chars_cap_size, u8* const sys_chars) {
     typedef u8 v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8 v_4_u8     __attribute__((ext_vector_type(4)));

     if (sys_enc_is_utf8)
          return (const t_str_cvt_result){.src_offset = str_cvt_sys_is_utf8};

     mbstate_t mbstate = {};
     char      char_buffer[16];

     u64 utf8_offset = 0;
     u64 sys_offset  = 0;
     while (utf8_chars_size - utf8_offset >= 64 && sys_chars_cap_size - sys_offset >= 64) {
          u64 const max_utf8_offset = utf8_offset + 64;

          v_64_u8_a1 const utf8_vec = *(const v_64_u8_a1*)&utf8_chars[utf8_offset];

          u64 const not_ascii_mask   = v_64_bool_to_u64(__builtin_convertvector((utf8_vec >= (u8)128) | (utf8_vec == (u8)0), v_64_bool));
          u64 const not_ascii_offset = not_ascii_mask == 0 ? 64 : __builtin_ctzll(not_ascii_mask);
          memcpy_le_64(&sys_chars[sys_offset], &utf8_chars[utf8_offset], not_ascii_offset);

          utf8_offset += not_ascii_offset;
          sys_offset  += not_ascii_offset;

          while (utf8_offset < max_utf8_offset) {
               u32       unicode_char    = utf8_chars[utf8_offset];
               u8  const utf8_char_size  = -__builtin_reduce_add((u8)unicode_char > (const v_4_u8){0, 0x7f, 0xdf, 0xef});
               u64 const new_utf8_offset = utf8_offset + utf8_char_size;

               if (new_utf8_offset > utf8_chars_size)
                    return (const t_str_cvt_result){.src_offset = utf8_offset, .dst_offset = sys_offset};

               switch (utf8_char_size) {
               case 0: case 1:
                    break;
               case 2: {
                    u8 const byte_1 = utf8_chars[utf8_offset + 1];
                    unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 ? (unicode_char & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
                    break;
               }
               case 3: {
                    u8 const byte_1 = utf8_chars[utf8_offset + 1];
                    u8 const byte_2 = utf8_chars[utf8_offset + 2];
                    unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 ? (unicode_char & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
                    break;
               }
               case 4: {
                    u8 const byte_1 = utf8_chars[utf8_offset + 1];
                    u8 const byte_2 = utf8_chars[utf8_offset + 2];
                    u8 const byte_3 = utf8_chars[utf8_offset + 3];
                    unicode_char =
                         (unicode_char & 0xf8) == 0xf0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 && (byte_3 & 0xc0) == 0x80
                              ? (unicode_char & 7) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                              : 0;
                    break;
               }
               default:
                    unreachable;
               }

               if (unicode_char == 0 || unicode_char > 1'114'111ul) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

               size_t sys_char_size = c32rtomb(char_buffer, unicode_char, &mbstate);
               if ((ssize_t)sys_char_size < 1) [[clang::unlikely]]
                    return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

               u64 const new_sys_offset = sys_offset + sys_char_size;
               if (new_sys_offset > sys_chars_cap_size)
                    return (const t_str_cvt_result){.src_offset = utf8_offset, .dst_offset = sys_offset};

               memcpy_le_16(&sys_chars[sys_offset], char_buffer, sys_char_size);

               utf8_offset = new_utf8_offset;
               sys_offset  = new_sys_offset;
          }
     }

     while (utf8_offset < utf8_chars_size && sys_offset < sys_chars_cap_size) {
          u32       unicode_char    = utf8_chars[utf8_offset];
          u8  const utf8_char_size  = -__builtin_reduce_add((u8)unicode_char > (const v_4_u8){0, 0x7f, 0xdf, 0xef});
          u64 const new_utf8_offset = utf8_offset + utf8_char_size;

          if (new_utf8_offset > utf8_chars_size) break;

          switch (utf8_char_size) {
          case 0: case 1:
               break;
          case 2: {
               u8 const byte_1 = utf8_chars[utf8_offset + 1];
               unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 ? (unicode_char & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
               break;
          }
          case 3: {
               u8 const byte_1 = utf8_chars[utf8_offset + 1];
               u8 const byte_2 = utf8_chars[utf8_offset + 2];
               unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 ? (unicode_char & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
               break;
          }
          case 4: {
               u8 const byte_1 = utf8_chars[utf8_offset + 1];
               u8 const byte_2 = utf8_chars[utf8_offset + 2];
               u8 const byte_3 = utf8_chars[utf8_offset + 3];
               unicode_char =
               (unicode_char & 0xf8) == 0xf0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 && (byte_3 & 0xc0) == 0x80
               ? (unicode_char & 7) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
               : 0;
               break;
          }
          default:
               unreachable;
          }

          if (unicode_char == 0 || unicode_char > 1'114'111ul) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__encoding};

          size_t sys_char_size = c32rtomb(char_buffer, unicode_char, &mbstate);
          if ((ssize_t)sys_char_size < 1) [[clang::unlikely]]
               return (const t_str_cvt_result){.src_offset = str_cvt_err__recoding};

          u64 const new_sys_offset = sys_offset + sys_char_size;
          if (new_sys_offset > sys_chars_cap_size) break;

          memcpy_le_16(&sys_chars[sys_offset], char_buffer, sys_char_size);

          utf8_offset = new_utf8_offset;
          sys_offset  = new_sys_offset;
     }

     return (const t_str_cvt_result){.src_offset = utf8_offset, .dst_offset = sys_offset};
}