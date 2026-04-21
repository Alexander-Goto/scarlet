#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/error/basic.h"
#include "../struct/holder/basic.h"

core t_any McoreFNwait(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.wait";

     t_any const thread_result_arg = args[0];
     if (thread_result_arg.bytes[15] != tid__thread) [[clang::unlikely]] {
          if (thread_result_arg.bytes[15] == tid__error) return thread_result_arg;

          ref_cnt__dec(thrd_data, thread_result_arg);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     t_thrd_result* const thrd_result = (t_thrd_result*)thread_result_arg.qwords[0];
     mtx_lock(&thrd_result->mtx.mtx);
     if (thrd_result->result.bytes[15] == tid__holder) {
          thrd_result->result.bytes[0] = ht__waiting_result;
          while (true) {
               cnd_wait(&thrd_result->cnd, &thrd_result->mtx.mtx);
               if (thrd_result->result.bytes[15] != tid__holder) break;
          }
     }
     mtx_unlock(&thrd_result->mtx.mtx);

     t_any result = thrd_result->result;
     mtx_destroy(&thrd_result->mtx.mtx);
     cnd_destroy(&thrd_result->cnd);
     free(thrd_result);

     if (result.bytes[15] == tid__box) {
          call_stack__push(thrd_data, owner);
          result = box__unshare__own(thrd_data, result, owner);
          call_stack__pop(thrd_data);
     }

     return result;
}