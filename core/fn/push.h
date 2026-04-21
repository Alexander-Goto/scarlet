#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/share.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/channel/basic.h"
#include "../struct/error/basic.h"

core t_any McoreFNpush(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.push";

     t_any const channel_arg = args[0];
     t_any       item        = args[1];
     if (channel_arg.bytes[15] != tid__channel || item.bytes[15] == tid__error || !channel__allow_write(channel_arg)) [[clang::unlikely]] {
          if (channel_arg.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, item);
               return channel_arg;
          }
          ref_cnt__dec(thrd_data, channel_arg);

          if (item.bytes[15] == tid__error) {
               return item;
          }
          ref_cnt__dec(thrd_data, item);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (channel_arg.bytes[15] != tid__channel)
               result = error__invalid_type(thrd_data, owner);
          else result = error__cant_write_to_channel(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     call_stack__push(thrd_data, owner);
     item = to_shared__own(thrd_data, item, sf__push, false, owner);
     call_stack__pop(thrd_data);

     if (item.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, channel_arg);
          return item;
     }

     u64 const ref_cnt = get_ref_cnt(channel_arg);
     if (ref_cnt > 1) set_ref_cnt(channel_arg, ref_cnt - 1);

     t_channel* const channel = ((const t_channel_ptr*)channel_arg.qwords[0])->channel;
     mtx_lock(&channel->mtx.mtx);
     if (channel->readers_len == 0) {
          mtx_unlock(&channel->mtx.mtx);

          if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, channel_arg);
          ref_cnt__dec(thrd_data, item);

          call_stack__push(thrd_data, owner);
          t_any const result = error__no_reading_channel(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     if (channel->idx_of_first_item + channel->items_len >= channel->items_cap) {
          if (channel->idx_of_first_item != 0) {
               memmove(channel->items, &channel->items[channel->idx_of_first_item], channel->items_len * 16);
               channel->idx_of_first_item = 0;
          } else {
               channel->items_cap = channel->items_cap * 3 / 2;
               channel->items     = aligned_realloc(16, channel->items, channel->items_cap * 16);
          }
     }

     channel->items[channel->idx_of_first_item + channel->items_len] = item;
     channel->items_len                                             += 1;

     if (channel->waiting_thrds_len != 0) {
          cnd_t* const cnd = &channel->cnd;
          mtx_unlock(&channel->mtx.mtx);
          cnd_signal(cnd);
     } else mtx_unlock(&channel->mtx.mtx);

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, channel_arg);

     return null;
}