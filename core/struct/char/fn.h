#pragma once

#include "../../common/corsar.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../../struct/error/fn.h"
#include "basic.h"

static t_any char__print(t_thrd_data* const thrd_data, t_any const char_, const char* const owner, FILE* const file) {
     assume(char_.bytes[15] == tid__char);

     u32 const unicode_char = corsar_code_to_unicode(char_to_code(char_.bytes));
     if (unicode_char == 0) [[clang::unlikely]] goto fail_text_recoding_label;

     char         buffer[16];
     mbstate_t    state         = {};
     size_t const written_bytes = c32rtomb(buffer, unicode_char, &state);
     if ((ssize_t)written_bytes < 0) [[clang::unlikely]]
          goto fail_text_recoding_label;

     if (fwrite(buffer, 1, written_bytes, file) != written_bytes) [[clang::unlikely]]
          return error__cant_print(thrd_data, owner);

     return null;

     fail_text_recoding_label:
     return error__fail_text_recoding(thrd_data, owner);
}

static t_any char__to_lower(t_any char_) {
     assert(char_.bytes[15] == tid__char);

     t_any     result      = {};
     u64 const lower_chars = char_to_lower(char_to_code(char_.bytes));

     u8 offset = corsar_code_to_ctf8_char(lower_chars & 0x3f'ffffull, result.bytes);
     if (lower_chars > 0x3f'ffffull) {
          offset += corsar_code_to_ctf8_char((lower_chars >> 22) & 0x1f'ffffull, &result.bytes[offset]);

          if (lower_chars > 0x7ff'ffff'ffffull)
               corsar_code_to_ctf8_char(lower_chars >> 43, &result.bytes[offset]);
     }

     return result;
}

static t_any char__to_upper(t_any char_) {
     assert(char_.bytes[15] == tid__char);

     t_any     result      = {};
     u64 const lower_chars = char_to_upper(char_to_code(char_.bytes));

     u8 offset = corsar_code_to_ctf8_char(lower_chars & 0x3f'ffffull, result.bytes);
     if (lower_chars > 0x3f'ffffull) {
          offset += corsar_code_to_ctf8_char((lower_chars >> 22) & 0x1f'ffffull, &result.bytes[offset]);

          if (lower_chars > 0x7ff'ffff'ffffull)
               corsar_code_to_ctf8_char(lower_chars >> 43, &result.bytes[offset]);
     }

     return result;
}
