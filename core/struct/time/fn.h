#pragma once

#include "../../common/corsar.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../../struct/error/fn.h"

static void time__disasm(t_any const time, i64 const time_zone, u64* const year, u8* const month, u8* const day, u8* const hour, u8* const minute, u8* const second) {
     assume(time.bytes[15] == tid__time);
     assume(time_zone >= -86400ll && time_zone <= 86400ll);
     assume(time.qwords[0] < 9223372036825516800);

     u64 const days_in_400_years    = 146097ull;
     u64 const days_in_leap_century = 36525ull;
     u64 const days_in_century      = 36524ull;
     u64 const days_in_leap_4_years = 1461;
     u64 const days_in_4_years      = 1460;

     if (-time_zone > (i64)time.qwords[0]) [[clang::unlikely]] {
          u32 const remain_seconds = time.qwords[0] + (u64)86400ull + (u64)time_zone;
          u32 const remain_minutes = remain_seconds / 60;

          *second = remain_seconds % 60;
          *hour   = remain_minutes / 60;
          *minute = remain_minutes % 60;
          *day    = 31;
          *month  = 12;
          *year   = 1969;
          return;
     }

     u64 const all_seconds = time.qwords[0] + (u64)time_zone;
     u64 const all_minutes = all_seconds / 60;
     *second               = all_seconds % 60;
     *minute               = all_minutes % 60;
     *hour                 = all_minutes % 1440 / 60;
     u64       remain_days = all_minutes / 1440;

     if (remain_days >= 10957) {
          *year        = 2000;
          remain_days -= 10957;

          *year       += remain_days / days_in_400_years * 400;
          remain_days %= days_in_400_years;

          if (remain_days >= days_in_leap_century) {
               *year       += 100;
               remain_days -= days_in_leap_century;
          }

          *year       += remain_days / days_in_century * 100;
          remain_days %= days_in_century;

          if (!(*year % 400 == 0 || (*year % 4 == 0 && *year % 100 != 0)) && remain_days >= days_in_4_years) {
               *year       += 4;
               remain_days -= days_in_4_years;
          }

          *year       += remain_days / days_in_leap_4_years * 4;
          remain_days %= days_in_leap_4_years;
     } else if (remain_days >= 730) {
          *year        = 1972;
          remain_days -= 730;

          *year       += remain_days / days_in_leap_4_years * 4;
          remain_days %= days_in_leap_4_years;
     } else *year = 1970;

     u64 days_in_year = *year % 400 == 0 || (*year % 4 == 0 && *year % 100 != 0) ? 366 : 365;
     for (
          u8 _cnt = 0;
          _cnt != 3 && remain_days >= days_in_year;

          remain_days -= days_in_year,
          *year       += 1,
          days_in_year = 365,
          _cnt        += 1
     );

     bool const year_is_leap = days_in_year == 366;

     typedef u16 t_vec __attribute__((ext_vector_type(16)));

     t_vec const days_in_months = year_is_leap ? (const t_vec){31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366, -1, -1, -1, -1} : (const t_vec){31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, -1, -1, -1, -1};
     u64   const leap_day       = year_is_leap;

     switch (-__builtin_reduce_add((u16)remain_days >= days_in_months)) {
     case 0:
          *month = 1;
          break;
     case 1:
          *month       = 2;
          remain_days -= 31;
          break;
     case 2:
          *month       = 3;
          remain_days -= 59 + leap_day;
          break;
     case 3:
          *month       = 4;
          remain_days -= 90 + leap_day;
          break;
     case 4:
          *month       = 5;
          remain_days -= 120 + leap_day;
          break;
     case 5:
          *month       = 6;
          remain_days -= 151 + leap_day;
          break;
     case 6:
          *month       = 7;
          remain_days -= 181 + leap_day;
          break;
     case 7:
          *month       = 8;
          remain_days -= 212 + leap_day;
          break;
     case 8:
          *month       = 9;
          remain_days -= 243 + leap_day;
          break;
     case 9:
          *month       = 10;
          remain_days -= 273 + leap_day;
          break;
     case 10:
          *month       = 11;
          remain_days -= 304 + leap_day;
          break;
     case 11:
          *month       = 12;
          remain_days -= 334 + leap_day;
          break;
     default:
          unreachable;
     }

     *day = remain_days + 1;

     assert(*year > 0);

     assert(*month  >= 1 && *month <= 12);
     assert(*day    >= 1 && *day   <= 31);
     assert(*hour   <  24);
     assert(*minute <  60);
     assert(*second <  60);
}

static u64 time__asm(i64 const time_zone, u64 const year, u8 const month, u8 const day, u8 const hour, u8 const minute, u8 const second) {
     assert(month != 0 && month <= 12);
     assert(day != 0 && day <= 31);
     assert(hour < 24);
     assert(minute < 60);
     assert(second < 60);
     assert(year >= 1969 && year <= 292277026596);

     u64 const seconds_in_400_years    = 12622780800ull;
     u64 const seconds_in_leap_century = 3155760000ull;
     u64 const seconds_in_century      = 3155673600ull;
     u64 const seconds_in_leap_4_years = 126230400ull;
     u64 const seconds_in_4_years      = 126144000ull;

     u32 const seconds_in_months[12] = {0, 2678400ul, 5097600ul, 7776000ul, 10368000ul, 13046400ul, 15638400ul, 18316800ul, 20995200ul, 23587200ul, 26265600ul, 28857600ul};
     u64 const days_in_moths[12]     = {31, 28, 31, 30, 31, 30, 31, 30, 30, 31, 30, 31};

     u64 result;
     u64 current_year;
     if (year >= 2000) {
          result       = 946684800;
          current_year = 2000;

          result       += (year - 2000) / 400 * seconds_in_400_years;
          current_year = year - year % 400;

          if (year - current_year >= 100) {
               result       += seconds_in_leap_century;
               current_year += 100;
          }

          if (year - current_year >= 100) {
               result      += (year - current_year) / 100 * seconds_in_century;
               current_year = year - year % 100;
          }
     } else if (year >= 1972) {
          result       = 63072000;
          current_year = 1972;
     } else if (year != 1969) {
          result       = 0;
          current_year = 1970;
     } else {
          if (day > days_in_moths[month - 1] || time_zone >= 0) return (u64)-1;

          result = 31536000ul - (seconds_in_months[month - 1] + (u32)(day - 1) * (u32)86400ul + hour * (u32)3600 + minute * (u32)60 + second);
          if (result > (u64)-time_zone) return (u64)-1;
          result += (u64)time_zone;

          return result;
     }

     if (!(current_year % 400 == 0 || (current_year % 4 == 0 && current_year % 100 != 0)) && year - current_year >= 4) {
          result       += seconds_in_4_years;
          current_year += 4;
     }

     if (year - current_year >= 4) {
          result      += (year - current_year) / 4 * seconds_in_leap_4_years;
          current_year = year - year % 4;
     }

     assume(year - current_year < 4);

     u64 seconds_in_year = current_year % 400 == 0 || (current_year % 4 == 0 && current_year % 100 != 0) ? 31622400ull : 31536000ull;
     while (current_year < year) {
          result         += seconds_in_year;
          current_year   += 1;
          seconds_in_year = 31536000ull;
     }

     bool const year_is_leap = seconds_in_year == 31622400ull;
     if (day > days_in_moths[month - 1] + (u32)(year_is_leap && month == 2)) return (u64)-1;

     result += seconds_in_months[month - 1] + (u32)(day - (u8)(year_is_leap && month > 2 ? 0 : 1)) * (u32)86400ul + hour * (u32)3600 + minute * (u32)60 + second;
     result -= (u64)time_zone;

     if (result >= 9223372036825516800) return (u64)-1;

     return result;
}

static t_any time__print(t_thrd_data* const thrd_data, t_any const time, const char* const owner, FILE* const file) {
     assume(time.bytes[15] == tid__time);

     u64 year;
     u8  month;
     u8  day;
     u8  hour;
     u8  minute;
     u8  second;
     time__disasm(time, -timezone, &year, &month, &day, &hour, &minute, &second);

     if (fprintf(file, "%02"PRIu8".%02"PRIu8".%04"PRIi64" %02"PRIu8":%02"PRIu8":%02"PRIu8" %+li.%02i", day, month, year, hour, minute, second, (-timezone) / 3600, (int)((timezone < 0 ? -timezone:timezone) % 3600 / 36)) < 0) [[clang::unlikely]]
          return error__cant_print(thrd_data, owner);
     return null;
}
