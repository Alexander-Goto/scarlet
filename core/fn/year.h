#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/time/fn.h"

core t_any McoreFNyear(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.year";

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

     u64 year;
     u8  month;
     u8  day;
     u8  hour;
     u8  minute;
     u8  second;
     time__disasm(time, time_zone.qwords[0], &year, &month, &day, &hour, &minute, &second);
     return (const t_any){.structure = {.value = year, .type = tid__int}};
}