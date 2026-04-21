#pragma once

#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../../struct/array/basic.h"
#include "../../struct/box/basic.h"
#include "../../struct/error/fn.h"
#include "basic.h"

static t_any token__print(t_thrd_data* const thrd_data, t_any const token, const char* const owner, FILE* const file) {
     char chars[26];
     u8   len;

     token_to_ascii_chars(token, &len, chars);

     if (fprintf(file, "%.*s", (int)len, chars) < 0) [[clang::unlikely]]
          return error__cant_print(thrd_data, owner);
     return null;
}

[[clang::always_inline]]
static u64 token__hash(t_any const token, u64 const seed) {
     u64 low_part  = token.qwords[0];
     u64 high_part = token.qwords[1];
     low_part      = low_part >> 2 | high_part << 62;
     high_part     = (high_part & (((u64)1 << 56) - 1)) | low_part << 56;

     low_part  &= seed;
     high_part &= ~seed;

     return low_part | high_part;
}