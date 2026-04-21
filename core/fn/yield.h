#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/type.h"
#include "../struct/null/basic.h"

[[clang::always_inline]]
core t_any McoreFNyield(t_thrd_data* const, const t_any*) {
     thrd_yield();
     return null;
}