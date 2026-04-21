#pragma once

#include "../../common/bool_vectors.h"
#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../common/xxh3.h"
#include "../common/slice.h"
#include "../error/fn.h"
#include "../token/basic.h"
#include "basic.h"

static void array__unpack__own(t_thrd_data* const thrd_data, t_any array, u64 const unpacking_items_len, u64 const max_array_item_idx, t_any* const unpacking_items, const char* const owner) {
     if (array.bytes[15] != tid__array) {
          if (!(array.bytes[15] == tid__error || array.bytes[15] == tid__null)) {
               ref_cnt__dec(thrd_data, array);
               array = error__invalid_type(thrd_data, owner);
          }

          ref_cnt__add(thrd_data, array, unpacking_items_len - 1, owner);
          for (u64 idx = 0; idx < unpacking_items_len; unpacking_items[idx] = array, idx += 1);
          return;
     }
     u64 const ref_cnt = get_ref_cnt(array);
     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     u32          const slice_offset = slice__get_offset(array);
     u32          const slice_len    = slice__get_len(array);
     const t_any* const array_items  = &((const t_any*)slice_array__get_items(array))[slice_offset];
     u64          const array_len    = slice_array__get_len(array);
     u64          const len          = slice_len <= slice_max_len ? slice_len : array_len;

     if (len < max_array_item_idx) [[clang::unlikely]] {
          if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);

          t_any const error = error__unpacking_not_enough_items(thrd_data, owner);
          set_ref_cnt(error, unpacking_items_len);

          for (u64 idx = 0; idx < unpacking_items_len; unpacking_items[idx] = error, idx += 1);
          return;
     }

     for (u64 unpacking_item_idx = 0; unpacking_item_idx < unpacking_items_len; unpacking_item_idx += 1) {
          u64   const array_item_idx          = unpacking_items[unpacking_item_idx].bytes[0];
          t_any const item                    = array_items[array_item_idx];
          unpacking_items[unpacking_item_idx] = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
}

static t_any array__dup__own(t_thrd_data* const thrd_data, t_any const array, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32          const slice_offset = slice__get_offset(array);
     u32          const slice_len    = slice__get_len(array);
     const t_any* const old_items    = &((const t_any*)slice_array__get_items(array))[slice_offset];
     u64          const ref_cnt      = get_ref_cnt(array);
     u64          const array_len    = slice_array__get_len(array);
     u64          const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(ref_cnt != 1);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any        new_array = array__init(thrd_data, len == 0 ? 1 : len, owner);
     slice_array__set_len(new_array, len);
     new_array              = slice__set_metadata(new_array, 0, slice_len);
     t_any* const new_items = slice_array__get_items(new_array);
     for (u64 idx = 0; idx < len; idx += 1) {
          t_any const item = old_items[idx];
          new_items[idx]   = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     return new_array;
}

static t_any array__concat__own(t_thrd_data* const thrd_data, t_any left, t_any right, const char* const owner) {
     assume(left.bytes[15] == tid__array);
     assume(right.bytes[15] == tid__array);

     u32 const right_slice_len = slice__get_len(right);
     u64 const right_array_len = slice_array__get_len(right);
     u64 const right_ref_cnt   = get_ref_cnt(right);
     u64 const right_len       = right_slice_len <= slice_max_len ? right_slice_len : right_array_len;

     if (right_len == 0) {
          ref_cnt__dec(thrd_data, right);
          return left;
     }

     u32    const right_slice_offset = slice__get_offset(right);
     u64    const right_array_cap    = slice_array__get_cap(right);
     t_any*       right_items        = slice_array__get_items(right);

     assert(right_slice_len <= slice_max_len || right_slice_offset == 0);
     assert(right_array_cap >= right_array_len);

     u32 const left_slice_len = slice__get_len(left);
     u64 const left_array_len = slice_array__get_len(left);
     u64 const left_ref_cnt   = get_ref_cnt(left);
     u64 const left_len       = left_slice_len <= slice_max_len ? left_slice_len : left_array_len;

     if (left_len == 0) {
          ref_cnt__dec(thrd_data, left);
          return right;
     }

     u32    const left_slice_offset = slice__get_offset(left);
     u64    const left_array_cap    = slice_array__get_cap(left);
     t_any*       left_items        = slice_array__get_items(left);

     assert(left_slice_len <= slice_max_len || left_slice_offset == 0);
     assert(left_array_cap >= left_array_len);

     u64 const new_len = left_len + right_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     if (left_items == right_items) {
          if (left_ref_cnt > 2) set_ref_cnt(left, left_ref_cnt - 2);
     } else {
          if (left_ref_cnt > 1) set_ref_cnt(left, left_ref_cnt - 1);
          if (right_ref_cnt > 1) set_ref_cnt(right, right_ref_cnt - 1);
     }

     int const left_is_mut  = left_ref_cnt == 1;
     int const right_is_mut = right_ref_cnt == 1;

     u8 const scenario =
          left_is_mut                                      |
          right_is_mut * 2                                 |
          (left_is_mut  && left_array_cap  >= new_len) * 4 |
          (right_is_mut && right_array_cap >= new_len) * 8 ;

     switch (scenario) {
     case 1: case 3:
          left.qwords[0] = (u64)aligned_realloc(16, (t_any*)left.qwords[0], (new_cap + 1) * 16);
          slice_array__set_cap(left, new_cap);
          left_items     = slice_array__get_items(left);
     case 5: case 7: case 15:
          slice_array__set_len(left, new_len);
          left = slice__set_metadata(left, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (left_slice_offset != 0) {
               for (u32 idx = 0; idx < left_slice_offset; ref_cnt__dec(thrd_data, left_items[idx]), idx += 1);
               memmove(left_items, &left_items[left_slice_offset], left_len * 16);
          }

          for (u64 idx = left_slice_offset + left_len; idx < left_array_len; ref_cnt__dec(thrd_data, left_items[idx]), idx += 1);

          if (right_is_mut)
               for (u32 idx = 0; idx < right_slice_offset; ref_cnt__dec(thrd_data, right_items[idx]), idx += 1);

          const t_any* const right_items_from_offset   = &right_items[right_slice_offset];
          t_any*       const right_items_in_left_array = &left_items[left_len];
          for (u64 idx = 0; idx < right_len; idx += 1) {
               t_any const right_item         = right_items_from_offset[idx];
               right_items_in_left_array[idx] = right_item;
               if (right_ref_cnt > 1) ref_cnt__inc(thrd_data, right_item, owner);
          }

          if (right_is_mut) {
               for (u64 idx = right_slice_offset + right_len; idx < right_array_len; ref_cnt__dec(thrd_data, right_items[idx]), idx += 1);

               free((void*)right.qwords[0]);
          }

          return left;
     case 2:
          right.qwords[0] = (u64)aligned_realloc(16, (t_any*)right.qwords[0], (new_cap + 1) * 16);
          slice_array__set_cap(right, new_cap);
          right_items     = slice_array__get_items(right);
     case 10: case 11:
          slice_array__set_len(right, new_len);
          right = slice__set_metadata(right, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          for (u32 idx = 0; idx < right_slice_offset; ref_cnt__dec(thrd_data, right_items[idx]), idx += 1);
          for (u64 idx = right_slice_offset + right_len; idx < right_array_len; ref_cnt__dec(thrd_data, right_items[idx]), idx += 1);

          if (right_slice_offset != left_len) memmove(&right_items[left_len], &right_items[right_slice_offset], right_len * 16);

          if (left_is_mut)
               for (u32 idx = 0; idx < left_slice_offset; ref_cnt__dec(thrd_data, left_items[idx]), idx += 1);

          const t_any* const left_items_from_offset = &left_items[left_slice_offset];
          for (u64 idx = 0; idx < left_len; idx += 1) {
               t_any const left_item = left_items_from_offset[idx];
               right_items[idx]      = left_item;
               if (left_ref_cnt > 1) ref_cnt__inc(thrd_data, left_item, owner);
          }

          if (left_is_mut) {
               for (u64 idx = left_slice_offset + left_len; idx < left_array_len; ref_cnt__dec(thrd_data, left_items[idx]), idx += 1);

               free((void*)left.qwords[0]);
          }

          return right;
     case 0: {
          t_any        new_array = array__init(thrd_data, new_len, owner);
          slice_array__set_len(new_array, new_len);
          new_array              = slice__set_metadata(new_array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          t_any* const new_items = slice_array__get_items(new_array);

          if (left_is_mut)
               for (u32 idx = 0; idx < left_slice_offset; ref_cnt__dec(thrd_data, left_items[idx]), idx += 1);

          const t_any* const left_items_from_offset = &left_items[left_slice_offset];
          for (u64 idx = 0; idx < left_len; idx += 1) {
               t_any const left_item = left_items_from_offset[idx];
               new_items[idx]        = left_item;
               if (left_ref_cnt > 1) ref_cnt__inc(thrd_data, left_item, owner);
          }

          if (left_is_mut) {
               for (u64 idx = left_slice_offset + left_len; idx < left_array_len; ref_cnt__dec(thrd_data, left_items[idx]), idx += 1);

               free((void*)left.qwords[0]);
          }

          if (right_is_mut)
               for (u32 idx = 0; idx < right_slice_offset; ref_cnt__dec(thrd_data, right_items[idx]), idx += 1);

          const t_any* const right_items_from_offset  = &right_items[right_slice_offset];
          t_any*       const right_items_in_new_array = &new_items[left_len];
          for (u64 idx = 0; idx < right_len; idx += 1) {
               t_any const right_item        = right_items_from_offset[idx];
               right_items_in_new_array[idx] = right_item;
               if (right_ref_cnt > 1) ref_cnt__inc(thrd_data, right_item, owner);
          }

          if (right_is_mut) {
               for (u64 idx = right_slice_offset + right_len; idx < right_array_len; ref_cnt__dec(thrd_data, right_items[idx]), idx += 1);

               free((void*)right.qwords[0]);
          } else if (right_ref_cnt == 2 && left_items == right_items) ref_cnt__dec__noinlined_part(thrd_data, right);

          return new_array;
     }
     default:
          unreachable;
     }
}

static inline t_any array__get_item__own(t_thrd_data* const thrd_data, t_any const array, t_any const idx, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assert(idx.bytes[15] == tid__int);

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (idx.qwords[0] >= len) [[clang::unlikely]] {
          if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt == 1) {
          const t_any* const items             = slice_array__get_items(array);
          u64          const item_idx_in_array = slice_offset + idx.qwords[0];
          for (u64 array_idx = 0; array_idx < item_idx_in_array; ref_cnt__dec(thrd_data, items[array_idx]), array_idx += 1);

          t_any const item = items[item_idx_in_array];

          for (u64 array_idx = item_idx_in_array + 1; array_idx < array_len; ref_cnt__dec(thrd_data, items[array_idx]), array_idx += 1);
          free((void*)array.qwords[0]);

          return item;
     }

     t_any const item = ((const t_any*)slice_array__get_items(array))[slice_offset + idx.qwords[0]];
     ref_cnt__inc(thrd_data, item, owner);

     return item;
}

static inline t_any array__slice__own(t_thrd_data* const thrd_data, t_any array, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(idx_from <= idx_to);
     assume(array.bytes[15] == tid__array);

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (idx_to > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const new_len = idx_to - idx_from;

     if (new_len == 0) {
          ref_cnt__dec(thrd_data, array);
          return empty_array;
     }

     if (new_len == len) return array;

     u64 const from_offset = slice_offset + idx_from;
     if (new_len <= slice_max_len && from_offset <= slice_max_offset) [[clang::likely]] {
          array = slice__set_metadata(array, from_offset, new_len);
          return array;
     }

     t_any* const items   = slice_array__get_items(array);
     u64    const ref_cnt = get_ref_cnt(array);
     if (ref_cnt == 1) {
          u64 const array_len = slice_array__get_len(array);
          if (from_offset != 0) {
               for (u32 idx = 0; idx < from_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
               memmove(items, &items[from_offset], new_len * 16);
          }
          for (u64 idx = from_offset + new_len; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

          slice_array__set_len(array, new_len);
          slice_array__set_cap(array, new_len);
          array           = slice__set_metadata(array, 0, slice_len);
          array.qwords[0] = (u64)aligned_realloc(16, (t_any*)array.qwords[0], (new_len + 1) * 16);
          return array;
     }

     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any        new_array = array__init(thrd_data, new_len, owner);
     t_any* const new_items = slice_array__get_items(new_array);
     new_array              = slice__set_metadata(new_array, 0, slice_len);
     slice_array__set_len(new_array, new_len);
     for (u64 idx = 0; idx < new_len; idx += 1) {
          t_any const item = items[from_offset + idx];
          new_items[idx]   = item;
          ref_cnt__inc(thrd_data, item, owner);
     }
     return new_array;
}

static t_any array__all__own(t_thrd_data* const thrd_data, t_any const array, t_any const default_result, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u32          const slice_offset      = slice__get_offset(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt == 1)
          for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     else if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any result = bool__true;
     u64   idx    = 0;
     for (; idx < len; idx += 1) {
          t_any const item = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, item, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &item, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               idx += 1;
               break;
          }

          if (fn_result.bytes[0] == 0) {
               result = bool__false;
               idx   += 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          free((void*)array.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any array__all_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const default_result, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u32          const slice_offset      = slice__get_offset(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt == 1)
          for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     else if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0]   = (const t_any){.bytes = {[15] = tid__int}};
     t_any result = bool__true;
     u64   idx    = 0;
     for (; idx < len; idx += 1) {
          fn_args[0].qwords[0] = idx;
          fn_args[1]           = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               idx += 1;
               break;
          }

          if (fn_result.bytes[0] == 0) {
               result = bool__false;
               idx   += 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          free((void*)array.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any array__any__own(t_thrd_data* const thrd_data, t_any const array, t_any const default_result, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u32          const slice_offset      = slice__get_offset(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt == 1)
          for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     else if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any result = bool__false;
     u64   idx    = 0;
     for (; idx < len; idx += 1) {
          t_any const item = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, item, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &item, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               idx += 1;
               break;
          }

          if (fn_result.bytes[0] == 1) {
               result = bool__true;
               idx   += 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          free((void*)array.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static t_any array__any_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const default_result, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u32          const slice_offset      = slice__get_offset(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt == 1)
          for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     else if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0]   = (const t_any){.bytes = {[15] = tid__int}};
     t_any result = bool__false;
     u64   idx    = 0;
     for (; idx < len; idx += 1) {
          fn_args[0].qwords[0] = idx;
          fn_args[1]           = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               idx += 1;
               break;
          }

          if (fn_result.bytes[0] == 1) {
               result = bool__true;
               idx += 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          free((void*)array.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

static inline t_any array__append__own(t_thrd_data* const thrd_data, t_any const array, t_any const new_item, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assert(type_is_common(new_item.bytes[15]));

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;
     t_any     new_array    = slice__set_metadata(array, 0, len < slice_max_len ? len + 1 : slice_max_len + 1);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (len == array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);

     u64 new_cap = (len + 1) * 3 / 2;
     new_cap     = new_cap < 8 ? 7 : (new_cap <= array_max_len ? new_cap : len + 1);

     if (ref_cnt == 1) {
          u64 const array_cap = slice_array__get_cap(array);
          if (array_cap <= len) {
               new_array.qwords[0] = (u64)aligned_realloc(16, (t_any*)new_array.qwords[0], (new_cap + 1) * 16);
               slice_array__set_cap(new_array, new_cap);
          }
          slice_array__set_len(new_array, len + 1);

          t_any* const items = slice_array__get_items(new_array);

          assume_aligned(items, 16);

          if (slice_offset != 0) {
               for (u64 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
               memmove(items, &items[slice_offset], len * 16);
          }

          if (slice_offset + len < array_len) ref_cnt__dec(thrd_data, items[slice_offset + len]);

          items[len] = new_item;

          for (u64 idx = slice_offset + len + 1; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

          return new_array;
     }

     const t_any* const items = &((const t_any*)slice_array__get_items(array))[slice_offset];
     new_array.qwords[0]      = (u64)aligned_alloc(16, (len + 2) * 16);
     set_ref_cnt(new_array, 1);

     slice_array__set_len(new_array, len + 1);
     slice_array__set_cap(new_array, len + 1);
     t_any* const new_items = slice_array__get_items(new_array);

     assume_aligned(items, 16);
     assume_aligned(new_items, 16);

     if (ref_cnt == 0) memcpy(new_items, items, len * 16);
     else {
          set_ref_cnt(array, ref_cnt - 1);

          for (u64 idx = 0; idx < len; idx += 1) {
               t_any const item = items[idx];
               new_items[idx]   = item;
               ref_cnt__inc(thrd_data, item, owner);
          }
     }

     new_items[len] = new_item;
     return new_array;
}

static t_any array__clear__own(t_thrd_data* const thrd_data, t_any array, const char* const owner) {
     assert(array.bytes[15] == tid__array);

     u64 const cap     = slice_array__get_cap(array);
     u64 const ref_cnt = get_ref_cnt(array);

     if (ref_cnt == 1) {
          u64 const array_len = slice_array__get_len(array);
          slice_array__set_len(array, 0);
          array                    = slice__set_metadata(array, 0, 0);
          const t_any* const items = slice_array__get_items(array);

          for (u64 idx = 0; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

          return array;
     }

     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     return array__init(thrd_data, cap, owner);
}

[[gnu::hot, clang::noinline]]
static t_any cmp(t_thrd_data* const thrd_data, t_any const arg_1, t_any const arg_2, const char* const owner);

static t_any array__cmp(t_thrd_data* const thrd_data, t_any const array_1, t_any const array_2, const char* const owner) {
     assert(array_1.bytes[15] == tid__array);
     assert(array_2.bytes[15] == tid__array);

     u32          const slice_1_offset = slice__get_offset(array_1);
     u32          const slice_1_len    = slice__get_len(array_1);
     const t_any* const items_1        = &((const t_any*)slice_array__get_items(array_1))[slice_1_offset];
     u64          const len_1          = slice_1_len <= slice_max_len ? slice_1_len : slice_array__get_len(array_1);

     u32          const slice_2_offset = slice__get_offset(array_2);
     u32          const slice_2_len    = slice__get_len(array_2);
     const t_any* const items_2        = &((const t_any*)slice_array__get_items(array_2))[slice_2_offset];
     u64          const len_2          = slice_2_len <= slice_max_len ? slice_2_len : slice_array__get_len(array_2);

     assert(slice_1_len <= slice_max_len || slice_1_offset == 0);
     assert(slice_2_len <= slice_max_len || slice_2_offset == 0);

     u64 const small_len = len_1 < len_2 ? len_1 : len_2;
     for (u64 idx = 0; idx < small_len; idx += 1) {
          t_any const item_cmp_result = cmp(thrd_data, items_1[idx], items_2[idx], owner);
          if (item_cmp_result.bytes[0] != equal_id) return item_cmp_result;
     }
     return len_1 < len_2 ? tkn__less : (len_2 < len_1 ? tkn__great : tkn__equal);
}

[[gnu::hot, clang::noinline]]
static t_any equal(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner);

static t_any array__equal(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner) {
     assert(left.bytes[15] == tid__array);
     assert(right.bytes[15] == tid__array);

     u32 const left_slice_len = slice__get_len(left);
     u64 const left_len       = left_slice_len <= slice_max_len ? left_slice_len : slice_array__get_len(left);

     u32 const right_slice_len = slice__get_len(right);
     u64 const right_len       = right_slice_len <= slice_max_len ? right_slice_len : slice_array__get_len(right);

     if (left_len != right_len) return bool__false;

     u32          const left_slice_offset  = slice__get_offset(left);
     const t_any* const left_items         = &((const t_any*)slice_array__get_items(left))[left_slice_offset];
     u32          const right_slice_offset = slice__get_offset(right);
     const t_any* const right_items        = &((const t_any*)slice_array__get_items(right))[right_slice_offset];

     assert(left_slice_len <= slice_max_len || left_slice_offset == 0);
     assert(right_slice_len <= slice_max_len || right_slice_offset == 0);

     for (u64 idx = 0; idx < left_len; idx += 1) {
          t_any const item_eq_result = equal(thrd_data, left_items[idx], right_items[idx], owner);

          assert(item_eq_result.bytes[15] == tid__bool);

          if (item_eq_result.bytes[0] == 0) return item_eq_result;
     }
     return bool__true;
}

static t_any array__copy__own(t_thrd_data* const thrd_data, t_any const dst, u64 const idx, t_any src, const char* const owner) {
     assume(dst.bytes[15] == tid__array);
     assert(src.bytes[15] == tid__array);

     u32          dst_slice_offset = slice__get_offset(dst);
     u32    const dst_slice_len    = slice__get_len(dst);
     u64    const dst_ref_cnt      = get_ref_cnt(dst);
     u64    const dst_array_len    = slice_array__get_len(dst);
     u64    const dst_len          = dst_slice_len <= slice_max_len ? dst_slice_len : dst_array_len;
     t_any* const dst_items        = slice_array__get_items(dst);

     assert(dst_slice_len <= slice_max_len || dst_slice_offset == 0);

     u32          src_slice_offset = slice__get_offset(src);
     u32    const src_slice_len    = slice__get_len(src);
     u64    const src_ref_cnt      = get_ref_cnt(src);
     u64    const src_array_len    = slice_array__get_len(src);
     u64    const src_len          = src_slice_len <= slice_max_len ? src_slice_len : src_array_len;
     t_any*       src_items        = slice_array__get_items(src);

     assert(src_slice_len <= slice_max_len || src_slice_offset == 0);

     if (dst_len < idx + src_len || (i64)idx < 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, dst);
          ref_cnt__dec(thrd_data, src);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (src_len == 0) {
          ref_cnt__dec(thrd_data, src);
          return dst;
     }

     if (dst_ref_cnt == 1) {
          if (src_ref_cnt == 1)
               for (u32 src_idx = 0; src_idx < src_slice_offset; ref_cnt__dec(thrd_data, src_items[src_idx]), src_idx += 1);

          t_any*       const src_items_in_dst     = &dst_items[dst_slice_offset + idx];
          const t_any* const src_item_from_offset = &src_items[src_slice_offset];
          for (u64 src_idx = 0; src_idx < src_len; src_idx += 1) {
               t_any* const dst_item_ptr = &src_items_in_dst[src_idx];
               t_any  const src_item     = src_item_from_offset[src_idx];
               if (src_ref_cnt > 1) ref_cnt__inc(thrd_data, src_item, owner);
               ref_cnt__dec(thrd_data, *dst_item_ptr);
               *dst_item_ptr = src_item;
          }

          if (src_ref_cnt == 1) {
               for (u64 src_idx = src_slice_offset + src_len; src_idx < src_array_len; ref_cnt__dec(thrd_data, src_items[src_idx]), src_idx += 1);

               free((void*)src.qwords[0]);
          } else if (src_ref_cnt != 0) set_ref_cnt(src, src_ref_cnt - 1);

          return dst;
     }
     if (dst_ref_cnt != 0) set_ref_cnt(dst, dst_ref_cnt - 1);

     if (src_ref_cnt == 1) {
          u64 const src_cap = slice_array__get_cap(src);
          for (u32 src_idx = 0; src_idx < src_slice_offset; ref_cnt__dec(thrd_data, src_items[src_idx]), src_idx += 1);
          for (u64 src_idx = src_slice_offset + src_len; src_idx < src_array_len; ref_cnt__dec(thrd_data, src_items[src_idx]), src_idx += 1);

          if (src_cap < dst_len) {
               src.qwords[0] = (u64)realloc((t_any*)src.qwords[0], (dst_len + 1) * 16);
               src_items     = slice_array__get_items(src);
          }

          if (src_slice_offset != idx) memmove(&src_items[idx], &src_items[src_slice_offset], src_len * 16);
          slice_array__set_len(src, dst_len);
          src = slice__set_metadata(src, 0, dst_slice_len);

          const t_any* const dst_items_from_offset = &dst_items[dst_slice_offset];
          for (u64 dst_idx = 0; dst_idx < idx; dst_idx += 1) {
               t_any const item   = dst_items_from_offset[dst_idx];
               src_items[dst_idx] = item;
               ref_cnt__inc(thrd_data, item, owner);
          }
          for (u64 dst_idx = idx + src_len; dst_idx < dst_len; dst_idx += 1) {
               t_any const item   = dst_items_from_offset[dst_idx];
               src_items[dst_idx] = item;
               ref_cnt__inc(thrd_data, item, owner);
          }
          return src;
     }

     if (dst_items == src_items) {
          if (dst_ref_cnt == 2) {
               t_any* const src_items_in_dst      = &dst_items[dst_slice_offset + idx];
               t_any* const src_items_from_offset = &dst_items[src_slice_offset];
               if (src_items_in_dst == src_items_from_offset) return dst;

               if (src_items_from_offset > src_items_in_dst) for (u64 src_idx = 0; src_idx < src_len; src_idx += 1) {
                    t_any* const dst_item_ptr = &src_items_in_dst[src_idx];
                    t_any  const src_item     = src_items_from_offset[src_idx];
                    ref_cnt__inc(thrd_data, src_item, owner);
                    ref_cnt__dec(thrd_data, *dst_item_ptr);
                    *dst_item_ptr = src_item;
               }
               else for (u64 src_idx = src_len - 1; src_idx != (u64)-1; src_idx -= 1) {
                    t_any* const dst_item_ptr = &src_items_in_dst[src_idx];
                    t_any  const src_item     = src_items_from_offset[src_idx];
                    ref_cnt__inc(thrd_data, src_item, owner);
                    ref_cnt__dec(thrd_data, *dst_item_ptr);
                    *dst_item_ptr = src_item;
               }
               return dst;
          }

          if (dst_ref_cnt != 0) set_ref_cnt(dst, dst_ref_cnt - 2);
     } else if (src_ref_cnt != 0) set_ref_cnt(src, src_ref_cnt - 1);

     t_any new_array = array__init(thrd_data, dst_len, owner);
     new_array       = slice__set_metadata(new_array, 0, dst_slice_len);
     slice_array__set_len(new_array, dst_len);

     t_any*       const new_items             = slice_array__get_items(new_array);
     const t_any* const dst_items_from_offset = &dst_items[dst_slice_offset];
     for (u64 dst_idx = 0; dst_idx < idx; dst_idx += 1) {
          t_any const item   = dst_items_from_offset[dst_idx];
          new_items[dst_idx] = item;
          ref_cnt__inc(thrd_data, item, owner);
     }
     const t_any* const src_items_from_offset  = &src_items[src_slice_offset];
     t_any*       const src_items_in_new_array = &new_items[idx];
     for (u64 src_idx = 0; src_idx < src_len; src_idx += 1) {
          t_any const item                = src_items_from_offset[src_idx];
          src_items_in_new_array[src_idx] = item;
          ref_cnt__inc(thrd_data, item, owner);
     }
     for (u64 dst_idx = idx + src_len; dst_idx < dst_len; dst_idx += 1) {
          t_any const item   = dst_items_from_offset[dst_idx];
          new_items[dst_idx] = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     return new_array;
}

static t_any array__delete__own(t_thrd_data* const thrd_data, t_any array, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(idx_from <= idx_to);
     assert(array.bytes[15] == tid__array);

     u32    const slice_offset = slice__get_offset(array);
     u32    const slice_len    = slice__get_len(array);
     u64    const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (idx_to > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const new_len = len - (idx_to - idx_from);
     if (new_len == len) return array;

     if (idx_from == 0 || idx_to == len) {
          u64 const slice_idx_from = idx_from == 0 ? idx_to : 0;
          u64 const slice_to_from  = idx_from == 0 ? len : idx_from;
          return array__slice__own(thrd_data, array, slice_idx_from, slice_to_from, owner);
     }

     u64 const ref_cnt = get_ref_cnt(array);
     if (ref_cnt == 1) {
          u64    const array_len = slice_array__get_len(array);
          t_any* const items     = (t_any*)slice_array__get_items(array);
          array                  = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          slice_array__set_len(array, new_len);
          if (slice_offset != 0) {
               for (u32 array_idx = 0; array_idx < slice_offset; ref_cnt__dec(thrd_data, items[array_idx]), array_idx += 1);
               memmove(items, &items[slice_offset], idx_from * 16);
          }

          for (u64 array_idx = idx_from; array_idx < idx_to; ref_cnt__dec(thrd_data, items[slice_offset + array_idx]), array_idx += 1);
          memmove(&items[idx_from], &items[slice_offset + idx_to], (len - idx_to) * 16);
          for (u64 array_idx = slice_offset + len; array_idx < array_len; ref_cnt__dec(thrd_data, items[array_idx]), array_idx += 1);

          return array;
     }

     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     const t_any* const items     = &((t_any*)slice_array__get_items(array))[slice_offset];
     t_any              new_array = array__init(thrd_data, new_len, owner);
     new_array                    = slice__set_metadata(new_array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
     slice_array__set_len(new_array, new_len);
     t_any*       const new_items = slice_array__get_items(new_array);

     for (u64 array_idx = 0; array_idx < idx_from; array_idx += 1) {
          t_any const item     = items[array_idx];
          new_items[array_idx] = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     for (u64 array_idx = idx_to; array_idx < len; array_idx += 1) {
          t_any const item                         = items[array_idx];
          new_items[array_idx + idx_from - idx_to] = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     return new_array;
}

static inline t_any array__drop__own(t_thrd_data* const thrd_data, t_any array, u64 const n, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32 const slice_len = slice__get_len(array);
     u64 const len       = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);
     if (n > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     return array__slice__own(thrd_data, array, n, len, owner);
}

static t_any array__drop_while__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32    const slice_offset = slice__get_offset(array);
     u32    const slice_len    = slice__get_len(array);
     t_any* const items        = slice_array__get_items(array);
     u64    const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64 n = 0;
     for (; n < len; n += 1) {
          t_any* const item_ptr = &items[slice_offset + n];
          ref_cnt__inc(thrd_data, *item_ptr, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, item_ptr, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, array);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) break;
     }
     ref_cnt__dec(thrd_data, fn);

     if (n == 0) return array;

     u64 const new_len = len - n;
     if (new_len == 0) {
          ref_cnt__dec(thrd_data, array);
          return empty_array;
     }

     u64 const n_offset = slice_offset + n;
     if (new_len <= slice_max_len && n_offset <= slice_max_offset) [[clang::likely]] {
          array = slice__set_metadata(array, n_offset, new_len);
          return array;
     }

     u64 const ref_cnt = get_ref_cnt(array);
     if (ref_cnt == 1) {
          slice_array__set_len(array, new_len);
          slice_array__set_cap(array, new_len);
          array = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          for (u64 idx = 0; idx < n_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          memmove(items, &items[n_offset], new_len * 16);
          for (u64 idx = n_offset + new_len; idx < len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

          array.qwords[0] = (u64)aligned_realloc(16, (t_any*)array.qwords[0], (new_len + 1) * 16);
          return array;
     }

     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any        new_array = array__init(thrd_data, new_len, owner);
     t_any* const new_items = slice_array__get_items(new_array);
     new_array              = slice__set_metadata(new_array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
     slice_array__set_len(new_array, new_len);
     for (u64 idx = 0; idx < new_len; idx += 1) {
          t_any const item = items[n_offset + idx];
          new_items[idx]   = item;
          ref_cnt__inc(thrd_data, item, owner);
     }
     return new_array;
}

static t_any array__each_to_each__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len < 2) {
          ref_cnt__dec(thrd_data, fn);
          return array;
     }

     u32 const slice_offset = slice__get_offset(array);
     t_any*    items        = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt != 1) {
          t_any const new_array = slice__set_metadata(array__init(thrd_data, len, owner), 0, slice_len);
          slice_array__set_len(new_array, len);
          t_any* const new_items = slice_array__get_items(new_array);

          if (ref_cnt == 0)
               memcpy(new_items, items, len * 16);
          else {
               set_ref_cnt(array, ref_cnt - 1);

               for (u64 idx = 0; idx < len; idx += 1) {
                    t_any const item = items[idx];
                    new_items[idx]   = item;
                    ref_cnt__inc(thrd_data, item, owner);
               }
          }

          array = new_array;
          items = new_items;
     }

     t_any fn_args[2];
     for (u64 idx_1 = 0; idx_1 < len - 1; idx_1 += 1) {
          fn_args[0] = items[idx_1];

          for (u64 idx_2 = idx_1 + 1; idx_2 < len; idx_2 += 1) {
               fn_args[1] = items[idx_2];

               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    items[idx_1].raw_bits = 0;
                    items[idx_2].raw_bits = 0;
                    ref_cnt__dec(thrd_data, array);
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || !type_is_common(box_items[0].bytes[15]) || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
                    items[idx_1].raw_bits = 0;
                    items[idx_2].raw_bits = 0;
                    ref_cnt__dec(thrd_data, array);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[0]   = box_items[0];
               items[idx_2] = box_items[1];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          items[idx_1] = fn_args[0];
     }
     ref_cnt__dec(thrd_data, fn);

     return array;
}

static t_any array__each_to_each_kv__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len < 2) {
          ref_cnt__dec(thrd_data, fn);
          return array;
     }

     u32 const slice_offset = slice__get_offset(array);
     t_any*    items        = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt != 1) {
          t_any const new_array = slice__set_metadata(array__init(thrd_data, len, owner), 0, slice_len);
          slice_array__set_len(new_array, len);
          t_any* const new_items = slice_array__get_items(new_array);

          if (ref_cnt == 0)
               memcpy(new_items, items, len * 16);
          else {
               set_ref_cnt(array, ref_cnt - 1);

               for (u64 idx = 0; idx < len; idx += 1) {
                    t_any const item = items[idx];
                    new_items[idx]   = item;
                    ref_cnt__inc(thrd_data, item, owner);
               }
          }

          array = new_array;
          items = new_items;
     }

     t_any fn_args[4];
     fn_args[0] = (const t_any){.bytes = {[15] = tid__int}};
     fn_args[2] = (const t_any){.bytes = {[15] = tid__int}};
     for (u64 idx_1 = 0; idx_1 < len - 1; idx_1 += 1) {
          fn_args[0].qwords[0] = idx_1;
          fn_args[1]           = items[idx_1];

          for (u64 idx_2 = idx_1 + 1; idx_2 < len; idx_2 += 1) {
               fn_args[2].qwords[0] = idx_2;
               fn_args[3]           = items[idx_2];

               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 4, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    items[idx_1].raw_bits = 0;
                    items[idx_2].raw_bits = 0;
                    ref_cnt__dec(thrd_data, array);
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || !type_is_common(box_items[0].bytes[15]) || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
                    items[idx_1].raw_bits = 0;
                    items[idx_2].raw_bits = 0;
                    ref_cnt__dec(thrd_data, array);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[1]   = box_items[0];
               items[idx_2] = box_items[1];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          items[idx_1] = fn_args[1];
     }
     ref_cnt__dec(thrd_data, fn);

     return array;
}

static t_any array__filter__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          ref_cnt__dec(thrd_data, fn);
          return array;
     }

     u32    const slice_offset   = slice__get_offset(array);
     u64    const ref_cnt        = get_ref_cnt(array);
     u64    const array_len      = slice_array__get_len(array);
     t_any* const original_items = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64    const len            = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64  part_lens[64];
     part_lens[0]      = 1;
     u8   current_part = 0;
     u64  odd_is_pass;

     {
          t_any const item = original_items[0];
          ref_cnt__inc(thrd_data, item, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &item, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, array);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          odd_is_pass = fn_result.bytes[0] ^ 1;
     }

     t_any      dst_array    = array;
     t_any*     dst_items    = slice_array__get_items(dst_array);
     t_any*     src_items    = original_items;
     bool const array_is_mut = ref_cnt == 1;
     bool       dst_is_mut   = array_is_mut;
     u64        dst_len      = 0;
     for (u64 idx = 1; idx < len; idx += 1) {
          t_any const item = original_items[idx];
          ref_cnt__inc(thrd_data, item, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &item, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_array.qwords[0] != array.qwords[0]) {
                    dst_array = slice__set_metadata(dst_array, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
                    slice_array__set_len(dst_array, dst_len);
                    ref_cnt__dec(thrd_data, dst_array);
               }
               ref_cnt__dec(thrd_data, array);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == ((current_part & 1) == odd_is_pass)) part_lens[current_part] += 1;
          else if (current_part != 63) {
               current_part           += 1;
               part_lens[current_part] = 1;
          } else {
               typedef u64 t_vec __attribute__((ext_vector_type(64), aligned(alignof(u64))));

               if (!dst_is_mut) {
                    u64 const droped_len = __builtin_reduce_add(
                         *(const t_vec*)part_lens &
                         (__builtin_convertvector(u64_to_v_64_bool(odd_is_pass ? 0x5555'5555'5555'5555ull : 0xaaaa'aaaa'aaaa'aaaaull), t_vec) * -1)
                    );
                    dst_array  = array__init(thrd_data, len - droped_len, owner);
                    dst_items  = slice_array__get_items(dst_array);
                    dst_is_mut = true;
               }

               if (array_is_mut) for (u8 part_idx = 0; part_idx < 64; part_idx += 1) {
                    u64 const part_len = part_lens[part_idx];
                    if ((part_idx & 1) == odd_is_pass) {
                         if (src_items == dst_items) dst_len += part_len;
                         else for (u64 passed_idx = 0; passed_idx < part_len; passed_idx += 1) {
                              t_any* const passed_item_ptr   = &src_items[passed_idx];
                              t_any* const replaced_item_ptr = &dst_items[dst_len];
                              ref_cnt__dec(thrd_data, *replaced_item_ptr);
                              *replaced_item_ptr = *passed_item_ptr;
                              passed_item_ptr->raw_bits = 0;
                              dst_len                  += 1;
                         }
                    } else for (u64 droped_idx = 0; droped_idx < part_len; droped_idx += 1) {
                         t_any* const droped_item_ptr = &src_items[droped_idx];
                         ref_cnt__dec(thrd_data, *droped_item_ptr);
                         droped_item_ptr->raw_bits = 0;
                    }
                    src_items = &src_items[part_len];
               }
               else for (u8 part_idx = 0; part_idx < 64; part_idx += 1) {
                    u64 const part_len = part_lens[part_idx];
                    if ((part_idx & 1) == odd_is_pass) for (u64 passed_idx = 0; passed_idx < part_len; passed_idx += 1) {
                         t_any const passed_item = src_items[passed_idx];
                         dst_items[dst_len]      = passed_item;
                         ref_cnt__inc(thrd_data, passed_item, owner);
                         dst_len += 1;
                    }

                    src_items = &src_items[part_len];
               }

               current_part = 0;
               part_lens[0] = 1;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (src_items == original_items) {
          u64 from = -1;
          u64 to;

          switch (current_part) {
          case 0:
               if (odd_is_pass == 0) return array;
               ref_cnt__dec(thrd_data, array);
               return empty_array;
          case 1:
               from = odd_is_pass ? part_lens[0] : 0;
               to   = odd_is_pass ? len : part_lens[0];
               break;
          case 2:
               if (odd_is_pass) {
                    from = part_lens[0];
                    to   = from + part_lens[1];
               }
               break;
          }

          if (from != (u64)-1) return array__slice__own(thrd_data, array, from, to, owner);

          typedef u64 t_vec __attribute__((ext_vector_type(8), aligned(alignof(u64))));

          if (!dst_is_mut) {
               u64         droped_len = 0;
               u8          part_idx   = 8;
               t_vec const mask       = __builtin_convertvector(u8_to_v_8_bool(odd_is_pass ? 0x55 : 0xaa), t_vec) * -1;
               for (; part_idx <= current_part; part_idx += 8)
                    droped_len += __builtin_reduce_add(*(const t_vec*)&part_lens[part_idx - 8] & mask);
               for (part_idx -= odd_is_pass ? 8 : 7; part_idx <= current_part; droped_len += part_lens[part_idx], part_idx += 2);

               dst_array = array__init(thrd_data, len - droped_len, owner);
               dst_items = slice_array__get_items(dst_array);
          }
     }

     if (array_is_mut) {
          for (u8 part_idx = 0; part_idx <= current_part; part_idx += 1) {
               u64 const part_len = part_lens[part_idx];
               if ((part_idx & 1) == odd_is_pass) {
                    if (src_items == dst_items) dst_len += part_len;
                    else for (u64 passed_idx = 0; passed_idx < part_len; passed_idx += 1) {
                         t_any* const passed_item_ptr   = &src_items[passed_idx];
                         t_any* const replaced_item_ptr = &dst_items[dst_len];
                         ref_cnt__dec(thrd_data, *replaced_item_ptr);
                         *replaced_item_ptr = *passed_item_ptr;
                         passed_item_ptr->raw_bits = 0;
                         dst_len                  += 1;
                    }
               } else for (u64 droped_idx = 0; droped_idx < part_len; droped_idx += 1) {
                    t_any* const droped_item_ptr = &src_items[droped_idx];
                    ref_cnt__dec(thrd_data, *droped_item_ptr);
                    droped_item_ptr->raw_bits = 0;
               }
               src_items = &src_items[part_len];
          }

          for (u64 idx = dst_len; idx < slice_offset; ref_cnt__dec(thrd_data, dst_items[idx]), idx += 1);
          for (u64 idx = slice_offset + len; idx < array_len; ref_cnt__dec(thrd_data, dst_items[idx]), idx += 1);
     } else {
          for (u8 part_idx = 0; part_idx <= current_part; part_idx += 1) {
               u64 const part_len = part_lens[part_idx];
               if ((part_idx & 1) == odd_is_pass) for (u64 passed_idx = 0; passed_idx < part_len; passed_idx += 1) {
                    t_any const passed_item = src_items[passed_idx];
                    dst_items[dst_len]      = passed_item;
                    ref_cnt__inc(thrd_data, passed_item, owner);
                    dst_len += 1;
               }
               src_items = &src_items[part_len];
          }

          ref_cnt__dec(thrd_data, array);
     }

     dst_array = slice__set_metadata(dst_array, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
     slice_array__set_len(dst_array, dst_len);
     return dst_array;
}

static t_any array__filter_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          ref_cnt__dec(thrd_data, fn);
          return array;
     }

     u32    const slice_offset   = slice__get_offset(array);
     u64    const ref_cnt        = get_ref_cnt(array);
     u64    const array_len      = slice_array__get_len(array);
     t_any* const original_items = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64    const len            = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64  part_lens[64];
     part_lens[0]      = 1;
     u8   current_part = 0;
     u64  odd_is_pass;

     t_any kv[2];
     kv[0] = (const t_any){.bytes = {[15] = tid__int}};

     {
          kv[1] = original_items[0];
          ref_cnt__inc(thrd_data, kv[1], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, array);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          odd_is_pass = fn_result.bytes[0] ^ 1;
     }

     t_any      dst_array    = array;
     t_any*     dst_items    = slice_array__get_items(dst_array);
     t_any*     src_items    = original_items;
     bool const array_is_mut = ref_cnt == 1;
     bool       dst_is_mut   = array_is_mut;
     u64        dst_len      = 0;
     for (u64 idx = 1; idx < len; idx += 1) {
          kv[0].qwords[0] = idx;
          kv[1]           = original_items[idx];
          ref_cnt__inc(thrd_data, kv[1], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_array.qwords[0] != array.qwords[0]) {
                    dst_array = slice__set_metadata(dst_array, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
                    slice_array__set_len(dst_array, dst_len);
                    ref_cnt__dec(thrd_data, dst_array);
               }
               ref_cnt__dec(thrd_data, array);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == ((current_part & 1) == odd_is_pass)) part_lens[current_part] += 1;
          else if (current_part != 63) {
               current_part           += 1;
               part_lens[current_part] = 1;
          } else {
               typedef u64 t_vec __attribute__((ext_vector_type(64), aligned(alignof(u64))));

               if (!dst_is_mut) {
                    u64 const droped_len = __builtin_reduce_add(
                         *(const t_vec*)part_lens &
                         (__builtin_convertvector(u64_to_v_64_bool(odd_is_pass ? 0x5555'5555'5555'5555ull : 0xaaaa'aaaa'aaaa'aaaaull), t_vec) * -1)
                    );
                    dst_array  = array__init(thrd_data, len - droped_len, owner);
                    dst_items  = slice_array__get_items(dst_array);
                    dst_is_mut = true;
               }

               if (array_is_mut) for (u8 part_idx = 0; part_idx < 64; part_idx += 1) {
                    u64 const part_len = part_lens[part_idx];
                    if ((part_idx & 1) == odd_is_pass) {
                         if (src_items == dst_items) dst_len += part_len;
                         else for (u64 passed_idx = 0; passed_idx < part_len; passed_idx += 1) {
                              t_any* const passed_item_ptr   = &src_items[passed_idx];
                              t_any* const replaced_item_ptr = &dst_items[dst_len];
                              ref_cnt__dec(thrd_data, *replaced_item_ptr);
                              *replaced_item_ptr = *passed_item_ptr;
                              passed_item_ptr->raw_bits = 0;
                              dst_len                  += 1;
                         }
                    } else for (u64 droped_idx = 0; droped_idx < part_len; droped_idx += 1) {
                         t_any* const droped_item_ptr = &src_items[droped_idx];
                         ref_cnt__dec(thrd_data, *droped_item_ptr);
                         droped_item_ptr->raw_bits = 0;
                    }
                    src_items = &src_items[part_len];
               }
               else for (u8 part_idx = 0; part_idx < 64; part_idx += 1) {
                    u64 const part_len = part_lens[part_idx];
                    if ((part_idx & 1) == odd_is_pass) for (u64 passed_idx = 0; passed_idx < part_len; passed_idx += 1) {
                         t_any const passed_item = src_items[passed_idx];
                         dst_items[dst_len]      = passed_item;
                         ref_cnt__inc(thrd_data, passed_item, owner);
                         dst_len += 1;
                    }

                    src_items = &src_items[part_len];
               }

               current_part = 0;
               part_lens[0] = 1;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (src_items == original_items) {
          u64 from = -1;
          u64 to;

          switch (current_part) {
          case 0:
               if (odd_is_pass == 0) return array;
               ref_cnt__dec(thrd_data, array);
               return empty_array;
          case 1:
               from = odd_is_pass ? part_lens[0] : 0;
               to   = odd_is_pass ? len : part_lens[0];
               break;
          case 2:
               if (odd_is_pass) {
                    from = part_lens[0];
                    to   = from + part_lens[1];
               }
               break;
          }

          if (from != (u64)-1) return array__slice__own(thrd_data, array, from, to, owner);

          typedef u64 t_vec __attribute__((ext_vector_type(8), aligned(alignof(u64))));

          if (!dst_is_mut) {
               u64         droped_len = 0;
               u8          part_idx   = 8;
               t_vec const mask       = __builtin_convertvector(u8_to_v_8_bool(odd_is_pass ? 0x55 : 0xaa), t_vec) * -1;
               for (; part_idx <= current_part; part_idx += 8)
                    droped_len += __builtin_reduce_add(*(const t_vec*)&part_lens[part_idx - 8] & mask);
               for (part_idx -= odd_is_pass ? 8 : 7; part_idx <= current_part; droped_len += part_lens[part_idx], part_idx += 2);

               dst_array = array__init(thrd_data, len - droped_len, owner);
               dst_items = slice_array__get_items(dst_array);
          }
     }

     if (array_is_mut) {
          for (u8 part_idx = 0; part_idx <= current_part; part_idx += 1) {
               u64 const part_len = part_lens[part_idx];
               if ((part_idx & 1) == odd_is_pass) {
                    if (src_items == dst_items) dst_len += part_len;
                    else for (u64 passed_idx = 0; passed_idx < part_len; passed_idx += 1) {
                         t_any* const passed_item_ptr   = &src_items[passed_idx];
                         t_any* const replaced_item_ptr = &dst_items[dst_len];
                         ref_cnt__dec(thrd_data, *replaced_item_ptr);
                         *replaced_item_ptr = *passed_item_ptr;
                         passed_item_ptr->raw_bits = 0;
                         dst_len                  += 1;
                    }
               } else for (u64 droped_idx = 0; droped_idx < part_len; droped_idx += 1) {
                    t_any* const droped_item_ptr = &src_items[droped_idx];
                    ref_cnt__dec(thrd_data, *droped_item_ptr);
                    droped_item_ptr->raw_bits = 0;
               }
               src_items = &src_items[part_len];
          }

          for (u64 idx = dst_len; idx < slice_offset; ref_cnt__dec(thrd_data, dst_items[idx]), idx += 1);
          for (u64 idx = slice_offset + len; idx < array_len; ref_cnt__dec(thrd_data, dst_items[idx]), idx += 1);
     } else {
          for (u8 part_idx = 0; part_idx <= current_part; part_idx += 1) {
               u64 const part_len = part_lens[part_idx];
               if ((part_idx & 1) == odd_is_pass) for (u64 passed_idx = 0; passed_idx < part_len; passed_idx += 1) {
                    t_any const passed_item = src_items[passed_idx];
                    dst_items[dst_len]      = passed_item;
                    ref_cnt__inc(thrd_data, passed_item, owner);
                    dst_len += 1;
               }
               src_items = &src_items[part_len];
          }

          ref_cnt__dec(thrd_data, array);
     }

     dst_array = slice__set_metadata(dst_array, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
     slice_array__set_len(dst_array, dst_len);
     return dst_array;
}

static t_any array__foldl__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32          const slice_offset      = slice__get_offset(array);
     u32          const slice_len         = slice__get_len(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt == 1)
          for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     else if (ref_cnt != 0 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = state;
     u64   idx  = 0;
     for (; idx < len; idx += 1) {
          fn_args[1] = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               idx += 1;
               break;
          }

          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] {
               idx += 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          free((void*)array.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any array__foldl1__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32          const slice_offset      = slice__get_offset(array);
     u32          const slice_len         = slice__get_len(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (len == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt == 1)
          for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     else if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = items_from_offset[0];
     if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[0], owner);

     u64 idx = 1;
     for (; idx < len; idx += 1) {
          fn_args[1] = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);

               idx += 1;
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] {
               idx += 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          free((void*)array.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any array__foldl_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32          const slice_offset      = slice__get_offset(array);
     u32          const slice_len         = slice__get_len(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt == 1)
          for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     else if (ref_cnt != 0 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[3];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__int}};
     u64   idx  = 0;
     for (; idx < len; idx += 1) {
          fn_args[1].qwords[0] = idx;
          fn_args[2]           = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[2], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);

               idx += 1;
               break;
          }

          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] {
               idx += 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          free((void*)array.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any array__foldr__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32          const slice_offset      = slice__get_offset(array);
     u32          const slice_len         = slice__get_len(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt == 1) {
          i64 const end = slice_offset + len;
          for (i64 idx = array_len - 1; idx >= end; ref_cnt__dec(thrd_data, items[idx]), idx -= 1);
     } else if (ref_cnt != 0 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = state;
     u64   idx  = len - 1;
     for (; idx != (u64)-1; idx -= 1) {
          fn_args[1] = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);

               idx -= 1;
               break;
          }

          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] {
               idx -= 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx != (u64)-1; ref_cnt__dec(thrd_data, items[idx]), idx -= 1);
          free((void*)array.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any array__foldr1__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32          const slice_offset      = slice__get_offset(array);
     u32          const slice_len         = slice__get_len(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (len == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt == 1) {
          i64 const end = slice_offset + len;
          for (i64 idx = array_len - 1; idx >= end; ref_cnt__dec(thrd_data, items[idx]), idx -= 1);
     } else if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = items_from_offset[len - 1];
     if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[0], owner);

     u64 idx = len - 2;
     for (; idx != (u64)-1; idx -= 1) {
          fn_args[1] = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);

               idx -= 1;
               break;
          }

          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] {
               idx -= 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx != (u64)-1; ref_cnt__dec(thrd_data, items[idx]), idx -= 1);
          free((void*)array.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any array__foldr_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32          const slice_offset      = slice__get_offset(array);
     u32          const slice_len         = slice__get_len(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt == 1) {
          i64 const end = slice_offset + len;
          for (i64 idx = array_len - 1; idx >= end; ref_cnt__dec(thrd_data, items[idx]), idx -= 1);
     } else if (ref_cnt != 0 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[3];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__int}};
     u64   idx  = len - 1;
     for (; idx != (u64)-1; idx -= 1) {
          fn_args[1].qwords[0] = idx;
          fn_args[2]           = items_from_offset[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[2], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);

               idx -= 1;
               break;
          }

          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] {
               idx -= 1;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += slice_offset; idx != (u64)-1; ref_cnt__dec(thrd_data, items[idx]), idx -= 1);
          free((void*)array.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

[[gnu::hot, clang::noinline]]
static t_any hash(t_thrd_data* const thrd_data, t_any const arg, u64 seed, const char* const owner);

static t_any array__hash(t_thrd_data* const thrd_data, t_any const array, u64 const seed, const char* const owner) {
     assert(array.bytes[15] == tid__array);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) return (const t_any){.structure = {.value = xxh3_get_hash(nullptr, 0, seed ^ array.bytes[15]), .type = tid__int}};

     u32 const slice_offset = slice__get_offset(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     const t_any* const items      = &((const t_any*)slice_array__get_items(array))[slice_offset];
     u64                array_hash = seed;

     assume_aligned(items, 16);

     for (u64 idx = 0; idx < len; idx += 1) {
          t_any const item_hash = hash(thrd_data, items[idx], array_hash, owner);
          if (item_hash.bytes[15] == tid__error) [[clang::unlikely]] return item_hash;

          assert(item_hash.bytes[15] == tid__int);

          array_hash = item_hash.qwords[0];
     }

     return (const t_any){.structure = {.value = array_hash, .type = tid__int}};
}

static t_any array__look_from_end_in__own(t_thrd_data* const thrd_data, t_any const array, t_any const looked_item, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32          const slice_offset = slice__get_offset(array);
     u32          const slice_len    = slice__get_len(array);
     const t_any* const items        = &((const t_any*)slice_array__get_items(array))[slice_offset];
     u64          const ref_cnt      = get_ref_cnt(array);
     u64          const array_len    = slice_array__get_len(array);
     u64          const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any result = null;
     for (u64 idx = len - 1; idx != (u64)-1; idx -= 1) {
          t_any const eq_result = equal(thrd_data, looked_item, items[idx], owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) {
               result = (const t_any){.structure = {.value = idx, .type = tid__int}};
               break;
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
     ref_cnt__dec(thrd_data, looked_item);

     return result;
}

static inline t_any array__look_in__own(t_thrd_data* const thrd_data, t_any const array, t_any const looked_item, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32          const slice_offset = slice__get_offset(array);
     u32          const slice_len    = slice__get_len(array);
     const t_any* const items        = &((const t_any*)slice_array__get_items(array))[slice_offset];
     u64          const ref_cnt      = get_ref_cnt(array);
     u64          const array_len    = slice_array__get_len(array);
     u64          const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any result = null;
     for (u64 idx = 0; idx < len; idx += 1) {
          t_any const eq_result = equal(thrd_data, looked_item, items[idx], owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) {
               result = (const t_any){.structure = {.value = idx, .type = tid__int}};
               break;
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
     ref_cnt__dec(thrd_data, looked_item);

     return result;
}

static t_any array__look_other_from_end_in__own(t_thrd_data* const thrd_data, t_any const array, t_any const not_looked_item, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32          const slice_offset = slice__get_offset(array);
     u32          const slice_len    = slice__get_len(array);
     const t_any* const items        = &((const t_any*)slice_array__get_items(array))[slice_offset];
     u64          const ref_cnt      = get_ref_cnt(array);
     u64          const array_len    = slice_array__get_len(array);
     u64          const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any result = null;
     for (u64 idx = len - 1; idx != (u64)-1; idx -= 1) {
          t_any const eq_result = equal(thrd_data, not_looked_item, items[idx], owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) {
               result = (const t_any){.structure = {.value = idx, .type = tid__int}};
               break;
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
     ref_cnt__dec(thrd_data, not_looked_item);

     return result;
}

static t_any array__look_other_in__own(t_thrd_data* const thrd_data, t_any const array, t_any const not_looked_item, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32          const slice_offset = slice__get_offset(array);
     u32          const slice_len    = slice__get_len(array);
     const t_any* const items        = &((const t_any*)slice_array__get_items(array))[slice_offset];
     u64          const ref_cnt      = get_ref_cnt(array);
     u64          const array_len    = slice_array__get_len(array);
     u64          const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any result = null;
     for (u64 idx = 0; idx < len; idx += 1) {
          t_any const eq_result = equal(thrd_data, not_looked_item, items[idx], owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) {
               result = (const t_any){.structure = {.value = idx, .type = tid__int}};
               break;
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
     ref_cnt__dec(thrd_data, not_looked_item);

     return result;
}

static t_any array__look_part_from_end_in__own(t_thrd_data* const thrd_data, t_any const array, t_any const part, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(part.bytes[15] == tid__array);

     u32          const array_slice_offset = slice__get_offset(array);
     u32          const array_slice_len    = slice__get_len(array);
     const t_any* const array_items        = &((const t_any*)slice_array__get_items(array))[array_slice_offset];
     u64          const array_ref_cnt      = get_ref_cnt(array);
     u64          const array_array_len    = slice_array__get_len(array);
     u64          const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     u32          const part_slice_offset = slice__get_offset(part);
     u32          const part_slice_len    = slice__get_len(part);
     const t_any* const part_items        = &((const t_any*)slice_array__get_items(part))[part_slice_offset];
     u64          const part_len          = part_slice_len <= slice_max_len ? part_slice_len : slice_array__get_len(part);

     assert(part_slice_len <= slice_max_len || part_slice_offset == 0);

     if (part_len == 0) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, part);
          return (const t_any){.structure = {.value = array_len, .type = tid__int}};
     }

     if (part_len > array_len) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, part);
          return null;
     }

     if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);

     t_any result = null;
     for (u64 array_idx = array_len - part_len; array_idx != (u64)-1; array_idx -= 1) {
          for (u64 part_idx = 0; part_idx < part_len; part_idx += 1) {
               t_any const eq_result = equal(thrd_data, array_items[array_idx + part_idx], part_items[part_idx], owner);

               assert(eq_result.bytes[15] == tid__bool);

               if (eq_result.bytes[0] == 0) goto main_loop;
          }

          result = (const t_any){.structure = {.value = array_idx, .type = tid__int}};
          break;

          main_loop:
     }

     if (array_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
     ref_cnt__dec(thrd_data, part);

     return result;
}

static t_any array__look_part_in__own(t_thrd_data* const thrd_data, t_any const array, t_any const part, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(part.bytes[15] == tid__array);

     u32          const part_slice_len    = slice__get_len(part);
     u32          const part_slice_offset = slice__get_offset(part);
     const t_any* const part_items        = &((const t_any*)slice_array__get_items(part))[part_slice_offset];
     u64          const part_len          = part_slice_len <= slice_max_len ? part_slice_len : slice_array__get_len(part);

     assert(part_slice_len <= slice_max_len || part_slice_offset == 0);

     if (part_len == 0) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, part);
          return (const t_any){.structure = {.value = 0, .type = tid__int}};
     }

     u32          const array_slice_offset = slice__get_offset(array);
     u32          const array_slice_len    = slice__get_len(array);
     const t_any* const array_items        = &((const t_any*)slice_array__get_items(array))[array_slice_offset];
     u64          const array_ref_cnt      = get_ref_cnt(array);
     u64          const array_array_len    = slice_array__get_len(array);
     u64          const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     if (part_len > array_len) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, part);
          return null;
     }

     if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);

     u64   const edge   = array_len - part_len + 1;
     t_any       result = null;
     for (u64 array_idx = 0; array_idx < edge; array_idx += 1) {
          for (u64 part_idx = 0; part_idx < part_len; part_idx += 1) {
               t_any const eq_result = equal(thrd_data, array_items[array_idx + part_idx], part_items[part_idx], owner);

               assert(eq_result.bytes[15] == tid__bool);

               if (eq_result.bytes[0] == 0) goto main_loop;
          }

          result = (const t_any){.structure = {.value = array_idx, .type = tid__int}};
          break;

          main_loop:
     }

     if (array_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
     ref_cnt__dec(thrd_data, part);

     return result;
}

static t_any array__map__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          ref_cnt__dec(thrd_data, fn);
          return array;
     }

     u32    const slice_offset = slice__get_offset(array);
     t_any* const items        = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64    const ref_cnt      = get_ref_cnt(array);
     u64    const array_len    = slice_array__get_len(array);
     u64    const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any  dst_array;
     t_any* dst_items;
     if (ref_cnt == 1)
          dst_items = items;
     else {
          if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

          dst_array = array__init(thrd_data, len, owner);
          dst_array = slice__set_metadata(dst_array, 0, slice_len);
          slice_array__set_len(dst_array, len);
          dst_items = slice_array__get_items(dst_array);
     }

     for (u64 idx = 0; idx < len; idx += 1) {
          t_any* const item_ptr = &items[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, *item_ptr, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, item_ptr, 1, owner);
          if (!type_is_common(fn_result.bytes[15])) [[clang::unlikely]] {
               if (ref_cnt == 1) {
                    item_ptr->raw_bits = 0;
                    ref_cnt__dec__noinlined_part(thrd_data, array);
               }

               ref_cnt__dec(thrd_data, fn);
               if (ref_cnt != 1) {
                    for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, dst_items[free_idx]), free_idx -= 1);
                    free((void*)dst_array.qwords[0]);
               }
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          dst_items[idx] = fn_result;
     }
     ref_cnt__dec(thrd_data, fn);

     if (ref_cnt == 1) return array;

     return dst_array;
}

static t_any array__map_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          ref_cnt__dec(thrd_data, fn);
          return array;
     }

     u32    const slice_offset = slice__get_offset(array);
     t_any* const items        = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64    const ref_cnt      = get_ref_cnt(array);
     u64    const array_len    = slice_array__get_len(array);
     u64    const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any  fn_args[2];
     fn_args[0]           = (const t_any){.bytes = {[15] = tid__int}};
     t_any  dst_array;
     t_any* dst_items;
     if (ref_cnt == 1)
          dst_items = items;
     else {
          if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

          dst_array = array__init(thrd_data, len, owner);
          dst_array = slice__set_metadata(dst_array, 0, slice_len);
          slice_array__set_len(dst_array, len);
          dst_items = slice_array__get_items(dst_array);
     }

     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[0].qwords[0] = idx;
          fn_args[1]           = items[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (!type_is_common(fn_result.bytes[15])) [[clang::unlikely]] {
               if (ref_cnt == 1) {
                    items[idx].raw_bits = 0;
                    ref_cnt__dec__noinlined_part(thrd_data, array);
               }

               ref_cnt__dec(thrd_data, fn);
               if (ref_cnt != 1) {
                    for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, dst_items[free_idx]), free_idx -= 1);
                    free((void*)dst_array.qwords[0]);
               }
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          dst_items[idx] = fn_result;
     }
     ref_cnt__dec(thrd_data, fn);

     if (ref_cnt == 1) return array;

     return dst_array;
}

static t_any array__map_with_state__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32    const slice_len       = slice__get_len(array);
     u32    const slice_offset    = slice__get_offset(array);
     t_any* const items           = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64    const ref_cnt         = get_ref_cnt(array);
     u64    const array_array_len = slice_array__get_len(array);
     u64    const len             = slice_len <= slice_max_len ? slice_len : array_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any  dst_array;
     t_any* dst_items;
     if (ref_cnt == 1)
          dst_items = items;
     else {
          if (ref_cnt != 0 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

          if (len == 0)
               dst_array = empty_array;
          else {
               dst_array = array__init(thrd_data, len, owner);
               dst_array = slice__set_metadata(dst_array, 0, slice_len);
               slice_array__set_len(dst_array, len);
               dst_items = slice_array__get_items(dst_array);
          }
     }

     t_any fn_args[2];
     fn_args[0] = state;
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[1] = items[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               if (ref_cnt == 1) {
                    items[idx].raw_bits = 0;
                    ref_cnt__dec__noinlined_part(thrd_data, array);
               } else {
                    for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, dst_items[free_idx]), free_idx -= 1);
                    free((void*)dst_array.qwords[0]);
                    if (state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);
               }

               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
               if (ref_cnt == 1) {
                    items[idx].raw_bits = 0;
                    ref_cnt__dec__noinlined_part(thrd_data, array);
               } else {
                    for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, dst_items[free_idx]), free_idx -= 1);
                    free((void*)dst_array.qwords[0]);
                    if (state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);
               }

               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]     = box_items[0];
          dst_items[idx] = box_items[1];
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[1]           = fn_args[0];

     if (ref_cnt == 1)
          new_box_items[0] = array;
     else {
          new_box_items[0] = dst_array;
          if (state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);
     }

     return new_box;
}

static t_any array__map_kv_with_state__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32    const slice_len       = slice__get_len(array);
     u32    const slice_offset    = slice__get_offset(array);
     t_any* const items           = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64    const ref_cnt         = get_ref_cnt(array);
     u64    const array_array_len = slice_array__get_len(array);
     u64    const len             = slice_len <= slice_max_len ? slice_len : array_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any  dst_array;
     t_any* dst_items;
     if (ref_cnt == 1)
          dst_items = items;
     else {
          if (ref_cnt != 0 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

          if (len == 0)
               dst_array = empty_array;
          else {
               dst_array = array__init(thrd_data, len, owner);
               dst_array = slice__set_metadata(dst_array, 0, slice_len);
               slice_array__set_len(dst_array, len);
               dst_items = slice_array__get_items(dst_array);
          }
     }

     t_any fn_args[3];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__int}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[1].qwords[0] = idx;
          fn_args[2]           = items[idx];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[2], owner);
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               if (ref_cnt == 1) {
                    items[idx].raw_bits = 0;
                    ref_cnt__dec__noinlined_part(thrd_data, array);
               } else {
                    for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, dst_items[free_idx]), free_idx -= 1);
                    free((void*)dst_array.qwords[0]);
                    if (state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);
               }

               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
               if (ref_cnt == 1) {
                    items[idx].raw_bits = 0;
                    ref_cnt__dec__noinlined_part(thrd_data, array);
               } else {
                    for (u64 free_idx = idx - 1; free_idx != (u64)-1; ref_cnt__dec(thrd_data, dst_items[free_idx]), free_idx -= 1);
                    free((void*)dst_array.qwords[0]);
                    if (state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);
               }

               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]     = box_items[0];
          dst_items[idx] = box_items[1];
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[1]           = fn_args[0];

     if (ref_cnt == 1)
          new_box_items[0] = array;
     else {
          new_box_items[0] = dst_array;
          if (state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);
     }

     return new_box;
}

static t_any array__prefix_of__own(t_thrd_data* const thrd_data, t_any const array, t_any const prefix, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(prefix.bytes[15] == tid__array);

     u32          const array_slice_offset = slice__get_offset(array);
     u32          const array_slice_len    = slice__get_len(array);
     const t_any* const array_items        = &((const t_any*)slice_array__get_items(array))[array_slice_offset];
     u64          const array_ref_cnt      = get_ref_cnt(array);
     u64          const array_array_len    = slice_array__get_len(array);
     u64          const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     u32          const prefix_slice_offset = slice__get_offset(prefix);
     u32          const prefix_slice_len    = slice__get_len(prefix);
     const t_any* const prefix_items        = &((const t_any*)slice_array__get_items(prefix))[prefix_slice_offset];
     u64          const prefix_ref_cnt      = get_ref_cnt(prefix);
     u64          const prefix_array_len    = slice_array__get_len(prefix);
     u64          const prefix_len          = prefix_slice_len <= slice_max_len ? prefix_slice_len : prefix_array_len;

     assert(prefix_slice_len <= slice_max_len || prefix_slice_offset == 0);

     if (prefix_len > array_len) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, prefix);
          return bool__false;
     }

     if (prefix.qwords[0] == array.qwords[0]) {
          if (array_ref_cnt > 2) set_ref_cnt(array, array_ref_cnt - 2);
     } else {
          if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);
          if (prefix_ref_cnt > 1) set_ref_cnt(prefix, prefix_ref_cnt - 1);
     }

     t_any result = bool__true;
     for (u64 idx = 0; idx < prefix_len; idx += 1) {
          t_any const eq_result = equal(thrd_data, array_items[idx], prefix_items[idx], owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) {
               result = bool__false;
               break;
          }
     }

     if (prefix.qwords[0] == array.qwords[0]) {
          if (array_ref_cnt == 2) ref_cnt__dec__noinlined_part(thrd_data, array);
     } else {
          if (array_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
          if (prefix_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, prefix);
     }

     return result;
}

[[clang::noinline]]
static t_any common_print(t_thrd_data* const thrd_data, t_any const arg, const char* const owner);

static t_any array__print(t_thrd_data* const thrd_data, t_any const array, const char* const owner) {
     assert(array.bytes[15] == tid__array);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          if (fwrite("[array]", 1, 7, stdout) != 7) [[clang::unlikely]] goto cant_print_label;
          return null;
     }

     u32 const slice_offset = slice__get_offset(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (fwrite("[array ", 1, 7, stdout) != 7) [[clang::unlikely]] goto cant_print_label;

     const t_any* const items = &((const t_any*)slice_array__get_items(array))[slice_offset];

     assume_aligned(items, 16);

     for (u64 idx = 0; idx < len; idx += 1) {
          t_any const print_item_result = common_print(thrd_data, items[idx], owner);
          if (print_item_result.bytes[15] == tid__error) [[clang::unlikely]] return print_item_result;

          const char* str;
          int         str_len;
          if (idx + 1 == len) {
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

static t_any array__qty__own(t_thrd_data* const thrd_data, t_any const array, t_any const looked_item, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32          const slice_offset      = slice__get_offset(array);
     u32          const slice_len         = slice__get_len(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     u64 result = 0;
     for (u64 idx = 0; idx < len; idx += 1) {
          t_any const item      = items_from_offset[idx];
          t_any const eq_result = equal(thrd_data, looked_item, item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          result += eq_result.bytes[0];
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);

     ref_cnt__dec(thrd_data, looked_item);
     return (const t_any){.structure = {.value = result, .type = tid__int}};
}

static t_any array__repeat__own(t_thrd_data* const thrd_data, t_any const part, u64 const count, const char* const owner) {
     assert(part.bytes[15] == tid__array);

     u32 const slice_len      = slice__get_len(part);
     u32 const slice_offset   = slice__get_offset(part);
     u64 const ref_cnt        = get_ref_cnt(part);
     u64 const part_array_len = slice_array__get_len(part);
     u64 const part_len       = slice_len <= slice_max_len ? slice_len : part_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64        array_len;
     bool const overflow   = __builtin_mul_overflow(part_len, count, &array_len);
     if (array_len > array_max_len || overflow) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, part);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (part_len == 0 || count == 0) {
          ref_cnt__dec(thrd_data, part);
          return empty_array;
     }

     if (count == 1) return part;

     t_any array;
     if (ref_cnt == 1) {
          u64 const part_array_cap = slice_array__get_cap(part);
          array                    = part;
          if (part_array_cap < array_len) {
               array.qwords[0] = (u64)aligned_realloc(16, (t_any*)array.qwords[0], (array_len + 1) * 16);
               slice_array__set_cap(array, array_len);
          }

          t_any* const items = slice_array__get_items(array);

          assume_aligned(items, 16);

          for (u64 idx = 0; idx < part_len; ref_cnt__add(thrd_data, items[slice_offset + idx], count - 1, owner), idx += 1);

          if (slice_offset != 0) {
               for (u64 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
               memmove(items, &items[slice_offset], part_len * 16);
          }
          for (u64 idx = slice_offset + part_len; idx < part_array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     } else {
          if (ref_cnt != 0) set_ref_cnt(part, ref_cnt - 1);

          array                          = array__init(thrd_data, array_len, owner);
          const t_any* const part_items  = &((const t_any*)slice_array__get_items(part))[slice_offset];
          t_any*       const array_items = slice_array__get_items(array);
          for (u64 idx = 0; idx < part_len; idx += 1) {
               t_any const item = part_items[idx];
               array_items[idx] = item;
               ref_cnt__add(thrd_data, item, count, owner);
          }
     }

     slice_array__set_len(array, array_len);
     array                            = slice__set_metadata(array, 0, array_len <= slice_max_len ? array_len : slice_max_len + 1);
     t_any* const array_items         = slice_array__get_items(array);
     u64    const part_size_in_string = part_len * 16;

     if (part_len == 1)
          for (u64 part_idx = 1; part_idx < count; array_items[part_idx] = array_items[0], part_idx += 1);
     else for (
          u64 part_idx = 1;
          part_idx < count;

          memcpy(&array_items[part_len * part_idx], &array_items[part_len * (part_idx - 1)], part_size_in_string),
          part_idx += 1
     );

     return array;
}

static t_any array__replace__own(t_thrd_data* const thrd_data, t_any array, t_any const old_item, t_any const new_item, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32    const slice_offset = slice__get_offset(array);
     u32    const slice_len    = slice__get_len(array);
     t_any* const items        = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64    const ref_cnt      = get_ref_cnt(array);
     u64    const array_len    = slice_array__get_len(array);
     u64    const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any* dst = items;
     for (u64 idx = 0; idx < len; idx += 1) {
          t_any const item      = items[idx];
          t_any const eq_result = equal(thrd_data, old_item, item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (ref_cnt == 1) {
               if (eq_result.bytes[0] == 1) {
                    ref_cnt__inc(thrd_data, new_item, owner);
                    ref_cnt__dec(thrd_data, item);
                    items[idx] = new_item;
               }
          } else {
               if (eq_result.bytes[0] == 1) {
                    if (dst == items) {
                         if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

                         array = array__init(thrd_data, len, owner);
                         slice_array__set_len(array, len);
                         array = slice__set_metadata(array, 0, slice_len);
                         dst   = slice_array__get_items(array);
                         for (u64 dst_idx = 0; dst_idx < idx; dst_idx += 1) {
                              t_any const src_item = items[dst_idx];
                              ref_cnt__inc(thrd_data, src_item, owner);
                              dst[dst_idx] = src_item;
                         }
                    }

                    ref_cnt__inc(thrd_data, new_item, owner);
                    dst[idx] = new_item;
               } else if (dst != items) {
                    ref_cnt__inc(thrd_data, item, owner);
                    dst[idx] = item;
               }
          }
     }

     ref_cnt__dec(thrd_data, old_item);
     ref_cnt__dec(thrd_data, new_item);
     return array;
}

static t_any array__replace_part__own(t_thrd_data* const thrd_data, t_any const array, t_any old_part, t_any new_part, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(old_part.bytes[15] == tid__array);
     assume(new_part.bytes[15] == tid__array);

     u32          const old_part_slice_offset = slice__get_offset(old_part);
     u32          const old_part_slice_len    = slice__get_len(old_part);
     const t_any* const old_part_items        = &((const t_any*)slice_array__get_items(old_part))[old_part_slice_offset];
     u64          const old_part_len          = old_part_slice_len <= slice_max_len ? old_part_slice_len : slice_array__get_len(old_part);

     assert(old_part_slice_len <= slice_max_len || old_part_slice_offset == 0);

     u32          const new_part_slice_offset = slice__get_offset(new_part);
     u32          const new_part_slice_len    = slice__get_len(new_part);
     const t_any* const new_part_items        = &((const t_any*)slice_array__get_items(new_part))[new_part_slice_offset];
     u64          const new_part_len          = new_part_slice_len <= slice_max_len ? new_part_slice_len : slice_array__get_len(new_part);

     assert(new_part_slice_len <= slice_max_len || new_part_slice_offset == 0);

     u32    const array_slice_offset = slice__get_offset(array);
     u32    const array_slice_len    = slice__get_len(array);
     t_any* const array_items        = &((t_any*)slice_array__get_items(array))[array_slice_offset];
     u64    const array_len          = array_slice_len <= slice_max_len ? array_slice_len : slice_array__get_len(array);

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     if (old_part_len == 0) [[clang::unlikely]] {
          t_any dst_array = array__init(thrd_data, (array_len + 1) * new_part_len + array_len, owner);
          ref_cnt__add(thrd_data, new_part, array_len + 1, owner);
          dst_array = array__concat__own(thrd_data, dst_array, new_part, owner);
          for (u64 idx = 0; idx < array_len; idx += 1) {
               t_any const array_item = array_items[idx];
               ref_cnt__inc(thrd_data, array_item, owner);
               dst_array = array__append__own(thrd_data, dst_array, array_item, owner);
               dst_array = array__concat__own(thrd_data, dst_array, new_part, owner);
          }

          ref_cnt__dec(thrd_data, old_part);
          ref_cnt__dec(thrd_data, new_part);
          ref_cnt__dec(thrd_data, array);
          return dst_array;
     }

     u64    dst_len;
     u64    dst_cap;
     t_any* dst_items;
     t_any  dst_array       = array;
     u64    occurrences_qty = 0;

     i64 const edge = array_len - old_part_len + 1;
     for (u64 array_idx = 0; (i64)array_idx < edge; array_idx += 1) {
          for (u64 old_idx = 0; old_idx < old_part_len; old_idx += 1) {
               t_any const eq_result = equal(thrd_data, array_items[array_idx + old_idx], old_part_items[old_idx], owner);

               assert(eq_result.bytes[15] == tid__bool);

               if (eq_result.bytes[0] == 0) goto main_loop;
          }

          if (old_part_len == new_part_len) {
               ref_cnt__inc(thrd_data, new_part, owner);
               dst_array = array__copy__own(thrd_data, dst_array, array_idx, new_part, owner);
          } else {
               u64 const dst_new_len = array_idx + (new_part_len - old_part_len) * occurrences_qty + new_part_len;

               if (occurrences_qty == 0) {
                    dst_array = array__init(thrd_data, array_len - old_part_len + new_part_len, owner);
                    dst_items = slice_array__get_items(dst_array);
                    dst_len   = 0;
                    dst_cap   = slice_array__get_cap(dst_array);
               } else if (dst_new_len > dst_cap) {
                    if (dst_new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);

                    dst_cap             = dst_new_len * 3 / 2;
                    dst_cap             = dst_cap > array_max_len ? dst_new_len : dst_cap;
                    dst_array.qwords[0] = (u64)aligned_realloc(16, (t_any*)dst_array.qwords[0], (dst_cap + 1) * 16);
                    dst_items           = slice_array__get_items(dst_array);
               }

               u64          const prefix_len          = dst_new_len - dst_len - new_part_len;
               const t_any* const prefix_items        = &array_items[array_idx - prefix_len];
               t_any*       const prefix_items_in_dst = &dst_items[dst_len];
               for (u64 prefix_idx = 0; prefix_idx < prefix_len; prefix_idx += 1) {
                    t_any const prefix_item         = prefix_items[prefix_idx];
                    prefix_items_in_dst[prefix_idx] = prefix_item;
                    ref_cnt__inc(thrd_data, prefix_item, owner);
               }
               dst_len += prefix_len;

               t_any* const new_part_items_in_dst = &dst_items[dst_len];
               for (u64 new_part_idx = 0; new_part_idx < new_part_len; new_part_idx += 1) {
                    t_any const new_part_item           = new_part_items[new_part_idx];
                    new_part_items_in_dst[new_part_idx] = new_part_item;
                    ref_cnt__inc(thrd_data, new_part_item, owner);
               }
               dst_len += new_part_len;
          }

          occurrences_qty += 1;
          array_idx       += old_part_len - 1;

          main_loop:
     }

     ref_cnt__dec(thrd_data, old_part);
     ref_cnt__dec(thrd_data, new_part);
     if (occurrences_qty == 0) return array;

     if (old_part_len != new_part_len) {
          u64 const last_part_len = array_len - (old_part_len - new_part_len) * occurrences_qty - dst_len;
          if (last_part_len != 0) {
               if (dst_len + last_part_len > dst_cap) {
                    dst_cap = dst_len + last_part_len;
                    if (dst_cap > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);

                    dst_array.qwords[0] = (u64)aligned_realloc(16, (t_any*)dst_array.qwords[0], (dst_cap + 1) * 16);
                    dst_items           = slice_array__get_items(dst_array);
               }

               const t_any* const last_part_items        = &array_items[array_len - last_part_len];
               t_any*       const last_part_items_in_dst = &dst_items[dst_len];
               for (u64 last_part_idx = 0; last_part_idx < last_part_len; last_part_idx += 1) {
                    t_any const last_part_item            = last_part_items[last_part_idx];
                    last_part_items_in_dst[last_part_idx] = last_part_item;
                    ref_cnt__inc(thrd_data, last_part_item, owner);
               }

               dst_len += last_part_len;
          }

          ref_cnt__dec(thrd_data, array);

          if (dst_len == 0) {
               ref_cnt__dec(thrd_data, dst_array);
               dst_array = empty_array;
          } else {
               dst_array = slice__set_metadata(dst_array, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
               slice_array__set_len(dst_array, dst_len);
               slice_array__set_cap(dst_array, dst_cap);
          }
     }
     return dst_array;
}

static t_any array__reserve__own(t_thrd_data* const thrd_data, t_any array, u64 const new_items_qty, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const current_cap  = slice_array__get_cap(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);
     assert(array_len <= current_cap);

     u64 const minimum_cap = len + new_items_qty;
     if (minimum_cap > array_max_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (current_cap >= minimum_cap) return array;

     u64 new_cap = minimum_cap * 3 / 2;
     new_cap     = new_cap > array_max_len ? array_max_len : new_cap;

     if (ref_cnt == 1) {
          slice_array__set_cap(array, new_cap);
          array.qwords[0] = (u64)realloc((void*)array.qwords[0], (new_cap + 1) * 16);
          return array;
     }

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any new_array = array__init(thrd_data, new_cap, owner);
     slice_array__set_len(new_array, len);
     new_array       = slice__set_metadata(new_array, 0, slice_len);

     const t_any* const old_items = &((const t_any*)slice_array__get_items(array))[slice_offset];
     t_any*       const new_items = slice_array__get_items(new_array);
     if (ref_cnt == 0) memcpy(new_items, old_items, len * 16);
     else for (u64 idx = 0; idx < len; idx += 1) {
          t_any const item = old_items[idx];
          new_items[idx]   = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     return new_array;
}

static t_any array__reverse__own(t_thrd_data* const thrd_data, t_any const array, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32 const slice_len = slice__get_len(array);
     if (slice_len == 0) {
          ref_cnt__dec(thrd_data, array);
          return empty_array;
     }

     u32    const slice_offset = slice__get_offset(array);
     t_any* const items        = &((t_any*)slice_array__get_items(array))[slice_offset];
     u64    const ref_cnt      = get_ref_cnt(array);
     u64    const array_len    = slice_array__get_len(array);
     u64    const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     typedef u64 v_8_u64 __attribute__((ext_vector_type(8), aligned(16)));
     typedef u64 v_4_u64 __attribute__((ext_vector_type(4), aligned(16)));

     if (ref_cnt == 1) {
          u64 idx = 0;
          for (; len > 4 + idx * 2; idx += 4) {
               v_8_u64* const part_from_begin_ptr = (v_8_u64*)&items[idx];
               v_8_u64  const part_from_begin     = *part_from_begin_ptr;
               v_8_u64* const part_from_end_ptr   = (v_8_u64*)&items[len - idx - 4];
               v_8_u64  const part_from_end       = *part_from_end_ptr;

               *part_from_begin_ptr = part_from_end.s67452301;
               *part_from_end_ptr   = part_from_begin.s67452301;
          }

          u8 const remain_items = len - idx * 2;
          switch (remain_items) {
          case 4: case 3: {
               v_4_u64* const part_from_begin_ptr = (v_4_u64*)&items[idx];
               v_4_u64  const part_from_begin     = *part_from_begin_ptr;
               v_4_u64* const part_from_end_ptr   = (v_4_u64*)&items[idx + remain_items - 2];
               v_4_u64  const part_from_end       = *part_from_end_ptr;

               *part_from_begin_ptr = part_from_end.s2301;
               *part_from_end_ptr   = part_from_begin.s2301;
               break;
          }
          case 2: {
               t_any const item = items[idx];
               items[idx]       = items[idx + 1];
               items[idx + 1]   = item;
               break;
          }
          default:
               assert((i8)remain_items < 2);
               break;
          }

          return array;
     }

     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any reversed_array        = array__init(thrd_data, len, owner);
     reversed_array              = slice__set_metadata(reversed_array, 0, slice_len);
     slice_array__set_len(reversed_array, len);
     t_any* const reversed_items = slice_array__get_items(reversed_array);

     for (u64 idx = 0; idx < len; idx += 1) {
          t_any const item              = items[idx];
          reversed_items[len - idx - 1] = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     return reversed_array;
}

static t_any array__self_copy__own(t_thrd_data* const thrd_data, t_any array, u64 const idx_from, u64 const idx_to, u64 const part_len, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32 const slice_len       = slice__get_len(array);
     u64 const ref_cnt         = get_ref_cnt(array);
     u64 const array_array_len = slice_array__get_len(array);
     u64 const array_len       = slice_len <= slice_max_len ? slice_len : array_array_len;

     if (idx_from > array_len || idx_to > array_len || part_len > array_len || idx_from + part_len > array_len || idx_to + part_len > array_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (part_len == 0 || idx_from == idx_to) return array;
     if (ref_cnt != 1) array = array__dup__own(thrd_data, array, owner);

     t_any* const items = &((t_any*)slice_array__get_items(array))[slice__get_offset(array)];
     if (idx_from > idx_to)
          for (u64 part_idx = 0; part_idx < part_len; part_idx += 1) {
               t_any  const from_item = items[idx_from + part_idx];
               ref_cnt__inc(thrd_data, from_item, owner);
               t_any* const to_item_ptr = &items[idx_to + part_idx];
               ref_cnt__dec(thrd_data, *to_item_ptr);
               *to_item_ptr = from_item;
          }
     else
          for (u64 part_idx = part_len - 1; part_idx != (u64)-1; part_idx -= 1) {
               t_any  const from_item = items[idx_from + part_idx];
               ref_cnt__inc(thrd_data, from_item, owner);
               t_any* const to_item_ptr = &items[idx_to + part_idx];
               ref_cnt__dec(thrd_data, *to_item_ptr);
               *to_item_ptr = from_item;
          }

     return array;
}

[[clang::always_inline]]
static void items__swap(t_any* const items, u64 const idx_1, u64 const idx_2) {
     t_any const buffer = items[idx_1];
     items[idx_1]       = items[idx_2];
     items[idx_2]       = buffer;
}

static t_any items__heap_sort__to_heap(t_thrd_data* const thrd_data, t_any* const items, u64 currend_idx, u64 const begin_idx, u64 const end_edge, bool const is_asc_order, const char* const owner) {
     while (true) {
          u64       largest_idx = currend_idx;
          u64 const left_idx    = 2 * currend_idx - begin_idx + 1;
          u64 const right_idx   = left_idx + 1;
          if (right_idx < end_edge) {
               t_any cmp_result = cmp(thrd_data, items[largest_idx], items[right_idx], owner);

               assert(cmp_result.bytes[15] == tid__token);

               switch (cmp_result.bytes[0]) {
               case less_id: case great_id:
                    if ((cmp_result.bytes[0] == less_id) == is_asc_order) largest_idx = right_idx;
               case equal_id:
                    break;

               [[clang::unlikely]]
               case not_equal_id:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case nan_cmp_id:
                    goto nan_cmp_label;
               default:
                    unreachable;
               }

               cmp_result = cmp(thrd_data, items[largest_idx], items[left_idx], owner);

               assert(cmp_result.bytes[15] == tid__token);

               switch (cmp_result.bytes[0]) {
               case less_id: case great_id:
                    if ((cmp_result.bytes[0] == less_id) == is_asc_order) largest_idx = left_idx;
               case equal_id:
                    break;

               [[clang::unlikely]]
               case not_equal_id:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case nan_cmp_id:
                    goto nan_cmp_label;
               default:
                    unreachable;
               }
          } else if (left_idx < end_edge) {
               t_any const cmp_result = cmp(thrd_data, items[largest_idx], items[left_idx], owner);

               assert(cmp_result.bytes[15] == tid__token);

               switch (cmp_result.bytes[0]) {
               case less_id: case great_id:
                    if ((cmp_result.bytes[0] == less_id) == is_asc_order) largest_idx = left_idx;
               case equal_id:
                    break;

               [[clang::unlikely]]
               case not_equal_id:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case nan_cmp_id:
                    goto nan_cmp_label;
               default:
                    unreachable;
               }
          }

          if (largest_idx == currend_idx) break;

          items__swap(items, currend_idx, largest_idx);
          currend_idx = largest_idx;
     }
     return null;

     cant_ord_label:
     return error__cant_ord(thrd_data, owner);

     nan_cmp_label:
     return error__nan_cmp(thrd_data, owner);
}

static t_any items__heap_sort(t_thrd_data* const thrd_data, t_any* const items, u64 const begin_idx, u64 const end_edge, bool const is_asc_order, const char* const owner) {
     for (u64 currend_idx = (end_edge - begin_idx) / 2 + begin_idx - 1; (i64)currend_idx >= (i64)begin_idx; currend_idx -= 1) {
          t_any const to_heap_result = items__heap_sort__to_heap(thrd_data, items, currend_idx, begin_idx, end_edge, is_asc_order, owner);
          if (to_heap_result.bytes[15] == tid__error) [[clang::unlikely]] return to_heap_result;
     }

     for (u64 current_edge = end_edge - 1; (i64)current_edge >= (i64)begin_idx; current_edge -= 1) {
          items__swap(items, begin_idx, current_edge);
          t_any const to_heap_result = items__heap_sort__to_heap(thrd_data, items, begin_idx, begin_idx, current_edge, is_asc_order, owner);
          if (to_heap_result.bytes[15] == tid__error) [[clang::unlikely]] return to_heap_result;
     }

     return null;
}

static t_any items__quick_sort(t_thrd_data* const thrd_data, t_any* const items, u64 const begin_idx, u64 const end_edge, u64 const rnd_num, bool const is_asc_order, const char* const owner) {
     u64 left_idx   = begin_idx;
     u64 right_edge = end_edge;
     while (true) {
          u64 const range_len    = right_edge - left_idx;
          u64       big_left_idx = -1;
          u64       big_right_edge;
          u64       small_left_idx;
          u64       small_right_edge;
          u64       small_range_len;
          if (range_len > 15) {
               u64 const range_len_25_percent = range_len / 4;
               u64       rnd_offset           = range_len & rnd_num;
               rnd_offset                     = rnd_offset < range_len_25_percent ? rnd_offset + range_len_25_percent : rnd_offset > range_len_25_percent * 3 ? rnd_offset - range_len_25_percent : rnd_offset;
               items__swap(items, left_idx, left_idx + rnd_offset);

               t_any const current_item = items[left_idx];
               u64         currend_idx  = left_idx;
               u64         opposite_idx = right_edge - 1;
               u64         direction    = -1;
               bool        cmp_modifier = is_asc_order;
               while (true) {
                    t_any const opposite_item = items[opposite_idx];
                    t_any const cmp_result = cmp(thrd_data, current_item, opposite_item, owner);

                    assert(cmp_result.bytes[15] == tid__token);

                    switch (cmp_result.bytes[0]) {
                    case less_id: case great_id:
                         if ((cmp_result.bytes[0] == less_id) == cmp_modifier) break;
                    case equal_id: {
                         items__swap(items, currend_idx, opposite_idx);

                         u64 const swap_buffer = currend_idx;
                         currend_idx           = opposite_idx;
                         opposite_idx          = swap_buffer;

                         direction    = -direction;
                         cmp_modifier = !cmp_modifier;
                         break;
                    }

                    [[clang::unlikely]]
                    case not_equal_id:
                         goto cant_ord_label;
                    [[clang::unlikely]]
                    case nan_cmp_id:
                         goto nan_cmp_label;
                    default:
                         unreachable;
                    }

                    i64 const remain_len = currend_idx - opposite_idx;
                    if (remain_len >= -1 && remain_len <= 1) break;

                    opposite_idx += direction;
               }

               u64 const left_range_len  = currend_idx - left_idx;
               u64 const right_range_len = right_edge - currend_idx - 1;
               if (left_range_len < right_range_len) {
                    small_left_idx   = left_idx;
                    small_right_edge = currend_idx;
                    small_range_len  = left_range_len;
                    big_left_idx     = currend_idx + 1;
                    big_right_edge   = right_edge;
               } else {
                    small_left_idx   = currend_idx + 1;
                    small_right_edge = right_edge;
                    small_range_len  = right_range_len;
                    big_left_idx     = left_idx;
                    big_right_edge   = currend_idx;
               }

               left_idx   = big_left_idx;
               right_edge = big_right_edge;
          } else {
               small_left_idx   = left_idx;
               small_right_edge = right_edge;
               small_range_len  = range_len;
          }

          if (small_range_len > 15) {
               t_any const sort_result = items__quick_sort(thrd_data, items, small_left_idx, small_right_edge, rnd_num, is_asc_order, owner);
               if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;
          } else if (small_range_len > 2) {
               t_any const sort_result = items__heap_sort(thrd_data, items, small_left_idx, small_right_edge, is_asc_order, owner);
               if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;
          } else if (small_range_len == 2) {
               t_any const cmp_result = cmp(thrd_data, items[small_left_idx], items[small_left_idx + 1], owner);

               assert(cmp_result.bytes[15] == tid__token);

               switch (cmp_result.bytes[0]) {
               case less_id: case great_id:
                    if ((cmp_result.bytes[0] == great_id) == is_asc_order) items__swap(items, small_left_idx, small_left_idx + 1);
               case equal_id:
                    break;

               [[clang::unlikely]]
               case not_equal_id:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case nan_cmp_id:
                    goto nan_cmp_label;
               default:
                    unreachable;
               }
          }

          if (big_left_idx == -1) break;
     }

     return null;

     cant_ord_label:
     return error__cant_ord(thrd_data, owner);

     nan_cmp_label:
     return error__nan_cmp(thrd_data, owner);
}

static t_any array__sort__own(t_thrd_data* const thrd_data, t_any array, bool const is_asc_order, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32 const slice_len = slice__get_len(array);
     if (slice_len < 2) return array;

     u64 const ref_cnt   = get_ref_cnt(array);
     u64 const array_len = slice_array__get_len(array);
     u64 const len       = slice_len <= slice_max_len ? slice_len : array_len;
     if (ref_cnt != 1) array = array__dup__own(thrd_data, array, owner);

     t_any* const items       = &((t_any*)slice_array__get_items(array))[slice__get_offset(array)];
     t_any  const sort_result = items__quick_sort(thrd_data, items, 0, len, static_rnd_num, is_asc_order, owner);
     if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec__noinlined_part(thrd_data, array);
          return sort_result;
     }

     return array;
}

static t_any items__heap_sort_fn__to_heap(t_thrd_data* const thrd_data, t_any* const items, t_any const fn, u64 currend_idx, u64 const begin_idx, u64 const end_edge, const char* const owner) {
     t_any fn_args[2];

     while (true) {
          u64       largest_idx = currend_idx;
          u64 const left_idx    = 2 * currend_idx - begin_idx + 1;
          u64 const right_idx   = left_idx + 1;
          if (right_idx < end_edge) {
               fn_args[0] = items[largest_idx];
               fn_args[1] = items[right_idx];
               ref_cnt__inc(thrd_data, fn_args[0], owner);
               ref_cnt__inc(thrd_data, fn_args[1], owner);
               t_any cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__less:
                    largest_idx = right_idx;
               case comptime_tkn__great: case comptime_tkn__equal:
                    break;

               [[clang::unlikely]]
               case comptime_tkn__not_equal:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case comptime_tkn__nan_cmp:
                    goto nan_cmp_label;
               [[clang::unlikely]]
               default:
                    if (cmp_result.bytes[15] != tid__token) {
                         if (cmp_result.bytes[15] == tid__error) return cmp_result;
                         ref_cnt__dec(thrd_data, cmp_result);
                         goto invalid_type_label;
                    }

                    goto ret_incorrect_value_label;
               }

               fn_args[0] = items[largest_idx];
               fn_args[1] = items[left_idx];
               ref_cnt__inc(thrd_data, fn_args[0], owner);
               ref_cnt__inc(thrd_data, fn_args[1], owner);
               cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__less:
                    largest_idx = left_idx;
               case comptime_tkn__great: case comptime_tkn__equal:
                    break;

               [[clang::unlikely]]
               case comptime_tkn__not_equal:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case comptime_tkn__nan_cmp:
                    goto nan_cmp_label;
               [[clang::unlikely]]
               default:
                    if (cmp_result.bytes[15] != tid__token) {
                         if (cmp_result.bytes[15] == tid__error) return cmp_result;
                         ref_cnt__dec(thrd_data, cmp_result);
                         goto invalid_type_label;
                    }

                    goto ret_incorrect_value_label;
               }
          } else if (left_idx < end_edge) {
               fn_args[0] = items[largest_idx];
               fn_args[1] = items[left_idx];
               ref_cnt__inc(thrd_data, fn_args[0], owner);
               ref_cnt__inc(thrd_data, fn_args[1], owner);
               t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__less:
                    largest_idx = left_idx;
               case comptime_tkn__great: case comptime_tkn__equal:
                    break;

               [[clang::unlikely]]
               case comptime_tkn__not_equal:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case comptime_tkn__nan_cmp:
                    goto nan_cmp_label;
               [[clang::unlikely]]
               default:
                    if (cmp_result.bytes[15] != tid__token) {
                         if (cmp_result.bytes[15] == tid__error) return cmp_result;
                         ref_cnt__dec(thrd_data, cmp_result);
                         goto invalid_type_label;
                    }

                    goto ret_incorrect_value_label;
               }
          }

          if (largest_idx == currend_idx) break;

          items__swap(items, currend_idx, largest_idx);
          currend_idx = largest_idx;
     }
     return null;

     invalid_type_label:
     return error__invalid_type(thrd_data, owner);

     cant_ord_label:
     return error__cant_ord(thrd_data, owner);

     nan_cmp_label:
     return error__nan_cmp(thrd_data, owner);

     ret_incorrect_value_label:
     return error__ret_incorrect_value(thrd_data, owner);
}

static t_any items__heap_sort_fn(t_thrd_data* const thrd_data, t_any* const items, t_any const fn, u64 const begin_idx, u64 const end_edge, const char* const owner) {
     for (u64 currend_idx = (end_edge - begin_idx) / 2 + begin_idx - 1; (i64)currend_idx >= (i64)begin_idx; currend_idx -= 1) {
          t_any const to_heap_result = items__heap_sort_fn__to_heap(thrd_data, items, fn, currend_idx, begin_idx, end_edge, owner);
          if (to_heap_result.bytes[15] == tid__error) [[clang::unlikely]] return to_heap_result;
     }

     for (u64 current_edge = end_edge - 1; (i64)current_edge >= (i64)begin_idx; current_edge -= 1) {
          items__swap(items, begin_idx, current_edge);
          t_any const to_heap_result = items__heap_sort_fn__to_heap(thrd_data, items, fn, begin_idx, begin_idx, current_edge, owner);
          if (to_heap_result.bytes[15] == tid__error) [[clang::unlikely]] return to_heap_result;
     }

     return null;
}

static t_any items__quick_sort_fn(t_thrd_data* const thrd_data, t_any* const items, t_any const fn, u64 const begin_idx, u64 const end_edge, u64 const rnd_num, const char* const owner) {
     t_any fn_args[2];

     u64 left_idx   = begin_idx;
     u64 right_edge = end_edge;
     while (true) {
          u64 const range_len    = right_edge - left_idx;
          u64       big_left_idx = -1;
          u64       big_right_edge;
          u64       small_left_idx;
          u64       small_right_edge;
          u64       small_range_len;
          if (range_len > 16) {
               u64 const range_len_25_percent = range_len / 4;
               u64       rnd_offset           = range_len & rnd_num;
               rnd_offset                     = rnd_offset < range_len_25_percent ? rnd_offset + range_len_25_percent : rnd_offset > range_len_25_percent * 3 ? rnd_offset - range_len_25_percent : rnd_offset;
               items__swap(items, left_idx, left_idx + rnd_offset);

               fn_args[0]        = items[left_idx];
               u64  currend_idx  = left_idx;
               u64  opposite_idx = right_edge - 1;
               u64  direction    = -1;
               bool cmp_modifier = true;
               while (true) {
                    fn_args[1] = items[opposite_idx];
                    ref_cnt__inc(thrd_data, fn_args[0], owner);
                    ref_cnt__inc(thrd_data, fn_args[1], owner);
                    t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

                    switch (cmp_result.raw_bits) {
                    case comptime_tkn__less: case comptime_tkn__great:
                         if ((cmp_result.bytes[0] == less_id) == cmp_modifier) break;
                    case comptime_tkn__equal: {
                         items__swap(items, currend_idx, opposite_idx);

                         u64 const swap_buffer = currend_idx;
                         currend_idx           = opposite_idx;
                         opposite_idx          = swap_buffer;

                         direction    = -direction;
                         cmp_modifier = !cmp_modifier;
                         break;
                    }

                    [[clang::unlikely]]
                    case comptime_tkn__not_equal:
                         goto cant_ord_label;
                    [[clang::unlikely]]
                    case comptime_tkn__nan_cmp:
                         goto nan_cmp_label;
                    [[clang::unlikely]]
                    default:
                         if (cmp_result.bytes[15] != tid__token) {
                              if (cmp_result.bytes[15] == tid__error) return cmp_result;
                              ref_cnt__dec(thrd_data, cmp_result);
                              goto invalid_type_label;
                         }

                         goto ret_incorrect_value_label;
                    }

                    i64 const remain_len = currend_idx - opposite_idx;
                    if (remain_len >= -1 && remain_len <= 1) break;

                    opposite_idx += direction;
               }

               u64 const left_range_len  = currend_idx - left_idx;
               u64 const right_range_len = right_edge - currend_idx - 1;
               if (left_range_len < right_range_len) {
                    small_left_idx   = left_idx;
                    small_right_edge = currend_idx;
                    small_range_len  = left_range_len;
                    big_left_idx     = currend_idx + 1;
                    big_right_edge   = right_edge;
               } else {
                    small_left_idx   = currend_idx + 1;
                    small_right_edge = right_edge;
                    small_range_len  = right_range_len;
                    big_left_idx     = left_idx;
                    big_right_edge   = currend_idx;
               }

               left_idx   = big_left_idx;
               right_edge = big_right_edge;
          } else {
               small_left_idx   = left_idx;
               small_right_edge = right_edge;
               small_range_len  = range_len;
          }

          if (small_range_len > 16) {
               t_any const sort_result = items__quick_sort_fn(thrd_data, items, fn, small_left_idx, small_right_edge, rnd_num, owner);
               if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;
          } else if (small_range_len > 2) {
               t_any const sort_result = items__heap_sort_fn(thrd_data, items, fn, small_left_idx, small_right_edge, owner);
               if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;
          } else if (small_range_len == 2) {
               ref_cnt__inc(thrd_data, items[small_left_idx], owner);
               ref_cnt__inc(thrd_data, items[small_left_idx + 1], owner);
               t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, &items[small_left_idx], 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__great:
                    items__swap(items, small_left_idx, small_left_idx + 1);
               case comptime_tkn__less: case comptime_tkn__equal:
                    break;

               [[clang::unlikely]]
               case comptime_tkn__not_equal:
                    goto cant_ord_label;
               [[clang::unlikely]]
               case comptime_tkn__nan_cmp:
                    goto nan_cmp_label;
               [[clang::unlikely]]
               default:
                    if (cmp_result.bytes[15] != tid__token) {
                         if (cmp_result.bytes[15] == tid__error) return cmp_result;
                         ref_cnt__dec(thrd_data, cmp_result);
                         goto invalid_type_label;
                    }

                    goto ret_incorrect_value_label;
               }
          }

          if (big_left_idx == -1) break;
     }

     return null;

     invalid_type_label:
     return error__invalid_type(thrd_data, owner);

     cant_ord_label:
     return error__cant_ord(thrd_data, owner);

     nan_cmp_label:
     return error__nan_cmp(thrd_data, owner);

     ret_incorrect_value_label:
     return error__ret_incorrect_value(thrd_data, owner);
}

static t_any array__sort_fn__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len < 2) {
          ref_cnt__dec(thrd_data, fn);
          return array;
     }

     u64 const ref_cnt   = get_ref_cnt(array);
     u64 const array_len = slice_array__get_len(array);
     u64 const len       = slice_len <= slice_max_len ? slice_len : array_len;
     if (ref_cnt != 1) array = array__dup__own(thrd_data, array, owner);

     t_any* const items       = &((t_any*)slice_array__get_items(array))[slice__get_offset(array)];
     t_any  const sort_result = items__quick_sort_fn(thrd_data, items, fn, 0, len, static_rnd_num, owner);
     ref_cnt__dec(thrd_data, fn);
     if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec__noinlined_part(thrd_data, array);
          return sort_result;
     }

     return array;
}

static t_any array__split__own(t_thrd_data* const thrd_data, t_any const array, t_any const separator, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32          const slice_offset = slice__get_offset(array);
     u32          const slice_len    = slice__get_len(array);
     const t_any* const items        = &((const t_any*)slice_array__get_items(array))[slice_offset];
     u64          const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any result         = array__init(thrd_data, 1, owner);
     u64   part_first_idx = 0;
     while (true) {
          u64 separator_idx = part_first_idx;
          for (; separator_idx < len; separator_idx += 1) {
               t_any const eq_result = equal(thrd_data, separator, items[separator_idx], owner);

               assert(eq_result.bytes[15] == tid__bool);

               if (eq_result.bytes[0] == 1) break;
          }

          ref_cnt__inc(thrd_data, array, owner);
          t_any const part = array__slice__own(thrd_data, array, part_first_idx, separator_idx, owner);
          part_first_idx   = separator_idx + 1;

          assert(part.bytes[15] == tid__array);

          result = array__append__own(thrd_data, result, part, owner);
          if (separator_idx == len) break;
     }

     ref_cnt__dec(thrd_data, array);
     ref_cnt__dec(thrd_data, separator);
     return result;
}

static t_any array__split_by_part__own(t_thrd_data* const thrd_data, t_any const array, t_any const separator, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(separator.bytes[15] == tid__array);

     u32          const array_slice_offset = slice__get_offset(array);
     u32          const array_slice_len    = slice__get_len(array);
     const t_any* const array_items        = &((const t_any*)slice_array__get_items(array))[array_slice_offset];
     u64          const array_len          = array_slice_len <= slice_max_len ? array_slice_len : slice_array__get_len(array);

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     u32          const separator_slice_offset = slice__get_offset(separator);
     u32          const separator_slice_len    = slice__get_len(separator);
     const t_any* const separator_items        = &((const t_any*)slice_array__get_items(separator))[separator_slice_offset];
     u64          const separator_len          = separator_slice_len <= slice_max_len ? separator_slice_len : slice_array__get_len(separator);

     assert(separator_slice_len <= slice_max_len || separator_slice_offset == 0);

     if (separator_len == 0) {
          ref_cnt__dec(thrd_data, separator);
          if (array_len == 0) return array;

          t_any        result = array__init(thrd_data, array_len, owner);
          result              = slice__set_metadata(result, 0, array_slice_len);
          slice_array__set_len(result, array_len);
          t_any* const parts  = slice_array__get_items(result);
          ref_cnt__add(thrd_data, array, array_len, owner);
          for (u64 part_idx = 0; part_idx < array_len; part_idx += 1) {
               parts[part_idx] = array__slice__own(thrd_data, array, part_idx, part_idx + 1, owner);
          }

          ref_cnt__dec(thrd_data, array);
          return result;
     }

     t_any       result               = array__init(thrd_data, 1, owner);
     u64         part_first_idx       = 0;
     i64   const edge                 = array_len - separator_len + 1;
     while (true) {
          u64 separator_idx = array_len;
          for (u64 array_idx = part_first_idx; (i64)array_idx < edge; array_idx += 1) {
               for (u64 cmp_idx = 0; cmp_idx < separator_len; cmp_idx += 1) {
                    t_any const eq_result = equal(thrd_data, array_items[array_idx + cmp_idx], separator_items[cmp_idx], owner);

                    assert(eq_result.bytes[15] == tid__bool);

                    if (eq_result.bytes[0] == 0) goto main_loop;
               }

               separator_idx = array_idx;
               break;

               main_loop:
          }

          ref_cnt__inc(thrd_data, array, owner);
          t_any const part = array__slice__own(thrd_data, array, part_first_idx, separator_idx, owner);
          part_first_idx   = separator_idx + separator_len;

          assert(part.bytes[15] == tid__array);

          result = array__append__own(thrd_data, result, part, owner);
          if (separator_idx == array_len) break;
     }

     ref_cnt__dec(thrd_data, array);
     ref_cnt__dec(thrd_data, separator);
     return result;
}

static void items__swap_blocks(t_any* const items, u64 const left_begin, u64 const right_begin, u64 const len) {
     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     u8 buffer[64];

     t_any* dst          = &items[left_begin];
     t_any* src          = &items[right_begin];
     u64    remain_items = len;
     for (; remain_items >= 4; dst = &dst[4], src = &src[4], remain_items -= 4) {
          memcpy_inline(buffer, dst, 64);
          memcpy_inline(dst, src, 64);
          memcpy_inline(src, buffer, 64);
     }

     switch (remain_items) {
     case 3:
          memcpy_inline(buffer, dst, 48);
          memcpy_inline(dst, src, 48);
          memcpy_inline(src, buffer, 48);
          break;
     case 2:
          memcpy_inline(buffer, dst, 32);
          memcpy_inline(dst, src, 32);
          memcpy_inline(src, buffer, 32);
          break;
     case 1:
          memcpy_inline(buffer, dst, 16);
          memcpy_inline(dst, src, 16);
          memcpy_inline(src, buffer, 16);
     case 0:
          break;
     default:
          unreachable;
     }
}

static void items__rotate_blocks(t_any* const items, u64 const left_begin, u64 const right_begin, u64 const right_end) {
     u64  left_size  = right_begin - left_begin;
     u64  right_size = right_end - right_begin;
     u64  small_block_begin;
     u64  small_block_size;
     u64* big_block_size;
     while (left_size != right_size) {
          u64 const swap_left_begin = right_begin - left_size;
          if (left_size > right_size) {
               small_block_begin = right_begin;
               small_block_size  = right_size;
               big_block_size    = &left_size;
          } else {
               small_block_begin = swap_left_begin + right_size;
               small_block_size  = left_size;
               big_block_size    = &right_size;
          }
          items__swap_blocks(items, swap_left_begin, small_block_begin, small_block_size);
          *big_block_size -= small_block_size;
     }

     items__swap_blocks(items, right_begin - left_size, right_begin, left_size);
}

static t_any items__merge_blocks(t_thrd_data* const thrd_data, t_any* const items, u64 left_begin, u64 right_begin, u64 right_end, bool const is_asc_order, const char* const owner) {
     while (true) {
          u64 const ro_left_begin  = left_begin;
          u64 const ro_right_begin = right_begin;
          u64 const ro_right_end   = right_end;

          if (!((ro_right_begin - ro_left_begin == 1) || (ro_right_end - ro_right_begin == 1))) {
               u64 const blocks_center_idx                  = (ro_left_begin + ro_right_end) / 2;
               u64 const blocks_center_idx_plus_right_begin = blocks_center_idx + ro_right_begin;
               u64 binary_search_begin;
               u64 binary_search_end;
               if (ro_right_begin > blocks_center_idx) {
                    binary_search_begin = blocks_center_idx_plus_right_begin - ro_right_end;
                    binary_search_end   = blocks_center_idx;
               } else {
                    binary_search_begin = ro_left_begin;
                    binary_search_end   = ro_right_begin;
               }
               while (true) {
                    u64         center_idx  = (binary_search_begin + binary_search_end) / 2;
                    t_any const center_item = items[center_idx];
                    t_any const cmp_result  = cmp(thrd_data, items[blocks_center_idx_plus_right_begin - center_idx - 1], center_item, owner);

                    assert(cmp_result.bytes[15] == tid__token);

                    switch (cmp_result.bytes[0]) {
                    case less_id: case great_id:
                         if ((cmp_result.bytes[0] == less_id) == is_asc_order) {
                              binary_search_end = center_idx;
                              break;
                         }
                    case equal_id:
                         binary_search_begin = center_idx + 1;
                         break;

                    [[clang::unlikely]]
                    case not_equal_id:
                         goto cant_ord_label;
                    [[clang::unlikely]]
                    case nan_cmp_id:
                         goto nan_cmp_label;
                    default:
                         unreachable;
                    }

                    if (binary_search_begin >= binary_search_end) break;
               }

               u64 const center_block_begin = binary_search_begin;
               u64 const center_block_end   = blocks_center_idx_plus_right_begin - center_block_begin;
               if (center_block_begin < ro_right_begin && ro_right_begin < center_block_end)
                    items__rotate_blocks(items, center_block_begin, ro_right_begin, center_block_end);

               bool const merge_first = ro_left_begin < center_block_begin && center_block_begin < blocks_center_idx;
               if (merge_first) {
                    right_begin = center_block_begin;
                    right_end   = blocks_center_idx;
               }

               bool const merge_second = blocks_center_idx < center_block_end && center_block_end < ro_right_end;
               if (merge_second) {
                    if (merge_first) {
                         t_any const merge_result = items__merge_blocks(thrd_data, items, blocks_center_idx, center_block_end, ro_right_end, is_asc_order, owner);
                         if (merge_result.bytes[15] == tid__error) [[clang::unlikely]] return merge_result;
                    } else {
                         left_begin  = blocks_center_idx;
                         right_begin = center_block_end;
                    }
               }

               if (!(merge_first | merge_second)) break;
          } else {
               bool const left_is_one = ro_right_begin - ro_left_begin == 1;

               u64         binary_search_begin = left_is_one ? ro_right_begin : ro_left_begin;
               u64         binary_search_end   = left_is_one ? ro_right_end : ro_right_begin;
               t_any const searched_item       = items[left_is_one ? ro_left_begin : ro_right_begin];
               while (true) {
                    u64   const center_idx  = (binary_search_begin + binary_search_end) / 2;
                    t_any const center_item = items[center_idx];
                    t_any const cmp_result  = cmp(thrd_data, searched_item, center_item, owner);

                    assert(cmp_result.bytes[15] == tid__token);

                    switch (cmp_result.bytes[0]) {
                    case less_id: case great_id:
                         if ((cmp_result.bytes[0] == less_id) == is_asc_order)
                         binary_search_end   = center_idx;
                         else binary_search_begin = center_idx + 1;
                         break;
                    case equal_id:
                         if (left_is_one) binary_search_end   = center_idx;
                         else             binary_search_begin = center_idx + 1;
                         break;

                    [[clang::unlikely]]
                    case not_equal_id:
                         goto cant_ord_label;
                    [[clang::unlikely]]
                    case nan_cmp_id:
                         goto nan_cmp_label;
                    default:
                         unreachable;
                    }

                    if (binary_search_begin >= binary_search_end) break;
               }

               if (left_is_one) {
                    binary_search_begin -= 1;
                    memmove(&items[ro_left_begin], &items[ro_left_begin + 1], (binary_search_begin - ro_left_begin) * 16);
               } else memmove(&items[binary_search_begin + 1], &items[binary_search_begin], (ro_right_begin - binary_search_begin) * 16);
               items[binary_search_begin] = searched_item;
               break;
          }
     }

     return null;

     cant_ord_label:
     return error__cant_ord(thrd_data, owner);

     nan_cmp_label:
     return error__nan_cmp(thrd_data, owner);
}

static t_any items__insertion_sort(t_thrd_data* const thrd_data, t_any* const items, u64 begin, u64 end, bool const is_asc_order, const char* const owner) {
     for (u64 last_idx = begin + 1; last_idx < end; last_idx += 1)
          for (u64 current_idx = last_idx; current_idx > begin; current_idx -= 1) {
               t_any const cmp_result  = cmp(thrd_data, items[current_idx], items[current_idx - 1], owner);

               assert(cmp_result.bytes[15] == tid__token);

               switch (cmp_result.bytes[0]) {
               case less_id: case great_id:
                    if ((cmp_result.bytes[0] == less_id) == is_asc_order) items__swap(items, current_idx, current_idx - 1);
               case equal_id:
                    break;

               [[clang::unlikely]]
               case not_equal_id:
                    return error__cant_ord(thrd_data, owner);
               [[clang::unlikely]]
               case nan_cmp_id:
                    return error__nan_cmp(thrd_data, owner);
               default:
                    unreachable;
               }
          }

     return null;
}

static t_any items__block_merge_sort(t_thrd_data* const thrd_data, t_any* const items, u64 const len, bool const is_asc_order, const char* const owner) {
     u64 block_size  = 16;
     u64 block_begin = 0;
     u64 block_end   = block_size;
     while (block_end <= len) {
          t_any const sort_result = items__insertion_sort(thrd_data, items, block_begin, block_end, is_asc_order, owner);
          if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;

          block_begin = block_end;
          block_end  += block_size;
     }

     t_any const sort_result = items__insertion_sort(thrd_data, items, block_begin, len, is_asc_order, owner);
     if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;

     while (block_size < len) {
          block_begin = 0;
          block_end   = block_size * 2;
          while (block_end <= len) {
               t_any const merge_result = items__merge_blocks(thrd_data, items, block_begin, block_begin + block_size, block_end, is_asc_order, owner);
               if (merge_result.bytes[15] == tid__error) [[clang::unlikely]] return merge_result;

               block_begin = block_end;
               block_end  += block_size * 2;
          }

          u64 const last_block_begin = block_begin + block_size;
          if (last_block_begin < len) {
               t_any const merge_result = items__merge_blocks(thrd_data, items, block_begin, last_block_begin, len, is_asc_order, owner);
               if (merge_result.bytes[15] == tid__error) [[clang::unlikely]] return merge_result;
          }

          block_size *= 2;
     }

     return null;
}

static t_any array__stable_sort__own(t_thrd_data* const thrd_data, t_any array, bool const is_asc_order, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32 const slice_len = slice__get_len(array);
     if (slice_len < 2) return array;

     u64 const ref_cnt   = get_ref_cnt(array);
     u64 const array_len = slice_array__get_len(array);
     u64 const len       = slice_len <= slice_max_len ? slice_len : array_len;
     if (ref_cnt != 1) array = array__dup__own(thrd_data, array, owner);

     t_any* const items       = &((t_any*)slice_array__get_items(array))[slice__get_offset(array)];
     t_any  const sort_result = items__block_merge_sort(thrd_data, items, len, is_asc_order, owner);
     if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec__noinlined_part(thrd_data, array);
          return sort_result;
     }

     return array;
}

static t_any items__merge_blocks_fn(t_thrd_data* const thrd_data, t_any* const items, u64 left_begin, u64 right_begin, u64 right_end, t_any const fn, const char* const owner) {
     t_any fn_args[2];

     while (true) {
          u64 const ro_left_begin  = left_begin;
          u64 const ro_right_begin = right_begin;
          u64 const ro_right_end   = right_end;

          if (!((ro_right_begin - ro_left_begin == 1) || (ro_right_end - ro_right_begin == 1))) {
               u64 const blocks_center_idx                  = (ro_left_begin + ro_right_end) / 2;
               u64 const blocks_center_idx_plus_right_begin = blocks_center_idx + ro_right_begin;
               u64 binary_search_begin;
               u64 binary_search_end;
               if (ro_right_begin > blocks_center_idx) {
                    binary_search_begin = blocks_center_idx_plus_right_begin - ro_right_end;
                    binary_search_end   = blocks_center_idx;
               } else {
                    binary_search_begin = ro_left_begin;
                    binary_search_end   = ro_right_begin;
               }
               while (true) {
                    u64 const center_idx = (binary_search_begin + binary_search_end) / 2;
                    fn_args[0]           = items[blocks_center_idx_plus_right_begin - center_idx - 1];
                    fn_args[1]           = items[center_idx];
                    ref_cnt__inc(thrd_data, fn_args[0], owner);
                    ref_cnt__inc(thrd_data, fn_args[1], owner);
                    t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

                    switch (cmp_result.raw_bits) {
                    case comptime_tkn__less:
                         binary_search_end = center_idx;
                         break;
                    case comptime_tkn__great: case comptime_tkn__equal:
                         binary_search_begin = center_idx + 1;
                         break;

                    [[clang::unlikely]]
                    case comptime_tkn__not_equal:
                         goto cant_ord_label;
                    [[clang::unlikely]]
                    case comptime_tkn__nan_cmp:
                         goto nan_cmp_label;
                    [[clang::unlikely]]
                    default:
                         if (cmp_result.bytes[15] != tid__token) {
                              if (cmp_result.bytes[15] == tid__error) return cmp_result;
                              ref_cnt__dec(thrd_data, cmp_result);
                              goto invalid_type_label;
                         }

                         goto ret_incorrect_value_label;
                    }

                    if (binary_search_begin >= binary_search_end) break;
               }

               u64 const center_block_begin = binary_search_begin;
               u64 const center_block_end   = blocks_center_idx_plus_right_begin - center_block_begin;
               if (center_block_begin < ro_right_begin && ro_right_begin < center_block_end)
                    items__rotate_blocks(items, center_block_begin, ro_right_begin, center_block_end);

               bool const merge_first = ro_left_begin < center_block_begin && center_block_begin < blocks_center_idx;
               if (merge_first) {
                    right_begin = center_block_begin;
                    right_end   = blocks_center_idx;
               }

               bool const merge_second = blocks_center_idx < center_block_end && center_block_end < ro_right_end;
               if (merge_second) {
                    if (merge_first) {
                         t_any const merge_result = items__merge_blocks_fn(thrd_data, items, blocks_center_idx, center_block_end, ro_right_end, fn, owner);
                         if (merge_result.bytes[15] == tid__error) [[clang::unlikely]] return merge_result;
                    } else {
                         left_begin  = blocks_center_idx;
                         right_begin = center_block_end;
                    }
               }

               if (!(merge_first | merge_second)) break;
          } else {
               bool const left_is_one = ro_right_begin - ro_left_begin == 1;

               u64 binary_search_begin = left_is_one ? ro_right_begin : ro_left_begin;
               u64 binary_search_end   = left_is_one ? ro_right_end : ro_right_begin;
               fn_args[0]              = items[left_is_one ? ro_left_begin : ro_right_begin];
               while (true) {
                    u64 const center_idx = (binary_search_begin + binary_search_end) / 2;
                    fn_args[1]           = items[center_idx];
                    ref_cnt__inc(thrd_data, fn_args[0], owner);
                    ref_cnt__inc(thrd_data, fn_args[1], owner);
                    t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

                    switch (cmp_result.raw_bits) {
                    case comptime_tkn__less:
                         binary_search_end   = center_idx;
                         break;
                    case comptime_tkn__great:
                         binary_search_begin = center_idx + 1;
                         break;
                    case comptime_tkn__equal:
                         if (left_is_one) binary_search_end   = center_idx;
                         else             binary_search_begin = center_idx + 1;
                         break;

                    [[clang::unlikely]]
                    case comptime_tkn__not_equal:
                         goto cant_ord_label;
                    [[clang::unlikely]]
                    case comptime_tkn__nan_cmp:
                         goto nan_cmp_label;
                    [[clang::unlikely]]
                    default:
                         if (cmp_result.bytes[15] != tid__token) {
                              if (cmp_result.bytes[15] == tid__error) return cmp_result;
                              ref_cnt__dec(thrd_data, cmp_result);
                              goto invalid_type_label;
                         }

                         goto ret_incorrect_value_label;
                    }

                    if (binary_search_begin >= binary_search_end) break;
               }

               if (left_is_one) {
                    binary_search_begin -= 1;
                    memmove(&items[ro_left_begin], &items[ro_left_begin + 1], (binary_search_begin - ro_left_begin) * 16);
               } else memmove(&items[binary_search_begin + 1], &items[binary_search_begin], (ro_right_begin - binary_search_begin) * 16);
               items[binary_search_begin] = fn_args[0];
               break;
          }
     }

     return null;

     invalid_type_label:
     return error__invalid_type(thrd_data, owner);

     cant_ord_label:
     return error__cant_ord(thrd_data, owner);

     nan_cmp_label:
     return error__nan_cmp(thrd_data, owner);

     ret_incorrect_value_label:
     return error__ret_incorrect_value(thrd_data, owner);
}

static t_any items__insertion_sort_fn(t_thrd_data* const thrd_data, t_any* const items, u64 begin, u64 end, t_any const fn, const char* const owner) {
     for (u64 last_idx = begin + 1; last_idx < end; last_idx += 1)
          for (u64 current_idx = last_idx; current_idx > begin; current_idx -= 1) {
               ref_cnt__inc(thrd_data, items[current_idx - 1], owner);
               ref_cnt__inc(thrd_data, items[current_idx], owner);
               t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, &items[current_idx - 1], 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__less: case comptime_tkn__great:
                    if (cmp_result.bytes[0] == great_id) items__swap(items, current_idx - 1, current_idx);
               case comptime_tkn__equal:
                    break;

               [[clang::unlikely]]
               case comptime_tkn__not_equal:
                    return error__cant_ord(thrd_data, owner);
               [[clang::unlikely]]
               case comptime_tkn__nan_cmp:
                    return error__nan_cmp(thrd_data, owner);
               [[clang::unlikely]]
               default:
                    if (cmp_result.bytes[15] != tid__token) {
                         if (cmp_result.bytes[15] == tid__error) return cmp_result;
                         ref_cnt__dec(thrd_data, cmp_result);
                         return error__invalid_type(thrd_data, owner);
                    }

                    return error__ret_incorrect_value(thrd_data, owner);
               }
          }

     return null;
}

static t_any items__block_merge_sort_fn(t_thrd_data* const thrd_data, t_any* const items, u64 const len, t_any const fn, const char* const owner) {
     u64 block_size  = 16;
     u64 block_begin = 0;
     u64 block_end   = block_size;
     while (block_end <= len) {
          t_any const sort_result = items__insertion_sort_fn(thrd_data, items, block_begin, block_end, fn, owner);
          if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;

          block_begin = block_end;
          block_end  += block_size;
     }

     t_any const sort_result = items__insertion_sort_fn(thrd_data, items, block_begin, len, fn, owner);
     if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;

     while (block_size < len) {
          block_begin = 0;
          block_end   = block_size * 2;
          while (block_end <= len) {
               t_any const merge_result = items__merge_blocks_fn(thrd_data, items, block_begin, block_begin + block_size, block_end, fn, owner);
               if (merge_result.bytes[15] == tid__error) [[clang::unlikely]] return merge_result;

               block_begin = block_end;
               block_end  += block_size * 2;
          }

          u64 const last_block_begin = block_begin + block_size;
          if (last_block_begin < len) {
               t_any const merge_result = items__merge_blocks_fn(thrd_data, items, block_begin, last_block_begin, len, fn, owner);
               if (merge_result.bytes[15] == tid__error) [[clang::unlikely]] return merge_result;
          }

          block_size *= 2;
     }

     return null;
}

static t_any array__stable_sort_fn__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len = slice__get_len(array);
     if (slice_len < 2) {
          ref_cnt__dec(thrd_data, fn);
          return array;
     }

     u64 const ref_cnt   = get_ref_cnt(array);
     u64 const array_len = slice_array__get_len(array);
     u64 const len       = slice_len <= slice_max_len ? slice_len : array_len;
     if (ref_cnt != 1) array = array__dup__own(thrd_data, array, owner);

     t_any* const items       = &((t_any*)slice_array__get_items(array))[slice__get_offset(array)];
     t_any  const sort_result = items__block_merge_sort_fn(thrd_data, items, len, fn, owner);
     ref_cnt__dec(thrd_data, fn);
     if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec__noinlined_part(thrd_data, array);
          return sort_result;
     }

     return array;
}

static t_any array__suffix_of__own(t_thrd_data* const thrd_data, t_any const array, t_any const suffix, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(suffix.bytes[15] == tid__array);

     u32          const array_slice_offset = slice__get_offset(array);
     u32          const array_slice_len    = slice__get_len(array);
     const t_any* const array_items        = &((const t_any*)slice_array__get_items(array))[array_slice_offset];
     u64          const array_ref_cnt      = get_ref_cnt(array);
     u64          const array_array_len    = slice_array__get_len(array);
     u64          const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     u32          const suffix_slice_offset = slice__get_offset(suffix);
     u32          const suffix_slice_len    = slice__get_len(suffix);
     const t_any* const suffix_items        = &((const t_any*)slice_array__get_items(suffix))[suffix_slice_offset];
     u64          const suffix_ref_cnt      = get_ref_cnt(suffix);
     u64          const suffix_array_len    = slice_array__get_len(suffix);
     u64          const suffix_len          = suffix_slice_len <= slice_max_len ? suffix_slice_len : suffix_array_len;

     if (suffix_len > array_len) {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, suffix);
          return bool__false;
     }

     if (suffix.qwords[0] == array.qwords[0]) {
          if (array_ref_cnt > 2) set_ref_cnt(array, array_ref_cnt - 2);
     } else {
          if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);
          if (suffix_ref_cnt > 1) set_ref_cnt(suffix, suffix_ref_cnt - 1);
     }

     t_any              result             = bool__true;
     const t_any* const array_suffix_items = &array_items[array_len - suffix_len];
     for (u64 idx = 0; idx < suffix_len; idx += 1) {
          t_any const eq_result = equal(thrd_data, array_suffix_items[idx], suffix_items[idx], owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) {
               result = bool__false;
               break;
          }
     }

     if (suffix.qwords[0] == array.qwords[0]) {
          if (array_ref_cnt == 2) ref_cnt__dec__noinlined_part(thrd_data, array);
     } else {
          if (array_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, array);
          if (suffix_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, suffix);
     }

     return result;
}

static t_any array__take_while__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32          const slice_offset = slice__get_offset(array);
     u32          const slice_len    = slice__get_len(array);
     const t_any* const items        = &((const t_any*)slice_array__get_items(array))[slice_offset];
     u64          const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64 n = 0;
     for (; n < len; n += 1) {
          const t_any* const item_ptr = &items[n];
          ref_cnt__inc(thrd_data, *item_ptr, owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, item_ptr, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, array);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) break;
     }
     ref_cnt__dec(thrd_data, fn);

     if (n == 0) {
          ref_cnt__dec(thrd_data, array);
          return empty_array;
     }

     if (n == len) return array;

     if (n <= slice_max_len) [[clang::likely]] {
          array = slice__set_metadata(array, slice_offset, n);
          return array;
     }

     assert(slice_offset == 0);

     u64 const ref_cnt = get_ref_cnt(array);
     if (ref_cnt == 1) {
          slice_array__set_len(array, n);
          slice_array__set_cap(array, n);
          for (u64 idx = n; idx < len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

          array           = slice__set_metadata(array, 0, slice_max_len + 1);
          array.qwords[0] = (u64)aligned_realloc(16, (t_any*)array.qwords[0], (n + 1) * 16);
          return array;
     }
     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any        new_array = array__init(thrd_data, n, owner);
     t_any* const new_items = slice_array__get_items(new_array);
     new_array              = slice__set_metadata(new_array, 0, slice_max_len + 1);
     slice_array__set_len(new_array, n);
     for (u64 idx = 0; idx < n; idx += 1) {
          t_any const item = items[idx];
          new_items[idx]   = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     return new_array;
}

static t_any array__to_box__own(t_thrd_data* const thrd_data, t_any const array, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32          const slice_offset      = slice__get_offset(array);
     u32          const slice_len         = slice__get_len(array);
     const t_any* const items             = slice_array__get_items(array);
     const t_any* const items_from_offset = &items[slice_offset];
     u64          const ref_cnt           = get_ref_cnt(array);
     u64          const array_len         = slice_array__get_len(array);
     u64          const len               = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (len > 16) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     if (len == 0) {
          if (ref_cnt == 1) {
               for (u64 idx = 0; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
               free((void*)array.qwords[0]);
          }

          return empty_box;
     }

     if (ref_cnt == 1)
          for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

     t_any              box         = box__new(thrd_data, len, owner);
     t_any*       const box_items   = box__get_items(box);
     if (ref_cnt > 1)
          for (u64 idx = 0; idx < len; idx += 1) {
               t_any const item = items_from_offset[idx];
               ref_cnt__inc(thrd_data, item, owner);
               box_items[idx] = item;
          }
     else {
          memcpy_le_256(box_items, items_from_offset, len * 16);

          if (ref_cnt == 1) {
               for (u64 idx = slice_offset + len; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
               free((void*)array.qwords[0]);
          }
     }

     return box;
}

static t_any array__unslice__own(t_thrd_data* const thrd_data, t_any array, const char* const owner) {
     assume(array.bytes[15] == tid__array);

     u32 const slice_len = slice__get_len(array);
     u64 const array_len = slice_array__get_len(array);
     u64 const array_cap = slice_array__get_cap(array);
     u64 const len       = slice_len <= slice_max_len ? slice_len : array_len;
     u64 const ref_cnt   = get_ref_cnt(array);

     if ((len == array_len && array_cap == array_len) || ref_cnt == 0) return array;

     u32 const slice_offset = slice__get_offset(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any* const items = slice_array__get_items(array);
     if (ref_cnt == 1) {
          if (slice_offset != 0) {
               for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
               memmove(items, &items[slice_offset], len * 16);
          }
          for (u64 idx = slice_offset + len; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

          slice_array__set_len(array, len);
          slice_array__set_cap(array, len);
          array           = slice__set_metadata(array, 0, slice_len);
          array.qwords[0] = (u64)aligned_realloc(16, (t_any*)array.qwords[0], (len + 1) * 16);
          return array;
     }
     set_ref_cnt(array, ref_cnt - 1);

     t_any        new_array = array__init(thrd_data, len, owner);
     new_array              = slice__set_metadata(new_array, 0, slice_len);
     slice_array__set_len(new_array, len);
     t_any* const new_items = slice_array__get_items(new_array);
     for (u64 idx = 0; idx < len; idx += 1) {
          t_any const item = items[slice_offset + idx];
          new_items[idx]   = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     return new_array;
}

static void array__zip_init_read(const t_any* const array, t_zip_read_state* const state, t_any* key, t_any*, u64* const result_len, t_any* const result) {
     assert(array->bytes[15] == tid__array);

     u64 const ref_cnt   = get_ref_cnt(*array);
     u64 const array_len = slice_array__get_len(*array);

     state->state.is_mut = ref_cnt == 1;
     if (state->state.is_mut) {
          set_ref_cnt(*array, 2);
          *result = *array;
     }

     state->state.item_position = &((t_any*)slice_array__get_items(*array))[slice__get_offset(*array)];
     u32 const slice_len        = slice__get_len(*array);
     *result_len                = slice_len <= slice_max_len ? slice_len : array_len;
     *key                       = (const t_any){.structure = {.value = -1, .type = tid__int}};
}

static void array__zip_read(t_thrd_data* const thrd_data, t_zip_read_state* const state, t_any* const key, t_any* const value, const char* const owner) {
     key->qwords[0] += 1;
     *value          = *state->state.item_position;
     if (state->state.is_mut) state->state.item_position->raw_bits = 0;
     else ref_cnt__inc(thrd_data, *value, owner);

     state->state.item_position = &state->state.item_position[1];
}

static void array__zip_init_search(const t_any* const array, t_zip_search_state* const state, t_any*, u64* const result_len, t_any* const result) {
     assert(array->bytes[15] == tid__array);

     u64 const ref_cnt         = get_ref_cnt(*array);
     u64 const array_array_len = slice_array__get_len(*array);

     state->array_state.is_mut = ref_cnt == 1;

     u32 const slice_len = slice__get_len(*array);
     u64 const array_len = slice_len <= slice_max_len ? slice_len : array_array_len;
     if (state->array_state.is_mut) {
          if (result->bytes[15] == tid__null) {
               set_ref_cnt(*array, 2);
               *result = *array;
          } else {
               u32 const result_slice_len = slice__get_len(*result);
               u64 const result_array_len = result_slice_len <= slice_max_len ? result_slice_len : slice_array__get_len(*result);

               if (result_array_len > array_len) {
                    set_ref_cnt(*result, 1);
                    set_ref_cnt(*array, 2);
                    *result = *array;
               }
          }
     }

     state->array_state.item_position = &((t_any*)slice_array__get_items(*array))[slice__get_offset(*array)];
     *result_len                      = *result_len > array_len ? array_len : *result_len;
}

static bool array__zip_search(t_thrd_data* const thrd_data, t_zip_search_state* const state, t_any const, t_any* const value, const char* const owner) {
     *value = *state->array_state.item_position;
     if (state->array_state.is_mut) state->array_state.item_position->raw_bits = 0;
     else ref_cnt__inc(thrd_data, *value, owner);

     state->array_state.item_position = &state->array_state.item_position[1];
     return true;
}

static void array__zip_write(t_thrd_data* const thrd_data, t_any* const array, t_any* const items, bool const is_new_array, t_any const key, t_any const value, const char* const) {
     if (is_new_array) {
          u64 const len = key.qwords[0] + 1;
          *array        = slice__set_metadata(*array, 0, len <= slice_max_len ? len : slice_max_len + 1);
          slice_array__set_len(*array, len);
     }

     items[key.qwords[0]] = value;
}

static t_any array__join(
     t_thrd_data* const thrd_data,
     u64          const parts_len,
     const t_any* const parts,
     u64          const separator_len,
     const t_any*       separator_items,
     bool         const separator_is_gc,
     u64          const result_len,
     u8           const step_size,
     const char*  const owner
) {
     if (parts_len == 0) return empty_array;

     u64 idx = 0;
     for (; parts[idx].bytes[15] == tid__null || parts[idx].bytes[15] == tid__holder; idx += step_size);

     if (parts_len == 1) {
          t_any const result = parts[idx];
          ref_cnt__inc(thrd_data, result, owner);
          return result;
     }

     u64 remain_items_len = parts_len;

     if (!separator_is_gc)
          for (u64 separator_item_idx = 0; separator_item_idx < separator_len; ref_cnt__add(thrd_data, separator_items[separator_item_idx], parts_len - 1, owner), separator_item_idx += 1);

     t_any        result       = array__init(thrd_data, result_len, owner);
     slice_array__set_len(result, result_len);
     result                    = slice__set_metadata(result, 0, result_len <= slice_max_len ? result_len : slice_max_len + 1);
     t_any* const result_items = slice_array__get_items(result);
     u64 current_result_len = 0;

     for (;; idx += step_size) {
          const t_any* const part_ptr = &parts[idx];
          switch (part_ptr->bytes[15]) {
          case tid__null: case tid__holder:
               continue;
          case tid__array: {
               t_any        const part              = *part_ptr;
               u32          const part_slice_offset = slice__get_offset(part);
               u32          const part_slice_len    = slice__get_len(part);
               u64          const part_len          = part_slice_len <= slice_max_len ? part_slice_len : slice_array__get_len(part);
               const t_any* const part_items        = &((const t_any*)slice_array__get_items(part))[part_slice_offset];

               for (u64 item_idx = 0; item_idx < part_len; item_idx += 1) {
                    t_any const part                 = part_items[item_idx];
                    result_items[current_result_len] = part;
                    ref_cnt__inc(thrd_data, part, owner);
                    current_result_len += 1;
               }
               break;
          }
          default:
               unreachable;
          }

          remain_items_len -= 1;
          if (remain_items_len == 0) break;

          memcpy(&result_items[current_result_len], separator_items, separator_len * 16);
          current_result_len += separator_len;
     }

     return result;
}

static t_any array__insert_to__own(t_thrd_data* const thrd_data, t_any array, t_any const new_item, u64 const position, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assert(type_is_common(new_item.bytes[15]));

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (position > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, new_item);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const new_len = len + 1;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);

     if (ref_cnt == 1) {
          u64 new_cap = new_len * 3 / 2;
          new_cap     = new_cap <= array_max_len ? new_cap : new_len;

          u64 const cap = slice_array__get_cap(array);
          slice_array__set_len(array, new_len);
          array         = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (cap < new_len) {
               array.qwords[0] = (u64)aligned_realloc(16, (t_any*)array.qwords[0], (new_cap + 1) * 16);
               slice_array__set_cap(array, new_cap);
          }

          t_any* const items = slice_array__get_items(array);

          assume_aligned(items, 16);

          if (slice_offset == 0) {
               for (u64 idx = slice_offset + len; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
               memmove(&items[position + 1], &items[position], (len - position) * 16);
               items[position] = new_item;
          } else {
               for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
               memmove(items, &items[slice_offset], position * 16);
               items[position] = new_item;
               memmove(&items[position + 1], &items[slice_offset + position], (len - position) * 16);
               for (u64 idx = slice_offset + len; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          }

          return array;
     }

     t_any              result    = array__init(thrd_data, new_len, owner);
     slice_array__set_len(result, new_len);
     result                       = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
     const t_any* const old_items = &((const t_any*)slice_array__get_items(array))[slice_offset];
     t_any*       const new_items = slice_array__get_items(result);

     assume_aligned(old_items, 16);
     assume_aligned(new_items, 16);

     if (ref_cnt == 0) {
          memcpy(new_items, old_items, position * 16);
          new_items[position] = new_item;
          memcpy(&new_items[position + 1], &old_items[position], (len - position) * 16);
     } else {
          set_ref_cnt(array, ref_cnt - 1);

          for (u64 idx = 0; idx < position; idx += 1) {
               t_any const item = old_items[idx];
               new_items[idx]   = item;
               ref_cnt__inc(thrd_data, item, owner);
          }
          new_items[position] = new_item;
          for (u64 idx = position; idx < len; idx += 1) {
               t_any const item   = old_items[idx];
               new_items[idx + 1] = item;
               ref_cnt__inc(thrd_data, item, owner);
          }
     }

     return result;
}

static t_any array__insert_part_to__own(t_thrd_data* const thrd_data, t_any array, t_any part, u64 const position, const char* const owner) {
     assume(array.bytes[15] == tid__array);
     assume(part.bytes[15] == tid__array);

     u32 const array_slice_offset = slice__get_offset(array);
     u32 const array_slice_len    = slice__get_len(array);
     u64 const array_array_len    = slice_array__get_len(array);
     u64 const array_array_cap    = slice_array__get_cap(array);
     t_any*    array_items        = slice_array__get_items(array);
     u64 const array_ref_cnt      = get_ref_cnt(array);
     u64 const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);
     assert(array_array_cap >= array_array_len);

     if (position > array_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, part);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (array_len == 0) {
          ref_cnt__dec(thrd_data, array);
          return part;
     }

     u32 const part_slice_offset = slice__get_offset(part);
     u32 const part_slice_len    = slice__get_len(part);
     u64 const part_array_len    = slice_array__get_len(part);
     u64 const part_array_cap    = slice_array__get_cap(part);
     t_any*    part_items        = slice_array__get_items(part);
     u64 const part_ref_cnt      = get_ref_cnt(part);
     u64 const part_len          = part_slice_len <= slice_max_len ? part_slice_len : part_array_len;

     assert(part_slice_len <= slice_max_len || part_slice_offset == 0);
     assert(part_array_cap >= part_array_len);

     if (part_len == 0) {
          ref_cnt__dec(thrd_data, part);
          return array;
     }

     u64 const new_len = array_len + part_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     if (array_items == part_items) {
          if (array_ref_cnt > 2) set_ref_cnt(array, array_ref_cnt - 2);
     } else {
          if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);
          if (part_ref_cnt > 1) set_ref_cnt(part, part_ref_cnt - 1);
     }

     int const array_is_mut = array_ref_cnt == 1;
     int const part_is_mut  = part_ref_cnt == 1;

     u8 const scenario =
          array_is_mut                                     |
          part_is_mut * 2                                  |
          (array_is_mut && array_array_cap >= new_len) * 4 |
          (part_is_mut  && part_array_cap  >= new_len) * 8 ;

     switch (scenario) {
     case 1: case 3:
          array.qwords[0] = (u64)aligned_realloc(16, (t_any*)array.qwords[0], (new_cap + 1) * 16);
          slice_array__set_cap(array, new_cap);
          array_items     = slice_array__get_items(array);
     case 5: case 7: case 15:
          slice_array__set_len(array, new_len);
          array = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (array_slice_offset != 0) {
               for (u32 idx = 0; idx < array_slice_offset; ref_cnt__dec(thrd_data, array_items[idx]), idx += 1);
               memmove(array_items, &array_items[array_slice_offset], position * 16);
          }

          for (u64 idx = array_slice_offset + array_len; idx < array_array_len; ref_cnt__dec(thrd_data, array_items[idx]), idx += 1);

          if (array_slice_offset != part_len) memmove(&array_items[position + part_len], &array_items[array_slice_offset + position], (array_len - position) * 16);

          if (part_is_mut)
               for (u32 idx = 0; idx < part_slice_offset; ref_cnt__dec(thrd_data, part_items[idx]), idx += 1);

          const t_any* const part_items_from_offset = &part_items[part_slice_offset];
          t_any*       const part_items_in_array    = &array_items[position];
          for (u64 idx = 0; idx < part_len; idx += 1) {
               t_any const part_item    = part_items_from_offset[idx];
               part_items_in_array[idx] = part_item;
               if (part_ref_cnt > 1) ref_cnt__inc(thrd_data, part_item, owner);
          }

          if (part_is_mut) {
               for (u64 idx = part_slice_offset + part_len; idx < part_array_len; ref_cnt__dec(thrd_data, part_items[idx]), idx += 1);

               free((void*)part.qwords[0]);
          }

          return array;
     case 2:
          part.qwords[0] = (u64)aligned_realloc(16, (t_any*)part.qwords[0], (new_cap + 1) * 16);
          slice_array__set_cap(part, new_cap);
          part_items     = slice_array__get_items(part);
     case 10: case 11:
          slice_array__set_len(part, new_len);
          part = slice__set_metadata(part, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          for (u32 idx = 0; idx < part_slice_offset; ref_cnt__dec(thrd_data, part_items[idx]), idx += 1);
          for (u64 idx = part_slice_offset + part_len; idx < part_array_len; ref_cnt__dec(thrd_data, part_items[idx]), idx += 1);

          if (part_slice_offset != position) memmove(&part_items[position], &part_items[part_slice_offset], part_len * 16);

          if (array_is_mut)
               for (u32 idx = 0; idx < array_slice_offset; ref_cnt__dec(thrd_data, array_items[idx]), idx += 1);

          t_any* const array_items_from_offset = &array_items[array_slice_offset];
          for (u64 idx = 0; idx < position; idx += 1) {
               t_any const array_item = array_items_from_offset[idx];
               part_items[idx]        = array_item;
               if (array_ref_cnt > 1) ref_cnt__inc(thrd_data, array_item, owner);
          }

          for (u64 idx = position; idx < array_len; idx += 1) {
               t_any const array_item     = array_items_from_offset[idx];
               part_items[idx + part_len] = array_item;
               if (array_ref_cnt > 1) ref_cnt__inc(thrd_data, array_item, owner);
          }

          if (array_is_mut) {
               for (u64 idx = array_slice_offset + array_len; idx < array_array_len; ref_cnt__dec(thrd_data, array_items[idx]), idx += 1);

               free((void*)array.qwords[0]);
          }

          return part;
     case 0: {
          t_any              result                  = array__init(thrd_data, new_len, owner);
          slice_array__set_len(result, new_len);
          result                                     = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          t_any*       const result_items            = slice_array__get_items(result);
          const t_any* const array_items_from_offset = &array_items[array_slice_offset];

          if (array_is_mut)
               for (u32 idx = 0; idx < array_slice_offset; ref_cnt__dec(thrd_data, array_items[idx]), idx += 1);

          for (u64 idx = 0; idx < position; idx += 1) {
               t_any const array_item = array_items_from_offset[idx];
               result_items[idx]      = array_item;
               if (array_ref_cnt > 1) ref_cnt__inc(thrd_data, array_item, owner);
          }

          if (part_is_mut)
               for (u32 idx = 0; idx < part_slice_offset; ref_cnt__dec(thrd_data, part_items[idx]), idx += 1);

          t_any*       const part_items_in_result   = &result_items[position];
          const t_any* const part_items_from_offset = &part_items[part_slice_offset];
          for (u64 idx = 0; idx < part_len; idx += 1) {
               t_any const part_item     = part_items_from_offset[idx];
               part_items_in_result[idx] = part_item;
               if (part_ref_cnt > 1) ref_cnt__inc(thrd_data, part_item, owner);
          }

          if (part_is_mut) {
               for (u64 idx = part_slice_offset + part_len; idx < part_array_len; ref_cnt__dec(thrd_data, part_items[idx]), idx += 1);

               free((void*)part.qwords[0]);
          }

          for (u64 idx = position; idx < array_len; idx += 1) {
               t_any const array_item       = array_items_from_offset[idx];
               result_items[idx + part_len] = array_item;
               if (array_ref_cnt > 1) ref_cnt__inc(thrd_data, array_item, owner);
          }

          if (array_is_mut) {
               for (u64 idx = array_slice_offset + array_len; idx < array_array_len; ref_cnt__dec(thrd_data, array_items[idx]), idx += 1);

               free((void*)array.qwords[0]);
          } else if (array_ref_cnt == 2 && array_items == part_items) ref_cnt__dec__noinlined_part(thrd_data, array);

          return result;
     }
     default:
          unreachable;
     }
}
