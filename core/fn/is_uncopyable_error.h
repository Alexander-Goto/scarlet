#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/error/fn.h"

core t_any McoreFNuncopyable_errorB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.uncopyable_error?";

     if (args->bytes[15] == tid__error) [[clang::unlikely]] return args[0];

     call_stack__push(thrd_data, owner);
     t_any const result_box = box__new(thrd_data, 2, owner);
     call_stack__pop(thrd_data);

     t_any* const result_items = box__get_items(result_box);
     result_items[0]           = args[0];
     result_items[1].bytes[15] = tid__null;
     return result_box;
}