#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"
#include "../struct/box/basic.h"
#include "../struct/error/basic.h"
#include "../struct/holder/basic.h"

core t_any McoreFNtime_limit_wait(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.time_limit_wait";

     t_any const thread   = args[0];
     t_any const nano_sec = args[1];
     if (thread.bytes[15] != tid__thread || nano_sec.bytes[15] != tid__int || (i64)nano_sec.qwords[0] < 0) [[clang::unlikely]] {
          if (thread.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, nano_sec);
               return thread;
          }
          ref_cnt__dec(thrd_data, thread);

          if (nano_sec.bytes[15] == tid__error) return nano_sec;
          ref_cnt__dec(thrd_data, nano_sec);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (thread.bytes[15] != tid__thread || nano_sec.bytes[15] != tid__int)
               result = error__invalid_type(thrd_data, owner);
          else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     t_thrd_result* const thrd_result = (t_thrd_result*)thread.qwords[0];
     mtx_lock(&thrd_result->mtx.mtx);
     bool has_result = thrd_result->result.bytes[15] != tid__holder;
     if (!(has_result || nano_sec.qwords[0] == 0)) {
          thrd_result->result.bytes[0] = ht__waiting_result;
          struct timespec timespec;
          if (timespec_get(&timespec, TIME_UTC) == 0) [[clang::unlikely]] fail("Could not get the current time.");

          u64 const nano_sec_sum = nano_sec.qwords[0] + timespec.tv_nsec;
          timespec.tv_sec       += nano_sec_sum / 1'000'000'000ull;
          timespec.tv_nsec       = nano_sec_sum % 1'000'000'000ull;

          while (true) {
               int const wait_result = cnd_timedwait(&thrd_result->cnd, &thrd_result->mtx.mtx, &timespec);
               if (wait_result == thrd_error) [[clang::unlikely]] fail("Can't block a condition variable.");

               has_result = thrd_result->result.bytes[15] != tid__holder;

               if (has_result) break;

               if (wait_result == thrd_timedout) {
                    thrd_result->result.bytes[0] = ht__used_thrd_result;
                    break;
               }
          }
     }
     mtx_unlock(&thrd_result->mtx.mtx);

     t_any result_flag;
     t_any result_thrd;
     t_any result_item;
     if (has_result) {
          result_item = thrd_result->result;

          mtx_destroy(&thrd_result->mtx.mtx);
          cnd_destroy(&thrd_result->cnd);
          free(thrd_result);

          if (result_item.bytes[15] == tid__error) [[clang::unlikely]] return result_item;

          result_flag           = bool__true;
          result_thrd.bytes[15] = tid__null;
          if (result_item.bytes[15] == tid__box) {
               call_stack__push(thrd_data, owner);
               result_item = box__unshare__own(thrd_data, result_item, owner);
               call_stack__pop(thrd_data);
          }
     } else {
          result_flag           = bool__false;
          result_thrd           = thread;
          result_item.bytes[15] = tid__null;
     }

     call_stack__push(thrd_data, owner);
     t_any const box = box__new(thrd_data, 3, owner);
     call_stack__pop(thrd_data);

     t_any* const items = box__get_items(box);
     items[0]           = result_flag;
     items[1]           = result_thrd;
     items[2]           = result_item;
     return box;
}