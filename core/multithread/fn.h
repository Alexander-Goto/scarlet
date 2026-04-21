#pragma once

#include "../common/fn.h"
#include "../common/include.h"
#include "../common/log.h"
#include "../common/macro.h"
#include "../common/share.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/common/fn_struct.h"
#include "../struct/holder/basic.h"
#include "var.h"

[[clang::noinline]]
static int new_thrd_fn(void* const thrd_arg) {
     const char* const owner = "function - core.thread";

     t_thrd_data* const thrd_data = thrd_arg;

     while (true) {
          assume(thrd_data->fn.bytes[15] == tid__short_fn || thrd_data->fn.bytes[15] == tid__fn);

          t_any fn_result = common_fn__call__own(thrd_data, thrd_data->fn, nullptr, 0, owner);
          {
               t_thrd_result* const thrd_result = thrd_data->result;
               mtx_lock(&thrd_result->mtx.mtx);
               switch (thrd_result->result.bytes[0]) {
               case ht__used_thrd_result:
                    fn_result           = fn_result.bytes[15] == tid__box && !box__is_global(fn_result) ? to_shared__own(thrd_data, fn_result, sf__thread, false, owner) : fn_result;
                    thrd_result->result = fn_result;
                    mtx_unlock(&thrd_result->mtx.mtx);
                    break;
               case ht__waiting_result:
                    fn_result           = fn_result.bytes[15] == tid__box && !box__is_global(fn_result) ? to_shared__own(thrd_data, fn_result, sf__thread, false, owner) : fn_result;
                    thrd_result->result = fn_result;
                    cnd_signal(&thrd_result->cnd);
                    mtx_unlock(&thrd_result->mtx.mtx);
                    break;
               case ht__unused_thrd_result:
                    mtx_unlock(&thrd_result->mtx.mtx);
                    mtx_destroy(&thrd_result->mtx.mtx);
                    cnd_destroy(&thrd_result->cnd);
                    free(thrd_result);
                    if (fn_result.bytes[15] == tid__error) [[clang::unlikely]] log__msg(log_lvl__may_require_attention, "A thread ended with an error, but the error was ignored.");
                    ref_cnt__dec(thrd_data, fn_result);
                    break;
               default:
                    unreachable;
               }
          }

          thrd_data->result = nullptr;
#ifdef CALL_STACK
          free(thrd_data->call_stack.stack);
          free(thrd_data->call_stack.tail_call_flags);
#endif

          struct timespec time;
          if (sub_thrd_wait_secs != 0) {
               timespec_get(&time, TIME_UTC);
               time.tv_sec += sub_thrd_wait_secs;
          }

          mtx_lock(&sub_thrds_mtx.mtx);
          active_sub_thrds                   -= 1;
          sub_thrds_data[thrd_data->idx]      = sub_thrds_data[active_sub_thrds];
          sub_thrds_data[thrd_data->idx]->idx = thrd_data->idx;
          thrd_data->idx                      = active_sub_thrds;
          sub_thrds_data[active_sub_thrds]    = thrd_data;

          if (main_thread_waiting && active_sub_thrds == 0) {
               main_thread_waiting = false;
               cnd_signal(&main_thrd_wait_cnd);
               cnd_wait(&thrd_data->wait_cnd, &sub_thrds_mtx.mtx);
               goto end;
          }

          if (sub_thrd_wait_secs != 0) {
               cnd_timedwait(&thrd_data->wait_cnd, &sub_thrds_mtx.mtx, &time);
               if (thrd_data->result == nullptr) goto end;
          } else while (true) {
               cnd_wait(&thrd_data->wait_cnd, &sub_thrds_mtx.mtx);
               if (thrd_data->result != nullptr) break;
               if (main_thread_waiting && active_sub_thrds == 0) goto end;
          }

          mtx_unlock(&sub_thrds_mtx.mtx);
     }

     end:
     sub_thrds_len                      -= 1;
     sub_thrds_data[thrd_data->idx]      = sub_thrds_data[sub_thrds_len];
     sub_thrds_data[thrd_data->idx]->idx = thrd_data->idx;

     if (sub_thrds_len == 0) {
          free(sub_thrds_data);
          sub_thrds_data = nullptr;
          sub_thrds_cap  = 0;
     } else if (sub_thrds_cap / 4 == sub_thrds_len && !(main_thread_waiting && active_sub_thrds == 0)) {
          sub_thrds_cap  = sub_thrds_len;
          sub_thrds_data = realloc(sub_thrds_data, sub_thrds_len * sizeof(t_thrd_data*));
     }

     assert(thrd_data->linear_allocator.states_len == 0);

     cnd_destroy(&thrd_data->wait_cnd);
     free(thrd_data->linear_allocator.mem);
     free(thrd_data->linear_allocator.states);
     free(thrd_data);

     if (main_thread_waiting && sub_thrds_len == 0) {
          main_thread_waiting = false;
          cnd_signal(&main_thrd_wait_cnd);
     }

     mtx_unlock(&sub_thrds_mtx.mtx);

     return 0;
}
