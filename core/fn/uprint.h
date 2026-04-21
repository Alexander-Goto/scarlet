#pragma once

#include "../app/env/var.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/common/slice.h"
#include "../struct/error/fn.h"
#include "../struct/null/basic.h"
#include "../struct/string/check.h"
#include "../struct/string/cvt.h"

[[clang::noinline]]
static t_any ustring__print(t_thrd_data* const thrd_data, t_any const string, const char* const owner, FILE* const file) {
     assert(string.bytes[15] == tid__byte_array || string.bytes[15] == tid__short_byte_array);

     u8 buffer[1024];
     if (string.bytes[15] == tid__short_byte_array) {
          u64 const string_size = short_byte_array__get_len(string);
          if (check_utf8_chars(string_size, string.bytes) != string_size) [[clang::unlikely]] goto invalid_enc_label;

          if (sys_enc_is_utf8 || __builtin_reduce_and(string.vector < 128) != 0) {
               if (fwrite(string.bytes, 1, string_size, file) != string_size) [[clang::unlikely]] goto cant_print_label;
               return null;
          }
          t_str_cvt_result const cvt_result = utf8_chars_to_sys_chars(string_size, string.bytes, 1024, buffer);
          if (cvt_result.src_offset != string_size) [[clang::unlikely]] {
               assert(cvt_result.src_offset == str_cvt_err__recoding);
               goto fail_text_recoding_label;
          }

          if (fwrite(buffer, 1, cvt_result.dst_offset, file) != cvt_result.dst_offset) [[clang::unlikely]] goto cant_print_label;

          return null;
     }

     u32 const slice_offset    = slice__get_offset(string);
     u32 const slice_len       = slice__get_len(string);
     u64 const string_size     = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);
     u8* const chars           = &((u8*)slice_array__get_items(string))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (sys_enc_is_utf8) {
          if (check_utf8_chars(string_size, chars) != string_size) [[clang::unlikely]] goto invalid_enc_label;
          if (fwrite(chars, 1, string_size, file) != string_size) [[clang::unlikely]] goto cant_print_label;
          return null;
     }

     for (u64 string_offset = 0; string_offset < string_size;) {
          t_str_cvt_result const cvt_result = utf8_chars_to_sys_chars(string_size - string_offset, &chars[string_offset], 1024, buffer);

          if ((i64)cvt_result.src_offset <= 0) [[clang::unlikely]] {
               switch (cvt_result.src_offset) {
               case 0: case str_cvt_err__encoding:
                    goto invalid_enc_label;
               case str_cvt_err__recoding:
                    goto fail_text_recoding_label;
               default:
                    unreachable;
               }
          }

          if (fwrite(buffer, 1, cvt_result.dst_offset, file) != cvt_result.dst_offset) [[clang::unlikely]] goto cant_print_label;

          string_offset += cvt_result.src_offset;
     }

     return null;

     cant_print_label:
     return error__cant_print(thrd_data, owner);

     invalid_enc_label:
     return error__invalid_enc(thrd_data, owner);

     fail_text_recoding_label:
     return error__fail_text_recoding(thrd_data, owner);
}

core t_any McoreFNuprint(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.uprint";

     t_any const string = args[0];

     if (!(string.bytes[15] == tid__short_byte_array || string.bytes[15] == tid__byte_array)) [[clang::unlikely]] {
          if (string.bytes[15] == tid__error) return string;

          ref_cnt__dec(thrd_data, string);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     call_stack__push(thrd_data, owner);

     t_any result = ustring__print(thrd_data, string, owner, stdout);

     assert(result.bytes[15] == tid__null || result.bytes[15] == tid__error);

     ref_cnt__dec(thrd_data, string);

     if (result.bytes[15] == tid__null && fflush(stdout) != 0)
          result = error__cant_print(thrd_data, owner);

     call_stack__pop(thrd_data);

     return result;
}

core t_any McoreFNuprintln(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.uprintln";

     t_any const string = args[0];

     if (!(string.bytes[15] == tid__short_byte_array || string.bytes[15] == tid__byte_array)) [[clang::unlikely]] {
          if (string.bytes[15] == tid__error) return string;

          ref_cnt__dec(thrd_data, string);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     call_stack__push(thrd_data, owner);

     t_any result = ustring__print(thrd_data, string, owner, stdout);

     assert(result.bytes[15] == tid__null || result.bytes[15] == tid__error);

     ref_cnt__dec(thrd_data, string);

     if (result.bytes[15] == tid__null && fwrite("\n", 1, 1, stdout) != 1)
          result = error__cant_print(thrd_data, owner);

     call_stack__pop(thrd_data);

     return result;
}
