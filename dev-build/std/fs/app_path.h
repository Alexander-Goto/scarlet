#pragma once

#include "../include/include.h"
#include "fs_path.h"

t_any MstdFNapp_path(t_thrd_data* const thrd_data, const t_any* const) {
     static const char* const owner = "constant - std.app_path";

     const char* const c_path   = get_app_path();
     u64         const path_len = strlen(c_path);

     if (path_len > max_path_len) [[clang::unlikely]] {
          t_any const error = builtin_error__new(thrd_data, owner, "long", "The path is too long.");

          call_stack__pop_if_tail_call(thrd_data);
          return error;
     }

     t_custom_types_data* const fs_path = malloc(sizeof(t_custom_types_data) + path_len + 1);
     fs_path->ref_cnt                   = 1;
     fs_path->fns.pointer               = (void*)&fs_path__fns;
     memcpy(&fs_path[1], c_path, path_len + 1);

     call_stack__pop_if_tail_call(thrd_data);
     return (const t_any){.qwords = {(u64)fs_path, path_len | (u64)tid__custom << 56}};
}