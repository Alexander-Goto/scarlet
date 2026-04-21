#pragma once

#include "../fs/fs_path.h"
#include "../include/basic.h"
#include "../include/include.h"

static const char* const process__mtd_wait     = "function - @std.process->wait";
static const char* const process__mtd_try_wait = "function - @std.process->try_wait";

struct {
     t_custom_types_data custom__data;

#ifdef DEBUG
     t_any file_name;
#endif

     int   result;
     pid_t pid;
} typedef t_process;

[[clang::always_inline]]
static t_process* process__data(t_any const process) {return (t_process*)process.qwords[0];}

static void process__free(t_thrd_data* const, const t_any process) {
     t_process* const data = process__data(process);

#ifdef DEBUG
     fs_path__ref_cnt_dec(data->file_name);
#endif

     pid_t const pid = data->pid;
     free((void*)process.qwords[0]);

     if (pid != 0) waitpid(pid, nullptr, 0);
}

static t_any process__call_mtd(t_thrd_data* const thrd_data, const char* const owner, t_any const mtd_tkn, const t_any* const args, u8 const args_len) {
     t_any const process = args[0];
     const char* mtd_name;

     switch (mtd_tkn.raw_bits) {
     case 0x9e98214d4eec88eaf8b81c70fe057d7uwb: { // wait
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = process__mtd_wait;
               goto wrong_num_fn_args;
          }

          t_process* const data = process__data(process);
          pid_t      const pid  = data->pid;

          t_any result;
          if (pid == 0)
               result = (const t_any){.structure = {.value = data->result, .type = tid__int}};
          else {
               int result_code;
               if (waitpid(pid, &result_code, 0) != pid) return (const t_any){.structure = {.value = 1, .type = tid__int}};

               data->result = result_code;
               data->pid    = 0;

               result = (const t_any){.structure = {.value = result_code, .type = tid__int}};
          }

          if (data->custom__data.ref_cnt == 1) process__free(thrd_data, process);
          else data->custom__data.ref_cnt -= 1;

          return result;
     }
     case 0x9e972ec2b9b5b964f8878a886aa5d13uwb: { // try_wait
          if (args_len != 1) [[clang::unlikely]] {
               mtd_name = process__mtd_try_wait;
               goto wrong_num_fn_args;
          }

          t_process* const data = process__data(process);
          pid_t      const pid  = data->pid;

          t_any result;
          int   result_code;
          if (pid == 0)
               result = (const t_any){.structure = {.value = data->result, .type = tid__int}};
          else if (waitpid(pid, &result_code, WNOHANG) == pid) {
               data->result = result_code;
               data->pid    = 0;

               result = (const t_any){.structure = {.value = result_code, .type = tid__int}};
          } else result = null;

          if (data->custom__data.ref_cnt == 1) process__free(thrd_data, process);
          else data->custom__data.ref_cnt -= 1;

          return result;
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

static bool process__has_mtd(t_any const mtd_tkn) {
     switch (mtd_tkn.raw_bits) {
     case 0x9e98214d4eec88eaf8b81c70fe057d7uwb: // wait
     case 0x9e972ec2b9b5b964f8878a886aa5d13uwb: // try_wait
          return true;
     default:
          return false;
     }
}

static t_any process__to_global_const(t_thrd_data* const thrd_data, t_any const process, const char* const owner) {fail_with_call_stack(thrd_data, "A process cannot be a global constant value.", owner);}

static t_any process__type(void) {return (const t_any){.bytes = "std.process"};}

static void process__dump(t_thrd_data* const thrd_data, t_any* const result, t_any const process, u64 const, u64 const, const char* const owner) {
#ifdef DEBUG
     const t_process* const data      = process__data(process);
     t_any            const file_name = data->file_name;
     dump__add_string__own(thrd_data, result, (const t_any){.bytes = "[process "}, owner);
     t_any const string = string_from_n_len_sysstr(thrd_data, fs_path__len(file_name), fs_path__path(file_name), owner);
     if (string.bytes[15] == tid__error) [[clang::unlikely]]
          ref_cnt__dec(thrd_data, string);
     else dump__add_string__own(thrd_data, result, string, owner);
     dump__add_string__own(thrd_data, result, (const t_any){.bytes = "]\n"}, owner);
#else
     dump__add_string__own(thrd_data, result, (const t_any){.bytes = "[process]\n"}, owner);
#endif
}

static t_any process__to_shared(t_thrd_data* const thrd_data, t_any const process, t_shared_fn const shared_fn, bool const nested, const char* const owner) {
     t_process* const data = process__data(process);
     if (data->custom__data.ref_cnt == 1) [[clang::likely]] return process;

     data->custom__data.ref_cnt -= 1;

     return builtin_error__new(thrd_data, owner, "share_invalid_type", "Sharing a used process from thread to thread.");
}

static t_custom_fns const process__fns = {
     .call_mtd        = process__call_mtd,
     .has_mtd         = process__has_mtd,
     .free_function   = process__free,
     .to_global_const = process__to_global_const,
     .type            = process__type,
     .dump            = process__dump,
     .to_shared       = process__to_shared,
};