#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/fn.h"
#include "../struct/time/fn.h"

core t_any McoreFNassemble_time(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.assemble_time";

     t_any const time_zone = args[0];
     t_any const year      = args[1];
     t_any const month     = args[2];
     t_any const day       = args[3];
     t_any const hour      = args[4];
     t_any const minute    = args[5];
     t_any const second    = args[6];
     if (
          time_zone.bytes[15]     != tid__int ||
          year.bytes[15]          != tid__int ||
          month.bytes[15]         != tid__int ||
          day.bytes[15]           != tid__int ||
          hour.bytes[15]          != tid__int ||
          minute.bytes[15]        != tid__int ||
          second.bytes[15]        != tid__int ||
          (i64)time_zone.qwords[0] < -86400ll ||
          (i64)time_zone.qwords[0] > 86400ll
     ) [[clang::unlikely]] {
          if (time_zone.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, year);
               ref_cnt__dec(thrd_data, month);
               ref_cnt__dec(thrd_data, day);
               ref_cnt__dec(thrd_data, hour);
               ref_cnt__dec(thrd_data, minute);
               ref_cnt__dec(thrd_data, second);
               return time_zone;
          }
          ref_cnt__dec(thrd_data, time_zone);

          if (year.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, month);
               ref_cnt__dec(thrd_data, day);
               ref_cnt__dec(thrd_data, hour);
               ref_cnt__dec(thrd_data, minute);
               ref_cnt__dec(thrd_data, second);
               return year;
          }
          ref_cnt__dec(thrd_data, year);

          if (month.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, day);
               ref_cnt__dec(thrd_data, hour);
               ref_cnt__dec(thrd_data, minute);
               ref_cnt__dec(thrd_data, second);
               return month;
          }
          ref_cnt__dec(thrd_data, month);

          if (day.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, hour);
               ref_cnt__dec(thrd_data, minute);
               ref_cnt__dec(thrd_data, second);
               return day;
          }
          ref_cnt__dec(thrd_data, day);

          if (hour.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, minute);
               ref_cnt__dec(thrd_data, second);
               return hour;
          }
          ref_cnt__dec(thrd_data, hour);

          if (minute.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, second);
               return minute;
          }
          ref_cnt__dec(thrd_data, minute);

          if (second.bytes[15] == tid__error) return second;
          ref_cnt__dec(thrd_data, second);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (
               time_zone.bytes[15] != tid__int ||
               year.bytes[15]      != tid__int ||
               month.bytes[15]     != tid__int ||
               day.bytes[15]       != tid__int ||
               hour.bytes[15]      != tid__int ||
               minute.bytes[15]    != tid__int ||
               second.bytes[15]    != tid__int
          ) result = error__invalid_type(thrd_data, owner);
          else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     if (
          (i64)year.qwords[0] <  1969ll          ||
          (i64)year.qwords[0] >  292277026596ll  ||
          month.qwords[0]     >  12              ||
          month.qwords[0]     == 0               ||
          day.qwords[0]       >  31              ||
          day.qwords[0]       == 0               ||
          hour.qwords[0]      >  23              ||
          minute.qwords[0]    >  59              ||
          second.qwords[0]    >  59
     ) return null;

     u64 const time_value = time__asm(time_zone.qwords[0], year.qwords[0], month.bytes[0], day.bytes[0], hour.bytes[0], minute.bytes[0], second.bytes[0]);
     if (time_value == (u64)-1) return null;

     return (const t_any){.structure = {.value = time_value, .type = tid__time}};
}