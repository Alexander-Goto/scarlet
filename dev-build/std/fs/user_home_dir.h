#pragma once

#include "../include/include.h"
#include "fs_path.h"

t_any MstdFNuser_home_dir(t_thrd_data* const thrd_data, const t_any* const) {
     static const char* const owner = "constant - std.user_home_dir";

     const char* home_dir = getenv("HOME");
     if (home_dir == nullptr) {
          struct passwd* const pw = getpwuid(getuid());
          if (pw == nullptr) {
               call_stack__pop_if_tail_call(thrd_data);
               return null;
          }

          home_dir           = pw->pw_dir;
          u64 const path_len = strlen(home_dir);

          if (path_len > max_path_len) [[clang::unlikely]] {
               t_any const error = builtin_error__new(thrd_data, owner, "long", "The path is too long.");

               call_stack__pop_if_tail_call(thrd_data);
               return error;
          }

          t_custom_types_data* const fs_path = malloc(sizeof(t_custom_types_data) + path_len + 1);
          fs_path->ref_cnt                   = 1;
          fs_path->fns.pointer               = (void*)&fs_path__fns;
          memcpy(&fs_path[1], home_dir, path_len + 1);

          call_stack__pop_if_tail_call(thrd_data);
          return (const t_any){.qwords = {(u64)fs_path, path_len | (u64)tid__custom << 56}};
     }

     t_any const string = string_from_ze_sysstr(thrd_data, home_dir, owner);
     if (string.bytes[15] == tid__error) {
          call_stack__pop_if_tail_call(thrd_data);
          return string;
     }

     return MstdFNfs_path_from_stringAAA(thrd_data, &string);
}