#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/fn.h"
#include "../struct/string/basic.h"
#include "../struct/string/cvt.h"

core t_any McoreFNustr_to_str(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.ustr_to_str";

     t_any const utf8_string = args[0];

     const u8* utf8_chars;
     u64       utf8_size;
     bool      need_to_free;
     switch (utf8_string.bytes[15]) {
     case tid__short_byte_array:
          utf8_chars   = utf8_string.bytes;
          utf8_size    = short_byte_array__get_len(utf8_string);
          need_to_free = false;
          break;
     case tid__byte_array: {
          u32 const slice_offset = slice__get_offset(utf8_string);
          u32 const slice_len    = slice__get_len(utf8_string);
          u64 const ref_cnt      = get_ref_cnt(utf8_string);
          u64 const array_len    = slice_array__get_len(utf8_string);
          utf8_chars             = &((const u8*)slice_array__get_items(utf8_string))[slice_offset];
          utf8_size              = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(utf8_string, ref_cnt - 1);
          need_to_free = ref_cnt == 1;
          break;
     }
     [[clang::unlikely]]
     case tid__error:
          return utf8_string;
     [[clang::unlikely]]
     default: {
          ref_cnt__dec(thrd_data, utf8_string);

          call_stack__push(thrd_data, owner);
          t_any const error = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return error;

     }}

     t_any result = {};
     if (utf8_size < 30) {
          t_str_cvt_result const cvt_result = utf8_chars_to_ctf8_chars(utf8_size, utf8_chars, 15, result.bytes, sys_char_codes);
          if (cvt_result.src_offset == utf8_size) {
               if (need_to_free) free((void*)utf8_string.qwords[0]);
               return result;
          }

          switch (cvt_result.src_offset) {
          case str_cvt_err__recoding:
               if (need_to_free) free((void*)utf8_string.qwords[0]);
               return null;
          [[clang::unlikely]]
          case 0: case str_cvt_err__encoding:
               if (need_to_free) free((void*)utf8_string.qwords[0]);
               goto invalid_enc_label;
          }
     }

     result                            = long_string__new(utf8_size);
     t_str_cvt_result const cvt_result = utf8_chars_to_corsar_chars(utf8_size, utf8_chars, utf8_size * 3, slice_array__get_items(result), sys_char_codes);
     if (need_to_free) free((void*)utf8_string.qwords[0]);

     if (cvt_result.src_offset != utf8_size) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, result);

          if (cvt_result.src_offset == str_cvt_err__recoding)
               return null;

          goto invalid_enc_label;
     }

     u64 const result_len = cvt_result.dst_offset / 3;
     if (utf8_size != result_len)
          result.qwords[0] = (u64)realloc((u8*)result.qwords[0], cvt_result.dst_offset + 16);

     result = slice__set_metadata(result, 0, result_len <= slice_max_len ? result_len : slice_max_len + 1);
     slice_array__set_cap(result, result_len);
     slice_array__set_len(result, result_len);

     return result;

     invalid_enc_label:
     call_stack__push(thrd_data, owner);
     t_any const error = error__invalid_enc(thrd_data, owner);
     call_stack__pop(thrd_data);
     return error;
}