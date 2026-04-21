#pragma once

#include "../../common/corsar.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../struct/array/fn.h"
#include "../../struct/string/basic.h"
#include "var.h"

static const char* const core_cli_args_gconst_name = "constant - core.cli_args";

static t_any cli_args;

static t_any gconst__cli_args(t_thrd_data* const thrd_data) {
     call_stack__push(thrd_data, core_cli_args_gconst_name);

     cli_args = array__init(thrd_data, app_args_len, core_cli_args_gconst_name);
     for (u64 idx = 0; idx < app_args_len; idx += 1) {
          t_any const arg = string_from_ze_sysstr(thrd_data, app_args[idx], core_cli_args_gconst_name);
          if (arg.bytes[15] == tid__error) [[clang::unlikely]] {
               call_stack__pop(thrd_data);

               ref_cnt__dec(thrd_data, cli_args);
               return arg;
          }

          assert(arg.bytes[15] == tid__short_string || arg.bytes[15] == tid__string);

          cli_args = array__append__own(thrd_data, cli_args, arg, core_cli_args_gconst_name);
     }

     call_stack__pop(thrd_data);

     return cli_args;
}
