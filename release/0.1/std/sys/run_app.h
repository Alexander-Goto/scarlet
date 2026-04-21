#pragma once

#include "../fs/file.h"
#include "../fs/fs_path.h"
#include "../include/include.h"
#include "process.h"

#define raf__stdout (u8)1
#define raf__stdin  (u8)2
#define raf__stderr (u8)4

static t_std_fs_path const pipe_stdout_fs_path_data = {
     .custom_data = (const t_custom_types_data) {
          .ref_cnt = 0,
          .fns     = (void*)&fs_path__fns,
     },
     .fs_path = "/\\PIPE STDOUT/\\\0"
};

static t_std_fs_path const pipe_stdin_fs_path_data = {
     .custom_data = (const t_custom_types_data) {
          .ref_cnt = 0,
          .fns     = (void*)&fs_path__fns,
     },
     .fs_path = "/\\PIPE STDIN/\\\0\0"
};

static t_std_fs_path const pipe_stderr_fs_path_data = {
     .custom_data = (const t_custom_types_data) {
          .ref_cnt = 0,
          .fns     = (void*)&fs_path__fns,
     },
     .fs_path = "/\\PIPE STDERR/\\\0"
};

static t_any const pipe_stdout_fs_path = {.structure = {.value = (u64)&pipe_stdout_fs_path_data, .data = {15}, .type = tid__custom}};
static t_any const pipe_stdin_fs_path  = {.structure = {.value = (u64)&pipe_stdin_fs_path_data, .data = {14}, .type = tid__custom}};
static t_any const pipe_stderr_fs_path = {.structure = {.value = (u64)&pipe_stderr_fs_path_data, .data = {15}, .type = tid__custom}};

t_any MstdFNrun_app(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - std.run_app";

     u8 buffer[113];

     t_any const app_file = args[0];
     t_any const app_args = args[1];
     t_any const env_vars = args[2];
     t_any const flags    = args[3];
     if (!(
          app_file.bytes[15] == tid__custom && custom__data(app_file)->fns.pointer == (void*)&fs_path__fns           &&
          app_args.bytes[15] == tid__array  && (env_vars.bytes[15] == tid__table || env_vars.bytes[15] == tid__null) &&
          flags.bytes[15] == tid__int       && flags.qwords[0] < 8
     )) [[clang::unlikely]] {
          if (app_file.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, app_args);
               ref_cnt__dec(thrd_data, env_vars);
               ref_cnt__dec(thrd_data, flags);
               return app_file;
          }

          if (app_args.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, app_file);
               ref_cnt__dec(thrd_data, env_vars);
               ref_cnt__dec(thrd_data, flags);
               return app_args;
          }

          if (env_vars.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, app_file);
               ref_cnt__dec(thrd_data, app_args);
               ref_cnt__dec(thrd_data, flags);
               return env_vars;
          }

          if (flags.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, app_file);
               ref_cnt__dec(thrd_data, app_args);
               ref_cnt__dec(thrd_data, env_vars);
               return flags;
          }

          if (
               app_file.bytes[15] == tid__custom && custom__data(app_file)->fns.pointer == (void*)&fs_path__fns           &&
               app_args.bytes[15] == tid__array  && (env_vars.bytes[15] == tid__table || env_vars.bytes[15] == tid__null) &&
               flags.bytes[15] == tid__int
          ) goto invalid_flags_label;

          goto invalid_type_label;
     }

     const char*       bin_name;
     const char* const c_app_file = fs_path__path(app_file);
     {
          u64 const last_separator_position = look_byte_from_end((u8*)c_app_file, fs_path__len(app_file), path_separator);
          if ((i64)last_separator_position < 1) goto not_run_label;
          bin_name = &c_app_file[last_separator_position + 1];
     }

     u32          const args_slice_offset = slice__get_offset(app_args);
     u32          const args_slice_len    = slice__get_len(app_args);
     u64          const args_len          = args_slice_len <= slice_max_len ? args_slice_len : slice_array__get_len(app_args);
     const t_any* const args_items        = &((const t_any*)slice_array__get_items(app_args))[args_slice_offset];
     char**       const c_args            = malloc((args_len + 2) * sizeof(char*));
     c_args[0]                            = (char*)bin_name;
     for (u64 arg_idx = 0; arg_idx < args_len; arg_idx += 1) {
          t_any const one_arg = args_items[arg_idx];
          switch (one_arg.bytes[15]) {
          case tid__short_string: {
               u8               const arg_string_size = short_string__get_size(one_arg);
               t_str_cvt_result const cvt_result      = ctf8_chars_to_sys_chars(arg_string_size, one_arg.bytes, 113, buffer);
               if (cvt_result.src_offset != arg_string_size) [[clang::unlikely]] {
                    for (u64 free_idx = arg_idx; free_idx != 0; free(c_args[free_idx]), free_idx -= 1);
                    free(c_args);
                    goto fail_text_recoding_label;
               }

               char* const c_one_arg = malloc(cvt_result.dst_offset + 1);
               memcpy(c_one_arg, buffer, cvt_result.dst_offset);
               c_one_arg[cvt_result.dst_offset] = 0;
               c_args[arg_idx + 1]              = c_one_arg;
               break;
          }
          case tid__string: {
               u32       const arg_slice_offset = slice__get_offset(one_arg);
               u32       const arg_slice_len    = slice__get_len(one_arg);
               u64       const arg_len          = arg_slice_len <= slice_max_len ? arg_slice_len : slice_array__get_len(one_arg);
               u64       const arg_size         = arg_len * 3;
               const u8* const arg_chars        = &((const u8*)slice_array__get_items(one_arg))[arg_slice_offset * 3];

               u64   c_arg_cap = (arg_len < 16 ? 16 : arg_len) + 17;
               char* c_one_arg = malloc(c_arg_cap);

               t_str_cvt_result current_offsets = {};
               while (true) {
                    t_str_cvt_result const cvt_result = corsar_chars_to_sys_chars(
                         arg_size - current_offsets.src_offset, &arg_chars[current_offsets.src_offset],
                         c_arg_cap - current_offsets.dst_offset, (u8*)&c_one_arg[current_offsets.dst_offset]
                    );

                    if ((i64)cvt_result.src_offset <= 0) [[clang::unlikely]] {
                         free(c_one_arg);
                         for (u64 free_idx = arg_idx; free_idx != 0; free(c_args[free_idx]), free_idx -= 1);
                         free(c_args);
                         goto fail_text_recoding_label;
                    }

                    current_offsets.src_offset += cvt_result.src_offset;
                    current_offsets.dst_offset += cvt_result.dst_offset;
                    if (current_offsets.src_offset == arg_size) {
                         if (c_arg_cap - current_offsets.dst_offset != 1)
                              c_one_arg = realloc(c_one_arg, current_offsets.dst_offset + 1);

                         c_one_arg[current_offsets.dst_offset] = 0;
                         c_args[arg_idx + 1]                   = c_one_arg;

                         break;
                    }

                    c_arg_cap = c_arg_cap * 3 / 2;
                    c_one_arg = realloc(c_one_arg, c_arg_cap);
               }

               break;
          }

          [[clang::unlikely]]
          default:
               for (u64 free_idx = arg_idx; free_idx != 0; free(c_args[free_idx]), free_idx -= 1);
               free(c_args);
               goto invalid_type_label;
          }
     }
     c_args[args_len + 1] = nullptr;

     u64    env_vars_len;
     char** c_env_vars;
     bool   need_free_env_vars = env_vars.bytes[15] != tid__null;
     if (!need_free_env_vars)
          c_env_vars = (char**)get_c_env_vars(&env_vars_len);
     else {
          t_any* const env_vars_kvs = table__get_kvs_and_len(env_vars, &env_vars_len);
          u64          remain_len   = env_vars_len;
          c_env_vars                = malloc((env_vars_len + 1) * sizeof(char*));
          for (u64 var_idx = 0; remain_len != 0; var_idx += 1) {
               u64   c_kv_len;
               u64   c_kv_cap;
               char* c_kv;

               t_any const key = env_vars_kvs[var_idx * 2];
               switch (key.bytes[15]) {
               case tid__null: case tid__holder:
                    continue;
               case tid__short_string: {
                    u8               const key_string_size = short_string__get_size(key);
                    t_str_cvt_result const cvt_result      = ctf8_chars_to_sys_chars(key_string_size, key.bytes, 113, buffer);
                    if (cvt_result.src_offset != key_string_size) [[clang::unlikely]] {
                         for (u64 free_idx = env_vars_len - remain_len - 1; free_idx != (u64)-1; free(c_env_vars[free_idx]), free_idx -= 1);
                         free(c_env_vars);
                         for (u64 free_idx = 1; free_idx <= args_len; free(c_args[free_idx]), free_idx += 1);
                         free(c_args);
                         goto fail_text_recoding_label;
                    }

                    c_kv_cap = cvt_result.dst_offset * 3 + 2;
                    c_kv_len = cvt_result.dst_offset + 1;
                    c_kv     = malloc(c_kv_cap);
                    memcpy(c_kv, buffer, cvt_result.dst_offset);
                    c_kv[cvt_result.dst_offset] = '=';

                    break;
               }
               case tid__string: {
                    u32       const key_slice_offset = slice__get_offset(key);
                    u32       const key_slice_len    = slice__get_len(key);
                    u64       const key_len          = key_slice_len <= slice_max_len ? key_slice_len : slice_array__get_len(key);
                    u64       const key_size         = key_len * 3;
                    const u8* const key_chars        = &((const u8*)slice_array__get_items(key))[key_slice_offset * 3];

                    c_kv_cap = (key_len < 16 ? 16 : key_len) * 3 + 18;
                    c_kv     = malloc(c_kv_cap);

                    t_str_cvt_result current_offsets = {};
                    while (true) {
                         t_str_cvt_result const cvt_result = corsar_chars_to_sys_chars(
                              key_size - current_offsets.src_offset, &key_chars[current_offsets.src_offset],
                              c_kv_cap - current_offsets.dst_offset, (u8*)&c_kv[current_offsets.dst_offset]
                         );

                         if ((i64)cvt_result.src_offset <= 0) [[clang::unlikely]] {
                              free(c_kv);
                              for (u64 free_idx = env_vars_len - remain_len - 1; free_idx != (u64)-1; free(c_env_vars[free_idx]), free_idx -= 1);
                              free(c_env_vars);
                              for (u64 free_idx = 1; free_idx <= args_len; free(c_args[free_idx]), free_idx += 1);
                              free(c_args);
                              goto fail_text_recoding_label;
                         }

                         current_offsets.src_offset += cvt_result.src_offset;
                         current_offsets.dst_offset += cvt_result.dst_offset;
                         if (current_offsets.src_offset == key_size) {
                              if (c_kv_cap == current_offsets.dst_offset) {
                                   c_kv_cap = c_kv_cap * 3 / 2 + 18;
                                   c_kv     = realloc(c_kv, c_kv_cap);
                              }

                              c_kv[current_offsets.dst_offset] = '=';
                              c_kv_len                         = current_offsets.dst_offset + 1;

                              break;
                         }

                         c_kv_cap = c_kv_cap * 3 / 2 + 18;
                         c_kv     = realloc(c_kv, c_kv_cap);
                    }

                    break;
               }

               [[clang::unlikely]]
               default:
                    for (u64 free_idx = env_vars_len - remain_len - 1; free_idx != (u64)-1; free(c_env_vars[free_idx]), free_idx -= 1);
                    free(c_env_vars);
                    for (u64 free_idx = 1; free_idx <= args_len; free(c_args[free_idx]), free_idx += 1);
                    free(c_args);
                    goto invalid_type_label;
               }

               t_any const value = env_vars_kvs[var_idx * 2 + 1];
               switch (value.bytes[15]) {
               case tid__null: case tid__holder:
                    unreachable;
               case tid__short_string: {
                    bool             const enought_cap       = (c_kv_cap - c_kv_len) >= 113;
                    u8               const value_string_size = short_string__get_size(value);
                    t_str_cvt_result const cvt_result        = ctf8_chars_to_sys_chars(value_string_size, value.bytes, 113, enought_cap ? (u8*)&c_kv[c_kv_len] : buffer);
                    if (cvt_result.src_offset != value_string_size) [[clang::unlikely]] {
                         free(c_kv);
                         for (u64 free_idx = env_vars_len - remain_len - 1; free_idx != (u64)-1; free(c_env_vars[free_idx]), free_idx -= 1);
                         free(c_env_vars);
                         for (u64 free_idx = 1; free_idx <= args_len; free(c_args[free_idx]), free_idx += 1);
                         free(c_args);
                         goto fail_text_recoding_label;
                    }

                    u64 const new_c_kv_len = c_kv_len + cvt_result.dst_offset;
                    if (c_kv_cap - new_c_kv_len != 1)
                         c_kv = realloc(c_kv, new_c_kv_len + 1);

                    if (!enought_cap)
                         memcpy(&c_kv[c_kv_len], buffer, cvt_result.dst_offset);

                    c_kv_len       = new_c_kv_len;
                    c_kv[c_kv_len] = 0;

                    break;
               }
               case tid__string: {
                    u32       const value_slice_offset = slice__get_offset(value);
                    u32       const value_slice_len    = slice__get_len(value);
                    u64       const value_len          = value_slice_len <= slice_max_len ? value_slice_len : slice_array__get_len(value);
                    u64       const value_size         = value_len * 3;
                    const u8* const value_chars        = &((const u8*)slice_array__get_items(value))[value_slice_offset * 3];

                    if (c_kv_len + value_len + 16 > c_kv_cap) {
                         c_kv_cap = c_kv_len + value_len + 17;
                         c_kv     = realloc(c_kv, c_kv_cap);
                    }

                    t_str_cvt_result current_offsets = {.dst_offset = c_kv_len};
                    while (true) {
                         t_str_cvt_result const cvt_result = corsar_chars_to_sys_chars(
                              value_size - current_offsets.src_offset, &value_chars[current_offsets.src_offset],
                              c_kv_cap - current_offsets.dst_offset, (u8*)&c_kv[current_offsets.dst_offset]
                         );

                         if ((i64)cvt_result.src_offset <= 0) [[clang::unlikely]] {
                              free(c_kv);
                              for (u64 free_idx = env_vars_len - remain_len - 1; free_idx != (u64)-1; free(c_env_vars[free_idx]), free_idx -= 1);
                              free(c_env_vars);
                              for (u64 free_idx = 1; free_idx <= args_len; free(c_args[free_idx]), free_idx += 1);
                              free(c_args);
                              goto fail_text_recoding_label;
                         }

                         current_offsets.src_offset += cvt_result.src_offset;
                         current_offsets.dst_offset += cvt_result.dst_offset;
                         if (current_offsets.src_offset == value_size) {
                              if (c_kv_cap - current_offsets.dst_offset != 1)
                                   c_kv = realloc(c_kv, current_offsets.dst_offset + 1);

                              c_kv[current_offsets.dst_offset] = 0;
                              break;
                         }

                         c_kv_cap = c_kv_cap * 3 / 2 + 17;
                         c_kv     = realloc(c_kv, c_kv_cap);
                    }

                    break;
               }

               [[clang::unlikely]]
               default:
                    free(c_kv);
                    for (u64 free_idx = env_vars_len - remain_len - 1; free_idx != (u64)-1; free(c_env_vars[free_idx]), free_idx -= 1);
                    free(c_env_vars);
                    for (u64 free_idx = 1; free_idx <= args_len; free(c_args[free_idx]), free_idx += 1);
                    free(c_args);
                    goto invalid_type_label;
               }

               c_env_vars[env_vars_len - remain_len] = c_kv;
               remain_len                           -= 1;
          }

          c_env_vars[env_vars_len] = nullptr;
     }

     call_stack__pop_if_tail_call(thrd_data);

     bool const need_stdout = (flags.bytes[0] & raf__stdout) == raf__stdout;
     bool const need_stdin  = (flags.bytes[0] & raf__stdin) == raf__stdin;
     bool const need_stderr = (flags.bytes[0] & raf__stderr) == raf__stderr;

     int stdout_fildes[2];
     int stdin_fildes[2];
     int stderr_fildes[2];

     FILE* child_stdout;
     FILE* child_stdin;
     FILE* child_stderr;

     if (need_stdout) {
          if (pipe(stdout_fildes) != 0) goto free_after_env_vars;

          child_stdout = fdopen(stdout_fildes[0], "rb");
          if (child_stdout == nullptr) {
               close(stdout_fildes[0]);
               close(stdout_fildes[1]);

               goto free_after_env_vars;
          }
     }

     if (need_stdin) {
          if (pipe(stdin_fildes) != 0) {
               if (need_stdout) {
                    fclose(child_stdout);
                    close(stdout_fildes[1]);
               }

               goto free_after_env_vars;
          }

          child_stdin = fdopen(stdin_fildes[1], "wb");
          if (child_stdin == nullptr) {
               close(stdin_fildes[0]);
               close(stdin_fildes[1]);

               if (need_stdout) {
                    fclose(child_stdout);
                    close(stdout_fildes[1]);
               }

               goto free_after_env_vars;
          }
     }

     if (need_stderr) {
          if (pipe(stderr_fildes) != 0) {
               if (need_stdout) {
                    fclose(child_stdout);
                    close(stdout_fildes[1]);
               }

               if (need_stdin) {
                    close(stdin_fildes[0]);
                    fclose(child_stdin);
               }

               goto free_after_env_vars;
          }

          child_stderr = fdopen(stderr_fildes[0], "rb");
          if (child_stderr == nullptr) {
               close(stderr_fildes[0]);
               close(stderr_fildes[1]);

               if (need_stdout) {
                    fclose(child_stdout);
                    close(stdout_fildes[1]);
               }

               if (need_stdin) {
                    close(stdin_fildes[0]);
                    fclose(child_stdin);
               }

               goto free_after_env_vars;
          }
     }

     int status_fildes[2];
     if (pipe(status_fildes) != 0) goto free_after_pipes;

     pid_t const child_pid = fork();
     if (child_pid == (pid_t)-1) {
          close(status_fildes[0]);
          close(status_fildes[1]);

          goto free_after_pipes;
     }

     if (child_pid != 0) {
          close(stdout_fildes[1]);
          close(stdin_fildes[0]);
          close(stderr_fildes[1]);
          close(status_fildes[1]);

          if (need_free_env_vars) {
               for (u64 free_idx = 0; free_idx < env_vars_len; free(c_env_vars[free_idx]), free_idx += 1);
               free(c_env_vars);
          }
          for (u64 free_idx = 1; free_idx <= args_len; free(c_args[free_idx]), free_idx += 1);
          free(c_args);

          char unneeded_buf;
          if (read(status_fildes[0], &unneeded_buf, 1) == 1) {

               close(stderr_fildes[0]);
               waitpid(child_pid, nullptr, 0);
               goto not_run_label;
          }

#ifndef DEBUG
          ref_cnt__dec(thrd_data, app_file);
#endif
          ref_cnt__dec(thrd_data, app_args);
          ref_cnt__dec(thrd_data, env_vars);

          t_any file_stdout = null;
          t_any file_stdin  = null;
          t_any file_stderr = null;

          if (need_stdout) {
               t_file* const new_file            = malloc(sizeof(t_file));
               new_file->custom_data.ref_cnt     = 1;
               new_file->custom_data.fns.pointer = (void*)&file__fns;
               new_file->c_file                  = child_stdout;
               new_file->is_flushed              = true;
               new_file->dont_close              = false;

#ifdef DEBUG
               new_file->file_name = pipe_stdout_fs_path;
#endif
               file_stdout = (const t_any){.structure = {.value = (u64)new_file, .type = tid__custom}};
          }

          if (need_stdin) {
               t_file* const new_file            = malloc(sizeof(t_file));
               new_file->custom_data.ref_cnt     = 1;
               new_file->custom_data.fns.pointer = (void*)&file__fns;
               new_file->c_file                  = child_stdin;
               new_file->is_flushed              = true;
               new_file->dont_close              = false;

#ifdef DEBUG
               new_file->file_name = pipe_stdin_fs_path;
#endif
               file_stdin = (const t_any){.structure = {.value = (u64)new_file, .type = tid__custom}};
          }

          if (need_stderr) {
               t_file* const new_file            = malloc(sizeof(t_file));
               new_file->custom_data.ref_cnt     = 1;
               new_file->custom_data.fns.pointer = (void*)&file__fns;
               new_file->c_file                  = child_stderr;
               new_file->is_flushed              = true;
               new_file->dont_close              = false;

#ifdef DEBUG
               new_file->file_name = pipe_stderr_fs_path;
#endif
               file_stderr = (const t_any){.structure = {.value = (u64)new_file, .type = tid__custom}};
          }

          t_process* const process_data          = malloc(sizeof(t_process));
          process_data->custom__data.ref_cnt     = 1;
          process_data->custom__data.fns.pointer = (void*)&process__fns;
          process_data->pid                      = child_pid;

#ifdef DEBUG
          process_data->file_name = app_file;
#endif

          t_any  const result       = box__new(thrd_data, 4, file__mtd_read);
          t_any* const result_items = box__get_items(result);
          result_items[0]           = (const t_any){.structure = {.value = (u64)process_data, .type = tid__custom}};
          result_items[1]           = file_stdout;
          result_items[2]           = file_stdin;
          result_items[3]           = file_stderr;

          return result;
     }

     close(status_fildes[0]);
     if (fcntl(status_fildes[1], F_SETFD, FD_CLOEXEC) == -1) {
          write(status_fildes[1], stderr_fildes, 1);
          exit(EXIT_FAILURE);
     }

     if (need_stdout) {
          dup2(stdout_fildes[1], STDOUT_FILENO);
          fclose(child_stdout);
          close(stdout_fildes[1]);
     }

     if (need_stdin)  {
          dup2(stdin_fildes[0], STDIN_FILENO);
          close(stdin_fildes[0]);
          fclose(child_stdin);
     }

     if (need_stderr) {
          dup2(stderr_fildes[1], STDERR_FILENO);
          fclose(child_stderr);
          close(stderr_fildes[1]);
     }

     execve(c_app_file, c_args, c_env_vars);

     write(status_fildes[1], stderr_fildes, 1);
     exit(EXIT_FAILURE);

     free_after_pipes:
     if (need_stdout) {
          fclose(child_stdout);
          close(stdout_fildes[1]);
     }

     if (need_stdin) {
          close(stdin_fildes[0]);
          fclose(child_stdin);
     }

     if (need_stderr) {
          fclose(child_stderr);
          close(stderr_fildes[1]);
     }

     free_after_env_vars:
     if (need_free_env_vars) {
          for (u64 free_idx = 0; free_idx < env_vars_len; free(c_env_vars[free_idx]), free_idx += 1);
          free(c_env_vars);
     }
     for (u64 free_idx = 1; free_idx <= args_len; free(c_args[free_idx]), free_idx += 1);
     free(c_args);

     not_run_label:
     ref_cnt__dec(thrd_data, app_file);
     ref_cnt__dec(thrd_data, app_args);
     ref_cnt__dec(thrd_data, env_vars);

     return null;

     invalid_type_label: {
          ref_cnt__dec(thrd_data, app_file);
          ref_cnt__dec(thrd_data, app_args);
          ref_cnt__dec(thrd_data, env_vars);
          ref_cnt__dec(thrd_data, flags);

          call_stack__push(thrd_data, owner);
          t_any const error = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);
          return error;
     }

     invalid_flags_label: {
          ref_cnt__dec(thrd_data, app_file);
          ref_cnt__dec(thrd_data, app_args);
          ref_cnt__dec(thrd_data, env_vars);

          call_stack__push(thrd_data, owner);
          t_any const error = builtin_error__new(thrd_data, owner, "flags", "Incorrect flags.");
          call_stack__pop(thrd_data);
          return error;
     }

     fail_text_recoding_label: {
          ref_cnt__dec(thrd_data, app_file);
          ref_cnt__dec(thrd_data, app_args);
          ref_cnt__dec(thrd_data, env_vars);

          call_stack__push(thrd_data, owner);
          t_any const error = error__fail_text_recoding(thrd_data, owner);
          call_stack__pop(thrd_data);
          return error;
     }
}