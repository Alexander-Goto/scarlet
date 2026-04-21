#pragma once

#include "../../common/fn.h"
#include "../../common/type.h"
#include "../../struct/common/slice.h"

#ifndef EXPORT_CORE_BASIC
static
#endif
t_any        const empty_array_data = {};
static t_any const empty_array      = {.structure = {.value = (u64)&empty_array_data, .type = tid__array}};

static t_any array__init(t_thrd_data* const thrd_data, u64 const cap, const char* const owner) {
     if (cap > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);

     if (cap == 0) return empty_array;

     t_any array = {.structure = {.value = (u64)aligned_alloc(16, (cap + 1) * 16), .type = tid__array}};
     set_ref_cnt(array, 1);
     slice_array__set_len(array, 0);
     slice_array__set_cap(array, cap);
     return array;
}