#pragma once

#include "../include/basic.h"
#include "../include/include.h"

t_any MstdFNread_cli_line(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - std.read_cli_line";

     u8   buffer[256];
     u16  buffer_len = 0;
     bool is_end     = false;
     for (; buffer_len < 256; buffer_len += 1) {
          u8 const readed_len = fread(&buffer[buffer_len], 1, 1, stdin);

          if (readed_len != 1 || buffer[buffer_len] == '\n') {
               if (readed_len != 1 && feof(stdin) == 0) {
                    call_stack__pop_if_tail_call(thrd_data);
                    return null;
               }

               is_end = true;
               break;
          }
     }

     if (buffer_len == 0) {
          call_stack__pop_if_tail_call(thrd_data);
          return (const t_any){};
     }

     if (buffer_len <= 113) {
          t_any                  string     = {};
          t_str_cvt_result const cvt_result = sys_chars_to_ctf8_chars(buffer_len, buffer, 15, string.bytes);

          if (cvt_result.src_offset == buffer_len) {
               call_stack__pop_if_tail_call(thrd_data);
               return string;
          }

          if ((i64)cvt_result.src_offset <= 0) {
               call_stack__pop_if_tail_call(thrd_data);
               switch (cvt_result.src_offset) {
                    case 0: case str_cvt_err__encoding:
                         return null;
                    default:
                         unreachable;
               }
          }
     }

     u64   string_cap   = buffer_len * 2 + 7;
     t_any string       = long_string__new(string_cap);
     u8*   string_chars = slice_array__get_items(string);

     t_str_cvt_result current_offsets = {};
     while (true) {
          t_str_cvt_result const cvt_result = sys_chars_to_corsar_chars(buffer_len, buffer, string_cap * 3 - current_offsets.dst_offset, &string_chars[current_offsets.dst_offset]);

          if ((i64)cvt_result.src_offset <= 0) {
               call_stack__pop_if_tail_call(thrd_data);
               free((void*)string.qwords[0]);

               switch (cvt_result.src_offset) {
                    case 0: case str_cvt_err__encoding:
                         return null;
                    default:
                         unreachable;
               }
          }

          current_offsets.dst_offset += cvt_result.dst_offset;
          u64 const string_len        = current_offsets.dst_offset / 3;
          if (cvt_result.src_offset == buffer_len && is_end) {
               if (string_len != string_cap)
                    string.qwords[0] = (u64)realloc((u8*)string.qwords[0], current_offsets.dst_offset + 16);

               string = slice__set_metadata(string, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);
               slice_array__set_cap(string, string_len);
               slice_array__set_len(string, string_len);

               call_stack__pop_if_tail_call(thrd_data);
               return string;
          }

          u32 const buffer_remain_len = buffer_len - cvt_result.src_offset;
          memmove(buffer, &buffer[cvt_result.src_offset], buffer_remain_len);
          for (buffer_len = buffer_remain_len; buffer_len < 256; buffer_len += 1) {
               u8 const readed_len = fread(&buffer[buffer_len], 1, 1, stdin);
               if (readed_len != 1 || buffer[buffer_len] == '\n') {
                    if (readed_len != 1 && feof(stdin) == 0) {
                         call_stack__pop_if_tail_call(thrd_data);
                         free((void*)string.qwords[0]);

                         return null;
                    }

                    is_end = true;
                    break;
               }
          }

          if (buffer_len == 0 && is_end) {
               if (string_len != string_cap)
                    string.qwords[0] = (u64)realloc((u8*)string.qwords[0], current_offsets.dst_offset + 16);

               string = slice__set_metadata(string, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);
               slice_array__set_cap(string, string_len);
               slice_array__set_len(string, string_len);

               call_stack__pop_if_tail_call(thrd_data);
               return string;
          }

          if (string_len + buffer_len + 7 > string_cap) {
               if (string_cap == array_max_len) [[clang::unlikely]] {
                    call_stack__push(thrd_data, owner);
                    fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);
               }

               string_cap       = (string_len + buffer_len + 7) * 2;
               string_cap       = string_cap > array_max_len ? array_max_len : string_cap;
               string.qwords[0] = (u64)realloc((u8*)string.qwords[0], string_cap * 3 + 16);
               string_chars     = slice_array__get_items(string);
          }
     }
}