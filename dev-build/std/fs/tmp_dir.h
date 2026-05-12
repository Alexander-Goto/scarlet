#pragma once

#include "../include/include.h"
#include "fs_path.h"

t_any MstdFNtmp_dir(t_thrd_data* const thrd_data, const t_any* const) {
     static const char* const owner = "constant - std.tmp_dir";

     const char* const env_tmp_dir = getenv("TMPDIR");
     if (env_tmp_dir == nullptr) {
          t_custom_types_data* const fs_path = malloc(sizeof(t_custom_types_data) + 5);
          fs_path->ref_cnt                   = 1;
          fs_path->fns.pointer               = (void*)&fs_path__fns;
          memcpy_inline(&fs_path[1], "/tmp\0", 5);

          call_stack__pop_if_tail_call(thrd_data);
          return (const t_any){.structure = {.value = (u64)fs_path, .data = {5}, .type = tid__custom}};
     }

     t_any const string = string_from_ze_sysstr(thrd_data, env_tmp_dir, owner);
     if (string.bytes[15] == tid__error) {
          call_stack__pop_if_tail_call(thrd_data);
          return string;
     }

     return MstdFNfs_path_from_stringAAA(thrd_data, &string);
}