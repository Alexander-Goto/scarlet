#pragma once

#include "../common/fn.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/channel/basic.h"

core t_any McoreFNmake_channel(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.make_channel";

     t_any const cap = args[0];
     if (cap.bytes[15] != tid__int || (i64)cap.qwords[0] < 1) [[clang::unlikely]] {
          if (cap.bytes[15] == tid__error)
               return cap;

          t_any error;
          call_stack__push(thrd_data, owner);
          if (cap.bytes[15] != tid__int) {
               ref_cnt__dec(thrd_data, cap);
               error = error__invalid_type(thrd_data, owner);
          } else error = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return error;
     }

     t_channel* const channel = aligned_alloc(alignof(t_channel), sizeof(t_channel));
     *channel                 = (const t_channel) {
          .main_ref_cnt      = 2,
          .writers_len       = 1,
          .readers_len       = 1,
          .waiting_thrds_len = 0,
          .items_len         = 0,
          .items_cap         = cap.qwords[0],
          .idx_of_first_item = 0,
          .items             = aligned_alloc(16, cap.qwords[0] * 16),
     };

     mtx_init(&channel->mtx.mtx, mtx_plain);
     cnd_init(&channel->cnd);

     t_channel_ptr* const in_channel_ptr = aligned_alloc(alignof(t_channel_ptr), sizeof(t_channel_ptr));
     *in_channel_ptr                     = (const t_channel_ptr) {
          .local_ref_cnt = 1,
          .channel       = channel,
     };
     t_channel_ptr* const out_channel_ptr = aligned_alloc(alignof(t_channel_ptr), sizeof(t_channel_ptr));
     *out_channel_ptr                     = (const t_channel_ptr) {
          .local_ref_cnt = 1,
          .channel       = channel,
     };

     t_any const in  = channel__set_write_flag((const t_any){.structure = {.value = (u64)in_channel_ptr, .type = tid__channel}}, true);
     t_any const out = channel__set_read_flag((const t_any){.structure = {.value = (u64)out_channel_ptr, .type = tid__channel}}, true);

     call_stack__push(thrd_data, owner);
     t_any  const result = box__new(thrd_data, 2, owner);
     call_stack__pop(thrd_data);

     t_any* const items = box__get_items(result);
     items[0]           = in;
     items[1]           = out;

     return result;
}
