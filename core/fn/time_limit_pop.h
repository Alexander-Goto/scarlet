#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/channel/basic.h"
#include "../struct/error/basic.h"

core t_any McoreFNtime_limit_pop(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.time_limit_pop";

     t_any const channel_arg = args[0];
     t_any const nano_sec    = args[1];
     if (
          channel_arg.bytes[15] != tid__channel ||
          nano_sec.bytes[15] != tid__int        ||
          !channel__allow_read(channel_arg)     ||
          (i64)nano_sec.qwords[0] < 0
     ) [[clang::unlikely]] {
          if (channel_arg.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, nano_sec);
               return channel_arg;
          }
          ref_cnt__dec(thrd_data, channel_arg);

          if (nano_sec.bytes[15] == tid__error) return nano_sec;
          ref_cnt__dec(thrd_data, nano_sec);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (channel_arg.bytes[15] != tid__channel || nano_sec.bytes[15] != tid__int)
               result = error__invalid_type(thrd_data, owner);
          else if (!channel__allow_read(channel_arg))
               result = error__cant_read_from_channel(thrd_data, owner);
          else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     u64 const ref_cnt = get_ref_cnt(channel_arg);
     if (ref_cnt > 1) set_ref_cnt(channel_arg, ref_cnt - 1);

     t_channel* const channel = ((const t_channel_ptr*)channel_arg.qwords[0])->channel;
     mtx_lock(&channel->mtx.mtx);
     if (channel->items_len == 0) {
          if (channel->writers_len == 0) [[clang::unlikely]] {
               mtx_unlock(&channel->mtx.mtx);
               goto no_writing_channel;
          }

          if (nano_sec.qwords[0] == 0) {
               mtx_unlock(&channel->mtx.mtx);
               if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, channel_arg);
               return null;
          }

          struct timespec timespec;
          if (timespec_get(&timespec, TIME_UTC) == 0) [[clang::unlikely]] fail("Could not get the current time.");

          u64 const nano_sec_sum = nano_sec.qwords[0] + timespec.tv_nsec;
          timespec.tv_sec       += nano_sec_sum / 1'000'000'000ull;
          timespec.tv_nsec       = nano_sec_sum % 1'000'000'000ull;

          while (true) {
               channel->waiting_thrds_len += 1;
               int const wait_result       = cnd_timedwait(&channel->cnd, &channel->mtx.mtx, &timespec);
               channel->waiting_thrds_len -= 1;
               if (wait_result == thrd_error) [[clang::unlikely]] fail("Can't block a condition variable.");

               if (channel->items_len != 0) break;
               if (channel->writers_len == 0) [[clang::unlikely]] {
                    mtx_unlock(&channel->mtx.mtx);
                    goto no_writing_channel;
               }
               if (wait_result == thrd_timedout) {
                    mtx_unlock(&channel->mtx.mtx);
                    if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, channel_arg);
                    return null;
               }
          }
     }

     t_any result = channel->items[channel->idx_of_first_item];

     channel->items_len -= 1;
     if (channel->items_len == 0) channel->idx_of_first_item = 0;
     else channel->idx_of_first_item += 1;

     mtx_unlock(&channel->mtx.mtx);
     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, channel_arg);

     if (result.bytes[15] == tid__box) {
          call_stack__push(thrd_data, owner);
          result = box__unshare__own(thrd_data, result, owner);
          call_stack__pop(thrd_data);
     }

     return result;

     no_writing_channel:
     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, channel_arg);

     call_stack__push(thrd_data, owner);
     t_any const error = error__no_writing_channel(thrd_data, owner);
     call_stack__pop(thrd_data);

     return error;
}