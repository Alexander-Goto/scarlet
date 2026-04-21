#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"

[[clang::always_inline]]
core t_any McoreFNscarlet__joinAAA(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.scarlet__join!!!";

     t_any const qword_1 = args[0];
     t_any const qword_2 = args[1];

     t_any const result = (const t_any){.qwords = {qword_1.qwords[0], qword_2.qwords[0]}};

     call_stack__push(thrd_data, owner);
     ref_cnt__inc(thrd_data, result, owner);
     call_stack__pop(thrd_data);

     ref_cnt__dec(thrd_data, qword_1);
     ref_cnt__dec(thrd_data, qword_2);
     return result;
}
