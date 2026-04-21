#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/time/fn.h"

core t_any McoreFNdisassemble_time(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.disassemble_time";

     t_any const time       = args[0];
     t_any const time_zone  = args[1];
     if (
          time.bytes[15] != tid__time        ||
          time_zone.bytes[15] != tid__int    ||
          (i64)time_zone.qwords[0] > 86400ll ||
          (i64)time_zone.qwords[0] < -86400ll
     ) [[clang::unlikely]] {
          if (time.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, time_zone);
               return time;
          }
          ref_cnt__dec(thrd_data, time);

          if (time_zone.bytes[15] == tid__error)
               return time_zone;
          ref_cnt__dec(thrd_data, time_zone);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (time.bytes[15] != tid__time || time_zone.bytes[15] != tid__int)
               result = error__invalid_type(thrd_data, owner);
          else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     typedef u64 t_small_vec __attribute__((ext_vector_type(8)));
     typedef u64 t_big_vec   __attribute__((ext_vector_type(16), aligned(ALIGN_FOR_CACHE_LINE)));

     [[gnu::aligned(alignof(t_small_vec))]]
     u64 components[8] = {};
     time__disasm(time, time_zone.qwords[0], (u64*)&components[0], (u8*)&components[1], (u8*)&components[2], (u8*)&components[3], (u8*)&components[4], (u8*)&components[5]);

     call_stack__push(thrd_data, owner);
     t_any box = box__new(thrd_data, 6, owner);
     call_stack__pop(thrd_data);

     *(t_big_vec*)box__get_items(box) = __builtin_shufflevector(*(const t_small_vec*)components, (const t_small_vec)((u64)tid__int << 56), 0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8);
     return box;
}