#pragma once

#include "../../common/const.h"
#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"

[[clang::always_inline]]
core_basic t_any breaker__get_result__own(t_thrd_data* const thrd_data, t_any breaker) {
     assume(breaker.bytes[15] == tid__breaker);

     u8 const idx = breaker.bytes[14];

     assume(idx < breakers_max_qty);

     memcpy_inline(&breaker.bytes[14], &thrd_data->breakers_data[idx], 2);
     thrd_data->free_breakers ^= (u32)1 << idx;

     return breaker;
}

core_basic inline t_any breaker__new(t_thrd_data* const thrd_data, t_any result, const char* const owner) {
     if (thrd_data->free_breakers == 0 || result.bytes[15] == tid__error) [[clang::unlikely]] {
          if (result.bytes[15] == tid__error) return result;

          fail_with_call_stack(thrd_data, "Number of breakers exceeded.", owner);
     }

     u8 const idx              = __builtin_ctzl(thrd_data->free_breakers);
     thrd_data->free_breakers ^= (u32)1 << idx;

     memcpy_inline(&thrd_data->breakers_data[idx], &result.bytes[14], 2);
     result.bytes[14] = idx;
     result.bytes[15] = tid__breaker;

     return result;
}