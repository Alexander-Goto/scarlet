#pragma once

#include "../../common/include.h"
#include "../../common/type.h"
#include "../../common/xxh3.h"
#include "../array/basic.h"
#include "../bool/basic.h"
#include "../common/hash_map.h"
#include "../error/fn.h"
#include "../holder/basic.h"
#include "../null/basic.h"
#include "basic.h"

[[gnu::hot, clang::noinline]]
static t_any equal(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner);

[[clang::always_inline]]
static t_any* table__key_ptr(t_thrd_data* const thrd_data, t_any const table, t_any const key, u64 const key_hash, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64    const last_idx     = hash_map__get_last_idx(table);
     u16    const chunk_size   = hash_map__get_chunk_size(table);
     t_any* const items        = table__get_kvs(table);
     u64    const items_len    = (last_idx + 1) * chunk_size * 2;
     u64    const first_idx    = (key_hash & last_idx) * chunk_size * 2;
     u64          remain_steps = items_len / 2;

     u64 idx = first_idx;

     assert(idx < items_len);

     t_any* result = nullptr;
     while (true) {
          t_any* const key_ptr = &items[idx];
          if (key_ptr->bytes[15] == tid__null) {
               if (result == nullptr)
                    return items_len != (hash_map_max_chunks * hash_map_max_chunk_size * 2) && items_len > 8 && (hash_map__get_len(table) * 5 / 4) * 2 > items_len ? nullptr : key_ptr;

               return result;
          }

          t_any const eq_result = equal(thrd_data, key, *key_ptr, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) return key_ptr;
          if (key_ptr->bytes[15] == tid__holder && result == nullptr) result = key_ptr;

          remain_steps -= 1;
          idx          += 2;
          idx           = idx == items_len ? 0 : idx;

          if (remain_steps == 0) return result;
     }
}

[[gnu::hot, clang::noinline]]
static t_any hash(t_thrd_data* const thrd_data, t_any const arg, u64 seed, const char* const owner);

static void table__unpack__own(t_thrd_data* const thrd_data, t_any table, u64 const unpacking_items_len, t_any* const unpacking_items, const char* const owner) {
     if (table.bytes[15] != tid__table) {
          if (!(table.bytes[15] == tid__error || table.bytes[15] == tid__null)) {
               ref_cnt__dec(thrd_data, table);
               table = error__invalid_type(thrd_data, owner);
          }

          for (u64 idx = 0; idx < unpacking_items_len; idx += 1) {
               t_any const key = unpacking_items[idx];
               if (key.bytes[15] != tid__error) [[clang::likely]] {
                    ref_cnt__inc(thrd_data, table, owner);
                    ref_cnt__dec(thrd_data, key);
                    unpacking_items[idx] = table;
               }
          }

          ref_cnt__dec(thrd_data, table);
          return;
     }

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     u64 const seed = hash_map__get_hash_seed(table);
     for (u64 idx = 0; idx < unpacking_items_len; idx += 1) {
          t_any const key = unpacking_items[idx];
          if (unpacking_items[idx].bytes[15] != tid__error) [[clang::likely]] {
               t_any const key_hash = hash(thrd_data, key, seed, owner);
               if (key_hash.bytes[15] == tid__error) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, key);
                    unpacking_items[idx] = key_hash;
                    continue;
               }

               assert(key_hash.bytes[15] == tid__int);

               const t_any* const key_in_table_ptr = table__key_ptr(thrd_data, table, key, key_hash.qwords[0], owner);
               ref_cnt__dec(thrd_data, key);
               if (key_in_table_ptr == nullptr || key_in_table_ptr->bytes[15] == tid__null || key_in_table_ptr->bytes[15] == tid__holder) unpacking_items[idx].bytes[15] = tid__null;
               else {
                    t_any const value = key_in_table_ptr[1];
                    ref_cnt__inc(thrd_data, value, owner);
                    unpacking_items[idx] = value;
               }
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, table);
}

static t_any table__all__own(t_thrd_data* const thrd_data, t_any const table, t_any const default_result, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64    const ref_cnt = get_ref_cnt(table);
     t_any* const kvs     = table__get_kvs(table);

     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     t_any result = bool__true;
     u64   idx    = 0;
     for (; remain_qty != 0; idx += 2) {
          t_any const value = kvs[idx + 1];
          if (value.bytes[15] == tid__null || value.bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt == 1) ref_cnt__dec(thrd_data, kvs[idx]);
          else ref_cnt__inc(thrd_data, value, owner);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &value, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 0) {
               result = bool__false;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               t_any const key = kvs[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, kvs[idx + 1]);
          }

          free((void*)table.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any table__all_kv__own(t_thrd_data* const thrd_data, t_any const table, t_any const default_result, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64    const ref_cnt = get_ref_cnt(table);
     t_any* const kvs     = table__get_kvs(table);

     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     t_any result = bool__true;
     u64   idx    = 0;
     for (; remain_qty != 0; idx += 2) {
          const t_any* const kv_ptr = &kvs[idx];
          if (kv_ptr->bytes[15] == tid__null || kv_ptr->bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt > 1) {
               ref_cnt__inc(thrd_data, kv_ptr[0], owner);
               ref_cnt__inc(thrd_data, kv_ptr[1], owner);
          }

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv_ptr, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 0) {
               result = bool__false;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               t_any const key = kvs[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, kvs[idx + 1]);
          }

          free((void*)table.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any table__any__own(t_thrd_data* const thrd_data, t_any const table, t_any const default_result, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64    const ref_cnt = get_ref_cnt(table);
     t_any* const kvs     = table__get_kvs(table);

     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     t_any result = bool__false;
     u64   idx    = 0;
     for (; remain_qty != 0; idx += 2) {
          t_any const value = kvs[idx + 1];
          if (value.bytes[15] == tid__null || value.bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt == 1) ref_cnt__dec(thrd_data, kvs[idx]);
          else ref_cnt__inc(thrd_data, value, owner);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &value, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 1) {
               result = bool__true;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               t_any const key = kvs[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, kvs[idx + 1]);
          }

          free((void*)table.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any table__any_kv__own(t_thrd_data* const thrd_data, t_any const table, t_any const default_result, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64    const ref_cnt = get_ref_cnt(table);
     t_any* const kvs     = table__get_kvs(table);

     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     t_any result = bool__false;
     u64   idx    = 0;
     for (; remain_qty != 0; idx += 2) {
          const t_any* const kv_ptr = &kvs[idx];
          if (kv_ptr->bytes[15] == tid__null || kv_ptr->bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt > 1) {
               ref_cnt__inc(thrd_data, kv_ptr[0], owner);
               ref_cnt__inc(thrd_data, kv_ptr[1], owner);
          }

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv_ptr, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 1) {
               result = bool__true;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               t_any const key = kvs[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, kvs[idx + 1]);
          }

          free((void*)table.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any table__clear__own(t_thrd_data* const thrd_data, t_any const table, const char* const owner) {
     assert(table.bytes[15] == tid__table);

     t_any* const items   = table__get_kvs(table);
     u64    const ref_cnt = get_ref_cnt(table);
     if (ref_cnt == 1) {
          u64 remain_qty = hash_map__get_len(table);
          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               t_any* const kv_ptr = &items[idx];
               if (kv_ptr->bytes[15] == tid__null || kv_ptr->bytes[15] == tid__holder) continue;

               remain_qty -= 1;

               ref_cnt__dec(thrd_data, kv_ptr[0]);
               ref_cnt__dec(thrd_data, kv_ptr[1]);
               kv_ptr[0].bytes[15] = tid__null;
               kv_ptr[1].bytes[15] = tid__null;
          }

          return hash_map__set_len(table, 0);
     }

     if (ref_cnt != 0) set_ref_cnt(table, ref_cnt - 1);
     u64 const items_len = (hash_map__get_last_idx(table) + 1) * hash_map__get_chunk_size(table);
     return table__init(thrd_data, items_len, owner);
}

static t_any table__equal(t_thrd_data* const thrd_data, t_any const left,  t_any const right, const char* const owner) {
     assert(left.bytes[15] == tid__table);
     assert(right.bytes[15] == tid__table);

     u64 remain_qty = hash_map__get_len(left);

     if (remain_qty != hash_map__get_len(right)) return bool__false;
     if (remain_qty == 0) return bool__true;

     t_any small;
     t_any big;

     if ((hash_map__get_last_idx(left) + 1) * hash_map__get_chunk_size(left) < (hash_map__get_last_idx(right) + 1) * hash_map__get_chunk_size(right)) {
          small         = left;
          big           = right;
     } else {
          small         = right;
          big           = left;
     }

     const t_any* const small_kvs = table__get_kvs(small);
     u64          const seed      = hash_map__get_hash_seed(big);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const key = small_kvs[idx];
          if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

          remain_qty          -= 1;
          t_any const key_hash = hash(thrd_data, key, seed, owner);

          assert(key_hash.bytes[15] == tid__int);

          const t_any* const key_ptr = table__key_ptr(thrd_data, big, key, key_hash.qwords[0], owner);
          if (key_ptr == nullptr || key_ptr->bytes[15] == tid__null || key_ptr->bytes[15] == tid__holder) return bool__false;
          t_any const eq_result = equal(thrd_data, small_kvs[idx + 1], key_ptr[1], owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) return eq_result;
     }

     return bool__true;
}

static inline t_any table__get_item__own(t_thrd_data* const thrd_data, t_any const table, t_any const key, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64   const seed     = hash_map__get_hash_seed(table);
     t_any const key_hash = hash(thrd_data, key, seed, owner);
     if (key_hash.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, key);
          return key_hash;
     }

     assert(key_hash.bytes[15] == tid__int);

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     const t_any* const key_ptr = table__key_ptr(thrd_data, table, key, key_hash.qwords[0], owner);
     t_any value;
     if (key_ptr == nullptr || key_ptr->bytes[15] == tid__null || key_ptr->bytes[15] == tid__holder) value.bytes[15] = tid__null;
     else {
          value = key_ptr[1];
          ref_cnt__inc(thrd_data, value, owner);
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, table);
     ref_cnt__dec(thrd_data, key);
     return value;
}

[[clang::noinline]]
static t_any table__enlarge__own(t_thrd_data* const thrd_data, t_any const table, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     const t_any* const kvs        = table__get_kvs(table);
     u64                remain_qty = hash_map__get_len(table);
     t_any              new_table  = table__init(thrd_data, (hash_map__get_last_idx(table) + 1) * hash_map__get_chunk_size(table) + 1, owner);
     hash_map__set_hash_seed(new_table, hash_map__get_hash_seed(table));

     u64 const seed = hash_map__get_hash_seed(new_table);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const key = kvs[idx];
          if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

          remain_qty          -= 1;
          t_any const key_hash = hash(thrd_data, key, seed, owner);

          assert(key_hash.bytes[15] == tid__int);

          t_any* const kv_ptr = table__key_ptr(thrd_data, new_table, key, key_hash.qwords[0], owner);

          assume(kv_ptr != nullptr);
          assert(kv_ptr->bytes[15] == tid__null);

          new_table = hash_map__set_len(new_table, hash_map__get_len(new_table) + 1);

          t_any const value = kvs[idx + 1];
          kv_ptr[0]         = key;
          kv_ptr[1]         = value;

          if (ref_cnt > 1) {
               ref_cnt__inc(thrd_data, key, owner);
               ref_cnt__inc(thrd_data, value, owner);
          }
     }

     if (ref_cnt == 1) free((void*)table.qwords[0]);

     return new_table;
}

[[clang::noinline]]
static t_any table__dup__own(t_thrd_data* const thrd_data, t_any const table, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64 const    items_len = (hash_map__get_last_idx(table) + 1) * hash_map__get_chunk_size(table) * 2;
     u64 const    mem_size  = (items_len + 1) * 16;
     t_any        new_table = table;
     new_table.qwords[0]    = (u64)aligned_alloc(16, mem_size);
     const t_any* old_data  = (const t_any*)table.qwords[0];
     t_any*       new_data  = (t_any*)new_table.qwords[0];

     new_data->raw_bits = old_data->raw_bits;
     set_ref_cnt(new_table, 1);

     const t_any* old_items = &old_data[1];
     t_any*       new_items = &new_data[1];

     u64 const ref_cnt = get_ref_cnt(table);

     assert(ref_cnt != 1);

     if (ref_cnt == 0)
          memcpy(new_items, old_items, mem_size - 16);
     else {
          set_ref_cnt(table, ref_cnt - 1);

          for (u64 idx = 0; idx < items_len; idx += 1) {
               t_any const item = old_items[idx];
               new_items[idx]   = item;
               ref_cnt__inc(thrd_data, item, owner);
          }
     }

     return new_table;
}

static t_any table__each_to_each__own(t_thrd_data* const thrd_data, t_any table, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty_1 = hash_map__get_len(table);
     if (remain_qty_1 < 2) {
          ref_cnt__dec(thrd_data, fn);
          return table;
     }

     if (get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);

     t_any* const kvs = table__get_kvs(table);
     t_any        fn_args[2];
     for (u64 idx_1 = 0; remain_qty_1 != 1; idx_1 += 2) {
          t_any const key_1 = kvs[idx_1];
          if (key_1.bytes[15] == tid__null || key_1.bytes[15] == tid__holder) continue;

          fn_args[0] = kvs[idx_1 + 1];
          remain_qty_1 -= 1;

          u64 remain_qty_2 = remain_qty_1;
          for (u64 idx_2 = idx_1 + 2; remain_qty_2 != 0; idx_2 += 2) {
               t_any const key_2 = kvs[idx_2];
               if (key_2.bytes[15] == tid__null || key_2.bytes[15] == tid__holder) continue;

               fn_args[1] = kvs[idx_2 + 1];
               remain_qty_2 -= 1;

               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    kvs[idx_1 + 1].raw_bits = 0;
                    kvs[idx_2 + 1].raw_bits = 0;
                    ref_cnt__dec(thrd_data, table);
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || !type_is_common(box_items[0].bytes[15]) || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
                    kvs[idx_1 + 1].raw_bits = 0;
                    kvs[idx_2 + 1].raw_bits = 0;
                    ref_cnt__dec(thrd_data, table);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[0]     = box_items[0];
               kvs[idx_2 + 1] = box_items[1];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          kvs[idx_1 + 1] = fn_args[0];
     }
     ref_cnt__dec(thrd_data, fn);

     return table;
}

static t_any table__each_to_each_kv__own(t_thrd_data* const thrd_data, t_any table, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty_1 = hash_map__get_len(table);
     if (remain_qty_1 < 2) {
          ref_cnt__dec(thrd_data, fn);
          return table;
     }

     if (get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);

     t_any* const kvs = table__get_kvs(table);
     t_any        fn_args[4];
     for (u64 idx_1 = 0; remain_qty_1 != 1; idx_1 += 2) {
          fn_args[0] = kvs[idx_1];
          if (fn_args[0].bytes[15] == tid__null || fn_args[0].bytes[15] == tid__holder) continue;

          fn_args[1] = kvs[idx_1 + 1];
          remain_qty_1 -= 1;

          u64 remain_qty_2 = remain_qty_1;
          for (u64 idx_2 = idx_1 + 2; remain_qty_2 != 0; idx_2 += 2) {
               fn_args[2] = kvs[idx_2];
               if (fn_args[2].bytes[15] == tid__null || fn_args[2].bytes[15] == tid__holder) continue;

               fn_args[3] = kvs[idx_2 + 1];
               remain_qty_2 -= 1;

               ref_cnt__inc(thrd_data, fn_args[0], owner);
               ref_cnt__inc(thrd_data, fn_args[2], owner);
               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 4, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    kvs[idx_1 + 1].raw_bits = 0;
                    kvs[idx_2 + 1].raw_bits = 0;
                    ref_cnt__dec(thrd_data, table);
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || !type_is_common(box_items[0].bytes[15]) || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
                    kvs[idx_1 + 1].raw_bits = 0;
                    kvs[idx_2 + 1].raw_bits = 0;
                    ref_cnt__dec(thrd_data, table);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[1]     = box_items[0];
               kvs[idx_2 + 1] = box_items[1];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          kvs[idx_1 + 1] = fn_args[1];
     }
     ref_cnt__dec(thrd_data, fn);

     return table;
}

static t_any table__builtin_unsafe_add_kv__own(t_thrd_data* const thrd_data, t_any table, t_any const key, t_any const value, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(key.bytes[15] != tid__error);
     assert(type_is_common(value.bytes[15]));

     u64   const seed     = hash_map__get_hash_seed(table);
     t_any const key_hash = hash(thrd_data, key, seed, owner);

     assume(key_hash.bytes[15] == tid__int);

     if (get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);

#ifndef NDEBUG
     bool first_time = true;
#endif
     while (true) {
          t_any* const kv_ptr = table__key_ptr(thrd_data, table, key, key_hash.qwords[0], owner);
          if (kv_ptr != nullptr) {
               if (!(kv_ptr->bytes[15] == tid__null || kv_ptr->bytes[15] == tid__holder)) [[clang::unlikely]] {
                    ref_cnt__dec__noinlined_part(thrd_data, table);
                    ref_cnt__dec(thrd_data, key);
                    ref_cnt__dec(thrd_data, value);
                    return error__not_uniq_key(thrd_data, owner);
               }

               table     = hash_map__set_len(table, hash_map__get_len(table) + 1);
               kv_ptr[0] = key;
               kv_ptr[1] = value;

               return table;
          }

          table = table__enlarge__own(thrd_data, table, owner);

#ifndef NDEBUG
          assert(first_time);
          first_time = false;
#endif
     }
}

static t_any table__builtin_add_kv__own(t_thrd_data* const thrd_data, t_any table, t_any const key, t_any const value, const char* const owner) {
     if (table.bytes[15] != tid__table || key.bytes[15] == tid__error || !type_is_common(value.bytes[15])) [[clang::unlikely]] {
          if (table.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, value);
               return table;
          }
          ref_cnt__dec(thrd_data, table);

          if (key.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, value);
               return key;
          }
          ref_cnt__dec(thrd_data, key);

          if (value.bytes[15] == tid__error) return value;
          ref_cnt__dec(thrd_data, value);

          return error__invalid_type(thrd_data, owner);
     }

     u64   const seed     = hash_map__get_hash_seed(table);
     t_any const key_hash = hash(thrd_data, key, seed, owner);
     if (key_hash.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, key);
          ref_cnt__dec(thrd_data, value);
          return key_hash;
     }

     assert(key_hash.bytes[15] == tid__int);

     if (get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);

     #ifndef NDEBUG
     bool first_time = true;
     #endif
     while (true) {
          t_any* const kv_ptr = table__key_ptr(thrd_data, table, key, key_hash.qwords[0], owner);
          if (kv_ptr != nullptr) {
               if (!(kv_ptr->bytes[15] == tid__null || kv_ptr->bytes[15] == tid__holder)) [[clang::unlikely]] {
                    ref_cnt__dec__noinlined_part(thrd_data, table);
                    ref_cnt__dec(thrd_data, key);
                    ref_cnt__dec(thrd_data, value);
                    return error__not_uniq_key(thrd_data, owner);
               }

               table     = hash_map__set_len(table, hash_map__get_len(table) + 1);
               kv_ptr[0] = key;
               kv_ptr[1] = value;

               return table;
          }

          table = table__enlarge__own(thrd_data, table, owner);

          #ifndef NDEBUG
          assert(first_time);
          first_time = false;
          #endif
     }
}

[[clang::noinline]]
static t_any table__fit__own(t_thrd_data* const thrd_data, t_any const table, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     t_any* const items      = table__get_kvs(table);
     u64          remain_qty = hash_map__get_len(table);

     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, table);
          return empty_table;
     }

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     t_any     new_table = table__init(thrd_data, remain_qty, owner);
     u64 const seed      = hash_map__get_hash_seed(new_table);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const key = items[idx];
          if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

          remain_qty          -= 1;
          t_any const key_hash = hash(thrd_data, key, seed, owner);

          assert(key_hash.bytes[15] == tid__int);

          t_any* const kv_ptr = table__key_ptr(thrd_data, new_table, key, key_hash.qwords[0], owner);

          assume(kv_ptr != nullptr);
          assert(kv_ptr->bytes[15] == tid__null);

          new_table = hash_map__set_len(new_table, hash_map__get_len(new_table) + 1);

          t_any const value = items[idx + 1];
          kv_ptr[0]         = key;
          kv_ptr[1]         = value;

          if (ref_cnt > 1) {
               ref_cnt__inc(thrd_data, key, owner);
               ref_cnt__inc(thrd_data, value, owner);
          }
     }

     if (ref_cnt == 1) free((void*)table.qwords[0]);

     return new_table;
}

static t_any table__filter__own(t_thrd_data* const thrd_data, t_any table, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64       remain_qty = hash_map__get_len(table);
     t_any*    items      = table__get_kvs(table);
     u64 const len        = remain_qty;
     u64       new_len    = len;
     bool      dst_is_mut = get_ref_cnt(table) == 1;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any* kv_ptr = &items[idx];
          if (kv_ptr->bytes[15] == tid__null || kv_ptr->bytes[15] == tid__holder) continue;

          remain_qty -= 1;
          ref_cnt__inc(thrd_data, kv_ptr[1], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &kv_ptr[1], 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_is_mut) table = hash_map__set_len(table, new_len);
               ref_cnt__dec(thrd_data, table);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) {
               if (!dst_is_mut) {
                    table      = table__dup__own(thrd_data, table, owner);
                    items      = table__get_kvs(table);
                    dst_is_mut = true;
               }

               kv_ptr = &items[idx];
               ref_cnt__dec(thrd_data, kv_ptr[0]);
               ref_cnt__dec(thrd_data, kv_ptr[1]);
               new_len -= 1;
               memset_inline(kv_ptr, tid__holder, 32);
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (new_len == len) return table;
     table = hash_map__set_len(table, new_len);

     if (new_len == 0) {
          ref_cnt__dec(thrd_data, table);
          return empty_table;
     }

     if (table__need_to_fit(table)) table = table__fit__own(thrd_data, table, owner);
     return table;
}

static t_any table__filter_kv__own(t_thrd_data* const thrd_data, t_any table, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64       remain_qty = hash_map__get_len(table);
     t_any*    items      = table__get_kvs(table);
     u64 const len        = remain_qty;
     u64       new_len    = len;
     bool      dst_is_mut = get_ref_cnt(table) == 1;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any* kv_ptr = &items[idx];
          if (kv_ptr->bytes[15] == tid__null || kv_ptr->bytes[15] == tid__holder) continue;

          remain_qty -= 1;
          ref_cnt__inc(thrd_data, kv_ptr[0], owner);
          ref_cnt__inc(thrd_data, kv_ptr[1], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv_ptr, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_is_mut) table = hash_map__set_len(table, new_len);
               ref_cnt__dec(thrd_data, table);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) {
               if (!dst_is_mut) {
                    table      = table__dup__own(thrd_data, table, owner);
                    items      = table__get_kvs(table);
                    dst_is_mut = true;
               }

               kv_ptr = &items[idx];
               ref_cnt__dec(thrd_data, kv_ptr[0]);
               ref_cnt__dec(thrd_data, kv_ptr[1]);
               new_len -= 1;
               memset_inline(kv_ptr, tid__holder, 32);
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (new_len == len) return table;
     table = hash_map__set_len(table, new_len);

     if (new_len == 0) {
          ref_cnt__dec(thrd_data, table);
          return empty_table;
     }

     if (table__need_to_fit(table)) table = table__fit__own(thrd_data, table, owner);
     return table;
}

static t_any table__foldl__own(t_thrd_data* const thrd_data, t_any const table, t_any const state, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64       remain_qty = hash_map__get_len(table);
     u64 const ref_cnt    = get_ref_cnt(table);
     if (ref_cnt > 1 && state.qwords[0] != table.qwords[0]) set_ref_cnt(table, ref_cnt - 1);

     t_any* const kvs = table__get_kvs(table);
     t_any        fn_args[2];
     fn_args[0]       = state;
     u64          idx = 0;
     for (; remain_qty != 0; idx += 2) {
          fn_args[1] = kvs[idx + 1];
          if (fn_args[1].bytes[15] == tid__null || fn_args[1].bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt == 1)
               ref_cnt__dec(thrd_data, kvs[idx]);
          else ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               t_any const key = kvs[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, kvs[idx + 1]);
          }

          free((void*)table.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == table.qwords[0]) ref_cnt__dec(thrd_data, table);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any table__foldl1__own(t_thrd_data* const thrd_data, t_any const table, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(table);

     if (remain_qty == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     t_any* const kvs  = table__get_kvs(table);
     t_any        fn_args[2];
     u64          idx  = 0;
     for (;; idx += 2) {
          fn_args[0] = kvs[idx + 1];
          if (fn_args[0].bytes[15] == tid__null || fn_args[0].bytes[15] == tid__holder) continue;

          remain_qty -= 1;
          if (ref_cnt == 1)
               ref_cnt__dec(thrd_data, kvs[idx]);
          else ref_cnt__inc(thrd_data, fn_args[0], owner);
          break;
     }

     for (idx += 2; remain_qty != 0; idx += 2) {
          fn_args[1] = kvs[idx + 1];
          if (fn_args[1].bytes[15] == tid__null || fn_args[1].bytes[15] == tid__holder) continue;

          remain_qty -= 1;
          if (ref_cnt == 1)
               ref_cnt__dec(thrd_data, kvs[idx]);
          else ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               t_any const key = kvs[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, kvs[idx + 1]);
          }

          free((void*)table.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any table__foldl_kv__own(t_thrd_data* const thrd_data, t_any const table, t_any const state, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64       remain_qty = hash_map__get_len(table);
     u64 const ref_cnt    = get_ref_cnt(table);
     if (ref_cnt > 1 && state.qwords[0] != table.qwords[0]) set_ref_cnt(table, ref_cnt - 1);

     t_any* const kvs = table__get_kvs(table);
     t_any        fn_args[3];
     fn_args[0]       = state;
     u64          idx = 0;
     for (; remain_qty != 0; idx += 2) {
          fn_args[1] = kvs[idx];
          if (fn_args[1].bytes[15] == tid__null || fn_args[1].bytes[15] == tid__holder) continue;

          remain_qty -= 1;
          fn_args[2]  = kvs[idx + 1];
          if (ref_cnt > 1) {
               ref_cnt__inc(thrd_data, fn_args[1], owner);
               ref_cnt__inc(thrd_data, fn_args[2], owner);
          }
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               t_any const key = kvs[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, kvs[idx + 1]);
          }

          free((void*)table.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == table.qwords[0]) ref_cnt__dec(thrd_data, table);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any table__hash(t_thrd_data* const thrd_data, t_any const table, u64 const seed, const char* const owner) {
     assert(table.bytes[15] == tid__table);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) return (const t_any){.structure = {.value = xxh3_get_hash(nullptr, 0, seed ^ table.bytes[15]), .type = tid__int}};

     const t_any* const items      = table__get_kvs(table);
     u64                table_hash = 0;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const key = items[idx];
          if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

          remain_qty          -= 1;
          t_any const key_hash = hash(thrd_data, key, seed, owner);
          if (key_hash.bytes[15] == tid__error) [[clang::unlikely]] return key_hash;

          t_any const value_hash = hash(thrd_data, items[idx + 1], key_hash.qwords[0], owner);
          if (value_hash.bytes[15] == tid__error) [[clang::unlikely]] return value_hash;

          assert(key_hash.bytes[15] == tid__int);
          assert(value_hash.bytes[15] == tid__int);

          table_hash ^= value_hash.qwords[0];
     }

     return (const t_any){.structure = {.value = table_hash, .type = tid__int}};
}

static t_any table__look_in__own(t_thrd_data* const thrd_data, t_any const table, t_any const looked_item, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, looked_item);
          return null;
     }

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     t_any* const kvs    = table__get_kvs(table);
     t_any        result = null;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (kvs[idx].bytes[15] == tid__null || kvs[idx].bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const eq_result = equal(thrd_data, kvs[idx + 1], looked_item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) {
               result = kvs[idx];
               ref_cnt__inc(thrd_data, result, owner);
               break;
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, table);
     ref_cnt__dec(thrd_data, looked_item);

     return result;
}

static t_any table__look_other_in__own(t_thrd_data* const thrd_data, t_any const table, t_any const not_looked_item, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, not_looked_item);
          return null;
     }

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     t_any* const kvs    = table__get_kvs(table);
     t_any        result = null;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (kvs[idx].bytes[15] == tid__null || kvs[idx].bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const eq_result = equal(thrd_data, kvs[idx + 1], not_looked_item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) {
               result = kvs[idx];
               ref_cnt__inc(thrd_data, result, owner);
               break;
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, table);
     ref_cnt__dec(thrd_data, not_looked_item);

     return result;
}

static t_any table__map__own(t_thrd_data* const thrd_data, t_any table, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, fn);
          return table;
     }

     if (get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);
     t_any* items = table__get_kvs(table);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (items[idx].bytes[15] == tid__null || items[idx].bytes[15] == tid__holder) continue;

          remain_qty            -= 1;
          t_any* const value_ptr = &items[idx + 1];
          t_any  const fn_result = common_fn__call__half_own(thrd_data, fn, value_ptr, 1, owner);
          if (!type_is_common(fn_result.bytes[15])) [[clang::unlikely]] {
               value_ptr->raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, table);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          *value_ptr = fn_result;
     }

     ref_cnt__dec(thrd_data, fn);

     return table;
}

static t_any table__map_kv__own(t_thrd_data* const thrd_data, t_any table, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, fn);
          return table;
     }

     if (get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);
     t_any* const items = table__get_kvs(table);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any* const kv_ptr = &items[idx];
          if (kv_ptr->bytes[15] == tid__null || kv_ptr->bytes[15] == tid__holder) continue;

          remain_qty -= 1;
          ref_cnt__inc(thrd_data, kv_ptr[0], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv_ptr, 2, owner);
          if (!type_is_common(fn_result.bytes[15])) [[clang::unlikely]] {
               kv_ptr[1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, table);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          kv_ptr[1] = fn_result;
     }

     ref_cnt__dec(thrd_data, fn);

     return table;
}

static t_any table__map_with_state__own(t_thrd_data* const thrd_data, t_any table, t_any const state, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty != 0 && get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);

     t_any* const kvs = table__get_kvs(table);
     t_any        fn_args[2];
     fn_args[0]       = state;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const key = kvs[idx];
          if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          fn_args[1] = kvs[idx + 1];
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               kvs[idx + 1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, table);
               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
               kvs[idx + 1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, table);
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]   = box_items[0];
          kvs[idx + 1] = box_items[1];
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[0]           = table;
     new_box_items[1]           = fn_args[0];

     return new_box;
}

static t_any table__map_kv_with_state__own(t_thrd_data* const thrd_data, t_any table, t_any const state, t_any const fn, const char* const owner) {
     assume(table.bytes[15] == tid__table);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty != 0 && get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);

     t_any* const kvs = table__get_kvs(table);
     t_any        fn_args[3];
     fn_args[0]       = state;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          fn_args[1] = kvs[idx];
          if (fn_args[1].bytes[15] == tid__null || fn_args[1].bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          fn_args[2] = kvs[idx + 1];
          ref_cnt__inc(thrd_data, fn_args[1], owner);
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               kvs[idx + 1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, table);
               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
               kvs[idx + 1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, table);
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]   = box_items[0];
          kvs[idx + 1] = box_items[1];
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[0]           = table;
     new_box_items[1]           = fn_args[0];

     return new_box;
}

[[clang::noinline]]
static t_any common_print(t_thrd_data* const thrd_data, t_any const arg, const char* const owner);

static t_any table__print(t_thrd_data* const thrd_data, t_any const table, const char* const owner) {
     assert(table.bytes[15] == tid__table);

     u64 remain_qty = hash_map__get_len(table) * 2;
     if (remain_qty == 0) {
          if (fwrite("[table]", 1, 7, stdout) != 7) [[clang::unlikely]] goto cant_print_label;
          return null;
     }

     if (fwrite("[table ", 1, 7, stdout) != 7) [[clang::unlikely]] goto cant_print_label;

     const t_any* const items  = table__get_kvs(table);
     bool               is_key = true;
     for (u64 idx = 0; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) {
               idx += 1;
               continue;
          }

          remain_qty                   -= 1;
          t_any const print_item_result = common_print(thrd_data, item, owner);
          if (print_item_result.bytes[15] == tid__error) [[clang::unlikely]] return print_item_result;

          const char* str;
          int         str_len;
          if (is_key) {
               str     = " = ";
               str_len = 3;
          } else if (remain_qty == 0) {
               str     = "]";
               str_len = 1;
          } else {
               str         = ", ";
               str_len     = 2;
          }

          is_key = !is_key;
          if (fwrite(str, 1, str_len, stdout) != str_len) [[clang::unlikely]] goto cant_print_label;
     }

     return null;

     cant_print_label:
     return error__cant_print(thrd_data, owner);
}

static t_any table__qty__own(t_thrd_data* const thrd_data, t_any table, t_any const looked_item, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, table);
          ref_cnt__dec(thrd_data, looked_item);
          return (const t_any){.bytes = {[15] = tid__int}};
     }

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     t_any* const items  = table__get_kvs(table);
     u64          result = 0;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (items[idx].bytes[15] == tid__null || items[idx].bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const eq_result = equal(thrd_data, items[idx + 1], looked_item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          result += eq_result.bytes[0];
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, table);
     ref_cnt__dec(thrd_data, looked_item);

     return (const t_any){.structure = {.value = result, .type = tid__int}};
}

static t_any table__replace__own(t_thrd_data* const thrd_data, t_any table, t_any const old_item, t_any const new_item, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64 remain_qty = hash_map__get_len(table);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, old_item);
          ref_cnt__dec(thrd_data, new_item);
          return table;
     }

     bool   is_mut = get_ref_cnt(table) == 1;
     t_any* items  = table__get_kvs(table);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (items[idx].bytes[15] == tid__null || items[idx].bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const eq_result = equal(thrd_data, items[idx + 1], old_item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) {
               if (!is_mut) {
                    table = table__dup__own(thrd_data, table, owner);
                    items = table__get_kvs(table);
                    is_mut = true;
               }

               ref_cnt__inc(thrd_data, new_item, owner);
               ref_cnt__dec(thrd_data, items[idx + 1]);
               items[idx + 1] = new_item;
          }
     }

     ref_cnt__dec(thrd_data, old_item);
     ref_cnt__dec(thrd_data, new_item);
     return table;
}

static t_any table__reserve__own(t_thrd_data* const thrd_data, t_any table, u64 const new_items_qty, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64 const len     = hash_map__get_len(table);
     u64 const new_cap = len + new_items_qty;
     if (new_cap > hash_map_max_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, table);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const current_cap = (hash_map__get_last_idx(table) + 1) * hash_map__get_chunk_size(table);
     if (current_cap >= new_cap) return table;

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     const t_any* const kvs        = table__get_kvs(table);
     u64                remain_qty = len;
     t_any              new_table  = table__init(thrd_data, new_cap, owner);
     hash_map__set_hash_seed(new_table, hash_map__get_hash_seed(table));

     u64 const seed = hash_map__get_hash_seed(new_table);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const key = kvs[idx];
          if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

          remain_qty          -= 1;
          t_any const key_hash = hash(thrd_data, key, seed, owner);

          assert(key_hash.bytes[15] == tid__int);

          t_any* const kv_ptr = table__key_ptr(thrd_data, new_table, key, key_hash.qwords[0], owner);

          assume(kv_ptr != nullptr);
          assert(kv_ptr->bytes[15] == tid__null);

          new_table = hash_map__set_len(new_table, hash_map__get_len(new_table) + 1);

          t_any const value = kvs[idx + 1];
          kv_ptr[0]         = key;
          kv_ptr[1]         = value;

          if (ref_cnt > 1) {
               ref_cnt__inc(thrd_data, key, owner);
               ref_cnt__inc(thrd_data, value, owner);
          }
     }

     if (ref_cnt == 1) free((void*)table.qwords[0]);

     return new_table;
}

static inline t_any table__take_one__own(t_thrd_data* const thrd_data, t_any const table, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     if (hash_map__get_len(table) == 0) {
          ref_cnt__dec(thrd_data, table);
          return null;
     }

     t_any*       items   = table__get_kvs(table);
     u64    const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     while (true) {
          if (!(items->bytes[15] == tid__null || items->bytes[15] == tid__holder)) {
               t_any const result = items[1];
               ref_cnt__inc(thrd_data, result, owner);
               if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, table);
               return result;
          }

          items = &items[2];
     }
}

static inline t_any table__take_one_kv__own(t_thrd_data* const thrd_data, t_any const table, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     if (hash_map__get_len(table) == 0) {
          ref_cnt__dec(thrd_data, table);
          return null;
     }

     t_any*       items   = table__get_kvs(table);
     u64    const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     while (true) {
          if (!(items->bytes[15] == tid__null || items->bytes[15] == tid__holder)) {
               t_any  const result    = box__new(thrd_data, 2, owner);
               t_any* const box_items = box__get_items(result);
               memcpy_inline(box_items, items, 32);
               ref_cnt__inc(thrd_data, box_items[0], owner);
               ref_cnt__inc(thrd_data, box_items[1], owner);
               if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, table);
               return result;
          }

          items = &items[2];
     }
}

static t_any table__to_array__own(t_thrd_data* const thrd_data, t_any const table, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64 const len = hash_map__get_len(table);
     if (len == 0) {
          ref_cnt__dec(thrd_data, table);
          return empty_array;
     }

     t_any        array       = array__init(thrd_data, len, owner);
     array                    = slice__set_metadata(array, 0, len <= slice_max_len ? len : slice_max_len + 1);
     slice_array__set_len(array, len);
     t_any* const array_items = slice_array__get_items(array);
     u64    const ref_cnt     = get_ref_cnt(table);
     u64          remain_qty  = len;
     t_any* const table_items = table__get_kvs(table);
     if (ref_cnt > 1) {
          set_ref_cnt(table, ref_cnt - 1);

          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               if (table_items[idx].bytes[15] == tid__null || table_items[idx].bytes[15] == tid__holder) continue;

               t_any const item = table_items[idx + 1];
               ref_cnt__inc(thrd_data, item, owner);
               array_items[len - remain_qty] = item;
               remain_qty                   -= 1;
          }
     } else {
          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               if (table_items[idx].bytes[15] == tid__null || table_items[idx].bytes[15] == tid__holder) continue;

               ref_cnt__dec(thrd_data, table_items[idx]);
               array_items[len - remain_qty] = table_items[idx + 1];
               remain_qty                   -= 1;
          }

          if (ref_cnt == 1) free((void*)table.qwords[0]);
     }

     return array;
}

static t_any table__to_box__own(t_thrd_data* const thrd_data, t_any const table, const char* const owner) {
     assume(table.bytes[15] == tid__table);

     u64 const len = hash_map__get_len(table);
     if (len == 0) {
          ref_cnt__dec(thrd_data, table);
          return empty_box;
     }

     if (len > 16) {
          ref_cnt__dec(thrd_data, table);
          return error__out_of_bounds(thrd_data, owner);
     }

     t_any        box         = box__new(thrd_data, len, owner);
     t_any* const box_items   = box__get_items(box);
     u64    const ref_cnt     = get_ref_cnt(table);
     u64          remain_qty  = len;
     t_any* const table_items = table__get_kvs(table);
     if (ref_cnt > 1) {
          set_ref_cnt(table, ref_cnt - 1);

          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               if (table_items[idx].bytes[15] == tid__null || table_items[idx].bytes[15] == tid__holder) continue;

               t_any const item = table_items[idx + 1];
               ref_cnt__inc(thrd_data, item, owner);
               box_items[len - remain_qty] = item;
               remain_qty                   -= 1;
          }
     } else {
          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               if (table_items[idx].bytes[15] == tid__null || table_items[idx].bytes[15] == tid__holder) continue;

               ref_cnt__dec(thrd_data, table_items[idx]);
               box_items[len - remain_qty] = table_items[idx + 1];
               remain_qty                   -= 1;
          }

          if (ref_cnt == 1) free((void*)table.qwords[0]);
     }

     return box;
}

static void table__zip_init_read(const t_any* const table, t_zip_read_state* const state, t_any*, t_any*, u64* const result_len, t_any* const) {
     assert(table->bytes[15] == tid__table);

     *result_len                = hash_map__get_len(*table);
     state->state.item_position = table__get_kvs(*table);
     state->state.is_mut        = get_ref_cnt(*table) == 1;
}

static void table__zip_read(t_thrd_data* const thrd_data, t_zip_read_state* const state, t_any* const key, t_any* const value, const char* const owner) {
     for (;
          state->state.item_position->bytes[15] == tid__null || state->state.item_position->bytes[15] == tid__holder;
          state->state.item_position = &state->state.item_position[2]
     );

     *key   = state->state.item_position[0];
     *value = state->state.item_position[1];
     if (state->state.is_mut) memset_inline(state->state.item_position, 0, 32);
     else {
          ref_cnt__inc(thrd_data, *key, owner);
          ref_cnt__inc(thrd_data, *value, owner);
     }

     state->state.item_position = &state->state.item_position[2];
}

static void table__zip_init_search(const t_any* const table, t_zip_search_state* const state, t_any*, u64* const result_len, t_any* const) {
     assert(table->bytes[15] == tid__table);

     u64    const last_idx   = hash_map__get_last_idx(*table);
     u16    const chunk_size = hash_map__get_chunk_size(*table);
     u64    const items_len  = (last_idx + 1) * chunk_size * 2;

     state->hash_map_state.hash_seed  = hash_map__get_hash_seed(*table);
     state->hash_map_state.last_idx   = last_idx;
     state->hash_map_state.items_len  = items_len;
     state->hash_map_state.steps      = items_len / 2;
     state->hash_map_state.items      = table__get_kvs(*table);
     state->hash_map_state.chunk_size = chunk_size;
     state->hash_map_state.is_mut     = get_ref_cnt(*table) == 1;

     u64 const table_len = hash_map__get_len(*table);
     *result_len         = *result_len < table_len ? *result_len : table_len;
}

static bool table__zip_search(t_thrd_data* const thrd_data, t_zip_search_state* const state, t_any const key, t_any* const value, const char* const owner) {
     t_any const key_hash = hash(thrd_data, key, state->hash_map_state.hash_seed, owner);

     assert(key_hash.bytes[15] == tid__int);
     u64    const first_idx    = (key_hash.qwords[0] & state->hash_map_state.last_idx) * state->hash_map_state.chunk_size * 2;
     u64          remain_steps = state->hash_map_state.steps;
     u64          idx          = first_idx;
     t_any* const items        = state->hash_map_state.items;
     u64    const items_len    = state->hash_map_state.items_len;

     assert(idx < items_len);

     while (true) {
          t_any* const key_ptr = &items[idx];

          if (key_ptr->bytes[15] == tid__null) return false;

          t_any const eq_result = equal(thrd_data, key, *key_ptr, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) {
               *value = key_ptr[1];
               if (state->hash_map_state.is_mut) {
                    ref_cnt__dec(thrd_data, *key_ptr);
                    memset_inline(key_ptr, tid__float, 32);
               }
               else ref_cnt__inc(thrd_data, *value, owner);

               return true;
          }

          remain_steps -= 1;
          idx          += 2;
          idx           = idx == items_len ? 0 : idx;

          if (remain_steps == 0) return false;
     }
}

static void table__zip_write(t_thrd_data* const thrd_data, t_any* const table, t_any* const, bool const, t_any const key, t_any const value, const char* const owner) {
     *table = table__builtin_add_kv__own(thrd_data, *table, key, value, owner);
}
