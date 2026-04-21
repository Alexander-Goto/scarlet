#pragma once

#include "../include/basic.h"
#include "../include/include.h"
#include "fs_path.h"

t_any MstdFNcurrent_dir(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - std.current_dir";

     call_stack__pop_if_tail_call(thrd_data);

     u64 const buffer_idx  = linear__alloc(&thrd_data->linear_allocator, 4096, 1);
     char*     buffer      = linear__get_mem_of_idx(&thrd_data->linear_allocator, buffer_idx);
     u32       buffer_size = 4096;
     while (true) {
          if (getcwd(buffer, buffer_size) == buffer) {
               u64 const path_len = look_byte_from_begin((u8*)buffer, 4096, 0);

               assert(path_len > 0);

               t_custom_types_data* const path = malloc(sizeof(t_custom_types_data) + path_len + 1);
               path->ref_cnt                   = 1;
               path->fns.pointer               = (void*)&fs_path__fns;
               memcpy(&path[1], buffer, path_len + 1);

               linear__free(&thrd_data->linear_allocator, buffer_idx);

               return (const t_any){.qwords = {(u64)path, path_len | (u64)tid__custom << 56}};
          }

          if (errno == ERANGE) {
               if (buffer_size == max_path_len) [[clang::unlikely]] {
                    linear__free(&thrd_data->linear_allocator, buffer_idx);
                    return null;
               }

               buffer_size = buffer_size * 3 / 2;
               buffer_size = buffer_size > max_path_len ? max_path_len : buffer_size;

               linear__last_realloc(&thrd_data->linear_allocator, buffer_size);
               buffer = linear__get_mem_of_idx(&thrd_data->linear_allocator, buffer_idx);
          } else {
               linear__free(&thrd_data->linear_allocator, buffer_idx);
               return null;
          }
     }
}