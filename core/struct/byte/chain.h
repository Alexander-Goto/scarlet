#pragma once

#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/type.h"

[[clang::always_inline]]
static u8 byte__chain_lshr(u8 const num, u8 const shifts) {
     return shifts < 8 ? num >> shifts : 0;
}

[[clang::always_inline]]
static u8 byte__chain_shl(u8 const num, u8 const shifts) {
     return shifts < 8 ? num << shifts : 0;
}

[[clang::always_inline]]
static u8 byte__chain_exp2(u8 const power) {
     return power < 8 ? 1 << power : 0;
}

[[clang::always_inline]]
static u8 byte__chain_power(u8 const num, u8 const power) {
     u8 result                   = 1;
     u8 num_pow_of_2_pow_of_step = num;
     u8 step_pow                 = power;
     do {
          if ((step_pow & 1) == 1) result *= num_pow_of_2_pow_of_step;

          step_pow                >>= 1;
          num_pow_of_2_pow_of_step *= num_pow_of_2_pow_of_step;
     } while (step_pow != 0);

     return result;
}

[[clang::always_inline]]
static u8 byte__chain_sqr(u8 const num) {
     return num * num;
}

[[clang::always_inline]]
static u8 byte__chain_max(u8 const num_1, u8 const num_2) {
     return num_1 > num_2 ? num_1 : num_2;
}

[[clang::always_inline]]
static u8 byte__chain_min(u8 const num_1, u8 const num_2) {
     return num_1 < num_2 ? num_1 : num_2;
}