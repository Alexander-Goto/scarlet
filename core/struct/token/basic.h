#pragma once

#include "../../common/bool_vectors.h"
#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../../struct/null/basic.h"

#define comptime_tkn__equal     0x9b955b841d49390cc1cde10fede912buwb
#define comptime_tkn__great     0x9ea79e4c45fddcbafbd4b353d2f535fuwb
#define comptime_tkn__less      0x9e79e620503f2d16f246dc372ac1cb7uwb
#define comptime_tkn__nan_cmp   0x9e1b8d461ca75d16de399f20a6eb3a7uwb
#define comptime_tkn__not_equal 0x9b85ab539a264d16bea78a034a954f7uwb

static t_any const tkn__equal     = {.raw_bits = comptime_tkn__equal};
static t_any const tkn__great     = {.raw_bits = comptime_tkn__great};
static t_any const tkn__less      = {.raw_bits = comptime_tkn__less};
static t_any const tkn__nan_cmp   = {.raw_bits = comptime_tkn__nan_cmp};
static t_any const tkn__not_equal = {.raw_bits = comptime_tkn__not_equal};

#define equal_id     0x2b
#define great_id     0x5f
#define less_id      0xb7
#define nan_cmp_id   0xa7
#define not_equal_id 0xf7

static t_any const tkn__invalid_str_fmt       = {.raw_bits=0x9685616acf3cbd78f94aefb9f843eefuwb};
static t_any const tkn__64_bits_is_not_enough = {.raw_bits=0x9547bff9f630b670a54891732d9bf27uwb};

core_basic t_any token_from_ascii_chars(u64 const len, const char* const chars) {
     typedef u8   v_16_u8    __attribute__((ext_vector_type(16)));
     typedef u8   v_16_u8_a1 __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8   v_32_u8    __attribute__((ext_vector_type(32)));
     typedef char v_32_char  __attribute__((ext_vector_type(32)));
     typedef u64  v_16_u64   __attribute__((ext_vector_type(16)));

     typedef unsigned _BitInt(256) u256 __attribute__((aligned(alignof(v_32_u8))));

     if (len == 0 || len > 25) return null;

     [[gnu::aligned(alignof(v_32_u8))]]
     u8 vec_chars[32];

     char const last_char          = chars[len - 1];
     u8   const len_without_suffix = last_char == '?' ? len - 1 : (
          last_char != '!' ? len : (
               len > 2 && chars[len - 3] == '!' ? len - 3 : len - 1
          )
     );
     u8 const suffix_size = len - len_without_suffix;

     u8 const small_part_len = len_without_suffix / 2;
     u8 const big_part_len   = len_without_suffix - small_part_len;
     memcpy_le_16(&vec_chars[16 - small_part_len], chars, small_part_len);
     memcpy_le_16(&vec_chars[32 - big_part_len], &chars[small_part_len], big_part_len);
     *(v_32_char*)vec_chars = __builtin_shufflevector(*(const v_32_char*)vec_chars, (const v_32_char){}, 31, 15, 30, 14, 29, 13, 28, 12, 27, 11, 26, 10, 25, 9, 24, 8, 23, 7, 22, 6, 21, 5, 20, 4, 19, 3, 18, 2, 17, 1, 16, 0);

     v_32_char const digit_mask_vec            = *(const v_32_u8*)vec_chars >= '0' & *(const v_32_u8*)vec_chars <= '9';
     v_32_char const underscore_mask_vec       = *(const v_32_u8*)vec_chars == '_';
     v_32_char const letter_mask_vec           = *(const v_32_u8*)vec_chars >= 'a' & *(const v_32_u8*)vec_chars <= 'z';
     u32       const chars_without_suffix_mask = ((u32)1 << len_without_suffix) - 1;

     if (
          (v_32_bool_to_u32(__builtin_convertvector(digit_mask_vec | underscore_mask_vec | letter_mask_vec, v_32_bool)) & chars_without_suffix_mask) != chars_without_suffix_mask ||
          len_without_suffix == 0 || len_without_suffix > 22                                                                                                                      ||
          (suffix_size == 3 && chars[len - 2] != '!')
     ) return null;

     u8 const suffix_code = suffix_size == 0 ? 3 : (suffix_size == 1 ? (last_char == '?' ? 2 : 1) : 0);

     *(v_32_char*)vec_chars =
          (digit_mask_vec & (*(const v_32_u8*)vec_chars - ('0' - 1)))   |
          (letter_mask_vec & (*(const v_32_u8*)vec_chars - ('a' - 12))) |
          (underscore_mask_vec & 11);
     *(v_32_u8*)vec_chars += suffix_code;
     *(v_32_u8*)vec_chars  = (*(const v_32_u8*)vec_chars - ((*(const v_32_u8*)vec_chars > 38) & 38));
     *(u256*)vec_chars    &= ((u256)1 << len_without_suffix * 8) - 1;

     v_16_u64 const power_of_39 = {8140406085191601uwb, 208728361158759uwb, 5352009260481uwb, 137231006679uwb, 3518743761uwb, 90224199uwb, 2313441uwb, 59319uwb, 1521, 39, 1, 0, 0, 0, 0, 0};
     v_16_u64 const power_of_37 = {4808584372417849uwb, 129961739795077uwb, 3512479453921uwb, 94931877133uwb, 2565726409uwb, 69343957uwb, 1874161uwb, 50653uwb, 1369, 37, 1, 0, 0, 0, 0, 0};
     u64      const low_part    = __builtin_reduce_add(__builtin_convertvector(*(const v_16_u8*)vec_chars, v_16_u64) * power_of_39);
     u64      const high_part   = len_without_suffix <= 11 ?
          __builtin_reduce_add(__builtin_convertvector(*(const v_16_u8_a1*)vec_chars, v_16_u64) * power_of_37) | (u64)5 << 56 :
          __builtin_reduce_add(__builtin_convertvector(*(const v_16_u8_a1*)&vec_chars[11], v_16_u64) * power_of_39);

     t_any token     = {.raw_bits = suffix_code | low_part << 2 | (u128)high_part << 61};
     token.bytes[15] = tid__token;

     return token;
}

core_basic void token_to_ascii_chars(t_any const token, u8* const len, char* const chars) {
     assert(token.bytes[15] == tid__token);

     typedef u64  v_16_u64   __attribute__((ext_vector_type(16)));
     typedef u8   v_16_u8    __attribute__((ext_vector_type(16)));
     typedef u8   v_16_u8_a1 __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8   v_32_u8    __attribute__((ext_vector_type(32)));
     typedef char v_32_char  __attribute__((ext_vector_type(32)));

     typedef unsigned _BitInt(256) u256 __attribute__((aligned(alignof(v_32_u8))));

     [[gnu::aligned(alignof(v_32_u8))]]
     u8 vec_chars[32];

     u8  const suffix_code = token.bytes[0] & 3;
     u64 const low_part    = token.qwords[0] >> 2 & ((u64)1 << 59) - 1;
     u64 const high_part   = (u64)(token.raw_bits >> 61) & ((u64)1 << 59) - 1;

     v_16_u64 const power_of_39__to_1 = {-1, 8140406085191601uwb, 208728361158759uwb, 5352009260481uwb, 137231006679uwb, 3518743761uwb, 90224199uwb, 2313441uwb, 59319uwb, 1521, 39, 1, 1, 1, 1, 1};
     v_16_u64 const power_of_39__to_0 = {8140406085191601uwb, 208728361158759uwb, 5352009260481uwb, 137231006679uwb, 3518743761uwb, 90224199uwb, 2313441uwb, 59319uwb, 1521, 39, 1, 1, 1, 1, 1, 1};

     v_16_u8 const low_chars = __builtin_convertvector(low_part % power_of_39__to_1 / power_of_39__to_0, v_16_u8);
     *(v_16_u8*)vec_chars    = low_chars;

     u16  const low_chars_mask = v_16_bool_to_u16(__builtin_convertvector(low_chars != 0, v_16_bool));
     u8         local_len      = __builtin_popcount(low_chars_mask);
     bool const len_gt_11      = (high_part & (u64)5 << 56) != (u64)5 << 56;

     assume(local_len != 0);
     assume(vec_chars[0] < 39);
     assert(((u16)1 << local_len) - 1 == low_chars_mask);
     assert(local_len < 11 && !len_gt_11 || local_len == 11);
     assert(len_gt_11 || (__builtin_reduce_add(__builtin_convertvector(low_chars, v_16_u64) * (const v_16_u64){
          4808584372417849uwb, 129961739795077uwb, 3512479453921uwb, 94931877133uwb, 2565726409uwb, 69343957uwb, 1874161uwb, 50653uwb, 1369, 37, 1, 0, 0, 0, 0, 0
     }) | (u64)5 << 56) == high_part);

     if (len_gt_11) {
          v_16_u8_a1 const high_chars  = __builtin_convertvector(high_part % power_of_39__to_1 / power_of_39__to_0, v_16_u8);
          *(v_16_u8_a1*)&vec_chars[11] = high_chars;

          u16 const high_chars_mask = v_16_bool_to_u16(__builtin_convertvector(high_chars != 0, v_16_bool));
          u8        high_len        = __builtin_popcount(high_chars_mask);
          local_len                += high_len;

          assume(vec_chars[11] < 39);
          assert(((u16)1 << high_len) - 1 == high_chars_mask);
     }

     *(v_32_char*)vec_chars = ((*(const v_32_char*)vec_chars <= suffix_code) & 38) + *(const v_32_char*)vec_chars - suffix_code;
     *(u256*)vec_chars     &= ((u256)1 << local_len * 8) - 1;

     assert(__builtin_reduce_or(*(v_32_u8*)vec_chars > 38) == 0);

     *(v_32_u8*)vec_chars = *(const v_32_u8*)vec_chars +
          ((*(const v_32_u8*)vec_chars > 10) & ('_' - '9' - 1)) +
          ((*(const v_32_u8*)vec_chars > 11) & ('a' - '_' - 1)) +
          ('0' - 1);

     *(v_32_char*)vec_chars = __builtin_shufflevector(*(const v_32_char*)vec_chars, (const v_32_char){}, 31, 29, 27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3, 1, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0);
     u8 const small_part_len = local_len / 2;
     u8 const big_part_len   = local_len - small_part_len;
     memcpy_le_16(chars, &vec_chars[16 - small_part_len], small_part_len);
     memcpy_le_16(&chars[small_part_len], &vec_chars[32 - big_part_len], big_part_len);

     switch (suffix_code) {
     case 0:
          memcpy_inline(&chars[local_len], "!!!", 4);
          local_len += 3;
          break;
     case 1:
          memcpy_inline(&chars[local_len], "!", 2);
          local_len += 1;
          break;
     case 2:
          memcpy_inline(&chars[local_len], "?", 2);
          local_len += 1;
          break;
     case 3:
          chars[local_len] = 0;
          break;
     default:
          unreachable;
     }

     assume(local_len != 0 && local_len < 26);

     *len = local_len;
}
