#pragma once

#include "../include/include.h"
#include "file.h"
#include "fs_path.h"

#define file_buffer_size 0x400'000ul

#define fom__read  (u8)1
#define fom__begin (u8)2
#define fom__write (u8)4
#define fom__end   (u8)8
#define fom__new   (u8)16

static const char* const file__mtd_to_cut       = "function - @std.file->cut";
static const char* const file__mtd_to_end       = "function - @std.file->end!";
static const char* const file__mtd_flush        = "function - @std.file->flush";
static const char* const file__mtd_read         = "function - @std.file->read";
static const char* const file__mtd_read_string  = "function - @std.file->read_string";
static const char* const file__mtd_read_tail    = "function - @std.file->read_tail";
static const char* const file__mtd_seek         = "function - @std.file->seek";
static const char* const file__mtd_to_start     = "function - @std.file->start!";
static const char* const file__mtd_tell         = "function - @std.file->tell";
static const char* const file__mtd_write        = "function - @std.file->write";
static const char* const file__mtd_write_string = "function - @std.file->write_string";

struct {
     t_custom_types_data custom_data;

#ifdef DEBUG
     t_any file_name;
#endif

     FILE* c_file;
     bool  is_flushed;
     bool  dont_close;
} typedef t_file;

[[clang::always_inline]]
static t_file* file__data(t_any const file) {return (t_file*)file.qwords[0];}

static void file__free(t_thrd_data* const, const t_any file) {
     t_file* const data = file__data(file);

#ifdef DEBUG
     fs_path__ref_cnt_dec(data->file_name);
#endif

     if (!data->is_flushed) [[clang::unlikely]]
          log__msg(log_lvl__may_require_attention, "Freeing an unflushed file.");

     if (!data->dont_close) fclose(data->c_file);
     free(data);
}

[[clang::always_inline]]
static void file__ref_cnt_dec(t_any const file) {
     u64* const ref_cnt = &file__data(file)->custom_data.ref_cnt;

     assert(*ref_cnt != 0);

     if (*ref_cnt == 1)
          file__free(nullptr, file);
     else *ref_cnt -= 1;
}

static t_any file__call_mtd(t_thrd_data* const thrd_data, const char* const owner, t_any const mtd_tkn, const t_any* const args, u8 const args_len) {
     const char* mtd_name;
     t_any const file = args[0];

     switch (mtd_tkn.raw_bits) {
     case 0x9e9a40617e3f1254f9247a6762a96f3uwb: { // cut
          t_file* const data = file__data(file);
          if (args_len != 1 || data->custom_data.ref_cnt != 1) [[clang::unlikely]] {
               mtd_name = file__mtd_to_cut;
               if (args_len != 1) goto wrong_num_fn_args;

               file__ref_cnt_dec(file);
               goto file_not_mut_label;
          }

          if (!data->is_flushed) {
               data->is_flushed = true;
               if (fflush(data->c_file) != 0) {
                    file__free(thrd_data, file);
                    return tkn__flush;
               }
          }

          int file_descriptor = fileno(data->c_file);

          assert(file_descriptor != -1);

          off_t const position = lseek(file_descriptor, 0, SEEK_CUR);
          if (position == (off_t)-1) {
               file__free(thrd_data, file);
               return tkn__cut;
          }

          if (ftruncate(file_descriptor, position) == 0) {
               data->is_flushed = false;
               return file;
          }

          file__free(thrd_data, file);
          return tkn__cut;
     }
     case 0x9a3305204291427e76f512b45a33e05uwb: { // end!
          t_file* const data = file__data(file);
          if (args_len != 1 || data->custom_data.ref_cnt != 1) [[clang::unlikely]] {
               mtd_name = file__mtd_to_end;
               if (args_len != 1) goto wrong_num_fn_args;

               file__ref_cnt_dec(file);
               goto file_not_mut_label;
          }

          if (!data->is_flushed) {
               data->is_flushed = true;
               if (fflush(data->c_file) != 0) {
                    file__free(thrd_data, file);
                    return tkn__flush;
               }
          }

          if (fseek(data->c_file, 0, SEEK_END) != 0) {
               file__free(thrd_data, file);
               return tkn__seek;
          }

          return file;
     }
     case 0x9b0885a478d1e7d0a40adc9dc46bbb3uwb: { // flush
          t_file* const data = file__data(file);
          if (args_len != 1 || data->custom_data.ref_cnt != 1) [[clang::unlikely]] {
               mtd_name = file__mtd_flush;
               if (args_len != 1) goto wrong_num_fn_args;

               file__ref_cnt_dec(file);
               goto file_not_mut_label;
          }

          if (!data->is_flushed) {
               data->is_flushed = true;
               if (fflush(data->c_file) != 0) {
                    file__free(thrd_data, file);
                    return null;
               }
          }

          return file;
     }
     case 0x9a78f2b273dba56085bd6eaef1b8d1buwb: { // read
          t_file* const data = file__data(file);
          if (args_len != 2 || data->custom_data.ref_cnt != 1 || !(args[1].bytes[15] == tid__byte_array || args[1].bytes[15] == tid__short_byte_array)) [[clang::unlikely]] {
               mtd_name = file__mtd_read;
               if (args_len != 2) goto wrong_num_fn_args;
               if (!(args[1].bytes[15] == tid__byte_array || args[1].bytes[15] == tid__short_byte_array)) goto invalid_type_label;

               file__ref_cnt_dec(file);
               ref_cnt__dec(thrd_data, args[1]);
               goto file_not_mut_label;
          }

          t_any        buffer       = args[1];
          t_any  const result       = box__new(thrd_data, 3, file__mtd_read);
          t_any* const result_items = box__get_items(result);
          if (!data->is_flushed) {
               data->is_flushed = true;
               if (fflush(data->c_file) != 0) {
                    file__free(thrd_data, file);
                    result_items[0] = tkn__flush;
                    result_items[1] = (const t_any){.bytes = {[15] = tid__int}};
                    result_items[2] = buffer;
                    return result;
               }
          }

          u8* bytes;
          u64 bytes_len;
          if (buffer.bytes[15] == tid__short_byte_array) {
               bytes     = buffer.bytes;
               bytes_len = short_byte_array__get_len(buffer);
          } else {
               u32 const slice_offset = slice__get_offset(buffer);
               u32 const slice_len    = slice__get_len(buffer);
               u64 const ref_cnt      = get_ref_cnt(buffer);
               u64 const array_len    = slice_array__get_len(buffer);
               bytes_len              = slice_len <= slice_max_len ? slice_len : array_len;

               assert(slice_len <= slice_max_len || slice_offset == 0);

               if (ref_cnt == 1)
                    bytes = &((u8*)slice_array__get_items(buffer))[slice_offset];
               else {
                    if (ref_cnt != 0) set_ref_cnt(buffer, ref_cnt - 1);
                    t_any new_buffer = long_byte_array__new(bytes_len);
                    new_buffer       = slice__set_metadata(new_buffer, 0, bytes_len <= slice_max_len ? bytes_len : slice_max_len + 1);
                    slice_array__set_len(new_buffer, bytes_len);
                    bytes = slice_array__get_items(new_buffer),
                    memcpy(bytes, &((u8*)slice_array__get_items(buffer))[slice_offset], bytes_len);
                    buffer = new_buffer;
               }
          }

          u64 readed_len  = fread(bytes, 1, bytes_len, data->c_file);
          result_items[1] = (const t_any){.structure = {.value = readed_len, .type = tid__int}};
          result_items[2] = buffer;
          if (readed_len == bytes_len || feof(data->c_file) != 0) {
               result_items[0] = file;
               return result;
          }

          file__free(thrd_data, file);
          result_items[0] = tkn__read;
          return result;
     }
     case 0x9adb25beb48af23a9a90478d90a2027uwb: { // read_string
          t_file* const data = file__data(file);
          if (args_len != 2 || data->custom_data.ref_cnt != 1 || !(args[1].raw_bits == tkn__ctf8.raw_bits || args[1].raw_bits == tkn__sys.raw_bits)) [[clang::unlikely]] {
               mtd_name = file__mtd_read_string;
               if (args_len != 2) goto wrong_num_fn_args;
               if (args[1].bytes[15] != tid__token) goto invalid_type_label;

               file__ref_cnt_dec(file);
               if (!(args[1].raw_bits == tkn__ctf8.raw_bits || args[1].raw_bits == tkn__sys.raw_bits)) {
                    call_stack__push(thrd_data, mtd_name);
                    t_any const error = builtin_error__new(thrd_data, mtd_name, "encoding", "Invalid encoding specified.");
                    call_stack__pop(thrd_data);

                    return error;
               }

               goto file_not_mut_label;
          }

          t_any  const encoding     = args[1];
          bool   const enc_is_sys   = encoding.bytes[0] == tkn__sys.bytes[0];
          t_any  const result       = box__new(thrd_data, 2, file__mtd_read_string);
          t_any* const result_items = box__get_items(result);
          if (!data->is_flushed) {
               data->is_flushed = true;
               if (fflush(data->c_file) != 0) {
                    file__free(thrd_data, file);

                    result_items[0] = tkn__flush;
                    result_items[1] = null;
                    return result;
               }
          }

          u64 const buffer_idx = linear__alloc(&thrd_data->linear_allocator, file_buffer_size, 1);
          u8* const buffer     = linear__get_mem_of_idx(&thrd_data->linear_allocator, buffer_idx);
          u32       readed_len = fread(buffer, 1, file_buffer_size, data->c_file);
          bool      is_eof     = feof(data->c_file) != 0;
          if (!(readed_len == file_buffer_size || is_eof)) {
               file__free(thrd_data, file);
               linear__free(&thrd_data->linear_allocator, buffer_idx);

               result_items[0] = tkn__read;
               result_items[1] = null;
               return result;
          }

          if (readed_len == 0) {
               linear__free(&thrd_data->linear_allocator, buffer_idx);

               result_items[0]          = file;
               result_items[1].raw_bits = 0;
               return result;
          }

          if (readed_len < 16 || (readed_len <= 113 && enc_is_sys)) {
               t_any            string = {};
               t_str_cvt_result cvt_result;
               if (enc_is_sys)
                    cvt_result = sys_chars_to_ctf8_chars(readed_len, buffer, 15, string.bytes);
               else if (check_ctf8_chars(readed_len, buffer) != readed_len)
                    cvt_result.src_offset = 0;
               else {
                    cvt_result.src_offset = readed_len;
                    memcpy_le_16(string.bytes, buffer, readed_len);
               }

               if (cvt_result.src_offset == readed_len) {
                    linear__free(&thrd_data->linear_allocator, buffer_idx);

                    result_items[0] = file;
                    result_items[1] = string;
                    return result;
               }

               if ((i64)cvt_result.src_offset <= 0) {
                    switch (cvt_result.src_offset) {
                    case 0: case str_cvt_err__encoding:
                         file__free(thrd_data, file);
                         linear__free(&thrd_data->linear_allocator, buffer_idx);

                         result_items[0] = tkn__encoding;
                         result_items[1] = null;
                         return result;
                    default:
                         unreachable;
                    }
               }
          }

          u64   string_cap   = readed_len * 2 + 7;
          string_cap         = string_cap > array_max_len ? array_max_len : string_cap;
          t_any string       = long_string__new(string_cap);
          u8*   string_chars = slice_array__get_items(string);

          t_str_cvt_result current_offsets = {};
          while (true) {
               t_str_cvt_result const cvt_result = enc_is_sys ?
                    sys_chars_to_corsar_chars(readed_len, buffer, string_cap * 3 - current_offsets.dst_offset, &string_chars[current_offsets.dst_offset]) :
                    ctf8_chars_to_corsar_chars(readed_len, buffer, string_cap * 3 - current_offsets.dst_offset, &string_chars[current_offsets.dst_offset]);

               if ((i64)cvt_result.src_offset <= 0) {
                    file__free(thrd_data, file);
                    linear__free(&thrd_data->linear_allocator, buffer_idx);
                    free((void*)string.qwords[0]);

                    switch (cvt_result.src_offset) {
                    case 0: case str_cvt_err__encoding:
                         result_items[0] = tkn__encoding;
                         result_items[1] = null;
                         return result;
                    default:
                         unreachable;
                    }
               }

               current_offsets.dst_offset += cvt_result.dst_offset;
               u64 const string_len        = current_offsets.dst_offset / 3;
               if (cvt_result.src_offset == readed_len && is_eof) {
                    linear__free(&thrd_data->linear_allocator, buffer_idx);

                    if (string_len != string_cap)
                         string.qwords[0] = (u64)realloc((u8*)string.qwords[0], current_offsets.dst_offset + 16);

                    string = slice__set_metadata(string, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);
                    slice_array__set_cap(string, string_len);
                    slice_array__set_len(string, string_len);

                    result_items[0] = file;
                    result_items[1] = string;
                    return result;
               }

               u32 const readed_remain_len = readed_len - cvt_result.src_offset;
               u32 const max_readed_len    = file_buffer_size - readed_remain_len;
               memmove(buffer, &buffer[cvt_result.src_offset], readed_remain_len);

               readed_len = fread(&buffer[readed_remain_len], 1, max_readed_len, data->c_file);
               is_eof     = feof(data->c_file) != 0;
               if (!(readed_len == max_readed_len || is_eof)) {
                    file__free(thrd_data, file);
                    linear__free(&thrd_data->linear_allocator, buffer_idx);
                    free((void*)string.qwords[0]);

                    result_items[0] = tkn__read;
                    result_items[1] = null;
                    return result;
               }

               readed_len += readed_remain_len;
               if (readed_len == 0 && is_eof) {
                    linear__free(&thrd_data->linear_allocator, buffer_idx);

                    if (string_len != string_cap)
                         string.qwords[0] = (u64)realloc((u8*)string.qwords[0], current_offsets.dst_offset + 16);

                    string = slice__set_metadata(string, 0, string_len <= slice_max_len ? string_len : slice_max_len + 1);
                    slice_array__set_cap(string, string_len);
                    slice_array__set_len(string, string_len);

                    result_items[0] = file;
                    result_items[1] = string;
                    return result;
               }

               if (string_len + readed_len + 7 > string_cap) {
                    if (string_cap == array_max_len) [[clang::unlikely]] {
                         call_stack__push(thrd_data, file__mtd_read_string);
                         fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", file__mtd_read_string);
                    }

                    string_cap       = (string_len + readed_len) * 3 / 2 + 7;
                    string_cap       = string_cap > array_max_len ? array_max_len : string_cap;
                    string.qwords[0] = (u64)realloc((u8*)string.qwords[0], string_cap * 3 + 16);
                    string_chars     = slice_array__get_items(string);
               }
          }
     }
     case 0x9b898ca914a91668bf6e80ad3427273uwb: { // read_tail
          t_file* const data = file__data(file);
          if (args_len != 1 || data->custom_data.ref_cnt != 1) [[clang::unlikely]] {
               mtd_name = file__mtd_read_tail;
               if (args_len != 1) goto wrong_num_fn_args;

               file__ref_cnt_dec(file);
               goto file_not_mut_label;
          }

          t_any  const result       = box__new(thrd_data, 2, file__mtd_read_string);
          t_any* const result_items = box__get_items(result);
          if (!data->is_flushed) {
               data->is_flushed = true;
               if (fflush(data->c_file) != 0) {
                    file__free(thrd_data, file);

                    result_items[0] = tkn__flush;
                    result_items[1] = null;
                    return result;
               }
          }

          u64 const buffer_idx = linear__alloc(&thrd_data->linear_allocator, file_buffer_size, 1);
          u8* const buffer     = linear__get_mem_of_idx(&thrd_data->linear_allocator, buffer_idx);
          u32       readed_len = fread(buffer, 1, file_buffer_size, data->c_file);
          bool      is_eof     = feof(data->c_file) != 0;
          if (!(readed_len == file_buffer_size || is_eof)) {
               file__free(thrd_data, file);
               linear__free(&thrd_data->linear_allocator, buffer_idx);

               result_items[0] = tkn__read;
               result_items[1] = null;
               return result;
          }

          if (readed_len < 15) {
               result_items[0] = file;
               result_items[1] = (const t_any){.bytes = {[14] = readed_len, [15] = tid__short_byte_array}};
               memcpy_le_16(result_items[1].bytes, buffer, readed_len);

               linear__free(&thrd_data->linear_allocator, buffer_idx);

               return result;
          }

          u64   bytes_cap    = is_eof ? readed_len : file_buffer_size * 3 / 2;
          t_any bytes_const  = long_byte_array__new(bytes_cap);
          u8*   bytes        = slice_array__get_items(bytes_const);
          u64   bytes_len    = 0;
          while (true) {
               memcpy(&bytes[bytes_len], buffer, readed_len);
               bytes_len += readed_len;

               if (is_eof) {
                    linear__free(&thrd_data->linear_allocator, buffer_idx);

                    if (bytes_len != bytes_cap)
                         bytes_const.qwords[0] = (u64)realloc((u8*)bytes_const.qwords[0], bytes_len + 16);

                    bytes_const = slice__set_metadata(bytes_const, 0, bytes_len <= slice_max_len ? bytes_len : slice_max_len + 1);
                    slice_array__set_cap(bytes_const, bytes_len);
                    slice_array__set_len(bytes_const, bytes_len);

                    result_items[0] = file;
                    result_items[1] = bytes_const;
                    return result;
               }

               readed_len = fread(buffer, 1, file_buffer_size, data->c_file);
               is_eof     = feof(data->c_file) != 0;
               if (!(readed_len == file_buffer_size || is_eof)) {
                    file__free(thrd_data, file);
                    linear__free(&thrd_data->linear_allocator, buffer_idx);
                    free((void*)bytes_const.qwords[0]);

                    result_items[0] = tkn__read;
                    result_items[1] = null;
                    return result;
               }

               if (bytes_len + readed_len > bytes_cap) {
                    if (bytes_cap == array_max_len) [[clang::unlikely]] {
                         call_stack__push(thrd_data, file__mtd_read_tail);
                         fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", file__mtd_read_tail);
                    }

                    bytes_cap             = (bytes_len + readed_len) * 3 / 2;
                    bytes_cap             = bytes_cap > array_max_len ? array_max_len : bytes_cap;
                    bytes_const.qwords[0] = (u64)realloc((u8*)bytes_const.qwords[0], bytes_cap + 16);
                    bytes                 = slice_array__get_items(bytes_const);
               }
          }
     }
     case 0x9b6837c648ce9fa0b85ebac344a2fe3uwb: { // seek
          t_file* const data = file__data(file);
          if (
               args_len != 3                 || data->custom_data.ref_cnt != 1 ||
               args[1].bytes[15] != tid__int || args[2].bytes[15] != tid__bool ||
               ((i64)args[1].qwords[0] < 0   && args[2].bytes[0] == 0)
          ) [[clang::unlikely]] {
               mtd_name = file__mtd_seek;
               if (args_len != 3) goto wrong_num_fn_args;
               if (args[1].bytes[15] != tid__int || args[2].bytes[15] != tid__bool) goto invalid_type_label;

               file__ref_cnt_dec(file);

               if (((i64)args[1].qwords[0] < 0 && args[2].bytes[0] == 0)) {
                    call_stack__push(thrd_data, mtd_name);
                    t_any const error = error__out_of_bounds(thrd_data, mtd_name);
                    call_stack__pop(thrd_data);

                    return error;
               }

               goto file_not_mut_label;
          }

          if (!data->is_flushed) {
               data->is_flushed = true;
               if (fflush(data->c_file) != 0) {
                    file__free(thrd_data, file);
                    return tkn__flush;
               }
          }

          if ((i64)(long)(i64)args[1].qwords[0] != (i64)args[1].qwords[0] || fseek(data->c_file, (i64)args[1].qwords[0], args[2].bytes[0] == 1 ? SEEK_CUR : SEEK_SET) != 0) {
               file__free(thrd_data, file);
               return tkn__seek;
          }

          return file;
     }
     case 0x9e63aa4e5f9979cced716ee7029f511uwb: { // start!
          t_file* const data = file__data(file);
          if (args_len != 1 || data->custom_data.ref_cnt != 1) [[clang::unlikely]] {
               mtd_name = file__mtd_to_start;
               if (args_len != 1) goto wrong_num_fn_args;

               file__ref_cnt_dec(file);
               goto file_not_mut_label;
          }

          if (!data->is_flushed) {
               data->is_flushed = true;
               if (fflush(data->c_file) != 0) {
                    file__free(thrd_data, file);
                    return tkn__flush;
               }
          }

          if (fseek(data->c_file, 0, SEEK_SET) != 0) {
               file__free(thrd_data, file);
               return tkn__seek;
          }

          return file;
     }
     case 0x9b8a8f6f759c8c7abfa231717f47f1fuwb: { // tell
          t_file* const data = file__data(file);
          if (args_len != 1 || data->custom_data.ref_cnt != 1) [[clang::unlikely]] {
               mtd_name = file__mtd_tell;
               if (args_len != 1) goto wrong_num_fn_args;

               file__ref_cnt_dec(file);
               goto file_not_mut_label;
          }

          long const position = ftell(data->c_file);
          if (position != -1) {
               t_any  const result       = box__new(thrd_data, 2, file__mtd_read_string);
               t_any* const result_items = box__get_items(result);
               result_items[0]           = file;
               result_items[1]           = (const t_any){.structure = {.value = position, .type = tid__int}};
               return result;
          }

          file__free(thrd_data, file);
          return null;
     }
     case 0x9aa799036752955a8f789d06ff89dffuwb: { // write
          t_file* const data = file__data(file);
          if (args_len != 2 || data->custom_data.ref_cnt != 1 || !(args[1].bytes[15] == tid__byte_array || args[1].bytes[15] == tid__short_byte_array)) [[clang::unlikely]] {
               mtd_name = file__mtd_write;
               if (args_len != 2) goto wrong_num_fn_args;
               if (!(args[1].bytes[15] == tid__byte_array || args[1].bytes[15] == tid__short_byte_array)) goto invalid_type_label;

               file__ref_cnt_dec(file);
               ref_cnt__dec(thrd_data, args[1]);
               goto file_not_mut_label;
          }

          const u8* bytes;
          u64       bytes_len;
          bool      need_to_free;
          t_any     bytes_arg = args[1];
          if (bytes_arg.bytes[15] == tid__short_byte_array) {
               need_to_free = false;
               bytes        = bytes_arg.bytes;
               bytes_len    = short_byte_array__get_len(bytes_arg);
          } else {
               u32 const slice_offset = slice__get_offset(bytes_arg);
               u32 const slice_len    = slice__get_len(bytes_arg);
               u64 const ref_cnt      = get_ref_cnt(bytes_arg);
               u64 const array_len    = slice_array__get_len(bytes_arg);
               bytes_len              = slice_len <= slice_max_len ? slice_len : array_len;
               bytes                  = &((const u8*)slice_array__get_items(bytes_arg))[slice_offset];

               assert(slice_len <= slice_max_len || slice_offset == 0);

               if (ref_cnt > 1) set_ref_cnt(bytes_arg, ref_cnt - 1);
               else need_to_free = ref_cnt == 1;
          }

          u64 const written_len = fwrite(bytes, 1, bytes_len, data->c_file);
          if (need_to_free) free((void*)bytes_arg.qwords[0]);
          if (written_len == bytes_len) {
               data->is_flushed = false;
               return file;
          }

          file__free(thrd_data, file);
          return null;
     }
     case 0x985c1e8f249c392a9a904fa403aa807uwb: { // write_string
          t_file* const data = file__data(file);
          if (
               args_len != 3                            || data->custom_data.ref_cnt != 1    ||
               !(args[1].bytes[15] == tid__short_string || args[1].bytes[15] == tid__string) ||
               !(args[2].raw_bits == tkn__ctf8.raw_bits || args[2].raw_bits == tkn__sys.raw_bits)
          ) [[clang::unlikely]] {
               mtd_name = file__mtd_write_string;
               if (args_len != 3) goto wrong_num_fn_args;
               if (args[2].bytes[15] != tid__token || !(args[1].bytes[15] == tid__short_string || args[1].bytes[15] == tid__string)) goto invalid_type_label;

               file__ref_cnt_dec(file);
               ref_cnt__dec(thrd_data, args[1]);
               if (!(args[2].raw_bits == tkn__ctf8.raw_bits || args[2].raw_bits == tkn__sys.raw_bits)) {
                    call_stack__push(thrd_data, mtd_name);
                    t_any const error = builtin_error__new(thrd_data, mtd_name, "encoding", "Invalid encoding specified.");
                    call_stack__pop(thrd_data);

                    return error;
               }

               goto file_not_mut_label;
          }

          t_any const string     = args[1];
          t_any const encoding   = args[2];
          bool  const enc_is_sys = encoding.bytes[0] == tkn__sys.bytes[0];
          if (string.bytes[15] == tid__short_string) {
               u64 const string_size = short_string__get_size(string);
               if (enc_is_sys) {
                    u8 buffer[113];
                    t_str_cvt_result const cvt_result = ctf8_chars_to_sys_chars(string_size, string.bytes, 113, buffer);
                    if (cvt_result.src_offset != string_size) [[clang::unlikely]] {
                         file__free(thrd_data, file);

                         mtd_name = file__mtd_write_string;
                         goto fail_text_recoding_label;
                    }

                    u16 const written_len = fwrite(buffer, 1, cvt_result.dst_offset, data->c_file);
                    if (written_len == cvt_result.dst_offset) {
                         data->is_flushed = false;
                         return file;
                    }

                    file__free(thrd_data, file);
                    return null;
               }

               u8 const written_len = fwrite(string.bytes, 1, string_size, data->c_file);
               if (written_len == string_size) {
                    data->is_flushed = false;
                    return file;
               }

               file__free(thrd_data, file);
               return null;
          }

          u32       const slice_offset = slice__get_offset(string);
          u32       const slice_len    = slice__get_len(string);
          u64       const ref_cnt      = get_ref_cnt(string);
          u64       const array_len    = slice_array__get_len(string);
          u64       const string_len   = slice_len <= slice_max_len ? slice_len : array_len;
          const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice_offset * 3];

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

          u32 const buffer_size = string_len * (enc_is_sys ? 16 : 4) > file_buffer_size ? file_buffer_size : string_len * (enc_is_sys ? 16 : 4);
          u64 const buffer_idx  = linear__alloc(&thrd_data->linear_allocator, buffer_size, 1);
          u8*       buffer      = linear__get_mem_of_idx(&thrd_data->linear_allocator, buffer_idx);
          u32       offset      = 0;
          if (enc_is_sys) {
               while (true) {
                    t_str_cvt_result cvt_result = corsar_chars_to_sys_chars(string_len * 3 - offset, &chars[offset], buffer_size, buffer);
                    if ((i64)cvt_result.src_offset <= 0) [[clang::unlikely]] {
                         linear__free(&thrd_data->linear_allocator, buffer_idx);
                         file__free(thrd_data, file);
                         if (ref_cnt == 1) free((void*)string.qwords[0]);
                         goto fail_text_recoding_label;
                    }

                    u32 const written_len = fwrite(buffer, 1, cvt_result.dst_offset, data->c_file);
                    if (written_len != cvt_result.dst_offset) {
                         linear__free(&thrd_data->linear_allocator, buffer_idx);
                         file__free(thrd_data, file);
                         if (ref_cnt == 1) free((void*)string.qwords[0]);
                         return null;
                    }

                    offset += cvt_result.src_offset;
                    if (offset == string_len * 3) {
                         linear__free(&thrd_data->linear_allocator, buffer_idx);
                         if (ref_cnt == 1) free((void*)string.qwords[0]);
                         data->is_flushed = false;
                         return file;
                    }
               }
          }

          while (true) {
               t_str_cvt_result cvt_result = corsar_chars_to_ctf8_chars(string_len * 3 - offset, &chars[offset], buffer_size, buffer);

               assert(cvt_result.src_offset > 0);

               u32 const written_len = fwrite(buffer, 1, cvt_result.dst_offset, data->c_file);
               if (written_len != cvt_result.dst_offset) {
                    linear__free(&thrd_data->linear_allocator, buffer_idx);
                    file__free(thrd_data, file);
                    if (ref_cnt == 1) free((void*)string.qwords[0]);
                    return null;
               }

               offset += cvt_result.src_offset;
               if (offset == string_len * 3) {
                    linear__free(&thrd_data->linear_allocator, buffer_idx);
                    if (ref_cnt == 1) free((void*)string.qwords[0]);
                    data->is_flushed = false;
                    return file;
               }
          }
     }

     [[clang::unlikely]]
     default:
          mtd_name = owner;
          goto invalid_type_label;
     }

     wrong_num_fn_args: {
          t_any error = handle_args_in_error_mtd(thrd_data, args, args_len);
          if (error.bytes[15] == tid__null) {
               call_stack__push(thrd_data, mtd_name);
               error = error__wrong_num_fn_args(thrd_data, mtd_name);
               call_stack__pop(thrd_data);
          }

          return error;
     }

     invalid_type_label: {
          t_any error = handle_args_in_error_mtd(thrd_data, args, args_len);
          if (error.bytes[15] == tid__null) {
               call_stack__push(thrd_data, mtd_name);
               error = error__invalid_type(thrd_data, mtd_name);
               call_stack__pop(thrd_data);
          }

          return error;
     }

     file_not_mut_label: {
          call_stack__push(thrd_data, mtd_name);
          t_any const error = builtin_error__new(thrd_data, mtd_name, "duplicate", "When calling a method of a constant of type \"std.file\", it must not have any copies.");
          call_stack__pop(thrd_data);

          return error;
     }

     fail_text_recoding_label:
     call_stack__push(thrd_data, mtd_name);
     t_any const error = error__fail_text_recoding(thrd_data, mtd_name);
     call_stack__pop(thrd_data);

     return error;
}

static bool file__has_mtd(t_any const mtd_tkn) {
     switch (mtd_tkn.raw_bits) {
     case 0x9e9a40617e3f1254f9247a6762a96f3uwb: // cut
     case 0x9a3305204291427e76f512b45a33e05uwb: // end!
     case 0x9b0885a478d1e7d0a40adc9dc46bbb3uwb: // flush
     case 0x9a78f2b273dba56085bd6eaef1b8d1buwb: // read
     case 0x9adb25beb48af23a9a90478d90a2027uwb: // read_string
     case 0x9b898ca914a91668bf6e80ad3427273uwb: // read_tail
     case 0x9b6837c648ce9fa0b85ebac344a2fe3uwb: // seek
     case 0x9e63aa4e5f9979cced716ee7029f511uwb: // start!
     case 0x9b8a8f6f759c8c7abfa231717f47f1fuwb: // tell
     case 0x9aa799036752955a8f789d06ff89dffuwb: // write
     case 0x985c1e8f249c392a9a904fa403aa807uwb: // write_string
          return true;
     default:
          return false;
     }
}

static t_any file__to_global_const(t_thrd_data* const thrd_data, t_any const, const char* const owner) {fail_with_call_stack(thrd_data, "A file cannot be a global constant value.", owner);}

static t_any file__type(void) {return (const t_any){.bytes = "std.file"};}

static void file__dump(t_thrd_data* const thrd_data, t_any* const result, t_any const file, u64 const, u64 const, const char* const owner) {
#ifdef DEBUG
     const t_file* const data      = file__data(file);
     t_any         const file_name = data->file_name;
     dump__add_string__own(thrd_data, result, (const t_any){.bytes = "[file "}, owner);
     t_any const string = string_from_n_len_sysstr(thrd_data, fs_path__len(file_name), fs_path__path(file_name), owner);
     if (string.bytes[15] == tid__error) [[clang::unlikely]]
          ref_cnt__dec(thrd_data, string);
     else dump__add_string__own(thrd_data, result, string, owner);
     dump__add_string__own(thrd_data, result, (const t_any){.bytes = "]\n"}, owner);
#else
     dump__add_string__own(thrd_data, result, (const t_any){.bytes = "[file]\n"}, owner);
#endif
}

static t_any file__to_shared(t_thrd_data* const thrd_data, t_any const file, t_shared_fn const shared_fn, bool const nested, const char* const owner) {
     t_file* const data = file__data(file);
     if (data->custom_data.ref_cnt == 1) [[clang::likely]] return file;

     file__ref_cnt_dec(file);

     return builtin_error__new(thrd_data, owner, "share_invalid_type", "Sharing a used file from thread to thread.");
}

static t_custom_fns const file__fns = {
     .call_mtd        = file__call_mtd,
     .has_mtd         = file__has_mtd,
     .free_function   = file__free,
     .to_global_const = file__to_global_const,
     .type            = file__type,
     .dump            = file__dump,
     .to_shared       = file__to_shared,
};

t_any MstdFNopen_file(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - std.open_file";

     t_any const file_name = args[0];
     t_any const mode      = args[1];

     const char* c_file_open_mode = nullptr;
     bool        seek_to_end      = false;
     switch (mode.qwords[0]) {
     case fom__read | fom__begin:
          c_file_open_mode = "rb";
          break;
     case fom__read | fom__write | fom__new:
          c_file_open_mode = "w+b";
          break;
     case fom__read | fom__write | fom__begin:
          c_file_open_mode = "r+b";
          break;
     case fom__read | fom__write | fom__end:
          c_file_open_mode = "r+b";
          seek_to_end      = true;
          break;
     case fom__write | fom__new:
          c_file_open_mode = "wb";
          break;
     }

     if (
          file_name.bytes[15] != tid__custom || mode.bytes[15] != tid__int ||
          c_file_open_mode == nullptr        || custom__data(file_name)->fns.pointer != &fs_path__fns
     ) [[clang::unlikely]] {
          if (file_name.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, mode);

               call_stack__pop_if_tail_call(thrd_data);
               return file_name;
          }

          if (mode.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, file_name);

               call_stack__pop_if_tail_call(thrd_data);
               return mode;
          }
          ref_cnt__dec(thrd_data, mode);

          call_stack__push(thrd_data, owner);
          t_any const error = file_name.bytes[15] != tid__custom || mode.bytes[15] != tid__int || custom__data(file_name)->fns.pointer != &fs_path__fns ?
               error__invalid_type(thrd_data, owner) :
               builtin_error__new(thrd_data, owner, "mode", "Invalid file open mode.");
          call_stack__pop(thrd_data);

          ref_cnt__dec(thrd_data, file_name);

          return error;
     }

     const char* const c_file_name = fs_path__path_for_c(file_name);

     if ((mode.bytes[0] & fom__write) == 0) {
          struct stat file_stat;
          if (stat(c_file_name, &file_stat) == 0 && (file_stat.st_mode & S_IFDIR) == S_IFDIR) {
               fs_path__ref_cnt_dec(file_name);

               call_stack__pop_if_tail_call(thrd_data);
               return tkn__not_file;
          }
     }

     FILE* const c_file = fopen(c_file_name, c_file_open_mode);
     if (c_file != nullptr) {
          if (seek_to_end && fseek(c_file, 0, SEEK_END) != 0) {
               fs_path__ref_cnt_dec(file_name);

               call_stack__pop_if_tail_call(thrd_data);
               return tkn__seek;
          }

          t_file* const new_file            = malloc(sizeof(t_file));
          new_file->custom_data.ref_cnt     = 1;
          new_file->custom_data.fns.pointer = (void*)&file__fns;
          new_file->c_file                  = c_file;
          new_file->is_flushed              = true;
          new_file->dont_close              = false;

#ifdef DEBUG
          new_file->file_name = file_name;
#else
          fs_path__ref_cnt_dec(file_name);
#endif
          call_stack__pop_if_tail_call(thrd_data);
          return (const t_any){.structure = {.value = (u64)new_file, .type = tid__custom}};
     }

     t_any result;
     switch (errno) {
     case EACCES: case EROFS:
          result = tkn__access; break;
     case EISDIR:
          result = tkn__not_file; break;
     case EMFILE: case ENFILE:
          result = tkn__max_open; break;
     case ENAMETOOLONG:
          result = tkn__long; break;
     case ENOENT: case ENOTDIR:
          result = tkn__not_exist; break;
     case ENOSPC:
          result = tkn__fs_limits; break;
     case ENOMEM:
          result = tkn__space; break;
     default:
          result = tkn__open;
     }

     fs_path__ref_cnt_dec(file_name);

     call_stack__pop_if_tail_call(thrd_data);
     return result;
}