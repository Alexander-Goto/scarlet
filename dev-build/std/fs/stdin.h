#pragma once

#include "../include/basic.h"
#include "../include/include.h"
#include "file.h"
#include "fs_path.h"

t_any MstdFNstdin(t_thrd_data* const thrd_data, const t_any* const) {
     t_file* const new_file            = malloc(sizeof(t_file));
     new_file->custom_data.ref_cnt     = 1;
     new_file->custom_data.fns.pointer = (void*)&file__fns;
     new_file->c_file                  = stdin;
     new_file->is_flushed              = true;
     new_file->dont_close              = true;

     #ifdef DEBUG
     new_file->file_name = stdin_fs_path;
     #endif

     call_stack__pop_if_tail_call(thrd_data);
     return (const t_any){.structure = {.value = (u64)new_file, .type = tid__custom}};
}