#pragma once

#include "../../common/corsar.h"
#include "../../common/macro.h"
#include "../../common/type.h"

[[clang::always_inline]]
static u32 char_to_code(const u8* const chr) {
     u32 code;
     memcpy_inline(&code, chr, 3);
     code = __builtin_bswap32(code) >> 8;

     assert(corsar_code_is_correct(code));

     return code;
}

[[clang::always_inline]]
static void char_from_code(u8* const chr, u32 code) {
     assert(corsar_code_is_correct(code));

     code = __builtin_bswap32(code) >> 8;
     memcpy_inline(chr, &code, 3);
}