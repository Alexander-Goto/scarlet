#pragma once

#include "../../common/corsar.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../struct/string/basic.h"
#include "../../struct/table/fn.h"
#include "var.h"

core_basic const char* const* get_c_env_vars(u64* const len) {
     *len = c_env_vars_len;
     return c_env_vars;
}

static const char* const core_env_vars_gconst_name = "constant - core.env_vars";

static t_any env_vars;

static t_any gconst__env_vars(t_thrd_data* const thrd_data) {
     call_stack__push(thrd_data, core_env_vars_gconst_name);

     u32 const vars_len = c_env_vars_len;
     env_vars = table__init(thrd_data, vars_len, core_env_vars_gconst_name);
     for (u64 idx = 0; idx < vars_len; idx += 1) {
          const char* const name_and_value    = c_env_vars[idx];
          const char* const separator_address = strchr(name_and_value, '=');
          if (separator_address == nullptr) [[clang::unlikely]] {
               fprintf(stderr, "Invalid environment variable format. (%s)\n", name_and_value);
               exit(EXIT_FAILURE);
          }

          u64 const separator_idx = (u64)separator_address - (u64)name_and_value;
          t_any const name = string_from_n_len_sysstr(thrd_data, separator_idx, name_and_value, core_env_vars_gconst_name);
          if (name.bytes[15] == tid__error) [[clang::unlikely]] {
               call_stack__pop(thrd_data);

               ref_cnt__dec(thrd_data, env_vars);
               return name;
          }

          assert(name.bytes[15] == tid__short_string || name.bytes[15] == tid__string);

          t_any const value = string_from_ze_sysstr(thrd_data, &separator_address[1], core_env_vars_gconst_name);
          if (value.bytes[15] == tid__error) [[clang::unlikely]] {
               call_stack__pop(thrd_data);

               ref_cnt__dec(thrd_data, name);
               ref_cnt__dec(thrd_data, env_vars);
               return value;
          }

          assert(value.bytes[15] == tid__short_string || value.bytes[15] == tid__string);

          env_vars = table__builtin_add_kv__own(thrd_data, env_vars, name, value, core_env_vars_gconst_name);
          if (env_vars.bytes[15] == tid__error) [[clang::unlikely]] break;
     }
     call_stack__pop(thrd_data);

     return env_vars;
}
