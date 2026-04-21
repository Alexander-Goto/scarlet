#pragma once

#ifdef __linux__

#include "../common/include.h"
#include "../common/type.h"

static bool is_sys_enc_ascii_comp(void) {
     char      sys_char[MB_CUR_MAX];
     mbstate_t state;
     for (char32_t code = 1; code < 128; code += 1) {
          state = (mbstate_t){};
          if (!(c32rtomb(sys_char, code, &state) == 1 && sys_char[0] == code)) return false;
     }

     return true;
}

[[clang::noinline]]
static void platform__check(void) {
     t_any const any_const = {.bytes = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 128}};
     if (!(
          (nullptr == NULL)                                                           &
          (u8)(CHAR_BIT == 8)                                                         &
          (u8)(MB_CUR_MAX <= 16)                                                      &
          is_sys_enc_ascii_comp()                                                     &
          sizeof(void*)               <= 8                                            &
          sizeof(t_any)               == 16                                           &
          sizeof(double)              == 8                                            &
          sizeof(long long)           == 8                                            &
          (u8)-1                      == 255                                          &
          (u64)-9223372036854775808wb == 9223372036854775808wb                        &
          any_const.raw_bits          == 0x800f'0e0d'0c0b'0a09'0807'0605'0403'0201uwb &
          any_const.qwords[0]         == 0x0807'0605'0403'0201ull                     &
          any_const.qwords[1]         == 0x800f'0e0d'0c0b'0a09ull                     &
          any_const.structure.value   == 0x0807'0605'0403'0201ull                     &
          any_const.structure.data[0] == 9                                            &
          any_const.structure.data[1] == 10                                           &
          any_const.structure.data[2] == 11                                           &
          any_const.structure.data[3] == 12                                           &
          any_const.structure.data[4] == 13                                           &
          any_const.structure.data[5] == 14                                           &
          any_const.structure.data[6] == 15                                           &
          any_const.structure.type    == 128                                          &
          (u64)(const t_ptr64){.bits = 0xfaafwb}.pointer               == 0xfaafwb    &
          (const t_any){.qwords = {0x4009'21fb'5444'2d18ull}}.floats[0] > 3.14        &
          (const t_any){.qwords = {0x4009'21fb'5444'2d18ull}}.floats[0] < 3.15        &
          __builtin_isnan(0.0/0.0)                                                    &
          __builtin_isinf(1.0/0.0)                                                    &
          __builtin_reduce_and(any_const.vector == (v_any_u8){1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 128}) == -1
     )) [[clang::unlikely]] {
          char        buffer[16];
          mbstate_t   state = {};
          size_t      idx   = 0;
          const char* msg   = "Unsupported platform.\n";
          while (msg[idx] != 0) {
               size_t const written_bytes = c8rtomb(buffer, msg[idx], &state);
               if (written_bytes == (const size_t)-1) exit(EXIT_FAILURE);

               if (written_bytes != 0) {
                    fwrite(buffer, 1, written_bytes, stderr);
                    idx += written_bytes;
               }
          }

          exit(EXIT_FAILURE);
     }
}

#else

static void platform__check(void) {
     char        buffer[16];
     mbstate_t   state = {};
     size_t      idx   = 0;
     const char* msg   = "Unsupported platform.\n";
     while (msg[idx] != 0) {
          size_t const written_bytes = c8rtomb(buffer, msg[idx], &state);
          if (written_bytes == (const size_t)-1) exit(EXIT_FAILURE);

          if (written_bytes != 0) {
               fwrite(buffer, 1, written_bytes, stdout);
               idx += written_bytes;
          }
     }

     exit(EXIT_FAILURE);
}

#endif