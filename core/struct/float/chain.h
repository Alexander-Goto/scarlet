#pragma once

#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/type.h"

[[clang::always_inline]]
static double float__chain_sqr(double const num) {
     return num * num;
}

[[clang::always_inline]]
static double float__chain_max(double const num_1, double const num_2) {
     return num_1 > num_2 ? num_1 : num_2;
}

[[clang::always_inline]]
static double float__chain_min(double const num_1, double const num_2) {
     return num_1 < num_2 ? num_1 : num_2;
}