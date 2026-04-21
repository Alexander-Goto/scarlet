#pragma once

#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/type.h"

[[clang::always_inline]]
static u64 int__chain_ashr(u64 const num, u64 const shifts) {
     return shifts < 64 ? (i64)num >> (i64)shifts : ((i64)num < 0 ? (u64)-1 : 0);
}

[[clang::always_inline]]
static u64 int__chain_div(u64 const left, u64 const right) {
     if (left == (u64)-9223372036854775808wb && right == -1)
          return (u64)-9223372036854775808wb;

     return (i64)left / (i64)right;
}

[[clang::always_inline]]
static u64 int__chain_mod(u64 const left, u64 const right) {
     if (left == (u64)-9223372036854775808wb && right == -1)
          return 0;

     return (i64)left % (i64)right;
}

[[clang::always_inline]]
static u64 int__chain_lshr(u64 const num, u64 const shifts) {
     return shifts < 64 ? num >> shifts : 0;
}

[[clang::always_inline]]
static u64 int__chain_shl(u64 const num, u64 const shifts) {
     return shifts < 64 ? num << shifts : 0;
}

[[clang::always_inline]]
static u64 int__chain_abs(u64 const num) {
     return (i64)num < 0 ? -num : num;
}

[[clang::always_inline]]
static u64 int__chain_exp2(u64 const power) {
     return power < 64 ? (u64)1 << power : 0;
}

[[clang::always_inline]]
static u64 int__chain_power(u64 const num, u64 const power) {
     u64 result   = 0;
     u64 step_pow = power;
     if ((i64)step_pow >= 0) {
          result                       = 1;
          u64 num_pow_of_2_pow_of_step = num;
          do {
               if ((step_pow & 1) == 1) result *= num_pow_of_2_pow_of_step;

               step_pow                >>= 1;
               num_pow_of_2_pow_of_step *= num_pow_of_2_pow_of_step;
          } while (step_pow != 0);
     }

     return result;
}

[[clang::always_inline]]
static u64 int__chain_sqr(u64 const num) {
     return num * num;
}

[[clang::always_inline]]
static u64 int__chain_max(u64 const num_1, u64 const num_2) {
     return (i64)num_1 > (i64)num_2 ? num_1 : num_2;
}

[[clang::always_inline]]
static u64 int__chain_min(u64 const num_1, u64 const num_2) {
     return (i64)num_1 < (i64)num_2 ? num_1 : num_2;
}