#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/channel/basic.h"
#include "../struct/error/basic.h"

core t_any McoreFNpop(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.pop";

     t_any const channel_arg = args[0];
     if (channel_arg.bytes[15] != tid__channel || !channel__allow_read(channel_arg)) [[clang::unlikely]] {
          if (channel_arg.bytes[15] == tid__error) return channel_arg;

          ref_cnt__dec(thrd_data, channel_arg);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (channel_arg.bytes[15] != tid__channel)
               result = error__invalid_type(thrd_data, owner);
          else result = error__cant_read_from_channel(thrd_data, owner);
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

          while (true) {
               channel->waiting_thrds_len += 1;
               cnd_wait(&channel->cnd, &channel->mtx.mtx);
               channel->waiting_thrds_len -= 1;
               if (channel->items_len != 0) break;
               if (channel->writers_len == 0) [[clang::unlikely]] {
                    mtx_unlock(&channel->mtx.mtx);
                    goto no_writing_channel;
               }
          }
     }

     t_any result_item = channel->items[channel->idx_of_first_item];

     channel->items_len -= 1;
     if (channel->items_len == 0) channel->idx_of_first_item = 0;
     else channel->idx_of_first_item += 1;

     mtx_unlock(&channel->mtx.mtx);
     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, channel_arg);

     if (result_item.bytes[15] == tid__box) {
          call_stack__push(thrd_data, owner);
          result_item = box__unshare__own(thrd_data, result_item, owner);
          call_stack__pop(thrd_data);
     }
     return result_item;

     no_writing_channel:
     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, channel_arg);

     call_stack__push(thrd_data, owner);
     t_any const result = error__no_writing_channel(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}