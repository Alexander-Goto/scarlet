#pragma once

#include "../include/basic.h"
#include "../include/include.h"

#define path_separator (u8)'/'
#define max_path_len   0xff'fffful

struct {
     t_custom_types_data custom_data;
     char                fs_path[16];
} typedef t_std_fs_path;

static const char* const fs_path__mtd_equal        = "function - @std.fs_path->core___equal__";
static const char* const fs_path__mtd_strong_equal = "function - @std.fs_path->core___strong_equal__";
static const char* const fs_path__mtd_cmp          = "function - @std.fs_path->core_cmp";
static const char* const fs_path__mtd_hash         = "function - @std.fs_path->core_hash";
static const char* const fs_path__mtd_print        = "function - @std.fs_path->core_print";
static const char* const fs_path__mtd_to_string    = "function - @std.fs_path->core_to_string";
static const char* const fs_path__mtd_is_abs       = "function - @std.fs_path->abs?";
static const char* const fs_path__mtd_base         = "function - @std.fs_path->base";
static const char* const fs_path__mtd_clean        = "function - @std.fs_path->clean";
static const char* const fs_path__mtd_dir          = "function - @std.fs_path->dir";
static const char* const fs_path__mtd_ext          = "function - @std.fs_path->ext";
static const char* const fs_path__mtd_join         = "function - @std.fs_path->join";
static const char* const fs_path__mtd_is_prefix_of = "function - @std.fs_path->prefix_of?";
static const char* const fs_path__mtd_split        = "function - @std.fs_path->split";

[[clang::always_inline]]
static char* fs_path__path(t_any const path) {return custom__outside_data(path);}

[[clang::always_inline]]
static char* fs_path__path_for_c(t_any const path) {return custom__outside_data(path);}

[[clang::always_inline]]
static u32 fs_path__len(t_any const path) {return path.qwords[1] & 0xff'fffful;}

[[clang::always_inline]]
static void fs_path__ref_cnt_dec(t_any const path) {
     t_custom_types_data* const data    = custom__data(path);
     u64                  const ref_cnt = data->ref_cnt;

     if (ref_cnt > 1) data->ref_cnt -= 1;
     else if (ref_cnt == 1) free(data);
}

static t_any fs_path__call_mtd(t_thrd_data* const thrd_data, const char* const owner, t_any const mtd_tkn, const t_any* const args, u8 const args_len) {
     const char* mtd_name;

     switch (mtd_tkn.raw_bits) {
     case 0x96aa3760c206f10667e2b8d661315cbuwb: { // core___equal__
          if (args_len != 2 || args[1].bytes[15] == tid__error) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_equal;
               goto wrong_num_fn_args;
          }

          t_any const left  = args[0];
          t_any const right = args[1];

          if (right.bytes[15] != tid__custom) {
               fs_path__ref_cnt_dec(left);
               ref_cnt__dec(thrd_data, right);
               return bool__false;
          }

          t_custom_types_data* const left_data     = custom__data(left);
          u64                  const left_ref_cnt  = left_data->ref_cnt;
          t_custom_types_data* const right_data    = custom__data(right);
          u64                  const right_ref_cnt = right_data->ref_cnt;

          if (left_ref_cnt > 1) left_data->ref_cnt -= 1;
          if (right_ref_cnt > 1) right_data->ref_cnt -= 1;

          t_any result;
          if (left_data->fns.pointer == right_data->fns.pointer) {
               u32 const left_len  = fs_path__len(left);
               u32 const right_len = fs_path__len(right);
               result = left_len == right_len && __builtin_memcmp(fs_path__path(left), fs_path__path(right), left_len) == 0 ? bool__true : bool__false;

               if (right_ref_cnt == 1) free(right_data);
          } else {
               result = bool__false;
               if (right_ref_cnt == 1) ((t_custom_fns*)right_data->fns.pointer)->free_function(thrd_data, right);
          }

          if (left_ref_cnt == 1) free(left_data);

          return result;
     }
     case 0x9346b0dd8b5f16906b399d4f24a1dc3uwb: { // core___strong_equal__
          if (args_len != 2 || args[1].bytes[15] != tid__custom) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_strong_equal;
               if (args_len != 2) goto wrong_num_fn_args;
               goto invalid_type_label;
          }

          t_any                const left       = args[0];
          t_custom_types_data* const left_data  = custom__data(left);
          t_any                const right      = args[1];
          t_custom_types_data* const right_data = custom__data(right);

          if (left_data->fns.pointer != right_data->fns.pointer) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_strong_equal;
               goto invalid_type_label;
          }

          u64 const left_ref_cnt  = left_data->ref_cnt;
          u64 const right_ref_cnt = right_data->ref_cnt;

          if (left_ref_cnt > 1) left_data->ref_cnt -= 1;
          if (right_ref_cnt > 1) right_data->ref_cnt -= 1;

          u32   const left_len  = fs_path__len(left);
          u32   const right_len = fs_path__len(right);
          t_any const result    = left_len == right_len && __builtin_memcmp(fs_path__path(left), fs_path__path(right), left_len) == 0 ? bool__true : bool__false;

          if (left_ref_cnt == 1) free(left_data);
          if (right_ref_cnt == 1) free(right_data);

          return result;
     }
     case 0x9e13409e49f1587adc8f049ebc7cfffuwb: { // core_cmp
          if (args_len != 2 || args[1].bytes[15] == tid__error) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_cmp;
               goto wrong_num_fn_args;
          }

          t_any const left  = args[0];
          t_any const right = args[1];

          if (right.bytes[15] != tid__custom) {
               fs_path__ref_cnt_dec(left);
               ref_cnt__dec(thrd_data, right);
               return tkn__not_equal;
          }

          t_custom_types_data* const left_data     = custom__data(left);
          u64                  const left_ref_cnt  = left_data->ref_cnt;
          t_custom_types_data* const right_data    = custom__data(right);
          u64                  const right_ref_cnt = right_data->ref_cnt;

          if (left_ref_cnt > 1) left_data->ref_cnt -= 1;
          if (right_ref_cnt > 1) right_data->ref_cnt -= 1;

          t_any result;
          if (left_data->fns.pointer == right_data->fns.pointer) {
               u32 const left_len     = fs_path__len(left);
               u32 const right_len    = fs_path__len(right);
               u32 const min_len      = left_len < right_len ? left_len : right_len;
               int const c_cmp_result = __builtin_memcmp(fs_path__path(left), fs_path__path(right), min_len);
               result = c_cmp_result < 0 ? tkn__less : (
                    c_cmp_result > 0 ? tkn__great :
                    left_len < right_len ? tkn__less : (left_len > right_len ? tkn__great : tkn__equal)
               );

               if (right_ref_cnt == 1) free(right_data);
          } else {
               result = tkn__not_equal;
               if (right_ref_cnt == 1) ((t_custom_fns*)right_data->fns.pointer)->free_function(thrd_data, right);
          }

          if (left_ref_cnt == 1) free(left_data);

          return result;
     }
     case 0x9b0210d4f2d9891ea2bf0166fdc6647uwb: { // core_hash
          if (args_len != 2 || args[1].bytes[15] != tid__int) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_hash;
               if (args_len != 2) goto wrong_num_fn_args;
               goto invalid_type_label;
          }

          t_any                const path    = args[0];
          t_custom_types_data* const data    = custom__data(path);
          u64                  const ref_cnt = data->ref_cnt;
          if (ref_cnt > 1) data->ref_cnt -= 1;

          u64 const hash = get_hash((u8*)fs_path__path(path), fs_path__len(path), args[1].qwords[0]);

          if (ref_cnt == 1) free(data);

          return (const t_any){.structure = {.value = hash, .type = tid__int}};
     }
     case 0x9e9751da1e0baa4cf88e3197b1fd633uwb: { // core_print
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_print;
               goto wrong_num_fn_args;
          }

          t_any                const path    = args[0];
          t_custom_types_data* const data    = custom__data(path);
          u64                  const ref_cnt = data->ref_cnt;

          if (ref_cnt > 1) data->ref_cnt -= 1;

          u32 const len    = fs_path__len(path);
          t_any     result = null;
          if (fwrite(fs_path__path(path), 1, len, stdout) != len) [[clang::unlikely]] {
               call_stack__push(thrd_data, fs_path__mtd_print);
               result = builtin_error__new(thrd_data, fs_path__mtd_print, "print", "Failed to print anything.");
               call_stack__pop(thrd_data);
          }

          if (ref_cnt == 1) free(data);
          return result;
     }
     case 0x96a2cd012bd01d009d58ab32f8f34efuwb: { // core_to_string
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_to_string;
               goto wrong_num_fn_args;
          }

          t_any                const path    = args[0];
          t_custom_types_data* const data    = custom__data(path);
          u64                  const ref_cnt = data->ref_cnt;

          if (ref_cnt > 1) data->ref_cnt -= 1;

          call_stack__push(thrd_data, fs_path__mtd_to_string);
          t_any const string = string_from_n_len_sysstr(thrd_data, fs_path__len(path), fs_path__path(path), fs_path__mtd_to_string);
          call_stack__pop(thrd_data);

          if (ref_cnt == 1) free(data);
          return string;
     }
     case 0x9e52a5df5911eb2aea07ef14f29a846uwb: { // abs?
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_is_abs;
               goto wrong_num_fn_args;
          }

          t_any const path   = args[0];
          t_any const result = fs_path__path(path)[0] == path_separator ? bool__true : bool__false;
          fs_path__ref_cnt_dec(path);

          return result;
     }
     case 0x9a97dc1b6704c1ae8c4fea04b568df7uwb: { // base
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_base;
               goto wrong_num_fn_args;
          }

          t_any                const path    = args[0];
          t_custom_types_data* const data    = custom__data(path);
          u64                  const ref_cnt = data->ref_cnt;

          if (ref_cnt > 1) data->ref_cnt -= 1;

          char* const path_chars = fs_path__path(path);
          u32   const path_len   = fs_path__len(path);

          t_any result;
          if (path_len == 1)
               result = (const t_any){.bytes = path_chars[0]};
          else {
               u32 const base_first_char_idx = look_byte_from_end((u8*)path_chars, path_len, path_separator) + (u64)1;

               call_stack__push(thrd_data, fs_path__mtd_base);
               result = string_from_n_len_sysstr(thrd_data, path_len - base_first_char_idx, &path_chars[base_first_char_idx], fs_path__mtd_base);
               call_stack__pop(thrd_data);
          }

          if (ref_cnt == 1) free(data);
          return result;
     }
     case 0x9bd512a1023db76acf5655daef013afuwb: { // clean
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_clean;
               goto wrong_num_fn_args;
          }

          t_any const path     = args[0];
          u32         path_len = fs_path__len(path);
          if (path_len == 1) return path;

          t_custom_types_data* const data        = custom__data(path);
          t_custom_types_data*       result_data = data;
          u64 const                  ref_cnt     = data->ref_cnt;
          bool                       path_is_mut = ref_cnt == 1;
          char*                      path_chars  = fs_path__path(path);
          u32                        path_cap    = path_len;

          u32 offset = 0;
          while (true) {
               u32 const dot_prefix_offset_idx = look_2_bytes_from_begin((u8*)&path_chars[offset], path_len - offset, (u16)path_separator | (u16)'.' << 8);
               if (dot_prefix_offset_idx == (u32)-1) break;

               u32  const dot_prefix_idx = offset + dot_prefix_offset_idx;
               char const next_char      = path_chars[dot_prefix_idx + 2];
               if (next_char == '.' && (path_chars[dot_prefix_idx + 3] == path_separator || path_chars[dot_prefix_idx + 3] == 0)) {
                    if (dot_prefix_idx == 0) [[clang::unlikely]] {
                         if (result_data != data) free(result_data);
                         else if (ref_cnt > 1) data->ref_cnt -= 1;
                         else if (ref_cnt == 1) free(data);

                         return tkn__roots_parent;
                    }

                    u32  const parent_prefix_separator_idx = look_byte_from_end((u8*)path_chars, dot_prefix_idx, path_separator);
                    char const parent_first_char     = path_chars[parent_prefix_separator_idx + (u32)1];
                    char const parent_second_char    = path_chars[parent_prefix_separator_idx + (u32)2];
                    bool const ignore                = parent_prefix_separator_idx == dot_prefix_idx - 3 && parent_first_char == '.' && parent_second_char == '.';
                    bool const only_parent_delete    = parent_prefix_separator_idx == (u32)-1 && dot_prefix_idx == 1 && parent_first_char == '.';
                    if (!(ignore || only_parent_delete)) {
                         assert(parent_prefix_separator_idx != (u32)-1);

                         path_len -= dot_prefix_idx + 3 - parent_prefix_separator_idx;
                         if (path_is_mut)
                              memmove(&path_chars[parent_prefix_separator_idx], &path_chars[dot_prefix_idx + 3], path_len - parent_prefix_separator_idx + 1);
                         else {
                              path_cap                   = path_len;
                              result_data                = malloc(sizeof(t_custom_types_data) + (path_len == 0 ? 1 : path_len) + 1);
                              result_data->ref_cnt       = 1;
                              result_data->fns           = data->fns;
                              char* const new_path_chars = (char*)&result_data[1];

                              memcpy(new_path_chars, path_chars, parent_prefix_separator_idx);
                              memcpy(&new_path_chars[parent_prefix_separator_idx], &path_chars[dot_prefix_idx + 3], path_len - parent_prefix_separator_idx + 1);

                              path_is_mut = true;
                              path_chars  = new_path_chars;

                              if (ref_cnt > 1) data->ref_cnt -= 1;
                              else if (ref_cnt == 1) free(data);
                         }

                         offset = parent_prefix_separator_idx;
                         if (offset == path_len) {
                              if (path_len == 0) {
                                   path_chars[0]  = path_separator;
                                   path_chars[1]  = 0;
                                   path_len       = 1;
                              }

                              break;
                         }
                    } else if (ignore)
                         offset += 3;
                    else {
                         path_len -= 2;
                         if (path_is_mut)
                              memmove(path_chars, &path_chars[2], path_len + 1);
                         else {
                              path_cap                   = path_len;
                              result_data                = malloc(sizeof(t_custom_types_data) + path_len + 1);
                              result_data->ref_cnt       = 1;
                              result_data->fns           = data->fns;
                              char* const new_path_chars = (char*)&result_data[1];

                              memcpy(new_path_chars, &path_chars[2], path_len + 1);

                              path_is_mut = true;
                              path_chars  = new_path_chars;

                              if (ref_cnt > 1) data->ref_cnt -= 1;
                              else if (ref_cnt == 1) free(data);
                         }

                         offset = 2;
                         if (offset == path_len) break;
                    }
               } else if (next_char == path_separator || next_char == 0) {
                    path_len -= 2;
                    if (path_is_mut)
                         memmove(&path_chars[dot_prefix_idx], &path_chars[dot_prefix_idx + 2], path_len - dot_prefix_idx + 1);
                    else {
                         path_cap                   = path_len;
                         result_data                = malloc(sizeof(t_custom_types_data) + (path_len == 0 ? 1 : path_len) + 1);
                         result_data->ref_cnt       = 1;
                         result_data->fns           = data->fns;
                         char* const new_path_chars = (char*)&result_data[1];

                         memcpy(new_path_chars, path_chars, dot_prefix_idx);
                         memcpy(&new_path_chars[dot_prefix_idx], &path_chars[dot_prefix_idx + 2], path_len - dot_prefix_idx + 1);

                         path_is_mut = true;
                         path_chars  = new_path_chars;

                         if (ref_cnt > 1) data->ref_cnt -= 1;
                         else if (ref_cnt == 1) free(data);
                    }

                    offset = dot_prefix_idx;
                    if (offset == path_len) {
                         if (path_len == 0) {
                              path_chars[0]  = path_separator;
                              path_chars[1]  = 0;
                              path_len       = 1;
                         }

                         break;
                    }
               } else offset += 2;
          }

          assert(path_chars[path_len] == 0);

          if (path_is_mut && path_len != path_cap) result_data = realloc(result_data, sizeof(t_custom_types_data) + path_len + 1);

          return (const t_any){.qwords = {(u64)result_data, path_len | (u64)tid__custom << 56}};
     }
     case 0x9e568a94f530ed62eacf81a9fd9e6d7uwb: { // dir
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_dir;
               goto wrong_num_fn_args;
          }

          t_any                const path    = args[0];
          t_custom_types_data*       data    = custom__data(path);
          u64                  const ref_cnt = data->ref_cnt;

          if (ref_cnt > 1) data->ref_cnt -= 1;

          char* const path_chars    = fs_path__path(path);
          u32   const path_len      = fs_path__len(path);
          u32   const separator_idx = look_byte_from_end((u8*)path_chars, path_len, path_separator);
          if (separator_idx == (u32)-1 || path_len == 1) {
               if (ref_cnt == 1) free(data);
               return null;
          }

          if (ref_cnt == 1) {
               if (separator_idx == 0) {
                    path_chars[1] = 0;
                    data          = realloc(data, sizeof(t_custom_types_data) + 2);
                    return (const t_any){.structure = {.value = (u64)data, .data = {1}, .type = tid__custom}};
               }

               path_chars[separator_idx] = 0;
               data                      = realloc(data, sizeof(t_custom_types_data) + separator_idx + 1);
               return (const t_any){.qwords = {(u64)data, separator_idx | (u64)tid__custom << 56}};
          }

          if (separator_idx == 0) {
               t_custom_types_data* const new_data       = malloc(sizeof(t_custom_types_data) + 2);
               new_data->ref_cnt                         = 1;
               new_data->fns                             = data->fns;
               char*                const new_path_chars = (char*)&new_data[1];
               new_path_chars[0] = path_separator;
               new_path_chars[1] = 0;
               return (const t_any){.structure = {.value = (u64)new_data, .data = {1}, .type = tid__custom}};
          }

          t_custom_types_data* const new_data       = malloc(sizeof(t_custom_types_data) + separator_idx + 1);
          new_data->ref_cnt                         = 1;
          new_data->fns                             = data->fns;
          char*                const new_path_chars = (char*)&new_data[1];
          memcpy(new_path_chars, path_chars, separator_idx);
          new_path_chars[separator_idx] = 0;
          return (const t_any){.qwords = {(u64)new_data, separator_idx | (u64)tid__custom << 56}};
     }
     case 0x9e9c2c58ac206daef9870c2ac113f37uwb: { // ext
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_ext;
               goto wrong_num_fn_args;
          }

          t_any                const path    = args[0];
          t_custom_types_data* const data    = custom__data(path);
          u64                  const ref_cnt = data->ref_cnt;

          if (ref_cnt > 1) data->ref_cnt -= 1;

          char* const path_chars = fs_path__path(path);
          u32   const path_len   = fs_path__len(path);

          t_any result;
          if (path_len < 3)
               result = null;
          else {
               u32 const base_first_char_idx = look_byte_from_end((u8*)path_chars, path_len, path_separator) + (u64)1;
               u32 const dot_idx             = look_byte_from_end((u8*)&path_chars[base_first_char_idx], path_len - base_first_char_idx, '.') + base_first_char_idx;
               if ((i32)dot_idx <= (i32)base_first_char_idx || (dot_idx - base_first_char_idx == 1 && dot_idx + 1 == path_len && path_chars[base_first_char_idx] == '.'))
                    result = null;
               else {
                    call_stack__push(thrd_data, fs_path__mtd_ext);
                    result = string_from_n_len_sysstr(thrd_data, path_len - dot_idx - 1, &path_chars[dot_idx + 1], fs_path__mtd_ext);
                    call_stack__pop(thrd_data);
               }
          }

          if (ref_cnt == 1) free(data);
          return result;
     }
     case 0x9bd80c0f35b6bd78cfeea6df18c4e1buwb: { // join
          if (args_len != 2 || args[1].bytes[15] != tid__custom) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_join;
               if (args_len != 2) goto wrong_num_fn_args;
               goto invalid_type_label;
          }

          t_any                const current_dir       = args[0];
          t_custom_types_data*       current_dir_data  = custom__data(current_dir);
          t_any                const path              = args[1];
          t_custom_types_data* const path_data         = custom__data(path);

          if (current_dir_data->fns.pointer != path_data->fns.pointer) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_join;
               goto invalid_type_label;
          }

          char* path_chars = fs_path__path(path);
          u32   path_len   = fs_path__len(path);

          if (path_len == 1 && (path_chars[0] == '.' || path_chars[0] == path_separator)) {
               fs_path__ref_cnt_dec(path);
               return current_dir;
          }

          u64 const current_dir_ref_cnt = current_dir_data->ref_cnt;
          u64 const path_ref_cnt        = path_data->ref_cnt;

          if (current_dir_ref_cnt > 1) current_dir_data->ref_cnt -= 1;
          if (path_ref_cnt > 1) path_data->ref_cnt -= 1;

          if (path_chars[0] == path_separator) {
               path_chars = &path_chars[1];
               path_len  -= 1;
          } else if (path_chars[0] == '.' && path_chars[1] == path_separator) {
               path_chars = &path_chars[2];
               path_len  -= 2;
          }

          char* current_dir_chars = fs_path__path(current_dir);
          u32   current_dir_len   = fs_path__len(current_dir);

          bool need_separator = current_dir_chars[current_dir_len - 1] != path_separator;
          u32 const result_len = current_dir_len + path_len + (u32)need_separator;
          if (result_len > max_path_len) {
               if (current_dir_ref_cnt == 1) free(current_dir_data);
               if (path_ref_cnt == 1) free(path_data);

               return tkn__long;
          }

          if (current_dir_ref_cnt == 1) {
               if (need_separator) {
                    current_dir_chars[current_dir_len] = path_separator;
                    current_dir_len                   += 1;
               }

               current_dir_data  = realloc(current_dir_data, sizeof(t_custom_types_data) + result_len + 1);
               current_dir_chars = (char*)&current_dir_data[1];
               memcpy(&current_dir_chars[current_dir_len], path_chars, path_len + 1);

               if (path_ref_cnt == 1) free(path_data);

               return (const t_any){.qwords = {(u64)current_dir_data, result_len | (u64)tid__custom << 56}};
          }

          t_custom_types_data* const result_data  = malloc(sizeof(t_custom_types_data) + result_len + 1);
          result_data->ref_cnt                    = 1;
          result_data->fns                        = current_dir_data->fns;
          char*                const result_chars = (char*)&result_data[1];

          memcpy(result_chars, current_dir_chars, current_dir_len);
          if (need_separator) {
               result_chars[current_dir_len] = path_separator;
               current_dir_len              += 1;
          }
          memcpy(&result_chars[current_dir_len], path_chars, path_len + 1);

          if (path_ref_cnt == 1) free(path_data);

          return (const t_any){.qwords = {(u64)result_data, result_len | (u64)tid__custom << 56}};
     }
     case 0x9a9b6e2adb2f16488d07bd8b2eb60bauwb: { // prefix_of?
          if (args_len != 2 || args[1].bytes[15] != tid__custom) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_is_prefix_of;
               if (args_len != 2) goto wrong_num_fn_args;
               goto invalid_type_label;
          }

          t_any                const prefix       = args[0];
          t_custom_types_data*       prefix_data  = custom__data(prefix);
          t_any                const path         = args[1];
          t_custom_types_data* const path_data    = custom__data(path);

          if (prefix_data->fns.pointer != path_data->fns.pointer) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_is_prefix_of;
               goto invalid_type_label;
          }

          u64 const prefix_ref_cnt = prefix_data->ref_cnt;
          u64 const path_ref_cnt   = path_data->ref_cnt;

          if (prefix_ref_cnt > 1) prefix_data->ref_cnt -= 1;
          if (path_ref_cnt > 1) path_data->ref_cnt -= 1;

          char* path_chars   = fs_path__path(path);
          u32   path_len     = fs_path__len(path);
          char* prefix_chars = fs_path__path(prefix);
          u32   prefix_len   = fs_path__len(prefix);

          t_any result;
          if (path_len < prefix_len)
               result = bool__false;
          else {
               char const next_char = path_chars[prefix_len];
               result = __builtin_memcmp(prefix_chars, path_chars, prefix_len) == 0 && (next_char == path_separator || next_char == 0) ? bool__true : bool__false;
          }

          if (prefix_ref_cnt == 1) free(prefix_data);
          if (path_ref_cnt == 1) free(path_data);

          return result;
     }
     case 0x9ea5fab7d03e24ecfb7fe5a8fc18773uwb: { // split
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = fs_path__mtd_split;
               goto wrong_num_fn_args;
          }

          t_any                const path    = args[0];
          t_custom_types_data* const data    = custom__data(path);
          u64                  const ref_cnt = data->ref_cnt;

          if (ref_cnt > 1) data->ref_cnt -= 1;

          char* path_chars = fs_path__path(path);
          u32   path_len   = fs_path__len(path);
          if (path_chars[0] == path_separator) {
               if (path_len == 1) {
                    if (ref_cnt == 1) free(data);
                    return empty_array;
               }

               path_chars = &path_chars[1];
               path_len  -= 1;
          }

          u32 array_len = 1;
          assume(path_len != 0); for (u32 offset = 0; offset < path_len; array_len += (u32)(path_chars[offset] == path_separator), offset += 1);
          t_any array = array__init(thrd_data, array_len, fs_path__mtd_split);
          array       = slice__set_metadata(array, 0, array_len <= slice_max_len ? array_len : slice_max_len + 1);
          slice_array__set_len(array, array_len);
          t_any* const items = slice_array__get_items(array);

          call_stack__push(thrd_data, fs_path__mtd_split);
          for (u32 item_idx = 0;;item_idx += 1) {
               u32  separator_idx  = look_byte_from_begin((u8*)path_chars, path_len, path_separator);
               bool last_iteration = separator_idx == (u32)-1;
               separator_idx       = last_iteration ? path_len : separator_idx;
               items[item_idx]     = string_from_n_len_sysstr(thrd_data, separator_idx, path_chars, fs_path__mtd_split);
               if (items[item_idx].bytes[15] == tid__error) [[clang::unlikely]] {
                    t_any const error = items[item_idx];
                    for (u32 free_idx = item_idx - 1; free_idx != (u32)-1; ref_cnt__dec(thrd_data, items[free_idx]), free_idx -= 1);
                    free((void*)array.qwords[0]);
                    if (ref_cnt == 1) free(data);
                    return error;
               }
               path_chars = &path_chars[separator_idx + 1];
               path_len  -= separator_idx + 1;

               if (last_iteration) break;
          }
          call_stack__pop(thrd_data);

          if (ref_cnt == 1) free(data);
          return array;
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
}

static bool fs_path__has_mtd(t_any const mtd_tkn) {
     switch (mtd_tkn.raw_bits) {
     case 0x96aa3760c206f10667e2b8d661315cbuwb: // core___equal__
     case 0x9346b0dd8b5f16906b399d4f24a1dc3uwb: // core___strong_equal__
     case 0x9e13409e49f1587adc8f049ebc7cfffuwb: // core_cmp
     case 0x9b0210d4f2d9891ea2bf0166fdc6647uwb: // core_hash
     case 0x9e9751da1e0baa4cf88e3197b1fd633uwb: // core_print
     case 0x96a2cd012bd01d009d58ab32f8f34efuwb: // core_to_string
     case 0x9e52a5df5911eb2aea07ef14f29a846uwb: // abs?
     case 0x9a97dc1b6704c1ae8c4fea04b568df7uwb: // base
     case 0x9bd512a1023db76acf5655daef013afuwb: // clean
     case 0x9e568a94f530ed62eacf81a9fd9e6d7uwb: // dir
     case 0x9e9c2c58ac206daef9870c2ac113f37uwb: // ext
     case 0x9bd80c0f35b6bd78cfeea6df18c4e1buwb: // join
     case 0x9a9b6e2adb2f16488d07bd8b2eb60bauwb: // prefix_of?
     case 0x9ea5fab7d03e24ecfb7fe5a8fc18773uwb: // split
          return true;
     default:
          return false;
     }
}

static void fs_path__free(t_thrd_data* const, const t_any path) {free((void*)path.qwords[0]);}

static t_any fs_path__to_global_const(t_thrd_data* const, t_any const path, const char* const) {
     t_custom_types_data* const data = custom__data(path);
     if (data->ref_cnt != 0) data->ref_cnt = 0;
     return path;
}

static t_any fs_path__type(void) {return (const t_any){.bytes = "std.fs_path"};}

static void fs_path__dump(t_thrd_data* const thrd_data, t_any* const result, t_any const path, u64 const, u64 const, const char* const owner) {
     dump__add_string__own(thrd_data, result, (const t_any){.bytes = "[fs_path "}, owner);
     t_any const string = string_from_n_len_sysstr(thrd_data, fs_path__len(path), fs_path__path(path), owner);
     if (string.bytes[15] == tid__error) [[clang::unlikely]]
          ref_cnt__dec(thrd_data, string);
     else dump__add_string__own(thrd_data, result, string, owner);
     dump__add_string__own(thrd_data, result, (const t_any){.bytes = "]\n"}, owner);
}

static t_any fs_path__to_shared(t_thrd_data* const thrd_data, t_any const path, t_shared_fn const shared_fn, bool const nested, const char* const owner) {
     t_custom_types_data* const data = custom__data(path);

     if (data->ref_cnt < 2)
          return path;

     data->ref_cnt -= 1;

     u32                  const len      = fs_path__len(path);
     t_custom_types_data* const new_data = malloc(sizeof(t_custom_types_data) + len + 1);
     new_data->ref_cnt                   = 1;
     new_data->fns                       = data->fns;
     memcpy(&new_data[1], &data[1], len + 1);

     return (const t_any){.qwords = {(u64)new_data, path.qwords[1]}};
}

static t_custom_fns const fs_path__fns = {
     .call_mtd        = fs_path__call_mtd,
     .has_mtd         = fs_path__has_mtd,
     .free_function   = fs_path__free,
     .to_global_const = fs_path__to_global_const,
     .type            = fs_path__type,
     .dump            = fs_path__dump,
     .to_shared       = fs_path__to_shared,
};

t_any MstdFNfs_path_from_stringAAA(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - std.fs_path";

     t_any const string = args[0];
     if (string.bytes[15] == tid__short_string) {
          u8 buffer[113];

          u64              const string_size = short_string__get_size(string);
          t_str_cvt_result const cvt_result  = ctf8_chars_to_sys_chars(string_size, string.bytes, 113, buffer);
          if (cvt_result.src_offset != string_size) [[clang::unlikely]] goto fail_text_recoding_label;

          buffer[cvt_result.dst_offset]          = 0;
          t_custom_types_data* const new_fs_path = malloc(sizeof(t_custom_types_data) + cvt_result.dst_offset + 1);
          new_fs_path->ref_cnt                   = 1;
          new_fs_path->fns.pointer               = (void*)&fs_path__fns;
          memcpy(&new_fs_path[1], buffer, cvt_result.dst_offset + 1);

          call_stack__pop_if_tail_call(thrd_data);
          return (const t_any){.qwords = {(u64)new_fs_path, cvt_result.dst_offset | (u64)tid__custom << 56}};
     }

     assume(string.bytes[15] == tid__string);

     u32 const slice_offset = slice__get_offset(string);
     u32 const slice_len    = slice__get_len(string);
     u64 const ref_cnt      = get_ref_cnt(string);
     u64 const array_len    = slice_array__get_len(string);
     u64 const string_len   = slice_len <= slice_max_len ? slice_len : array_len;
     u64 const string_size  = string_len * 3;
     u8* const string_chars = &((u8*)slice_array__get_items(string))[slice_offset * 3];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(string, ref_cnt - 1);

     u64                  sysstr_cap   = (string_len < 16 ? 16 : string_len) + 17;
     t_custom_types_data* fs_path_mem  = malloc(sizeof(t_custom_types_data) + sysstr_cap);
     fs_path_mem->ref_cnt              = 1;
     fs_path_mem->fns.pointer          = (void*)&fs_path__fns;
     u8*                  sysstr_chars = (u8*)&fs_path_mem[1];

     t_str_cvt_result current_offsets = {};
     while (true) {
          t_str_cvt_result const cvt_result = corsar_chars_to_sys_chars(
               string_size - current_offsets.src_offset, &string_chars[current_offsets.src_offset],
               sysstr_cap - current_offsets.dst_offset, &sysstr_chars[current_offsets.dst_offset]
          );

          if ((i64)cvt_result.src_offset <= 0) [[clang::unlikely]] {
               free(fs_path_mem);
               if (ref_cnt == 1) free((void*)string.qwords[0]);
               goto fail_text_recoding_label;
          }

          current_offsets.src_offset += cvt_result.src_offset;
          current_offsets.dst_offset += cvt_result.dst_offset;

          if (current_offsets.dst_offset > max_path_len) {
               free(fs_path_mem);
               if (ref_cnt == 1) free((void*)string.qwords[0]);

               call_stack__pop_if_tail_call(thrd_data);
               return tkn__long;
          }

          if (current_offsets.src_offset == string_size) {
               if (ref_cnt == 1) free((void*)string.qwords[0]);

               if (sysstr_cap - current_offsets.dst_offset != 1) {
                    fs_path_mem = realloc(fs_path_mem, sizeof(t_custom_types_data) + current_offsets.dst_offset + 1);
                    sysstr_chars = (u8*)&fs_path_mem[1];
               }
               sysstr_chars[current_offsets.dst_offset] = 0;

               call_stack__pop_if_tail_call(thrd_data);
               return (const t_any){.qwords = {(u64)fs_path_mem, current_offsets.dst_offset | (u64)tid__custom << 56}};
          }

          sysstr_cap   = sysstr_cap * 3 / 2;
          fs_path_mem  = realloc(fs_path_mem, sizeof(t_custom_types_data) + sysstr_cap);
          sysstr_chars = (u8*)&fs_path_mem[1];
     }

     fail_text_recoding_label:
     t_any const error = error__fail_text_recoding(thrd_data, owner);

     call_stack__pop_if_tail_call(thrd_data);
     return error;
}

static t_std_fs_path const stdout_fs_path_data = {
     .custom_data = (const t_custom_types_data) {
          .ref_cnt = 0,
          .fns     = (void*)&fs_path__fns,
     },
     .fs_path = "/\\STDOUT/\\\0\0\0\0\0\0"
};

static t_std_fs_path const stdin_fs_path_data = {
     .custom_data = (const t_custom_types_data) {
          .ref_cnt = 0,
          .fns     = (void*)&fs_path__fns,
     },
     .fs_path = "/\\STDIN/\\\0\0\0\0\0\0\0"
};

static t_std_fs_path const stderr_fs_path_data = {
     .custom_data = (const t_custom_types_data) {
          .ref_cnt = 0,
          .fns     = (void*)&fs_path__fns,
     },
     .fs_path = "/\\STDERR/\\\0\0\0\0\0\0"
};

static t_any const stdout_fs_path = {.structure = {.value = (u64)&stdout_fs_path_data, .data = {15}, .type = tid__custom}};
static t_any const stdin_fs_path  = {.structure = {.value = (u64)&stdin_fs_path_data, .data = {14}, .type = tid__custom}};
static t_any const stderr_fs_path = {.structure = {.value = (u64)&stderr_fs_path_data, .data = {15}, .type = tid__custom}};