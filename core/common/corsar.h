#pragma once

#include "../app/env/var.h"
#include "fn.h"
#include "include.h"
#include "macro.h"
#include "type.h"

// auto-generated: begin //
typedef u32 v_char        __attribute__((ext_vector_type(32)));
typedef i32 v_char_offset __attribute__((ext_vector_type(32)));

struct {
     v_char_offset offsets;
     v_char        from;
     v_char        to;
} typedef t_char_codes;

static const t_char_codes* sys_char_codes;

[[clang::always_inline]]
core_string const t_char_codes* get_sys_char_codes(void) {return sys_char_codes;}

static const t_char_codes* const en_char_codes = &(const t_char_codes) {
     .offsets = (const v_char_offset){0xffffffuwb, 0x100037auwb, 0x100033auwb, 0x1000336uwb, 0x100038fuwb, 0x100038euwb, 0x100038duwb, 0x10003a9uwb, 0x100036auwb, 0x1000366uwb},
     .from    = (const v_char){0, 1024, 1029, 1037, 1039, 1045, 1077, 1104, 1109, 1117},
     .to      = (const v_char){128, 1026, 1031, 1039, 1046, 1078, 1104, 1106, 1111, 1119}
};

static const t_char_codes* const ru_char_codes = en_char_codes;

static const t_char_codes* const be_char_codes = &(const t_char_codes) {
     .offsets = (const v_char_offset){0xffffffuwb, 0x1000338uwb, 0x100033auwb, 0x1000336uwb, 0x100034duwb, 0x100034cuwb, 0x100038euwb, 0x100034cuwb, 0x100034buwb, 0x100038euwb, 0x100034duwb, 0x100034cuwb, 0x100038duwb, 0x100034cuwb, 0x100034buwb, 0x100038duwb, 0x100034duwb, 0x1000368uwb, 0x100036auwb, 0x1000366uwb},
     .from    = (const v_char){0, 1024, 1029, 1037, 1039, 1045, 1047, 1048, 1059, 1064, 1066, 1077, 1079, 1080, 1091, 1096, 1098, 1104, 1109, 1117},
     .to      = (const v_char){128, 1026, 1031, 1039, 1046, 1048, 1049, 1060, 1065, 1067, 1078, 1080, 1081, 1092, 1097, 1099, 1104, 1106, 1111, 1119}
};

[[clang::always_inline]]
static bool char_is_ctrl(u32 const corsar_code) {
     typedef u32 t_vec __attribute__((ext_vector_type(4)));

     return __builtin_reduce_or(corsar_code > (const t_vec){0, 9, 126} & corsar_code < (const t_vec){9, 32, 128}) != 0;
}

[[clang::always_inline]]
static bool char_is_space(u32 const corsar_code) {return corsar_code == 9 || corsar_code == 32;}

[[clang::always_inline]]
static bool char_is_symbol(u32 const corsar_code) {
     typedef u32 t_vec __attribute__((ext_vector_type(4)));

     return __builtin_reduce_or(corsar_code > (const t_vec){32, 57, 90, 122} & corsar_code < (const t_vec){48, 65, 97, 127}) != 0;
}

[[clang::always_inline]]
static bool char_is_digit(u32 const corsar_code) {return corsar_code > 47 & corsar_code < 58;}

[[clang::always_inline]]
static bool char_is_letter(u32 const corsar_code) {
     typedef u32 t_vec __attribute__((ext_vector_type(4)));

     return __builtin_reduce_or(corsar_code > (const t_vec){64, 96, 127} & corsar_code < (const t_vec){91, 123, 258}) != 0;
}

[[clang::always_inline]]
static bool char_is_graphic(u32 const corsar_code) {
     typedef u32 t_vec __attribute__((ext_vector_type(4)));

     return __builtin_reduce_or(corsar_code > (const t_vec){8, 31, 127} & corsar_code < (const t_vec){10, 127, 258}) != 0;
}

[[clang::always_inline]]
static bool char_is_upper(u32 const corsar_code) {
     typedef u32 t_vec __attribute__((ext_vector_type(4)));

     return __builtin_reduce_or(corsar_code > (const t_vec){64, 127, 193} & corsar_code < (const t_vec){91, 161, 226}) != 0;
}

[[clang::always_inline]]
static bool char_is_lower(u32 const corsar_code) {
     typedef u32 t_vec __attribute__((ext_vector_type(4)));

     return __builtin_reduce_or(corsar_code > (const t_vec){96, 160, 225} & corsar_code < (const t_vec){123, 194, 258}) != 0;
}

[[clang::always_inline]]
static u64 char_to_upper(u32 const corsar_code) {
     typedef u32 t_vec    __attribute__((ext_vector_type(4)));
     typedef i32 v_offset __attribute__((ext_vector_type(4)));

     return corsar_code + __builtin_reduce_or(corsar_code > (const t_vec){96, 160, 225} & corsar_code < (const t_vec){123, 194, 258} & (const v_offset){-32, -33, -32});
}

[[clang::always_inline]]
static u64 char_to_lower(u32 const corsar_code) {
     typedef u32 t_vec    __attribute__((ext_vector_type(4)));
     typedef i32 v_offset __attribute__((ext_vector_type(4)));

     return corsar_code + __builtin_reduce_or(corsar_code > (const t_vec){64, 127, 193} & corsar_code < (const t_vec){91, 161, 226} & (const v_offset){32, 33, 32});
}

[[clang::always_inline]]
static u16 char_lang(u32 const corsar_code) {
     typedef u32 t_vec  __attribute__((ext_vector_type(4)));
     typedef i32 v_lang __attribute__((ext_vector_type(4)));

     return __builtin_reduce_or(corsar_code > (const t_vec){193, 64, 96, 127} & corsar_code < (const t_vec){258, 91, 123, 194} & (const v_lang){25954, 28261, 28261, 30066});
}

core_string inline u32 corsar_code_to_unicode(u32 const corsar_code) {
     typedef u32 t_vec    __attribute__((ext_vector_type(32)));
     typedef i32 v_offset __attribute__((ext_vector_type(32)));

     u32 const offset = __builtin_reduce_or(
          corsar_code > (const t_vec){0, 127, 133, 134, 166, 167, 193, 199, 200, 202, 203, 214, 215, 220, 231, 232, 234, 235, 246, 247, 252} &
          corsar_code < (const t_vec){128, 134, 135, 167, 168, 194, 200, 201, 203, 204, 215, 216, 221, 232, 233, 235, 236, 247, 248, 253, 258} &
          (const v_offset){0xffffffuwb, 0x100038fuwb, 0x100037auwb, 0x100038euwb, 0x10003a9uwb, 0x100038duwb, 0x100034duwb, 0x1000338uwb, 0x100034cuwb, 0x100033auwb, 0x100034cuwb, 0x1000336uwb, 0x100034buwb, 0x100034duwb, 0x1000368uwb, 0x100034cuwb, 0x100036auwb, 0x100034cuwb, 0x1000366uwb, 0x100034buwb, 0x100034duwb}
     );
     return offset == 0 ? 0 : corsar_code + offset - (u32)0xff'ffff;
}

core_string inline u32 unicode_to_corsar_code(u32 const unicode, const t_char_codes* const lang_codes) {
     u32 const offset = __builtin_reduce_or(unicode > lang_codes->from & unicode < lang_codes->to & lang_codes->offsets);
     return offset == 0 ? 0 : unicode - offset + (u32)0xff'ffff;
}
// auto-generated: end //

[[clang::always_inline]]
static bool corsar_code_is_correct(u32 const code) {
     return code != 0 && code <= 257;
}

[[clang::always_inline]]
static u8 char_size_in_ctf8(u32 const corsar_code) {
     typedef u32 v_4_u32 __attribute__((ext_vector_type(4)));
     return -__builtin_reduce_add(corsar_code > (const v_4_u32){0, 0x7f, 0x7ff, 0xfffful});
}

static inline u8 ctf8_char_to_corsar_code(const u8* const str, u32* corsar_code) {
     typedef u8  v_4_u8  __attribute__((ext_vector_type(4), aligned(1)));
     typedef u32 v_4_u32 __attribute__((ext_vector_type(4)));

     u32       local_corsar_code = (u8)(str[0]);
     u8  const char_size         = -__builtin_reduce_add((u8)local_corsar_code > (const v_4_u8){0, 0x7f, 0x9f, 0xaf});
     switch (char_size) {
     case 0:
          return 0;
     case 1:
          break;
     case 2:
          local_corsar_code = (local_corsar_code & 0x1f) << 6 | (u8)(str[1]) & 0x3f;
          break;
     case 3:
          local_corsar_code = (local_corsar_code & 0xf) << 12 | ((u8)(str[1]) & 0x3f) << 6 | (u8)(str[2]) & 0x3f;
          break;
     case 4: {
          local_corsar_code = __builtin_reduce_or((__builtin_convertvector(*(const v_4_u8*)str, v_4_u32) & (const v_4_u32){0xf, 0x3f, 0x3f, 0x3f}) << (const v_4_u32){18, 12, 6, 0});
          break;
     }
     default:
          unreachable;
     }

     assert(corsar_code_is_correct(local_corsar_code));

     *corsar_code = local_corsar_code;
     return char_size;
}

static inline u8 ctf8_char_to_corsar_char(const u8* const str, u8* corsar_char) {
     u32       corsar_code;
     u8  const char_size = ctf8_char_to_corsar_code(str, &corsar_code);

     if (char_size != 0) {
          corsar_code = __builtin_bswap32(corsar_code) >> 8;
          memcpy_inline(corsar_char, &corsar_code, 3);
     }
     return char_size;
}

static inline u64 ctf8_str_ze_lt16_to_corsar_chars(const u8* str, u8* chars) {
     u64 len = 0;
     while (true) {
          u8 const char_size = ctf8_char_to_corsar_char(str, chars);
          if (char_size == 0) {
               assert(len < 16);

               return len;
          }

          str   = &str[char_size];
          chars = &chars[3];
          len  += 1;
     }
}

static inline u8 corsar_code_to_ctf8_char(u32 const corsar_code, u8* const ctf8_char) {
     assert(corsar_code_is_correct(corsar_code));

     typedef u32 v_4_u32 __attribute__((ext_vector_type(4)));

     u64 const char_size = char_size_in_ctf8(corsar_code);
     switch (char_size) {
     case 1:
          ctf8_char[0] = corsar_code;
          break;
     case 2:
          ctf8_char[0] = corsar_code >> 6 | 0x80;
          ctf8_char[1] = corsar_code & 0x3f | 0xc0;
          break;
     case 3:
          ctf8_char[0] = corsar_code >> 12 | 0xa0;
          ctf8_char[1] = corsar_code >> 6 & 0x3f | 0xc0;
          ctf8_char[2] = corsar_code & 0x3f | 0xc0;
          break;
     case 4:
          *(ua_u32*)ctf8_char = __builtin_reduce_or(
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
     return char_size;
}
