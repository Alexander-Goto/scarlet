#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/type.h"
#include "../struct/int/fn.h"

[[clang::always_inline]]
core t_any McoreFNrnd(t_thrd_data* const thrd_data, const t_any*) {
     return (const t_any) {.structure = {.value = int__rnd(thrd_data), .type = tid__int}};
}