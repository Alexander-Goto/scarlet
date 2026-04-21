#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"
#include "../struct/error/fn.h"

[[clang::always_inline]]
core t_any McoreFNerrorB(t_thrd_data* const thrd_data, const t_any* const args) {
     t_any const arg = args[0];
     ref_cnt__dec(thrd_data, arg);

     return arg.bytes[15] == tid__error ? bool__true : bool__false;
}
