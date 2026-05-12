#pragma once

#include "../include/basic.h"
#include "../include/include.h"
#include "fs_path.h"

t_any MstdFNlist_dir(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - std.list_dir";

     call_stack__push(thrd_data, owner);

     t_any const path = args[0];
     if (path.bytes[15] != tid__custom || custom__data(path)->fns.pointer != (void*)&fs_path__fns) [[clang::unlikely]] {
          if (path.bytes[15] == tid__error) return path;
          ref_cnt__dec(thrd_data, path);

          t_any const error = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return error;
     }

     t_custom_types_data* const data = custom__data(path);

     u64 const ref_cnt = data->ref_cnt;
     if (ref_cnt > 1) data->ref_cnt = ref_cnt - 1;

     const char* const c_path = fs_path__path(path);

     struct stat fs_stat;
     if (stat(c_path, &fs_stat) != 0) {
          if (ref_cnt == 1) fs_path__free(thrd_data, path);

          call_stack__pop(thrd_data);
          switch (errno) {
          case EACCES:               return tkn__access;
          case ENAMETOOLONG:         return tkn__long;
          case ENOENT: case ENOTDIR: return tkn__not_exist;
          default:                   return tkn__list;
          }
     }

     if ((fs_stat.st_mode & S_IFDIR) != S_IFDIR) {
          if (ref_cnt == 1) fs_path__free(thrd_data, path);

          call_stack__pop(thrd_data);
          return tkn__not_dir;
     }

     DIR* const dir = opendir(c_path);
     if (ref_cnt == 1) fs_path__free(thrd_data, path);
     if (dir == nullptr) {
          call_stack__pop(thrd_data);
          switch (errno) {
          case EACCES:                                  return tkn__access;
          case ENAMETOOLONG: case ENOENT: case ENOTDIR: unreachable;
          default:                                      return tkn__list;
          }
     }

     int const dir_descriptor = dirfd(dir);
     if (dir_descriptor == -1) {
          call_stack__pop(thrd_data);
          closedir(dir);
          return tkn__list;
     }

     struct dirent* entry;
     errno = 0;
     while (true) {
          entry               = readdir(dir);
          bool const no_error = errno == 0;
          if (entry == nullptr) {
               call_stack__pop(thrd_data);
               closedir(dir);
               return no_error ? empty_array : tkn__list;
          }

          const char* const name = entry->d_name;
          if (!(name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0)))) break;
     }

     u64       buffer_len = 0;
     u64       buffer_cap = 192;
     u64 const buffer_idx = linear__alloc(&thrd_data->linear_allocator, buffer_cap * 16, 16);
     t_any*    buffer     = linear__get_mem_of_idx(&thrd_data->linear_allocator, buffer_idx);
     while (true) {
          const char* const c_name = entry->d_name;
          if (!(c_name[0] == '.' && (c_name[1] == 0 || (c_name[1] == '.' && c_name[2] == 0)))) {
               char entry_type;
               switch (entry->d_type) {
               case DT_DIR:
                    entry_type = 'D';
                    break;
               case DT_LNK:
                    if (fstatat(dir_descriptor, c_name, &fs_stat, 0) != 0)
                         entry_type = 'U';
                    else entry_type = (fs_stat.st_mode & S_IFDIR) == S_IFDIR ? 'D' : 'F';

                    break;
               case DT_UNKNOWN:
                    entry_type = 'U';
                    break;
               default:
                    entry_type = 'F';
               }

               t_any name = string_from_ze_sysstr(thrd_data, c_name, owner);
               switch (name.bytes[15]) {
               case tid__short_string: {
                    u8 const string_size = short_string__get_size(name);
                    if (string_size != 15)
                         name.bytes[string_size] = entry_type;
                    else {
                         u8 const new_len  = short_string__get_len(name) + 1;
                         t_any    new_name = long_string__new(new_len);
                         slice_array__set_len(new_name, new_len);
                         new_name              = slice__set_metadata(new_name, 0, new_len);
                         u8*  const name_chars = slice_array__get_items(new_name);
                         bool const cvt_ok     = ctf8_chars_to_corsar_chars(string_size, name.bytes, (new_len - 1) * 3, name_chars).src_offset == string_size;

                         assert(cvt_ok);

                         name_chars[new_len * 3 - 3] = 0;
                         name_chars[new_len * 3 - 2] = 0;
                         name_chars[new_len * 3 - 1] = entry_type;

                         name = new_name;
                    }

                    break;
               }
               case tid__string: {
                    assert(slice__get_offset(name) == 0);
                    assert(get_ref_cnt(name) == 1);

                    u32 const slice_len  = slice__get_len(name);
                    u64 const array_len  = slice_array__get_len(name);
                    u64 const array_cap  = slice_array__get_cap(name);
                    u64 const old_len    = slice_len <= slice_max_len ? slice_len : array_len;
                    u64 const new_len    = old_len + 1;
                    u8*       name_chars = slice_array__get_items(name);

                    assert(array_cap >= array_len);

                    name = slice__set_metadata(name, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
                    slice_array__set_len(name, new_len);
                    if (array_cap == array_len) {
                         name.qwords[0] = (u64)realloc((u8*)name.qwords[0], new_len * 3 + 16);
                         name_chars     = slice_array__get_items(name);
                         slice_array__set_cap(name, new_len);
                    }

                    name_chars[new_len * 3 - 3] = 0;
                    name_chars[new_len * 3 - 2] = 0;
                    name_chars[new_len * 3 - 1] = entry_type;

                    break;
               }

               [[clang::unlikely]]
               case tid__error:
                    closedir(dir);
                    call_stack__pop(thrd_data);
                    for (u64 free_idx = buffer_len - 1; buffer_len != (u64)-1; ref_cnt__dec(thrd_data, buffer[free_idx]), free_idx -= 1);
                    linear__free(&thrd_data->linear_allocator, buffer_idx);
                    return name;
               default:
                    unreachable;
               }

               if (buffer_len == buffer_cap) {
                    if (buffer_cap == array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);

                    buffer_cap = buffer_cap * 3 / 2;
                    buffer_cap = buffer_cap > array_max_len ? array_max_len : buffer_cap;
                    linear__last_realloc(&thrd_data->linear_allocator, buffer_cap * 16);
                    buffer = linear__get_mem_of_idx(&thrd_data->linear_allocator, buffer_idx);
               }

               buffer[buffer_len] = name;
               buffer_len        += 1;
          }

          errno               = 0;
          entry               = readdir(dir);
          bool const no_error = errno == 0;
          if (entry == nullptr) {
               closedir(dir);
               call_stack__pop(thrd_data);

               t_any result;
               if (no_error) {
                    result = array__init(thrd_data, buffer_len, owner);
                    result = slice__set_metadata(result, 0, buffer_len <= slice_max_len ? buffer_len : slice_max_len + 1);
                    slice_array__set_len(result, buffer_len);
                    memcpy(slice_array__get_items(result), buffer, buffer_len * 16);
               } else {
                    for (u64 free_idx = buffer_len - 1; buffer_len != (u64)-1; ref_cnt__dec(thrd_data, buffer[free_idx]), free_idx -= 1);
                    result = tkn__list;
               }

               linear__free(&thrd_data->linear_allocator, buffer_idx);
               return result;
          }
     }
}