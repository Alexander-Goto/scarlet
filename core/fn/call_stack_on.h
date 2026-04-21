#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"

[[clang::always_inline]]
core t_any McoreFNcall_stack_onB(t_thrd_data* const, const t_any* const) {
#ifdef CALL_STACK
     return bool__true;
#else
     return bool__false;
#endif
}