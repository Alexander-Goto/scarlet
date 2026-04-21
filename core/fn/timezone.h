#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/type.h"

[[clang::always_inline]]
core t_any McoreFNtimezone(t_thrd_data* const, const t_any*) {
     return (const t_any){.structure = {.value = -timezone, .type = tid__int}};
}