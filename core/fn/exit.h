#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/type.h"

[[clang::noinline, noreturn]]
core t_any McoreFNexit(t_thrd_data* const thrd_data, const t_any*) {exit(EXIT_SUCCESS);}