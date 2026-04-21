#pragma once

#include "../../common/fn.h"
#include "../../common/include.h"
#include "../../common/type.h"
#include "../../common/xxh3.h"
#include "../array/basic.h"
#include "../bool/basic.h"
#include "../common/hash_map.h"
#include "../error/fn.h"
#include "basic.h"

[[clang::noinline]]
static t_any set__dup__own(t_thrd_data* const thrd_data, t_any const set, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     u64 const items_len = (hash_map__get_last_idx(set) + 1) * hash_map__get_chunk_size(set);
     u64 const mem_size  = (items_len + 1) * 16;
     t_any     new_set   = set;

     new_set.qwords[0]     = (u64)aligned_alloc(16, mem_size);
     const t_any* old_data = (const t_any*)set.qwords[0];
     t_any*       new_data = (t_any*)new_set.qwords[0];

     new_data->raw_bits = old_data->raw_bits;
     set_ref_cnt(new_set, 1);

     const t_any* old_items = &old_data[1];
     t_any*       new_items = &new_data[1];

     u64 const ref_cnt = get_ref_cnt(set);

     assert(ref_cnt != 1);

     if (ref_cnt == 0)
          memcpy(new_items, old_items, mem_size - 16);
     else {
          set_ref_cnt(set, ref_cnt - 1);

          for (u64 idx = 0; idx < items_len; idx += 1) {
               t_any const item = old_items[idx];
               new_items[idx]   = item;
               ref_cnt__inc(thrd_data, item, owner);
          }
     }

     return new_set;
}

[[gnu::hot, clang::noinline]]
static t_any equal(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner);

[[clang::always_inline]]
static t_any* set__key_ptr(t_thrd_data* const thrd_data, t_any const set, t_any const key, u64 const key_hash, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     u64    const last_idx     = hash_map__get_last_idx(set);
     u16    const chunk_size   = hash_map__get_chunk_size(set);
     t_any* const items        = set__get_items(set);
     u64    const items_len    = (last_idx + 1) * chunk_size;
     u64    const first_idx    = (key_hash & last_idx) * chunk_size;
     u64          remain_steps = items_len;

     u64 idx = first_idx;

     assert(idx < items_len);

     t_any* result = nullptr;
     while (true) {
          t_any* const key_ptr = &items[idx];
          if (key_ptr->bytes[15] == tid__null) {
               if (result == nullptr)
                    return items_len != (hash_map_max_chunks * hash_map_max_chunk_size) && items_len > 4 && hash_map__get_len(set) * 5 / 4 > items_len ? nullptr : key_ptr;

               return result;
          }

          t_any const eq_result = equal(thrd_data, key, *key_ptr, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) return key_ptr;
          if (key_ptr->bytes[15] == tid__holder && result == nullptr) result = key_ptr;

          remain_steps -= 1;
          idx          += 1;
          idx           = idx == items_len ? 0 : idx;

          if (remain_steps == 0) return result;
     }
}

[[gnu::hot, clang::noinline]]
static t_any hash(t_thrd_data* const thrd_data, t_any const arg, u64 seed, const char* const owner);

static void set__unpack__own(t_thrd_data* const thrd_data, t_any set, u64 const unpacking_items_len, t_any* const unpacking_items, const char* const owner) {
     if (set.bytes[15] != tid__set) {
          if (!(set.bytes[15] == tid__error || set.bytes[15] == tid__null)) {
               ref_cnt__dec(thrd_data, set);
               set = error__invalid_type(thrd_data, owner);
          }

          for (u64 idx = 0; idx < unpacking_items_len; idx += 1) {
               t_any const key = unpacking_items[idx];
               if (key.bytes[15] != tid__error) [[clang::likely]] {
                    ref_cnt__inc(thrd_data, set, owner);
                    ref_cnt__dec(thrd_data, key);
                    unpacking_items[idx] = set;
               }
          }

          ref_cnt__dec(thrd_data, set);
          return;
     }

     u64 const ref_cnt = get_ref_cnt(set);
     u64 const seed    = hash_map__get_hash_seed(set);
     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

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

               const t_any* const key_in_table_ptr = set__key_ptr(thrd_data, set, key, key_hash.qwords[0], owner);
               ref_cnt__dec(thrd_data, key);
               if (key_in_table_ptr == nullptr || key_in_table_ptr->bytes[15] == tid__null || key_in_table_ptr->bytes[15] == tid__holder)
                    unpacking_items[idx].bytes[15] = tid__null;
               else {
                    ref_cnt__inc(thrd_data, *key_in_table_ptr, owner);
                    unpacking_items[idx] = *key_in_table_ptr;
               }
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, set);
}

static t_any set__clear__own(t_thrd_data* const thrd_data, t_any const set, const char* const owner) {
     assert(set.bytes[15] == tid__set);

     t_any* const items   = set__get_items(set);
     u64    const ref_cnt = get_ref_cnt(set);
     if (ref_cnt == 1) {
          u64 remain_qty = hash_map__get_len(set);
          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               t_any* const item_ptr = &items[idx];
               if (item_ptr->bytes[15] == tid__null || item_ptr->bytes[15] == tid__holder) continue;

               remain_qty -= 1;

               ref_cnt__dec(thrd_data, item_ptr[0]);
               item_ptr[0].bytes[15] = tid__null;
          }

          return hash_map__set_len(set, 0);
     }

     if (ref_cnt != 0) set_ref_cnt(set, ref_cnt - 1);

     u64 const items_len = (hash_map__get_last_idx(set) + 1) * hash_map__get_chunk_size(set);
     return set__init(thrd_data, items_len, owner);
}

static t_any set__equal(t_thrd_data* const thrd_data, t_any const left,  t_any const right, const char* const owner) {
     assert(left.bytes[15] == tid__set);
     assert(right.bytes[15] == tid__set);

     u64 remain_qty = hash_map__get_len(left);

     if (remain_qty != hash_map__get_len(right)) return bool__false;
     if (remain_qty == 0) return bool__true;

     t_any small;
     t_any big;

     if ((hash_map__get_last_idx(left) + 1) * hash_map__get_chunk_size(left) < (hash_map__get_last_idx(right) + 1) * hash_map__get_chunk_size(right)) {
          small = left;
          big   = right;
     } else {
          small = right;
          big   = left;
     }

     const t_any* const small_items = set__get_items(small);
     u64          const seed        = hash_map__get_hash_seed(big);
     for (u64 idx = 0; remain_qty != 0; idx += 1) {
          t_any const item = small_items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const item_hash = hash(thrd_data, item, seed, owner);

          assert(item_hash.bytes[15] == tid__int);

          const t_any* const key_ptr = set__key_ptr(thrd_data, big, item, item_hash.qwords[0], owner);
          if (key_ptr == nullptr || key_ptr->bytes[15] == tid__null || key_ptr->bytes[15] == tid__holder) return bool__false;
     }

     return bool__true;
}

[[clang::noinline]]
static t_any set__enlarge__own(t_thrd_data* const thrd_data, t_any const set, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     u64 const ref_cnt = get_ref_cnt(set);
     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

     const t_any* const items      = set__get_items(set);
     u64                remain_qty = hash_map__get_len(set);
     t_any              new_set    = set__init(thrd_data, (hash_map__get_last_idx(set) + 1) * hash_map__get_chunk_size(set) + 1, owner);
     hash_map__set_hash_seed(new_set, hash_map__get_hash_seed(set));

     u64 const seed = hash_map__get_hash_seed(new_set);
     for (u64 idx = 0; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const item_hash = hash(thrd_data, item, seed, owner);

          assert(item_hash.bytes[15] == tid__int);

          t_any* const key_ptr = set__key_ptr(thrd_data, new_set, item, item_hash.qwords[0], owner);

          assume(key_ptr != nullptr);
          assert(key_ptr->bytes[15] == tid__null);

          *key_ptr = item;
          new_set  = hash_map__set_len(new_set, hash_map__get_len(new_set) + 1);
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, item, owner);
     }

     if (ref_cnt == 1) free((void*)set.qwords[0]);

     return new_set;
}

static inline t_any set__get_item__own(t_thrd_data* const thrd_data, t_any const set, t_any const key, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     u64   const seed     = hash_map__get_hash_seed(set);
     t_any const key_hash = hash(thrd_data, key, seed, owner);
     if (key_hash.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, set);
          ref_cnt__dec(thrd_data, key);
          return key_hash;
     }

     assert(key_hash.bytes[15] == tid__int);

     u64 const ref_cnt = get_ref_cnt(set);
     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

     const t_any* const key_ptr = set__key_ptr(thrd_data, set, key, key_hash.qwords[0], owner);
     t_any        result;
     if (key_ptr == nullptr || key_ptr->bytes[15] == tid__null || key_ptr->bytes[15] == tid__holder)
          result = null;
     else {
          result = *key_ptr;
          ref_cnt__inc(thrd_data, result, owner);
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, set);

     ref_cnt__dec(thrd_data, key);
     return result;
}

static t_any set__builtin_unsafe_add_item__own(t_thrd_data* const thrd_data, t_any set, t_any const item, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     u64   const seed      = hash_map__get_hash_seed(set);
     t_any const item_hash = hash(thrd_data, item, seed, owner);

     assume(item_hash.bytes[15] == tid__int);

     if (get_ref_cnt(set) != 1) set = set__dup__own(thrd_data, set, owner);

#ifndef NDEBUG
     bool first_time = true;
#endif
     while (true) {
          t_any* const item_ptr = set__key_ptr(thrd_data, set, item, item_hash.qwords[0], owner);
          if (item_ptr != nullptr) {
               if (!(item_ptr->bytes[15] == tid__null || item_ptr->bytes[15] == tid__holder)) [[clang::unlikely]] {
                    ref_cnt__dec__noinlined_part(thrd_data, set);
                    ref_cnt__dec(thrd_data, item);
                    return error__not_uniq_key(thrd_data, owner);
               }

               *item_ptr = item;
               set       = hash_map__set_len(set, hash_map__get_len(set) + 1);

               return set;
          }

          set = set__enlarge__own(thrd_data, set, owner);

#ifndef NDEBUG
          assert(first_time);
          first_time = false;
#endif
     }
}

static t_any set__builtin_add_item__own(t_thrd_data* const thrd_data, t_any set, t_any const item, const char* const owner) {
     if (set.bytes[15] != tid__set || item.bytes[15] == tid__error) [[clang::unlikely]] {
          if (set.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, item);
               return set;
          }
          ref_cnt__dec(thrd_data, set);

          if (item.bytes[15] == tid__error) return item;
          ref_cnt__dec(thrd_data, item);

          return error__invalid_type(thrd_data, owner);
     }

     u64   const seed      = hash_map__get_hash_seed(set);
     t_any const item_hash = hash(thrd_data, item, seed, owner);
     if (item_hash.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, set);
          ref_cnt__dec(thrd_data, item);
          return item_hash;
     }

     assert(item_hash.bytes[15] == tid__int);

     if (get_ref_cnt(set) != 1) set = set__dup__own(thrd_data, set, owner);

     #ifndef NDEBUG
     bool first_time = true;
     #endif
     while (true) {
          t_any* const item_ptr = set__key_ptr(thrd_data, set, item, item_hash.qwords[0], owner);
          if (item_ptr != nullptr) {
               if (!(item_ptr->bytes[15] == tid__null || item_ptr->bytes[15] == tid__holder)) [[clang::unlikely]] {
                    ref_cnt__dec__noinlined_part(thrd_data, set);
                    ref_cnt__dec(thrd_data, item);
                    return error__not_uniq_key(thrd_data, owner);
               }

               *item_ptr = item;
               set       = hash_map__set_len(set, hash_map__get_len(set) + 1);

               return set;
          }

          set = set__enlarge__own(thrd_data, set, owner);

          #ifndef NDEBUG
          assert(first_time);
          first_time = false;
          #endif
     }
}

static t_any set__all__own(t_thrd_data* const thrd_data, t_any const set, t_any const default_result, t_any const fn, const char* const owner) {
     assume(set.bytes[15] == tid__set);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(set);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, set);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64    const ref_cnt = get_ref_cnt(set);
     t_any* const items   = set__get_items(set);

     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

     t_any result = bool__true;
     u64   idx    = 0;
     for (; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt > 1) ref_cnt__inc(thrd_data, item, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &item, 1, owner);
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
          for (idx += 1; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, item);
          }

          free((void*)set.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any set__any__own(t_thrd_data* const thrd_data, t_any const set, t_any const default_result, t_any const fn, const char* const owner) {
     assume(set.bytes[15] == tid__set);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(set);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, set);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64    const ref_cnt = get_ref_cnt(set);
     t_any* const items   = set__get_items(set);

     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

     t_any result = bool__false;
     u64   idx    = 0;
     for (; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt > 1) ref_cnt__inc(thrd_data, item, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &item, 1, owner);
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
          for (idx += 1; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, item);
          }

          free((void*)set.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static inline t_any set__append__own(t_thrd_data* const thrd_data, t_any set, t_any const item, const char* const owner) {
     assume(set.bytes[15] == tid__set);
     assert(type_is_common(item.bytes[15]));

     u64   const seed      = hash_map__get_hash_seed(set);
     t_any const item_hash = hash(thrd_data, item, seed, owner);
     if (item_hash.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, set);
          ref_cnt__dec(thrd_data, item);
          return item_hash;
     }

     assert(item_hash.bytes[15] == tid__int);

     if (get_ref_cnt(set) != 1) set = set__dup__own(thrd_data, set, owner);

     #ifndef NDEBUG
     bool first_time = true;
     #endif
     while (true) {
          t_any* const item_ptr = set__key_ptr(thrd_data, set, item, item_hash.qwords[0], owner);
          if (item_ptr != nullptr) {
               if (item_ptr->bytes[15] == tid__null || item_ptr->bytes[15] == tid__holder) {
                    *item_ptr = item;
                    set       = hash_map__set_len(set, hash_map__get_len(set) + 1);
               } else {
                    ref_cnt__dec(thrd_data, *item_ptr);
                    *item_ptr = item;
               }

               return set;
          }

          set = set__enlarge__own(thrd_data, set, owner);

          #ifndef NDEBUG
          assert(first_time);
          first_time = false;
          #endif
     }
}

static inline t_any set__append_new__own(t_thrd_data* const thrd_data, t_any set, t_any const item, const char* const owner) {
     assume(set.bytes[15] == tid__set);
     assert(type_is_common(item.bytes[15]));

     u64   const seed      = hash_map__get_hash_seed(set);
     t_any const item_hash = hash(thrd_data, item, seed, owner);
     if (item_hash.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, set);
          ref_cnt__dec(thrd_data, item);
          return item_hash;
     }

     assert(item_hash.bytes[15] == tid__int);

     if (get_ref_cnt(set) != 1) set = set__dup__own(thrd_data, set, owner);

     #ifndef NDEBUG
     bool first_time = true;
     #endif
     while (true) {
          t_any* const item_ptr = set__key_ptr(thrd_data, set, item, item_hash.qwords[0], owner);
          if (item_ptr != nullptr) {
               if (!(item_ptr->bytes[15] == tid__null || item_ptr->bytes[15] == tid__holder)) {
                    ref_cnt__dec(thrd_data, set);
                    ref_cnt__dec(thrd_data, item);
                    return null;
               }

               *item_ptr = item;
               set       = hash_map__set_len(set, hash_map__get_len(set) + 1);

               return set;
          }

          set = set__enlarge__own(thrd_data, set, owner);

          #ifndef NDEBUG
          assert(first_time);
          first_time = false;
          #endif
     }
}

[[clang::noinline]]
static t_any set__fit__own(t_thrd_data* const thrd_data, t_any const set, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     t_any* const items      = set__get_items(set);
     u64          remain_qty = hash_map__get_len(set);

     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, set);
          return empty_set;
     }

     u64 const ref_cnt = get_ref_cnt(set);
     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

     t_any       new_set = set__init(thrd_data, remain_qty, owner);
     u64   const seed    = hash_map__get_hash_seed(new_set);
     for (u64 idx = 0; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const item_hash = hash(thrd_data, item, seed, owner);

          assert(item_hash.bytes[15] == tid__int);

          t_any* const key_ptr = set__key_ptr(thrd_data, new_set, item, item_hash.qwords[0], owner);

          assume(key_ptr != nullptr);
          assert(key_ptr->bytes[15] == tid__null);

          *key_ptr = item;
          new_set  = hash_map__set_len(new_set, hash_map__get_len(new_set) + 1);
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, item, owner);
     }

     if (ref_cnt == 1) free((void*)set.qwords[0]);

     return new_set;
}

static t_any set__filter__own(t_thrd_data* const thrd_data, t_any set, t_any const fn, const char* const owner) {
     assume(set.bytes[15] == tid__set);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64          remain_qty = hash_map__get_len(set);
     t_any*       items      = set__get_items(set);
     u64    const len        = remain_qty;
     u64          new_len    = len;
     bool         dst_is_mut = get_ref_cnt(set) == 1;
     for (u64 idx = 0; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty -= 1;
          ref_cnt__inc(thrd_data, item, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &item, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_is_mut) set = hash_map__set_len(set, new_len);
               ref_cnt__dec(thrd_data, set);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) {
               if (!dst_is_mut) {
                    set        = set__dup__own(thrd_data, set, owner);
                    items      = set__get_items(set);
                    dst_is_mut = true;
               }

               t_any* const item_ptr = &items[idx];
               ref_cnt__dec(thrd_data, *item_ptr);
               item_ptr[0].bytes[15] = tid__holder;
               new_len              -= 1;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (new_len == len) return set;
     set = hash_map__set_len(set, new_len);

     if (new_len == 0) {
          ref_cnt__dec(thrd_data, set);
          return empty_set;
     }

     if (set__need_to_fit(set)) set = set__fit__own(thrd_data, set, owner);
     return set;
}

static t_any set__foldl__own(t_thrd_data* const thrd_data, t_any const set, t_any const state, t_any const fn, const char* const owner) {
     assume(set.bytes[15] == tid__set);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64       remain_qty = hash_map__get_len(set);
     u64 const ref_cnt    = get_ref_cnt(set);
     if (ref_cnt > 1 && state.qwords[0] != set.qwords[0]) set_ref_cnt(set, ref_cnt - 1);

     t_any* const items = set__get_items(set);
     t_any        fn_args[2];
     fn_args[0]         = state;
     u64          idx   = 0;
     for (; remain_qty != 0; idx += 1) {
          fn_args[1] = items[idx];
          if (fn_args[1].bytes[15] == tid__null || fn_args[1].bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 1; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, item);
          }

          free((void*)set.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == set.qwords[0]) ref_cnt__dec(thrd_data, set);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any set__foldl1__own(t_thrd_data* const thrd_data, t_any const set, t_any const fn, const char* const owner) {
     assume(set.bytes[15] == tid__set);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64 remain_qty = hash_map__get_len(set);

     if (remain_qty == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, set);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const ref_cnt = get_ref_cnt(set);
     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

     t_any* const items   = set__get_items(set);
     t_any        fn_args[2];

     u64 idx = 0;
     for (;; idx += 1) {
          fn_args[0] = items[idx];
          if (fn_args[0].bytes[15] == tid__null || fn_args[0].bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[0], owner);
          break;
     }

     for (idx += 1; remain_qty != 0; idx += 1) {
          fn_args[1] = items[idx];
          if (fn_args[1].bytes[15] == tid__null || fn_args[1].bytes[15] == tid__holder) continue;

          remain_qty -= 1;

          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 1; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, item);
          }

          free((void*)set.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any set__hash(t_thrd_data* const thrd_data, t_any const set, u64 const seed, const char* const owner) {
     assert(set.bytes[15] == tid__set);

     u64 remain_qty = hash_map__get_len(set);

     if (remain_qty == 0) return (const t_any){.structure = {.value = xxh3_get_hash(nullptr, 0, seed ^ set.bytes[15]), .type = tid__int}};

     const t_any* const items    = set__get_items(set);
     u64                set_hash = 0;
     for (u64 idx = 0; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const item_hash = hash(thrd_data, item, seed, owner);
          if (item_hash.bytes[15] == tid__error) [[clang::unlikely]] return item_hash;

          assert(item_hash.bytes[15] == tid__int);

          set_hash ^= item_hash.qwords[0];
     }

     return (const t_any){.structure = {.value = set_hash, .type = tid__int}};
}

static t_any set__look_other_in__own(t_thrd_data* const thrd_data, t_any const set, t_any const not_looked_item, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     u64 remain_qty = hash_map__get_len(set);

     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, set);
          ref_cnt__dec(thrd_data, not_looked_item);
          return null;
     }

     u64 const ref_cnt = get_ref_cnt(set);
     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

     t_any* const items  = set__get_items(set);
     t_any        result = null;
     for (u64 idx = 0; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const eq_result = equal(thrd_data, item, not_looked_item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) {
               ref_cnt__inc(thrd_data, item, owner);
               result = item;
               break;
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, set);
     ref_cnt__dec(thrd_data, not_looked_item);

     return result;
}

[[clang::noinline]]
static t_any common_print(t_thrd_data* const thrd_data, t_any const arg, const char* const owner);

static t_any set__print(t_thrd_data* const thrd_data, t_any const set, const char* const owner) {
     assert(set.bytes[15] == tid__set);

     u64 remain_qty = hash_map__get_len(set);
     if (remain_qty == 0) {
          if (fwrite("[set]", 1, 5, stdout) != 5) [[clang::unlikely]] goto cant_print_label;
          return null;
     }

     if (fwrite("[set ", 1, 5, stdout) != 5) [[clang::unlikely]] goto cant_print_label;

     const t_any* const items = set__get_items(set);
     for (u64 idx = 0; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty                   -= 1;
          t_any const print_item_result = common_print(thrd_data, item, owner);
          if (print_item_result.bytes[15] == tid__error) [[clang::unlikely]] return print_item_result;

          const char* str;
          int         str_len;
          if (remain_qty == 0) {
               str     = "]";
               str_len = 1;
          } else {
               str     = ", ";
               str_len = 2;
          }

          if (fwrite(str, 1, str_len, stdout) != str_len) [[clang::unlikely]] goto cant_print_label;
     }

     return null;

     cant_print_label:
     return error__cant_print(thrd_data, owner);
}

static t_any set__reserve__own(t_thrd_data* const thrd_data, t_any set, u64 const new_items_qty, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     u64 const len     = hash_map__get_len(set);
     u64 const new_cap = len + new_items_qty;
     if (new_cap > hash_map_max_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, set);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const current_cap = (hash_map__get_last_idx(set) + 1) * hash_map__get_chunk_size(set);
     if (current_cap >= new_cap) return set;

     u64 const ref_cnt = get_ref_cnt(set);
     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

     const t_any* const items      = set__get_items(set);
     u64                remain_qty = len;
     t_any              new_set    = set__init(thrd_data, new_cap, owner);
     hash_map__set_hash_seed(new_set, hash_map__get_hash_seed(set));

     u64 const seed = hash_map__get_hash_seed(new_set);
     for (u64 idx = 0; remain_qty != 0; idx += 1) {
          t_any const item = items[idx];
          if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

          remain_qty           -= 1;
          t_any const item_hash = hash(thrd_data, item, seed, owner);

          assert(item_hash.bytes[15] == tid__int);

          t_any* const key_ptr = set__key_ptr(thrd_data, new_set, item, item_hash.qwords[0], owner);

          assume(key_ptr != nullptr);
          assert(key_ptr->bytes[15] == tid__null);

          *key_ptr = item;
          new_set  = hash_map__set_len(new_set, hash_map__get_len(new_set) + 1);
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, item, owner);
     }

     if (ref_cnt == 1) free((void*)set.qwords[0]);

     return new_set;
}

static inline t_any set__take_one__own(t_thrd_data* const thrd_data, t_any const set, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     if (hash_map__get_len(set) == 0) {
          ref_cnt__dec(thrd_data, set);
          return null;
     }

     t_any*       items   = set__get_items(set);
     u64    const ref_cnt = get_ref_cnt(set);
     if (ref_cnt > 1) set_ref_cnt(set, ref_cnt - 1);

     while (true) {
          if (!(items->bytes[15] == tid__null || items->bytes[15] == tid__holder)) {
               t_any const result = *items;
               ref_cnt__inc(thrd_data, result, owner);
               if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, set);
               return result;
          }

          items = &items[1];
     }
}

static t_any set__to_array__own(t_thrd_data* const thrd_data, t_any const set, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     u64 const len = hash_map__get_len(set);
     if (len == 0) {
          ref_cnt__dec(thrd_data, set);
          return empty_array;
     }

     t_any        array       = array__init(thrd_data, len, owner);
     array                    = slice__set_metadata(array, 0, len <= slice_max_len ? len : slice_max_len + 1);
     slice_array__set_len(array, len);
     t_any* const array_items = slice_array__get_items(array);
     u64    const ref_cnt     = get_ref_cnt(set);
     u64          remain_qty  = len;
     t_any* const set_items   = set__get_items(set);
     if (ref_cnt > 1) {
          set_ref_cnt(set, ref_cnt - 1);

          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               if (set_items[idx].bytes[15] == tid__null || set_items[idx].bytes[15] == tid__holder) continue;

               t_any const item = set_items[idx];
               ref_cnt__inc(thrd_data, item, owner);
               array_items[len - remain_qty] = item;
               remain_qty                   -= 1;
          }
     } else {
          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               if (set_items[idx].bytes[15] == tid__null || set_items[idx].bytes[15] == tid__holder) continue;

               array_items[len - remain_qty] = set_items[idx];
               remain_qty                   -= 1;
          }

          if (ref_cnt == 1) free((void*)set.qwords[0]);
     }

     return array;
}

static t_any set__to_box__own(t_thrd_data* const thrd_data, t_any const set, const char* const owner) {
     assume(set.bytes[15] == tid__set);

     u64 const len = hash_map__get_len(set);
     if (len == 0) {
          ref_cnt__dec(thrd_data, set);
          return empty_box;
     }

     if (len > 16) {
          ref_cnt__dec(thrd_data, set);
          return error__out_of_bounds(thrd_data, owner);
     }

     t_any        box        = box__new(thrd_data, len, owner);
     t_any* const box_items  = box__get_items(box);
     u64          remain_qty = len;
     u64    const ref_cnt    = get_ref_cnt(set);
     t_any* const set_items  = set__get_items(set);
     if (ref_cnt > 1) {
          set_ref_cnt(set, ref_cnt - 1);

          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               if (set_items[idx].bytes[15] == tid__null || set_items[idx].bytes[15] == tid__holder) continue;

               t_any const item = set_items[idx];
               ref_cnt__inc(thrd_data, item, owner);
               box_items[len - remain_qty] = item;
               remain_qty                 -= 1;
          }
     } else {
          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               if (set_items[idx].bytes[15] == tid__null || set_items[idx].bytes[15] == tid__holder) continue;

               box_items[len - remain_qty] = set_items[idx];
               remain_qty                 -= 1;
          }

          if (ref_cnt == 1) free((void*)set.qwords[0]);
     }

     return box;
}

static void set__zip_init_read(const t_any* const set, t_zip_read_state* const state, t_any*, t_any*, u64* const result_len, t_any* const) {
     assert(set->bytes[15] == tid__set);

     *result_len                = hash_map__get_len(*set);
     state->state.item_position = set__get_items(*set);
     state->state.is_mut        = get_ref_cnt(*set) == 1;
}

static void set__zip_read(t_thrd_data* const thrd_data, t_zip_read_state* const state, t_any* const key, t_any* const value, const char* const owner) {
     for (;
          state->state.item_position->bytes[15] == tid__null || state->state.item_position->bytes[15] == tid__holder;
          state->state.item_position = &state->state.item_position[1]
     );

     *key   = state->state.item_position[0];
     *value = *key;
     if (state->state.is_mut) {
          state->state.item_position->raw_bits = 0;
          ref_cnt__inc(thrd_data, *key, owner);
     } else ref_cnt__add(thrd_data, *key, 2, owner);

     state->state.item_position = &state->state.item_position[1];
}

static void set__zip_init_search(const t_any* const set, t_zip_search_state* const state, t_any*, u64* const result_len, t_any* const) {
     assert(set->bytes[15] == tid__set);

     u64 const last_idx   = hash_map__get_last_idx(*set);
     u16 const chunk_size = hash_map__get_chunk_size(*set);
     u64 const items_len  = (last_idx + 1) * chunk_size;

     state->hash_map_state.hash_seed  = hash_map__get_hash_seed(*set);
     state->hash_map_state.last_idx   = last_idx;
     state->hash_map_state.items_len  = items_len;
     state->hash_map_state.steps      = items_len;
     state->hash_map_state.items      = set__get_items(*set);
     state->hash_map_state.chunk_size = chunk_size;
     state->hash_map_state.is_mut     = get_ref_cnt(*set) == 1;

     u64 const set_len = hash_map__get_len(*set);
     *result_len       = *result_len < set_len ? *result_len : set_len;
}

static bool set__zip_search(t_thrd_data* const thrd_data, t_zip_search_state* const state, t_any const key, t_any* const value, const char* const owner) {
     t_any const key_hash = hash(thrd_data, key, state->hash_map_state.hash_seed, owner);

     assert(key_hash.bytes[15] == tid__int);

     u64    const first_idx    = (key_hash.qwords[0] & state->hash_map_state.last_idx) * state->hash_map_state.chunk_size;
     u64          remain_steps = state->hash_map_state.steps;
     u64          idx          = first_idx;
     t_any* const items        = state->hash_map_state.items;
     u64    const items_len    = state->hash_map_state.items_len;

     assert(idx < items_len);

     while (true) {
          t_any* const item_ptr = &items[idx];

          if (item_ptr->bytes[15] == tid__null) return false;

          t_any const eq_result = equal(thrd_data, key, *item_ptr, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) {
               *value = *item_ptr;
               if (state->hash_map_state.is_mut) memset_inline(item_ptr, tid__float, 16);
               else ref_cnt__inc(thrd_data, *value, owner);

               return true;
          }

          remain_steps -= 1;
          idx          += 1;
          idx           = idx == items_len ? 0 : idx;

          if (remain_steps == 0) return false;
     }
}
