#pragma once

#include "../../common/const.h"
#include "../../common/corsar.h"
#include "../../common/fn.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../../multithread/var.h"
#include "../../operator/test.h"
#include "../../platform/get_cpu_core_num.h"
#include "../../platform/rnd.h"
#include "../../struct/string/basic.h"
#include "../env/var.h"

[[clang::noinline]]
static void init_app(t_thrd_data** const thrd_data, int const argc, const char* const* const argv, const char* const* const envp) {
     *thrd_data                  = aligned_alloc(alignof(t_thrd_data), sizeof(t_thrd_data));
     (*thrd_data)->free_boxes    = boxes_mask;
     (*thrd_data)->free_breakers = breakers_mask;

     u64 rnd_nums[5];
     platform__init_rnd_nums(rnd_nums);

     memcpy_inline(&(*thrd_data)->rnd_num_src, rnd_nums, 32);
     static_rnd_num = rnd_nums[4];
     app_args_len   = argc - 1;
     app_args       = &(argv[1]);

     {
          c_env_vars     = envp;
          c_env_vars_len = 0;
          for (; envp[c_env_vars_len] != nullptr; c_env_vars_len += 1);
     }

     (*thrd_data)->linear_allocator = (const t_linear_allocator){.mem = nullptr, .states = nullptr};

#ifdef CALL_STACK
     (*thrd_data)->call_stack.stack             = nullptr;
     (*thrd_data)->call_stack.tail_call_flags   = nullptr;
     (*thrd_data)->call_stack.len               = 0;
     (*thrd_data)->call_stack.cap               = 0;
     (*thrd_data)->call_stack.next_is_tail_call = false;
#endif

     (*thrd_data)->thrd_id = 0;
     cpu_cores_num         = platform__get_cpu_cure_num();
     mtx_init(&sub_thrds_mtx.mtx, mtx_plain);
     mtx_init(&tests_mtx.mtx, mtx_plain);
     const char* const sub_thrd_wait_secs__str = getenv("SCAR__THREAD_WAIT_TIME");
     if (sub_thrd_wait_secs__str != nullptr && sscanf(sub_thrd_wait_secs__str, "%"SCNu16, &sub_thrd_wait_secs) != 1)
          [[clang::unlikely]] fail("Invalid value in environment variable \"SCAR__THREAD_WAIT_TIME\".");

     sys_locale = setlocale(LC_ALL, "");
     if (sys_locale == nullptr) [[clang::unlikely]] sys_locale = "en_US";

     u32   const sys_locale_len = strlen(sys_locale);
     char* const syslocale_copy = malloc(sys_locale_len + 1);
     memcpy(syslocale_copy, sys_locale, sys_locale_len + 1);
     sys_locale = syslocale_copy;

     setlocale(LC_NUMERIC, "C");

     const char* const separator     = memchr(sys_locale, '_', sys_locale_len);
     u32               separator_idx = separator == nullptr ? sys_locale_len : separator - sys_locale;
     separator_idx                   = separator_idx < 4 ? separator_idx : 4;
     memcpy_le_16(&sys_lang, sys_locale, separator_idx);

     switch (sys_lang) {
     case (u32)'r' | ((u32)'u' << 8): sys_char_codes = ru_char_codes;
     case (u32)'b' | ((u32)'e' << 8): sys_char_codes = be_char_codes;
     default:                         sys_char_codes = en_char_codes;
     }

     for (u64 idx = sys_locale_len - 1; idx >= 4; idx -= 1)
          if (
               sys_locale[idx] == '8' &&
               ((
                    (sys_locale[idx-4] == 'U' || sys_locale[idx-4] == 'u') &&
                    (sys_locale[idx-3] == 'T' || sys_locale[idx-3] == 't') &&
                    (sys_locale[idx-2] == 'F' || sys_locale[idx-2] == 'f') &&
                    (sys_locale[idx-1] == '-' || sys_locale[idx-1] == ' '  || sys_locale[idx-1] == '_')
               ) || (
                    (sys_locale[idx-3] == 'U' || sys_locale[idx-3] == 'u') &&
                    (sys_locale[idx-2] == 'T' || sys_locale[idx-2] == 't') &&
                    (sys_locale[idx-1] == 'F' || sys_locale[idx-1] == 'f')
               ))
          ) {
               sys_enc_is_utf8 = true;
               break;
          }

     tzset();
}

[[clang::noinline]]
static void deinit_app(t_thrd_data* const thrd_data) {
     assert(thrd_data->linear_allocator.states_len == 0);

     free(thrd_data->linear_allocator.mem);
     free(thrd_data->linear_allocator.states);

#ifdef CALL_STACK
     free(thrd_data->call_stack.stack);
     free(thrd_data->call_stack.tail_call_flags);
#endif
     free(thrd_data);

     mtx_lock(&sub_thrds_mtx.mtx);
     cnd_init(&main_thrd_wait_cnd);

     if (active_sub_thrds != 0) {
          main_thread_waiting = true;
          do cnd_wait(&main_thrd_wait_cnd, &sub_thrds_mtx.mtx);
          while (main_thread_waiting);
     }

     if (sub_thrds_len != 0) {
          for (u32 idx = 0; idx < sub_thrds_len; cnd_signal(&sub_thrds_data[idx]->wait_cnd), idx += 1);

          main_thread_waiting = true;
          do cnd_wait(&main_thrd_wait_cnd, &sub_thrds_mtx.mtx);
          while (main_thread_waiting);
     }

     cnd_destroy(&main_thrd_wait_cnd);
     mtx_unlock(&sub_thrds_mtx.mtx);

     mtx_destroy(&sub_thrds_mtx.mtx);
     free(sub_thrds_data);
     free(sys_locale);

     mtx_destroy(&tests_mtx.mtx);
     u64 const tst_qty = tests_qty;
     if (tst_qty != 0) {
          if (tests_fail_qty == 0) {
               printf("\x1B[32m%"PRIu64"/%"PRIu64"\x1B[0m\n", tst_qty, tst_qty);
          } else {
               printf("\x1B[31m%"PRIu64"/%"PRIu64"\x1B[0m\nFails:\n", tst_qty - tests_fail_qty, tst_qty);

               for (u64 idx = 0; idx < tests_fail_qty; idx += 1) {
                    t_co_ords const co_ords = tests_fail_co_ords[idx];
                    printf("  File: %s, line: %"PRIu64", row: %"PRIu64".\n", co_ords.file, co_ords.line, co_ords.row);
               }
               free(tests_fail_co_ords);
          }
     }
}
