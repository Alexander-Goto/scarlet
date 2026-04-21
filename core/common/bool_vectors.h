#pragma once

#include "include.h"
#include "macro.h"
#include "type.h"

typedef bool v_8_bool  __attribute__((ext_vector_type(8)));
typedef bool v_16_bool __attribute__((ext_vector_type(16)));
typedef bool v_32_bool __attribute__((ext_vector_type(32)));
typedef bool v_64_bool __attribute__((ext_vector_type(64)));

[[clang::always_inline]]
static u8 v_8_bool_to_u8(v_8_bool const vec) {
     assert(sizeof(v_8_bool) == 1);

     u8 result;
     memcpy_inline(&result, &vec, 1);
     return result;
}

[[clang::always_inline]]
static v_8_bool u8_to_v_8_bool(u8 const mask) {
     assert(sizeof(v_8_bool) == 1);

     v_8_bool result;
     memcpy_inline(&result, &mask, 1);
     return result;
}

[[clang::always_inline]]
static u16 v_16_bool_to_u16(v_16_bool const vec) {
     assert(sizeof(v_16_bool) == 2);

     u16 result;
     memcpy_inline(&result, &vec, 2);
     return result;
}

[[clang::always_inline]]
static v_16_bool u16_to_v_16_bool(u16 const mask) {
     assert(sizeof(v_16_bool) == 2);

     v_16_bool result;
     memcpy_inline(&result, &mask, 2);
     return result;
}

[[clang::always_inline]]
static u32 v_32_bool_to_u32(v_32_bool const vec) {
     assert(sizeof(v_32_bool) == 4);

     u32 result;
     memcpy_inline(&result, &vec, 4);
     return result;
}

[[clang::always_inline]]
static v_32_bool u32_to_v_32_bool(u32 const mask) {
     assert(sizeof(v_32_bool) == 4);

     v_32_bool result;
     memcpy_inline(&result, &mask, 4);
     return result;
}

[[clang::always_inline]]
static u64 v_64_bool_to_u64(v_64_bool const vec) {
     assert(sizeof(v_64_bool) == 8);

     u64 result;
     memcpy_inline(&result, &vec, 8);
     return result;
}

[[clang::always_inline]]
static v_64_bool u64_to_v_64_bool(u64 const mask) {
     assert(sizeof(v_64_bool) == 8);

     v_64_bool result;
     memcpy_inline(&result, &mask, 8);
     return result;
}