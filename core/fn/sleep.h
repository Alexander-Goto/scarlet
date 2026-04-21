#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/basic.h"
#include "../struct/null/basic.h"

core t_any McoreFNsleep(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.sleep";

     t_any const all_nano_sec = args[0];
     if (all_nano_sec.bytes[15] != tid__int || (i64)all_nano_sec.qwords[0] < 0) [[clang::unlikely]] {
          if (all_nano_sec.bytes[15] == tid__error) return all_nano_sec;

          t_any result;
          call_stack__push(thrd_data, owner);
          if (all_nano_sec.bytes[15] != tid__int) {
               ref_cnt__dec(thrd_data, all_nano_sec);
               result = error__invalid_type(thrd_data, owner);
          } else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     u64       const sec      = all_nano_sec.qwords[0] / 1'000'000'000ull;
     u64       const nano_sec = all_nano_sec.qwords[0] % 1'000'000'000ull;
     struct timespec time     = {.tv_sec = sec, .tv_nsec = nano_sec};
     if (thrd_sleep(&time, &time) == 0) [[clang::likely]] return null;

     call_stack__push(thrd_data, owner);
     t_any const result = error__insomnia(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}