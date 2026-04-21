#pragma once

#include "../struct/box/basic.h"
#include "../struct/breaker/basic.h"
#include "../struct/channel/basic.h"
#include "../struct/common/hash_map.h"
#include "../struct/common/slice.h"
#include "../struct/custom/basic.h"
#include "../struct/error/basic.h"
#include "../struct/fn/basic.h"
#include "../struct/holder/basic.h"
#include "../struct/obj/basic.h"
#include "../struct/set/basic.h"
#include "../struct/table/basic.h"
#include "fn.h"
#include "include.h"
#include "log.h"
#include "macro.h"
#include "type.h"
#include "types_lists.h"

static void ref_cnt__add(t_thrd_data* const thrd_data, t_any const arg, u64 const val, const char* const owner) {
     u8 const type = arg.bytes[15];
     assert(type_is_correct(type));

     if (type_is_val(type) || val == 0) return;

     if (type_with_ref_cnt(type)) [[clang::likely]] {
          u64 ref_cnt = type == tid__fn ? fn__get_ref_cnt(thrd_data, arg) : get_ref_cnt(arg);
          if (ref_cnt == 0) return;

          bool const overflow = __builtin_add_overflow(ref_cnt, val, &ref_cnt);
          if (ref_cnt > ref_cnt_max || overflow) [[clang::unlikely]] fail_with_call_stack(thrd_data, "Reference counter overflow.", owner);

          type == tid__fn ? fn__set_ref_cnt(thrd_data, arg, ref_cnt) : set_ref_cnt(arg, ref_cnt);
          return;
     }

     if (type == tid__thread || type == tid__breaker) [[clang::unlikely]]
          fail_with_call_stack(thrd_data, type == tid__thread ? "Attempting to copy a thread." : "Attempting to duplicate a breaker.", owner);

     if (!box__is_global(arg)) [[clang::unlikely]] fail_with_call_stack(thrd_data, "Attempting to duplicate a box.", owner);
}

static inline void ref_cnt__inc(t_thrd_data* const thrd_data, t_any const arg, const char* const owner) {
     u8 const type = arg.bytes[15];
     assert(type_is_correct(type));

     if (type_is_val(type)) return;

     if (type_with_ref_cnt(type)) [[clang::likely]] {
          u64 const ref_cnt = type == tid__fn ? fn__get_ref_cnt(thrd_data, arg) : get_ref_cnt(arg);
          if (ref_cnt == 0) return;
          if (ref_cnt == ref_cnt_max) [[clang::unlikely]] fail_with_call_stack(thrd_data, "Reference counter overflow.", owner);

          assume(ref_cnt < ref_cnt_max);

          type == tid__fn ? fn__set_ref_cnt(thrd_data, arg, ref_cnt + 1) : set_ref_cnt(arg, ref_cnt + 1);
          return;
     }

     if (type == tid__thread || type == tid__breaker) [[clang::unlikely]]
          fail_with_call_stack(thrd_data, type == tid__thread ? "Attempting to copy a thread." : "Attempting to duplicate a breaker.", owner);

     if (!box__is_global(arg)) [[clang::unlikely]] fail_with_call_stack(thrd_data, "Attempting to duplicate a box.", owner);
}

[[gnu::hot, clang::noinline]]
core_basic void ref_cnt__dec__noinlined_part(t_thrd_data* const thrd_data, t_any const arg);

static inline void ref_cnt__dec(t_thrd_data* const thrd_data, t_any const arg) {
     u8 const type = arg.bytes[15];
     assert(type_is_correct(type));

     if (type_is_val(type)) return;

     if (type_with_ref_cnt(type)) {
          u64 const ref_cnt = type == tid__fn ? fn__get_ref_cnt(thrd_data, arg) : get_ref_cnt(arg);
          if (ref_cnt != 1) {
               if (ref_cnt != 0) type == tid__fn ? fn__set_ref_cnt(thrd_data, arg, ref_cnt - 1) : set_ref_cnt(arg, ref_cnt - 1);
               return;
          }
     }
     ref_cnt__dec__noinlined_part(thrd_data, arg);
}

[[gnu::hot, clang::noinline]]
core_basic void ref_cnt__dec__noinlined_part(t_thrd_data* const thrd_data, t_any const arg) {
     u8 const type = arg.bytes[15];
     assert(!type_is_val(type));

     const t_any* freed_items     = nullptr;
     u64          freed_items_len = 0;
     void*        freed_memory    = (void*)arg.qwords[0];

     switch (type) {
     case tid__box:
          if (box__is_global(arg)) return;

          freed_items     = box__get_items(arg);
          freed_items_len = box__get_len(arg);
          if (!box__is_shared(arg)) [[clang::likely]] {
               u8 const idx = box__get_idx(arg);
               thrd_data->free_boxes ^= (u32)1 << idx;
               freed_memory           = nullptr;
          }

          break;
     case tid__obj: {
          const t_any* const mtds = obj__get_mtds(arg);
          ref_cnt__dec(thrd_data, *mtds);
          const t_any* const fields     = &mtds[1];
          u64                remain_qty = obj__get_fields_len(arg);
          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free(freed_memory);
          return;
     }
     case tid__byte_array: case tid__string:
          free(freed_memory);
          return;
     case tid__array:
          freed_items     = slice_array__get_items(arg);
          freed_items_len = slice_array__get_len(arg);
          break;
     case tid__fn: {
          u8   const params_cap   = fn__get_params_cap(arg);
          u64  const borrowed_len = fn__get_borrowed_len(arg);

          freed_items     = fn__get_args(thrd_data, arg);
          freed_items_len = params_cap + borrowed_len;

          if (fn__is_linear_alloc(arg)) {
               freed_memory = nullptr;
               linear__free(&thrd_data->linear_allocator, arg.qwords[0]);
          }

          break;
     }
     case tid__table: {
          const t_any* const kvs        = table__get_kvs(arg);
          u64                remain_qty = hash_map__get_len(arg);

          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               t_any const key = kvs[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, kvs[idx + 1]);
          }

          free(freed_memory);
          return;
     }
     case tid__set: {
          const t_any* const items      = set__get_items(arg);
          u64                remain_qty = hash_map__get_len(arg);

          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, item);
          }

          free(freed_memory);
          return;
     }
     case tid__custom: {
          void (*const free_function)(t_thrd_data* const, t_any const) = custom__get_fns(arg)->free_function;
          free_function(thrd_data, arg);
          return;
     }
     case tid__thread: {
          t_thrd_result* const thrd_result = (t_thrd_result*)arg.qwords[0];
          mtx_lock(&thrd_result->mtx.mtx);
          if (thrd_result->result.bytes[15] == tid__holder) {
               if (thrd_result->result.bytes[0] == ht__used_thrd_result)
                    thrd_result->result.bytes[0] = ht__unused_thrd_result;

               mtx_unlock(&thrd_result->mtx.mtx);
               freed_memory = nullptr;
          } else {
               mtx_unlock(&thrd_result->mtx.mtx);
               if (thrd_result->result.bytes[15] == tid__error) [[clang::unlikely]] log__msg(log_lvl__may_require_attention, "A thread ended with an error, but the error was ignored.");

               freed_items     = &thrd_result->result;
               freed_items_len = 1;

               mtx_destroy(&thrd_result->mtx.mtx);
               cnd_destroy(&thrd_result->cnd);
          }
          break;
     }
     case tid__channel: {
          t_channel* const channel = ((t_channel_ptr*)arg.qwords[0])->channel;
          mtx_lock(&channel->mtx.mtx);

          if (channel->main_ref_cnt > 1) {
               channel->main_ref_cnt -= 1;
               bool const current_is_reader = channel__allow_read(arg);
               bool const current_is_writer = channel__allow_write(arg);

               if (current_is_reader) channel->readers_len -= 1;
               if (current_is_writer) channel->writers_len -= 1;

               if (channel->writers_len == 0 && channel->waiting_thrds_len != 0) [[clang::unlikely]] cnd_broadcast(&channel->cnd);
               mtx_unlock(&channel->mtx.mtx);
          } else {
               mtx_unlock(&channel->mtx.mtx);
               mtx_destroy(&channel->mtx.mtx);
               cnd_destroy(&channel->cnd);

               freed_items_len = channel->items_len;
               freed_items     = &channel->items[channel->idx_of_first_item];
               freed_memory    = channel->items;

               free(channel);
               free((void*)arg.qwords[0]);

               if (freed_items_len != 0) [[clang::unlikely]] log__msg(log_lvl__may_require_attention, "The channel containing items is freed.");
          }
          break;
     }
     case tid__breaker: {
          t_any const freed_item = breaker__get_result__own(thrd_data, arg);
          ref_cnt__dec(thrd_data, freed_item);
          return;
     }
     case tid__error: {
          t_error* const error = (t_error*)arg.qwords[0];
          #ifdef CALL_STACK
          free(error->call_stack);
          #endif
          freed_items     = &error->msg;
          freed_items_len = 2;
          break;
     }
     default:
          unreachable;
     }

     for (u64 idx = 0; idx < freed_items_len; idx += 1) ref_cnt__dec(thrd_data, freed_items[idx]);

     if (freed_memory != nullptr) free(freed_memory);
}
