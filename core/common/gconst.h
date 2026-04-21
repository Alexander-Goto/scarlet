#pragma once

#include "../struct/array/fn.h"
#include "../struct/box/basic.h"
#include "../struct/byte_array/fn.h"
#include "../struct/common/fn_struct.h"
#include "../struct/common/hash_map.h"
#include "../struct/common/slice.h"
#include "../struct/fn/basic.h"
#include "../struct/obj/basic.h"
#include "../struct/set/basic.h"
#include "../struct/string/fn.h"
#include "../struct/table/basic.h"
#include "fn.h"
#include "macro.h"
#include "ref_cnt.h"
#include "type.h"

static const char* const recursive_gconst_mask       = "The global constant \"%s\" refers to itself.\n";
static const char* const recursive_switch_table_mask = "Recursively create table for 'switch' operator. (%s).\n";

[[clang::noinline]]
core_basic t_any to_global_const(t_thrd_data* const thrd_data, t_any const arg, const char* const owner) {
     u8 const type = arg.bytes[15];
     assert(type_is_correct(type));

     if (type_is_val(type)) return arg;

     t_any* items     = nullptr;
     u64    items_len = 0;
     t_any  result    = arg;

     if (type == tid__box || type == tid__breaker) {
          if (type == tid__breaker) [[clang::unlikely]]
               fail_with_call_stack(thrd_data, "A breaker cannot be a global constant value.", owner);

          if (box__is_global(result)) return result;

          u8 const idx = box__get_idx(result);
          thrd_data->free_boxes ^= (u32)1 << idx;

          items     = box__get_items(result);
          items_len = box__get_len(result);

          u64    const mem_size      = items_len * 16;
          t_any* const new_items_mem = aligned_alloc(16, mem_size);

          result = (const t_any){.structure = {.value = (u64)new_items_mem, .type = tid__box}};
          result = box__set_metadata(result, items_len, true, 0, false);
          memcpy_le_256(new_items_mem, items, mem_size);
     } else {
          u64 const ref_cnt = get_ref_cnt(result);
          if (ref_cnt == 0) return result;

          switch (type) {
          [[clang::unlikely]]
          case tid__error:
               error__show_and_exit(thrd_data, result);
          case tid__obj: {
               if (hash_map__get_len(result) == 0) {
                    ref_cnt__dec(thrd_data, result);
                    return empty_obj;
               }

               set_ref_cnt(result, 0);

               t_any* const mtds       = obj__get_mtds(result);
               *mtds                   = to_global_const(thrd_data, *mtds, owner);
               items                   = &mtds[1];
               u64          remain_qty = obj__get_fields_len(result);

               for (u64 idx = 0; remain_qty != 0; idx += 2) {
                    if (items[idx].bytes[15] != tid__token) continue;

                    remain_qty -= 1;
                    items[idx + 1] = to_global_const(thrd_data, items[idx + 1], owner);
               }
               return result;
          }
          case tid__table: {
               u64 remain_qty = hash_map__get_len(result) * 2;

               if (remain_qty == 0) {
                    ref_cnt__dec(thrd_data, result);
                    return empty_table;
               }
               set_ref_cnt(result, 0);

               items = table__get_kvs(result);

               for (u64 idx = 0; remain_qty != 0; idx += 1) {
                    t_any* const item = &items[idx];
                    if (item->bytes[15] == tid__null || item->bytes[15] == tid__holder) {
                         idx += 1;
                         continue;
                    }

                    remain_qty -= 1;
                    *item = to_global_const(thrd_data, *item, owner);
               }
               return result;
          }
          case tid__set: {
               u64 remain_qty = hash_map__get_len(result);
               if (remain_qty == 0) {
                    ref_cnt__dec(thrd_data, result);
                    return empty_set;
               }

               set_ref_cnt(result, 0);

               items = set__get_items(result);

               for (u64 idx = 0; remain_qty != 0; idx += 1) {
                    t_any* const item = &items[idx];
                    if (item->bytes[15] == tid__null || item->bytes[15] == tid__holder) continue;

                    remain_qty -= 1;
                    *item = to_global_const(thrd_data, *item, owner);
               }
               return result;
          }
          case tid__byte_array: {
               u32 const slice_offset = slice__get_offset(result);
               u32 const slice_len    = slice__get_len(result);
               u64 const array_len    = slice_array__get_len(result);
               u64 const array_cap    = slice_array__get_cap(result);
               u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

               assert(slice_len <= slice_max_len || slice_offset == 0);
               assume(array_cap >= array_len);

               if (len == array_cap) break;

               if (ref_cnt == 1) {
                    u8* const bytes = slice_array__get_items(result);
                    if (slice_offset != 0) memmove(bytes, &bytes[slice_offset], len);

                    slice_array__set_len(result, len);
                    slice_array__set_cap(result, len);
                    result           = slice__set_metadata(result, 0, slice_len);
                    result.qwords[0] = (u64)realloc((u8*)result.qwords[0], len + 16);
               } else {
                    set_ref_cnt(result, ref_cnt - 1);

                    t_any new_array = long_byte_array__new(len);
                    new_array       = slice__set_metadata(new_array, 0, slice_len);
                    slice_array__set_len(new_array, len);
                    memcpy(slice_array__get_items(new_array), &((const u8*)slice_array__get_items(result))[slice_offset], len);
                    result = new_array;
               }
               break;
          }
          case tid__string: {
               u32 const slice_offset = slice__get_offset(result);
               u32 const slice_len    = slice__get_len(result);
               u64 const array_len    = slice_array__get_len(result);
               u64 const array_cap    = slice_array__get_cap(result);
               u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

               assert(slice_len <= slice_max_len || slice_offset == 0);
               assume(array_cap >= array_len);

               if (len == array_cap) break;

               if (ref_cnt == 1) {
                    u8* const chars = slice_array__get_items(result);
                    if (slice_offset != 0) memmove(chars, &chars[slice_offset * 3], len * 3);

                    slice_array__set_len(result, len);
                    slice_array__set_cap(result, len);
                    result           = slice__set_metadata(result, 0, slice_len);
                    result.qwords[0] = (u64)realloc((u8*)result.qwords[0], len * 3 + 16);
               } else {
                    set_ref_cnt(result, ref_cnt - 1);

                    t_any new_array = long_string__new(len);
                    new_array       = slice__set_metadata(new_array, 0, slice_len);
                    slice_array__set_len(new_array, len);
                    memcpy(slice_array__get_items(new_array), &((const u8*)slice_array__get_items(result))[slice_offset * 3], len * 3);
                    result = new_array;
               }
               break;
          }
          case tid__array: {
               u32 const slice_offset = slice__get_offset(result);
               u32 const slice_len    = slice__get_len(result);
               u64 const array_len    = slice_array__get_len(result);
               u64 const array_cap    = slice_array__get_cap(result);
               items_len              = slice_len <= slice_max_len ? slice_len : array_len;
               items                  = slice_array__get_items(result);

               assert(slice_len <= slice_max_len || slice_offset == 0);
               assume(array_cap >= array_len);
               assume_aligned(items, 16);

               if (items_len == array_cap) break;
               if (items_len == 0) {
                    ref_cnt__dec(thrd_data, result);
                    return empty_array;
               }

               if (ref_cnt == 1) {
                    if (slice_offset != 0) {
                         for (u64 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
                         memmove(items, &items[slice_offset], items_len * 16);
                    }
                    for (u64 idx = slice_offset + items_len; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

                    slice_array__set_len(result, items_len);
                    slice_array__set_cap(result, items_len);
                    result           = slice__set_metadata(result, 0, slice_len);
                    result.qwords[0] = (u64)aligned_realloc(16, (t_any*)result.qwords[0], (items_len + 1) * 16);
                    items            = slice_array__get_items(result);
               } else {
                    set_ref_cnt(result, ref_cnt - 1);

                    t_any new_array = array__init(thrd_data, items_len, owner);
                    new_array       = slice__set_metadata(new_array, 0, slice_len);
                    slice_array__set_len(new_array, items_len);
                    items           = slice_array__get_items(new_array);
                    memcpy(items, &((const t_any*)slice_array__get_items(result))[slice_offset], items_len * 16);
                    result = new_array;
               }
               break;
          }
          case tid__fn: {
               assert(!fn__is_linear_alloc(result));

               u8  const params_cap   = fn__get_params_cap(result);
               u64 const borrowed_len = fn__get_borrowed_len(result);

               items     = fn__get_args(thrd_data, result);
               items_len = params_cap + borrowed_len;
               break;
          }
          case tid__thread:
               unreachable;
          [[clang::unlikely]]
          case tid__channel:
               fail_with_call_stack(thrd_data, "A channel cannot be a global constant value.", owner);
          case tid__custom: {
               t_any (*const to_global_const__fn)(t_thrd_data* const, t_any const, const char* const) = custom__get_fns(result)->to_global_const;
               return to_global_const__fn(thrd_data, result, owner);
          }
          default:
               unreachable;
          }

          set_ref_cnt(result, 0);
     }

     for (u64 idx = 0; idx < items_len; idx += 1) {
          t_any* const item = &items[idx];
          *item = to_global_const(thrd_data, *item, owner);
     }

     return result;
}
