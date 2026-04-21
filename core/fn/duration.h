#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

core t_any McoreFNduration(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.duration";

     t_any const time_1 = args[0];
     t_any const time_2 = args[1];
     if (time_1.bytes[15] != tid__time || time_2.bytes[15] != tid__time) [[clang::unlikely]] {
          if (time_1.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, time_2);
               return time_1;
          }
          ref_cnt__dec(thrd_data, time_1);

          if (time_2.bytes[15] == tid__error)
               return time_2;
          ref_cnt__dec(thrd_data, time_2);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     u64 const time_1_u64 = time_1.qwords[0];
     u64 const time_2_u64 = time_2.qwords[0];
     u64 const duration   = time_2_u64 > time_1_u64 ? time_2_u64 - time_1_u64 : time_1_u64 - time_2_u64;

     return (const t_any){.structure = {.value = duration, .type = tid__int}};
}
