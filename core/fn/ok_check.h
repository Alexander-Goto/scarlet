#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"
#include "fail.h"

[[clang::always_inline]]
core t_any McoreFNokA(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.ok!";

     if (args->bytes[15] != tid__error) [[clang::likely]] return args[0];

     t_any          const error      = args[0];
     const t_error* const error_data = (const t_error*)error.qwords[0];
     t_any          const msg        = error_data->msg;

     call_stack__push(thrd_data, owner);

     ref_cnt__inc(thrd_data, msg, owner);
     ref_cnt__dec(thrd_data, error);

     McoreFNfail(thrd_data, &msg);
}