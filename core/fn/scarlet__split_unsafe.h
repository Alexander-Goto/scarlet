#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"

[[clang::always_inline]]
core t_any McoreFNscarlet__splitAAA(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.scarlet__split!!!";

     t_any const arg = args[0];
     ref_cnt__dec(thrd_data, arg);

     call_stack__push(thrd_data, owner);
     t_any const result = box__new(thrd_data, 2, owner);
     call_stack__pop(thrd_data);

     t_any* const items = box__get_items(result);
     items[0]           = (const t_any){.structure = {.value = arg.qwords[0], .type = tid__int}};
     items[1]           = (const t_any){.structure = {.value = arg.qwords[1], .type = tid__int}};
     return result;
}
