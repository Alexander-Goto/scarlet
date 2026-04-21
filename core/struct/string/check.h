#pragma once

#include "../../common/bool_vectors.h"
#include "../../common/corsar.h"
#include "../../common/const.h"
#include "../../common/fn.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/type.h"

core_string u64 check_corsar_chars(u64 const size, const u8* const chars) {
     assert(size % 3 == 0);

     typedef u8 v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));

     u64 offset = 0;
     while (size - offset > 64) {
          v_64_u8_a1 const corsar_vec = *(const v_64_u8_a1*)&chars[offset];

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

          u64 const max_offset     = offset + 63;
          u64       not_ascii_mask = ascii_mask ^ zero_mask ^ 0x9249'2492'4924'9249ull;
          while (true) {
               u64 const not_ascii_offset = __builtin_ctzll(not_ascii_mask);
               not_ascii_mask           >>= not_ascii_offset;
               offset                    += not_ascii_offset;

               if (offset >= max_offset) break;

               u32 code;
               memcpy_inline(&code, &chars[offset], 3);
               code = __builtin_bswap32(code) >> 8;

               if (!corsar_code_is_correct(code)) return -(offset + 1);

               offset          += 3;
               not_ascii_mask >>= 3;
          }
     }

     for (; offset < size; offset += 3) {
          u32 code;
          memcpy_inline(&code, &chars[offset], 3);
          code = __builtin_bswap32(code) >> 8;

          if (!corsar_code_is_correct(code)) return -(offset + 1);
     }

     return offset;
}

core_string u64 check_ctf8_chars(u64 const size, const u8* const chars) {
     typedef u8 v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8 v_4_u8     __attribute__((ext_vector_type(4)));

     u64 offset = 0;
     while (size - offset >= 64) {
          v_64_u8_a1 const ctf8_vec       = *(const v_64_u8_a1*)&chars[offset];
          u64              not_ascii_mask = v_64_bool_to_u64(__builtin_convertvector((ctf8_vec >= (u8)128) | (ctf8_vec == (u8)0), v_64_bool));
          for (u64 const max_offset = offset + 64; offset < max_offset;) {
               if ((not_ascii_mask & 1) == 0) {
                    if (not_ascii_mask == 0)
                         offset = max_offset;
                    else {
                         u64 const not_ascii_offset = __builtin_ctzll(not_ascii_mask);
                         not_ascii_mask           >>= not_ascii_offset;
                         offset                    += not_ascii_offset;
                    }
               } else {
                    u32       corsar_code     = chars[offset];
                    u8  const ctf8_char_size  = -__builtin_reduce_add((u8)corsar_code > (const v_4_u8){0, 0x7f, 0x9f, 0xaf});
                    u64 const new_ctf8_offset = offset + ctf8_char_size;
                    if (new_ctf8_offset > size) return offset;

                    switch (ctf8_char_size) {
                    case 0: case 1:
                         break;
                    case 2: {
                         u8 const byte_1 = chars[offset + 1];
                         corsar_code = byte_1 >= 0xc0 ? (corsar_code & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
                         break;
                    }
                    case 3: {
                         u8 const byte_1 = chars[offset + 1];
                         u8 const byte_2 = chars[offset + 2];
                         corsar_code = byte_1 >= 0xc0 && byte_2 >= 0xc0 ? (corsar_code & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
                         break;
                    }
                    case 4: {
                         u8 const byte_1 = chars[offset + 1];
                         u8 const byte_2 = chars[offset + 2];
                         u8 const byte_3 = chars[offset + 3];
                         corsar_code =
                              corsar_code < 0xc0 && byte_1 >= 0xc0 && byte_2 >= 0xc0 && byte_3 >= 0xc0
                                   ? (corsar_code & 0xf) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                                   : 0;
                         break;
                    }
                    default:
                         unreachable;
                    }

                    if (!corsar_code_is_correct(corsar_code)) return -(offset + 1);

                    not_ascii_mask >>= ctf8_char_size;
                    offset           = new_ctf8_offset;
               }
          }
     }

     while (offset < size) {
          u32       corsar_code     = chars[offset];
          u8  const ctf8_char_size  = -__builtin_reduce_add((u8)corsar_code > (const v_4_u8){0, 0x7f, 0x9f, 0xaf});
          u64 const new_ctf8_offset = offset + ctf8_char_size;
          if (new_ctf8_offset > size) return offset;

          switch (ctf8_char_size) {
          case 0: case 1:
               break;
          case 2: {
               u8 const byte_1 = chars[offset + 1];
               corsar_code = byte_1 >= 0xc0 ? (corsar_code & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
               break;
          }
          case 3: {
               u8 const byte_1 = chars[offset + 1];
               u8 const byte_2 = chars[offset + 2];
               corsar_code = byte_1 >= 0xc0 && byte_2 >= 0xc0 ? (corsar_code & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
               break;
          }
          case 4: {
               u8 const byte_1 = chars[offset + 1];
               u8 const byte_2 = chars[offset + 2];
               u8 const byte_3 = chars[offset + 3];
               corsar_code =
                    corsar_code < 0xc0 && byte_1 >= 0xc0 && byte_2 >= 0xc0 && byte_3 >= 0xc0
                         ? (corsar_code & 0xf) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                         : 0;
               break;
          }
          default:
               unreachable;
          }

          if (!corsar_code_is_correct(corsar_code)) return -(offset + 1);

          offset = new_ctf8_offset;
     }

     return offset;
}

core_string u64 check_utf8_chars(u64 const size, const u8* const chars) {
     typedef u8 v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8 v_4_u8     __attribute__((ext_vector_type(4)));

     u64 offset = 0;
     while (size - offset >= 64) {
          v_64_u8_a1 const utf8_vec       = *(const v_64_u8_a1*)&chars[offset];
          u64              not_ascii_mask = v_64_bool_to_u64(__builtin_convertvector((utf8_vec >= (u8)128) | (utf8_vec == (u8)0), v_64_bool));
          for (u64 const max_offset = offset + 64; offset < max_offset;) {
               if ((not_ascii_mask & 1) == 0) {
                    if (not_ascii_mask == 0)
                         offset = max_offset;
                    else {
                         u64 const not_ascii_offset = __builtin_ctzll(not_ascii_mask);
                         not_ascii_mask           >>= not_ascii_offset;
                         offset                    += not_ascii_offset;
                    }
               } else {
                    u32       unicode_char    = chars[offset];
                    u8  const utf8_char_size  = -__builtin_reduce_add((u8)unicode_char > (const v_4_u8){0, 0x7f, 0xdf, 0xef});
                    u64 const new_utf8_offset = offset + utf8_char_size;

                    if (new_utf8_offset > size) return offset;

                    switch (utf8_char_size) {
                    case 0: case 1:
                         break;
                    case 2: {
                         u8 const byte_1 = chars[offset + 1];
                         unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 ? (unicode_char & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
                         break;
                    }
                    case 3: {
                         u8 const byte_1 = chars[offset + 1];
                         u8 const byte_2 = chars[offset + 2];
                         unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 ? (unicode_char & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
                         break;
                    }
                    case 4: {
                         u8 const byte_1 = chars[offset + 1];
                         u8 const byte_2 = chars[offset + 2];
                         u8 const byte_3 = chars[offset + 3];
                         unicode_char =
                         (unicode_char & 0xf8) == 0xf0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 && (byte_3 & 0xc0) == 0x80
                         ? (unicode_char & 7) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
                         : 0;
                         break;
                    }
                    default:
                         unreachable;
                    }

                    if (unicode_char == 0 || unicode_char > 1'114'111ul) return -(offset + 1);

                    not_ascii_mask >>= utf8_char_size;
                    offset           = new_utf8_offset;
               }
          }
     }

     while (offset < size) {
          u32       unicode_char    = chars[offset];
          u8  const utf8_char_size  = -__builtin_reduce_add((u8)unicode_char > (const v_4_u8){0, 0x7f, 0xdf, 0xef});
          u64 const new_utf8_offset = offset + utf8_char_size;

          if (new_utf8_offset > size) break;

          switch (utf8_char_size) {
          case 0: case 1:
               break;
          case 2: {
               u8 const byte_1 = chars[offset + 1];
               unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 ? (unicode_char & 0x1f) << 6 | (u32)byte_1 & 0x3f : 0;
               break;
          }
          case 3: {
               u8 const byte_1 = chars[offset + 1];
               u8 const byte_2 = chars[offset + 2];
               unicode_char    = unicode_char >= 0xc0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 ? (unicode_char & 0xf) << 12 | ((u32)byte_1 & 0x3f) << 6 | (u32)byte_2 & 0x3f : 0;
               break;
          }
          case 4: {
               u8 const byte_1 = chars[offset + 1];
               u8 const byte_2 = chars[offset + 2];
               u8 const byte_3 = chars[offset + 3];
               unicode_char =
               (unicode_char & 0xf8) == 0xf0 && (byte_1 & 0xc0) == 0x80 && (byte_2 & 0xc0) == 0x80 && (byte_3 & 0xc0) == 0x80
               ? (unicode_char & 7) << 18 | ((u32)byte_1 & 0x3f) << 12 | ((u32)byte_2 & 0x3f) << 6 | byte_3 & 0x3f
               : 0;
               break;
          }
          default:
               unreachable;
          }

          if (unicode_char == 0 || unicode_char > 1'114'111ul) return -(offset + 1);

          offset = new_utf8_offset;
     }

     return offset;
}