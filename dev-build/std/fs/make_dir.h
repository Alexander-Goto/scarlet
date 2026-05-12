#pragma once

#include "../include/basic.h"
#include "../include/include.h"
#include "fs_path.h"

t_any MstdFNmake_dir(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - std.make_dir";

     t_any const path = args[0];
     if (path.bytes[15] != tid__custom || custom__data(path)->fns.pointer != (void*)&fs_path__fns) [[clang::unlikely]] {
          if (path.bytes[15] == tid__error) return path;
          ref_cnt__dec(thrd_data, path);

          call_stack__push(thrd_data, owner);
          t_any const error = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return error;
     }

     call_stack__pop_if_tail_call(thrd_data);

     t_custom_types_data* const data = custom__data(path);

     u64 const ref_cnt = data->ref_cnt;
     if (ref_cnt > 1) data->ref_cnt = ref_cnt - 1;

     bool const success = mkdir(fs_path__path(path), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
     if (ref_cnt == 1) fs_path__free(thrd_data, path);

     if (success) return null;

     switch (errno) {
     case EACCES: case EROFS:   return tkn__access;
     case EEXIST:               return tkn__exists;
     case ENAMETOOLONG:         return tkn__long;
     case ENOENT: case ENOTDIR: return tkn__not_exist;
     case ENOSPC:               return tkn__space;
     default:                   return tkn__make;
     }
}