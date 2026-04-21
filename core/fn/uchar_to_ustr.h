#pragma once

#include "../common/corsar.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/fn.h"

core t_any McoreFNuchar_to_ustr(t_thrd_data* const thrd_data, const t_any* const args) {
     typedef u32 v_4_u32 __attribute__((ext_vector_type(4)));

     const char* const owner = "function - core.uchar_to_ustr";

     t_any const uchar     = args[0];
     u64   const uchar_u64 = uchar.qwords[0];
     if (uchar.bytes[15] != tid__int || uchar_u64 > 1'114'111ull || uchar_u64 == 0) [[clang::unlikely]] {
          if (uchar.bytes[15] == tid__error) return uchar;

          t_any result;
          call_stack__push(thrd_data, owner);
          if (uchar.bytes[15] != tid__int) {
               ref_cnt__dec(thrd_data, uchar);
               result = error__invalid_type(thrd_data, owner);
          } else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     u8  const utf8_char_size = -__builtin_reduce_add(uchar_u64 > (const v_4_u32){0, 0x7f, 0x7ff, 0xffff});
     t_any     result         = {.bytes = {[14] = utf8_char_size, [15] = tid__short_byte_array}};
     switch (utf8_char_size) {
     case 1:
          result.bytes[0] = uchar_u64;
          break;
     case 2:
          result.bytes[0] = uchar_u64 >> 6 | 0xc0;
          result.bytes[1] = uchar_u64 & 0x3f | 0x80;
          break;
     case 3:
          result.bytes[0] = uchar_u64 >> 12 | 0xe0;
          result.bytes[1] = uchar_u64 >> 6 & 0x3f | 0x80;
          result.bytes[2] = uchar_u64 & 0x3f | 0x80;
          break;
     case 4:
          *(ua_u32*)&result.bytes[0] = __builtin_reduce_or(
               (
                    (uchar_u64 >> (const v_4_u32){18, 12, 6, 0}) &
                    (const v_4_u32){7, 0x3f, 0x3f, 0x3f}            |
                    (const v_4_u32){0xf0, 0x80, 0x80, 0x80}
               ) << (const v_4_u32){0, 8, 16, 24}
          );
          break;
     default:
          unreachable;
     }

     return result;
}