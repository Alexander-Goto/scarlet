#pragma once

#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../common/slice.h"

[[clang::always_inline]]
static u8 short_byte_array__get_len(t_any const array) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8 const len = array.bytes[14];

     assume(len < 15);

     return len;
}

[[clang::always_inline]]
static t_any short_byte_array__set_len(t_any array, u8 const len) {
     assert(array.bytes[15] == tid__short_byte_array);
     assert(len < 15);

     array.bytes[14] = len;
     return array;
}

static t_any long_byte_array__new(u64 const cap) {
     assume(cap <= array_max_len);

     t_any array = {.structure = {.value = (u64)malloc(cap + 16), .type = tid__byte_array}};
     set_ref_cnt(array, 1);
     slice_array__set_len(array, 0);
     slice_array__set_cap(array, cap);
     return array;
}