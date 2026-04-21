#pragma once

#include "../../common/include.h"
#include "../../common/type.h"

[[clang::always_inline]]
static u8 byte__sqrt(u8 const byte) {
     typedef u8 t_vec __attribute__((ext_vector_type(16)));

     return __builtin_reduce_add(((const t_vec){0, 3, 8, 15, 24, 35, 48, 63, 80, 99, 120, 143, 168, 195, 224, 255} < byte) & 1);
}