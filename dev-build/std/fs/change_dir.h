#pragma once

#include "../include/basic.h"
#include "../include/include.h"
#include "fs_path.h"

t_any MstdFNchange_dir(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - std.change_dir";

     t_any const path = args[0];
     if (path.bytes[15] != tid__custom || custom__data(path)->fns.pointer != (void*)&fs_path__fns) [[clang::unlikely]] {
          if (path.bytes[15] == tid__error) return path;
          ref_cnt__dec(thrd_data, path);

          call_stack__push(thrd_data, owner);
          t_any const error = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return error;
     }

     t_custom_types_data* const data = custom__data(path);

     u64 const ref_cnt = data->ref_cnt;
     if (ref_cnt > 1) data->ref_cnt = ref_cnt - 1;

     const char* const c_path = fs_path__path(path);

     struct stat dir_stat;
     if (stat(c_path, &dir_stat) != 0) {
          if (ref_cnt == 1) fs_path__free(thrd_data, path);

          switch (errno) {
               case EACCES:               return tkn__access;
               case ENAMETOOLONG:         return tkn__long;
               case ENOENT: case ENOTDIR: return tkn__not_exist;
               default:                   return tkn__delete;
          }
     }

     if ((dir_stat.st_mode & S_IFDIR) != S_IFDIR) {
          if (ref_cnt == 1) fs_path__free(thrd_data, path);
          return tkn__not_dir;
     }

     bool const success = chdir(c_path) == 0;

     if (ref_cnt == 1) fs_path__free(thrd_data, path);

     if (success) return null;

     switch (errno) {
     case EACCES:                                  return tkn__access;
     case ENAMETOOLONG: case ENOENT: case ENOTDIR: unreachable;
     default:                                      return tkn__change_dir;
     }
}