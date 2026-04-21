#pragma once

#include "../multithread/type.h"
#include "../struct/array/basic.h"
#include "../struct/box/basic.h"
#include "../struct/byte_array/basic.h"
#include "../struct/channel/basic.h"
#include "../struct/common/hash_map.h"
#include "../struct/common/slice.h"
#include "../struct/custom/basic.h"
#include "../struct/error/basic.h"
#include "../struct/fn/basic.h"
#include "../struct/holder/basic.h"
#include "../struct/obj/basic.h"
#include "../struct/set/basic.h"
#include "../struct/string/basic.h"
#include "../struct/table/basic.h"
#include "fn.h"
#include "include.h"
#include "log.h"
#include "macro.h"
#include "ref_cnt.h"
#include "type.h"

[[clang::noinline]]
core_basic t_any to_shared__noinlined_part__own(t_thrd_data* const thrd_data, t_any const arg, t_shared_fn const shared_fn, bool const nested, const char* const owner);

static inline t_any to_shared__own(t_thrd_data* const thrd_data, t_any const arg, t_shared_fn const shared_fn, bool const nested, const char* const owner) {
     if (type_is_val(arg.bytes[15]) && arg.bytes[15] != tid__null) {
          assert(arg.bytes[15] != tid__holder || nested);
          return arg;
     }

     return to_shared__noinlined_part__own(thrd_data, arg, shared_fn, nested, owner);
}

[[clang::noinline]]
core_basic t_any to_shared__noinlined_part__own(t_thrd_data* const thrd_data, t_any arg, t_shared_fn const shared_fn, bool const nested, const char* const owner) {
     switch (arg.bytes[15]) {
     case tid__null:
          if (!nested) goto share_invalid_type_label;

          return arg;
     case tid__box: {
          if (box__is_global(arg)) return arg;

          u8 const idx           = box__get_idx(arg);
          thrd_data->free_boxes ^= (u32)1 << idx;

          u8 const len = box__get_len(arg);
          t_box    box;
          memcpy_le_256(box.items, thrd_data->boxes[idx].items, len * 16);

          t_any* const shared_items = aligned_alloc(16, len * 16);
          for (u8 idx = 0; idx < len; idx += 1) {
               t_any const shared_item = to_shared__own(thrd_data, box.items[idx], shared_fn, true, owner);
               if (shared_item.bytes[15] == tid__error) [[clang::unlikely]] {
                    for (u64 free_idx = idx + 1; free_idx < len; ref_cnt__dec(thrd_data, box.items[free_idx]), free_idx += 1);
                    for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, shared_items[free_idx]), free_idx -= 1);
                    free(shared_items);
                    return shared_item;
               }
               shared_items[idx] = shared_item;
          }

          return box__set_metadata((const t_any){.structure = {.value = (u64)shared_items, .type = tid__box}}, len, false, 0, true);
     }
     case tid__obj: {
          u64 const ref_cnt = get_ref_cnt(arg);
          switch (ref_cnt) {
          case 0:
               return arg;
          case 1: {
               u64          remain_qty = hash_map__get_len(arg);
               t_any* const mtds       = obj__get_mtds(arg);
               if (mtds->bytes[15] != tid__null) {
                    remain_qty -= 1;
                    t_any const shared_mtds = to_shared__own(thrd_data, *mtds, shared_fn, true, owner);
                    if (shared_mtds.bytes[15] == tid__error) [[clang::unlikely]] {
                         mtds->raw_bits = 0;
                         ref_cnt__dec(thrd_data, arg);
                         return shared_mtds;
                    }
               }

               t_any* const items = &mtds[1];
               for (u64 idx = 0; remain_qty != 0; idx += 2) {
                    if (items[idx].bytes[15] != tid__token) continue;

                    remain_qty      -= 1;
                    t_any const item = to_shared__own(thrd_data, items[idx + 1], shared_fn, true, owner);
                    if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                         items[idx + 1].raw_bits = 0;
                         ref_cnt__dec(thrd_data, arg);
                         return item;
                    }

                    items[idx + 1] = item;
               }

               return arg;
          }
          default: {
               set_ref_cnt(arg, ref_cnt - 1);

               u64          const items_len = (hash_map__get_last_idx(arg) + 1) * hash_map__get_chunk_size(arg) * 2 + 1;
               t_any              new_obj   = arg;
               new_obj.qwords[0]            = (u64)aligned_alloc(16, (items_len + 1) * 16);
               const t_any* const old_data  = (const t_any*)arg.qwords[0];
               t_any*       const new_data  = (t_any*)new_obj.qwords[0];

               new_data->raw_bits = old_data->raw_bits;
               set_ref_cnt(new_obj, 1);

               const t_any* const old_items = &old_data[1];
               t_any*       const new_items = &new_data[1];

               t_any mtds = old_items[0];
               ref_cnt__inc(thrd_data, mtds, owner);
               mtds = to_shared__own(thrd_data, mtds, shared_fn, true, owner);
               if (mtds.bytes[15] == tid__error) [[clang::unlikely]] {
                    free((void*)new_obj.qwords[0]);
                    return mtds;
               }
               new_items[0] = mtds;

               for (u64 idx = 1; idx < items_len; idx += 2) {
                    if (old_items[idx].bytes[15] == tid__token) {
                         t_any item = old_items[idx + 1];
                         ref_cnt__inc(thrd_data, item, owner);
                         item = to_shared__own(thrd_data, item, shared_fn, true, owner);
                         if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                              for (u64 free_idx = idx - 1; free_idx != (u64)-2; ref_cnt__dec(thrd_data, new_items[free_idx]), free_idx -= 2);
                              free((void*)new_obj.qwords[0]);
                              return item;
                         }

                         new_items[idx]     = old_items[idx];
                         new_items[idx + 1] = item;
                    } else memcpy_inline(&new_items[idx], &old_items[idx], 32);
               }

               return new_obj;
          }}
     }
     case tid__table: {
          u64 const ref_cnt = get_ref_cnt(arg);
          switch (ref_cnt) {
          case 0:
               return arg;
          case 1: {
               t_any* const items      = table__get_kvs(arg);
               u64          remain_qty = hash_map__get_len(arg) * 2;
               for (u64 idx = 0; remain_qty != 0; idx += 1) {
                    t_any item = items[idx];
                    if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) {
                         idx += 1;
                         continue;
                    }

                    remain_qty -= 1;
                    item        = to_shared__own(thrd_data, item, shared_fn, true, owner);
                    if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                         items[idx].raw_bits = 0;
                         ref_cnt__dec(thrd_data, arg);
                         return item;
                    }

                    items[idx] = item;
               }

               return arg;
          }
          default: {
               set_ref_cnt(arg, ref_cnt - 1);

               u64          const items_len = (hash_map__get_last_idx(arg) + 1) * hash_map__get_chunk_size(arg) * 2;
               t_any              new_table = arg;
               new_table.qwords[0]          = (u64)aligned_alloc(16, (items_len + 1) * 16);
               const t_any* const old_data  = (const t_any*)arg.qwords[0];
               t_any*       const new_data  = (t_any*)new_table.qwords[0];

               new_data->raw_bits = old_data->raw_bits;
               set_ref_cnt(new_table, 1);

               const t_any* const old_items = &old_data[1];
               t_any*       const new_items = &new_data[1];

               for (u64 idx = 0; idx < items_len; idx += 1) {
                    t_any item = old_items[idx];
                    if (!(item.bytes[15] == tid__null || item.bytes[15] == tid__holder)) {
                         ref_cnt__inc(thrd_data, item, owner);
                         item = to_shared__own(thrd_data, item, shared_fn, true, owner);
                         if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                              for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, new_items[free_idx]), free_idx -= 1);
                              free((void*)new_table.qwords[0]);
                              return item;
                         }
                    }
                    new_items[idx] = item;
               }

               return new_table;
          }}
     }
     case tid__set: {
          u64 const ref_cnt = get_ref_cnt(arg);
          switch (ref_cnt) {
          case 0:
               return arg;
          case 1: {
               t_any* const items      = set__get_items(arg);
               u64          remain_qty = hash_map__get_len(arg);
               for (u64 idx = 0; remain_qty != 0; idx += 1) {
                    t_any item = items[idx];
                    if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

                    remain_qty -= 1;
                    item        = to_shared__own(thrd_data, item, shared_fn, true, owner);
                    if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                         items[idx].raw_bits = 0;
                         ref_cnt__dec(thrd_data, arg);
                         return item;
                    }

                    items[idx]  = item;
               }

               return arg;
          }
          default: {
               set_ref_cnt(arg, ref_cnt - 1);

               u64          const items_len = (hash_map__get_last_idx(arg) + 1) * hash_map__get_chunk_size(arg);
               t_any              new_table = arg;
               new_table.qwords[0]          = (u64)aligned_alloc(16, (items_len + 1) * 16);
               const t_any* const old_data  = (const t_any*)arg.qwords[0];
               t_any*       const new_data  = (t_any*)new_table.qwords[0];

               new_data->raw_bits = old_data->raw_bits;
               set_ref_cnt(new_table, 1);

               const t_any* const old_items = &old_data[1];
               t_any*       const new_items = &new_data[1];

               for (u64 idx = 0; idx < items_len; idx += 1) {
                    t_any item = old_items[idx];
                    if (!(item.bytes[15] == tid__null || item.bytes[15] == tid__holder)) {
                         ref_cnt__inc(thrd_data, item, owner);
                         item = to_shared__own(thrd_data, item, shared_fn, true, owner);
                         if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                              for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, new_items[free_idx]), free_idx -= 1);
                              free((void*)new_table.qwords[0]);
                              return item;
                         }
                    }
                    new_items[idx] = item;
               }

               return new_table;
          }}
     }
     case tid__byte_array: {
          u64 const ref_cnt = get_ref_cnt(arg);
          if (ref_cnt < 2) return arg;

          set_ref_cnt(arg, ref_cnt - 1);

          u64 const array_len    = slice_array__get_len(arg);
          u32 const slice_offset = slice__get_offset(arg);
          u32 const slice_len    = slice__get_len(arg);
          u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          t_any array = long_byte_array__new(len);
          array       = slice__set_metadata(array, 0, slice_len);
          slice_array__set_len(array, len);
          memcpy(slice_array__get_items(array), &((const u8*)slice_array__get_items(arg))[slice_offset], len);

          return array;
     }
     case tid__string: {
          u64 const ref_cnt = get_ref_cnt(arg);
          if (ref_cnt < 2) return arg;

          set_ref_cnt(arg, ref_cnt - 1);

          u64 const array_len    = slice_array__get_len(arg);
          u32 const slice_offset = slice__get_offset(arg);
          u32 const slice_len    = slice__get_len(arg);
          u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          t_any string = long_string__new(len);
          string       = slice__set_metadata(string, 0, slice_len);
          slice_array__set_len(string, len);
          memcpy(slice_array__get_items(string), &((const u8*)slice_array__get_items(arg))[slice_offset * 3], len * 3);

          return string;
     }
     case tid__array: {
          u64 const ref_cnt = get_ref_cnt(arg);
          switch (get_ref_cnt(arg)) {
          case 0:
               return arg;
          case 1: {
               u32    const slice_offset = slice__get_offset(arg);
               u32    const slice_len    = slice__get_len(arg);
               u64    const array_len    = slice_array__get_len(arg);
               u64    const len          = slice_len <= slice_max_len ? slice_len : array_len;
               t_any* const items        = slice_array__get_items(arg);

               for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
               for (u64 idx = 0; idx < len; idx += 1) {
                    t_any const item = to_shared__own(thrd_data, items[slice_offset + idx], shared_fn, true, owner);
                    if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                         for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, items[free_idx]), free_idx -= 1);
                         for (idx += slice_offset + 1; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
                         free((void*)arg.qwords[0]);
                         return item;
                    }

                    items[idx] = item;
               }
               for (u64 idx = slice_offset + len; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

               arg = slice__set_metadata(arg, 0, slice_len);
               slice_array__set_len(arg, len);

               return arg;
          }
          default: {
               set_ref_cnt(arg, ref_cnt - 1);

               u32 const slice_len = slice__get_len(arg);
               if (slice_len == 0) return empty_array;

               u64          const array_len    = slice_array__get_len(arg);
               u32          const slice_offset = slice__get_offset(arg);
               u64          const len          = slice_len <= slice_max_len ? slice_len : array_len;
               const t_any* const items        = &((const t_any*)slice_array__get_items(arg))[slice_offset];

               assert(slice_len <= slice_max_len || slice_offset == 0);

               t_any        new_array = array__init(thrd_data, len, owner);
               new_array              = slice__set_metadata(new_array, 0, slice_len);
               slice_array__set_len(new_array, len);
               t_any* const new_items = slice_array__get_items(new_array);

               for (u64 idx = 0; idx < len; idx += 1) {
                    t_any item = items[idx];
                    ref_cnt__inc(thrd_data, item, owner);
                    item = to_shared__own(thrd_data, item, shared_fn, true, owner);
                    if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                         for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, new_items[free_idx]), free_idx -= 1);
                         free((void*)new_array.qwords[0]);
                         return item;
                    }

                    new_items[idx] = item;
               }

               return new_array;
          }}
     }
     case tid__fn: {
          u64 const ref_cnt = get_ref_cnt(arg);
          switch (ref_cnt) {
          case 0:
               return arg;
          case 1: {
               u64    const items_len   = fn__get_params_cap(arg) + fn__get_borrowed_len(arg);
               t_any* const items       = fn__get_args(thrd_data, arg);

               assume_aligned(items, 16);

               for (u64 idx = 0; idx < items_len; idx += 1) {
                    t_any const item = to_shared__own(thrd_data, items[idx], shared_fn, true, owner);
                    if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                         items[idx].raw_bits = 0;
                         ref_cnt__dec(thrd_data, arg);
                         return item;
                    }

                    items[idx] = item;
               }

               return arg;
          }
          default: {
               set_ref_cnt(arg, ref_cnt - 1);

               u64    const items_len   = fn__get_params_cap(arg) + fn__get_borrowed_len(arg);
               t_any* const items       = fn__get_args(thrd_data, arg);

               assume_aligned(items, 16);

               t_any const new_fn = {.qwords = {(u64)aligned_alloc(16, (items_len + 1) * 16), arg.qwords[1]}};
               set_ref_cnt(new_fn, 1);
               *fn__get_address(thrd_data, new_fn) = *fn__get_address(thrd_data, arg);
               t_any* const new_items   = fn__get_args(thrd_data, new_fn);
               for (u64 idx = 0; idx < items_len; idx += 1) {
                    t_any item = items[idx];
                    ref_cnt__inc(thrd_data, item, owner);
                    item = to_shared__own(thrd_data, item, shared_fn, true, owner);
                    if (item.bytes[15] == tid__error) [[clang::unlikely]] {
                         for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, new_items[free_idx]), free_idx -= 1);
                         free((void*)new_fn.qwords[0]);
                         return item;
                    }

                    new_items[idx] = item;
               }

               return new_fn;
          }}
     }
     case tid__channel: {
          if (shared_fn != sf__thread) goto share_invalid_type_label;

          u64 const ref_cnt = get_ref_cnt(arg);
          if (ref_cnt == 1) return arg;

          set_ref_cnt(arg, ref_cnt - 1);

          const t_channel_ptr* const channel_ptr = (const t_channel_ptr*)arg.qwords[0];
          t_channel*           const channel     = channel_ptr->channel;

          t_channel_ptr* const new_channel_ptr = aligned_alloc(alignof(t_channel_ptr), sizeof(t_channel_ptr));
          new_channel_ptr->local_ref_cnt       = 1;
          new_channel_ptr->channel             = channel;

          mtx_lock(&channel->mtx.mtx);
          channel->main_ref_cnt += 1;
          if (channel__allow_read(arg)) channel->readers_len += 1;
          if (channel__allow_write(arg)) channel->writers_len += 1;
          mtx_unlock(&channel->mtx.mtx);

          return (const t_any){.qwords = {(u64)new_channel_ptr, arg.qwords[1]}};
     }
     case tid__custom:
          if (get_ref_cnt(arg) == 0) return arg;

          t_any (*const to_shared)(t_thrd_data* const, t_any const, t_shared_fn const, bool const, const char* const) = custom__get_fns(arg)->to_shared;
          return to_shared(thrd_data, arg, shared_fn, nested, owner);

     [[clang::unlikely]]
     case tid__error:
          return arg;
     [[clang::unlikely]]
     case tid__breaker:
     [[clang::unlikely]]
     case tid__thread:
          goto share_invalid_type_label;
     default:
          unreachable;
     }

     share_invalid_type_label:
     ref_cnt__dec(thrd_data, arg);
     return error__share_invalid_type(thrd_data, owner);
}
