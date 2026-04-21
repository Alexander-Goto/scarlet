#pragma once

#include "../../common/const.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../array/basic.h"
#include "../bool/basic.h"
#include "../common/slice.h"
#include "../error/fn.h"
#include "../token/basic.h"
#include "basic.h"

static void byte_array__unpack__own(t_thrd_data* const thrd_data, t_any array, u64 const unpacking_items_len, u64 const max_array_item_idx, t_any* const unpacking_items, const char* const owner) {
     if (!(array.bytes[15] == tid__short_byte_array || array.bytes[15] == tid__byte_array)) {
          if (!(array.bytes[15] == tid__error || array.bytes[15] == tid__null)) {
               ref_cnt__dec(thrd_data, array);
               array = error__invalid_type(thrd_data, owner);
          }

          ref_cnt__add(thrd_data, array, unpacking_items_len - 1, owner);
          for (u64 idx = 0; idx < unpacking_items_len; unpacking_items[idx] = array, idx += 1);
          return;
     }

     u64       array_len;
     const u8* bytes;
     bool      need_to_free;
     if (array.bytes[15] == tid__short_byte_array) {
          array_len    = short_byte_array__get_len(array);
          bytes        = array.bytes;
          need_to_free = false;
     } else {
          u64 const ref_cnt = get_ref_cnt(array);
          if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);
          need_to_free = ref_cnt == 1;

          u32 const slice_offset = slice__get_offset(array);
          u32 const slice_len    = slice__get_len(array);
          bytes                  = &((const u8*)slice_array__get_items(array))[slice_offset];
          array_len              = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);
     }

     if (array_len < max_array_item_idx) [[clang::unlikely]] {
          if (need_to_free) free((void*)array.qwords[0]);
          t_any const error = error__unpacking_not_enough_items(thrd_data, owner);
          set_ref_cnt(error, unpacking_items_len);

          for (u64 idx = 0; idx < unpacking_items_len; unpacking_items[idx] = error, idx += 1);
          return;
     }

     for (u64 unpacking_item_idx = 0; unpacking_item_idx < unpacking_items_len; unpacking_item_idx += 1) {
          u64    const array_item_idx         = unpacking_items[unpacking_item_idx].bytes[0];
          unpacking_items[unpacking_item_idx] = (const t_any){.bytes = {bytes[array_item_idx], [15] = tid__byte}};
     }

     if (need_to_free) free((void*)array.qwords[0]);
}

static inline t_any concat__short_byte_array__short_byte_array(t_any left, t_any const right) {
     assert(left.bytes[15] == tid__short_byte_array);
     assert(right.bytes[15] == tid__short_byte_array);

     u8 const left_len  = short_byte_array__get_len(left);
     u8 const right_len = short_byte_array__get_len(right);
     u8 const new_len   = left_len + right_len;

     if (new_len < 15) {
          left.raw_bits |= right.raw_bits << left_len * 8;
          left.bytes[15] = tid__short_byte_array;
          left           = short_byte_array__set_len(left, new_len) ;
          return left;
     }

     t_any array = long_byte_array__new(new_len < 16 ? 16 : new_len);
     array       = slice__set_metadata(array, 0, new_len);
     slice_array__set_len(array, new_len);

     u8* const bytes = slice_array__get_items(array);
     memcpy_inline(bytes, left.bytes, 16);
     *(ua_u128*)&bytes[(i8)new_len - (i8)16] &= (u128)-1 >> right_len * 8;
     *(ua_u128*)&bytes[(i8)new_len - (i8)16] |= right.raw_bits << (16 - right_len) * 8;
     return array;
}

static inline t_any concat__short_byte_array__long_byte_array__own(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner) {
     assert(left.bytes[15] == tid__short_byte_array);
     assume(right.bytes[15] == tid__byte_array);

     u8 const left_len = short_byte_array__get_len(left);
     if (left_len == 0) return right;

     u32 const right_slice_len    = slice__get_len(right);
     u32 const right_slice_offset = slice__get_offset(right);
     u64 const right_array_len    = slice_array__get_len(right);
     u64 const right_ref_cnt      = get_ref_cnt(right);
     u64 const right_len          = right_slice_len <= slice_max_len ? right_slice_len : right_array_len;

     assert(right_len <= slice_max_len || right_slice_offset == 0);

     u64 const new_len = left_len + right_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

     t_any array;
     if (right_ref_cnt == 1) {
          u64 new_cap = new_len * 3 / 2;
          new_cap     = new_cap <= array_max_len ? new_cap : new_len;

          u64 const right_array_cap = slice_array__get_cap(right);
          array                     = right;
          slice_array__set_len(array, new_len);
          array                     = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (right_array_cap < new_len) {
               array.qwords[0] = (u64)realloc((u8*)array.qwords[0], new_cap + 16);
               slice_array__set_cap(array, new_cap);
          }

          u8* const bytes = slice_array__get_items(array);
          if (left_len != right_slice_offset) memmove(&bytes[left_len], &bytes[right_slice_offset], right_len);
          *(ua_u128*)bytes &= (u128)-1 << left_len * 8;
          *(ua_u128*)bytes |= left.raw_bits & ((u128)1 << left_len * 8) - 1;
     } else {
          if (right_ref_cnt != 0) set_ref_cnt(right, right_ref_cnt - 1);

          array               = long_byte_array__new(new_len);
          slice_array__set_len(array, new_len);
          array               = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          u8* const new_bytes = slice_array__get_items(array);
          memcpy_inline(new_bytes, left.bytes, 16);
          memcpy(&new_bytes[left_len], &((u8*)slice_array__get_items(right))[right_slice_offset], right_len);
     }

     return array;
}

static inline t_any concat__long_byte_array__short_byte_array__own(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner) {
     assume(left.bytes[15] == tid__byte_array);
     assert(right.bytes[15] == tid__short_byte_array);

     u8 const right_len = short_byte_array__get_len(right);
     if (right_len == 0) return left;

     u32 const left_slice_len    = slice__get_len(left);
     u32 const left_slice_offset = slice__get_offset(left);
     u64 const left_array_len    = slice_array__get_len(left);
     u64 const left_ref_cnt      = get_ref_cnt(left);
     u64 const left_len          = left_slice_len <= slice_max_len ? left_slice_len : left_array_len;

     assert(left_len <= slice_max_len || left_slice_offset == 0);

     u64 const new_len = left_len + right_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

     t_any array;
     if (left_ref_cnt == 1) {
          u64 new_cap = new_len * 3 / 2;
          new_cap     = new_cap <= array_max_len ? new_cap : new_len;

          u64 const left_array_cap = slice_array__get_cap(left);
          array                    = left;
          slice_array__set_len(array, new_len);
          array                    = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (left_array_cap < new_len) {
               array.qwords[0]   = (u64)realloc((u8*)array.qwords[0], new_cap + 16);
               slice_array__set_cap(array, new_cap);
          }

          u8* const bytes = slice_array__get_items(array);
          if (left_slice_offset != 0) memmove(bytes, &bytes[left_slice_offset], left_len);
          *(ua_u128*)&bytes[(i64)new_len - 16] &= (u128)-1 >> right_len * 8;
          *(ua_u128*)&bytes[(i64)new_len - 16] |= right.raw_bits << (16 - right_len) * 8;
     } else {
          if (left_ref_cnt != 0) set_ref_cnt(left, left_ref_cnt - 1);

          array               = long_byte_array__new(new_len);
          slice_array__set_len(array, new_len);
          array               = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          u8* const new_bytes = slice_array__get_items(array);
          memcpy(new_bytes, &((u8*)slice_array__get_items(left))[left_slice_offset], left_len);
          *(ua_u128*)&new_bytes[(i64)new_len - 16] &= (u128)-1 >> right_len * 8;
          *(ua_u128*)&new_bytes[(i64)new_len - 16] |= right.raw_bits << (16 - right_len) * 8;
     }

     return array;
}

static inline t_any concat__long_byte_array__long_byte_array__own(t_thrd_data* const thrd_data, t_any left, t_any right, const char* const owner) {
     assume(left.bytes[15] == tid__byte_array);
     assume(right.bytes[15] == tid__byte_array);

     u32 const left_slice_offset = slice__get_offset(left);
     u32 const left_slice_len    = slice__get_len(left);
     u64 const left_ref_cnt      = get_ref_cnt(left);
     u64 const left_array_len    = slice_array__get_len(left);
     u64 const left_array_cap    = slice_array__get_cap(left);
     u64 const left_len          = left_slice_len <= slice_max_len ? left_slice_len : left_array_len;
     u8*       left_bytes        = slice_array__get_items(left);

     assert(left_slice_len <= slice_max_len || left_slice_offset == 0);
     assert(left_array_cap >= left_array_len);

     u32 const right_slice_offset = slice__get_offset(right);
     u32 const right_slice_len    = slice__get_len(right);
     u64 const right_ref_cnt      = get_ref_cnt(right);
     u64 const right_array_len    = slice_array__get_len(right);
     u64 const right_array_cap    = slice_array__get_cap(right);
     u64 const right_len          = right_slice_len <= slice_max_len ? right_slice_len : right_array_len;
     u8*       right_bytes        = slice_array__get_items(right);

     assert(right_slice_len <= slice_max_len || right_slice_offset == 0);
     assert(right_array_cap >= right_array_len);

     u64 const new_len = left_len + right_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     if (left_bytes == right_bytes) {
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
          left.qwords[0] = (u64)realloc((u8*)left.qwords[0], new_cap + 16);
          slice_array__set_cap(left, new_cap);
          left_bytes     = slice_array__get_items(left);
     case 5: case 7: case 15:
          slice_array__set_len(left, new_len);
          left = slice__set_metadata(left, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (left_slice_offset != 0) memmove(left_bytes, &left_bytes[left_slice_offset], left_len);
          memcpy(&left_bytes[left_len], &right_bytes[right_slice_offset], right_len);

          if (right_is_mut) free((void*)right.qwords[0]);

          return left;
     case 2:
          right.qwords[0] = (u64)realloc((u8*)right.qwords[0], new_cap + 16);
          slice_array__set_cap(right, new_cap);
          right_bytes     = slice_array__get_items(right);
     case 10: case 11:
          slice_array__set_len(right, new_len);
          right = slice__set_metadata(right, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          if (right_slice_offset != left_len) memmove(&right_bytes[left_len], &right_bytes[right_slice_offset], right_len);
          memcpy(right_bytes, &left_bytes[left_slice_offset], left_len);

          if (left_is_mut) free((void*)left.qwords[0]);

          return right;
     case 0: {
          t_any     array     = long_byte_array__new(new_len);
          slice_array__set_len(array, new_len);
          array               = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          u8* const new_bytes = slice_array__get_items(array);
          memcpy(new_bytes, &left_bytes[left_slice_offset], left_len);
          memcpy(&new_bytes[left_len], &right_bytes[right_slice_offset], right_len);

          if (left_bytes == right_bytes) {
               if (left_ref_cnt == 2) free((void*)left.qwords[0]);
          } else {
               if (left_is_mut) free((void*)left.qwords[0]);
               if (right_is_mut) free((void*)right.qwords[0]);
          }

          return array;
     }
     default:
          unreachable;
     }
}

static inline t_any short_byte_array__get_item(t_thrd_data* const thrd_data, t_any const array, t_any const idx, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assert(idx.bytes[15] == tid__int);

     if (idx.qwords[0] >= short_byte_array__get_len(array)) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     return (const t_any){.structure = {.value = array.bytes[idx.qwords[0]], .type = tid__byte}};
}

static inline t_any short_byte_array__slice(t_thrd_data* const thrd_data, t_any array, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(idx_from <= idx_to);
     assert(array.bytes[15] == tid__short_byte_array);

     if (idx_to > short_byte_array__get_len(array)) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     u8 const len     = idx_to - idx_from;
     array.raw_bits >>= idx_from * 8;
     array.raw_bits  &= ((u128)1 << len * 8) - 1;
     array.bytes[15]  = tid__short_byte_array;
     array            = short_byte_array__set_len(array, len);
     return array;
}

static inline t_any long_byte_array__get_item__own(t_thrd_data* const thrd_data, t_any const array, t_any const idx, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assert(idx.bytes[15] == tid__int);

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (idx.qwords[0] >= len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any const result = {.structure = {.value = ((const u8*)slice_array__get_items(array))[slice_offset + idx.qwords[0]], .type = tid__byte}};

     if (ref_cnt == 1) free((void*)array.qwords[0]);

     return result;
}

static inline t_any long_byte_array__slice__own(t_thrd_data* const thrd_data, t_any array, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(idx_from <= idx_to);

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (idx_to > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const new_len = idx_to - idx_from;
     if (new_len == len) return array;

     u8* const bytes       = slice_array__get_items(array);
     u64 const from_offset = slice_offset + idx_from;
     if (new_len < 15) {
          u64 const ref_cnt = get_ref_cnt(array);
          if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

          t_any short_array;
          memcpy_inline(short_array.bytes, &bytes[(i64)(slice_offset + idx_to) - 16], 16);
          short_array.raw_bits  = new_len == 0 ? 0 : short_array.raw_bits >> (16 - new_len) * 8;
          short_array.bytes[15] = tid__short_byte_array;
          short_array           = short_byte_array__set_len(short_array, new_len);

          if (ref_cnt == 1) free((void*)array.qwords[0]);

          return short_array;
     }

     if (new_len <= slice_max_len && from_offset <= slice_max_offset) [[clang::likely]] {
          array = slice__set_metadata(array, from_offset, new_len);
          return array;
     }

     u64 const ref_cnt = get_ref_cnt(array);
     if (ref_cnt == 1) {
          if (from_offset != 0) memmove(bytes, &bytes[from_offset], new_len);
          slice_array__set_len(array, new_len);
          slice_array__set_cap(array, new_len);
          array           = slice__set_metadata(array, 0, slice_len);
          array.qwords[0] = (u64)realloc((u8*)array.qwords[0], new_len + 16);
          return array;
     }

     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any     new_array = long_byte_array__new(new_len);
     new_array           = slice__set_metadata(new_array, 0, slice_len);
     slice_array__set_len(new_array, new_len);
     u8* const new_bytes = slice_array__get_items(new_array);
     memcpy(new_bytes, &bytes[from_offset], new_len);

     return new_array;
}

static t_any byte_array__all__own(t_thrd_data* const thrd_data, t_any const array, t_any const default_result, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__short_byte_array || array.bytes[15] == tid__byte_array);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64       len;
     const u8* bytes;
     bool      need_to_free;
     if (array.bytes[15] == tid__short_byte_array) {
          len = short_byte_array__get_len(array);
          if (len == 0) {
               ref_cnt__dec(thrd_data, fn);
               return default_result;
          }

          bytes        = array.bytes;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(array);
          u32 const slice_len    = slice__get_len(array);
          bytes                  = &((const u8*)slice_array__get_items(array))[slice_offset];
          u64 const ref_cnt      = get_ref_cnt(array);
          u64 const array_len    = slice_array__get_len(array);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

          need_to_free = ref_cnt == 1;
     }

     t_any result = bool__true;
     t_any byte   = {.bytes = {[15] = tid__byte}};
     for (u64 idx = 0; idx < len; idx += 1) {
          byte.bytes[0] = bytes[idx];

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
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

     if (need_to_free) free((void*)array.qwords[0]);
     ref_cnt__dec(thrd_data, fn);

     return result;
}

static t_any byte_array__all_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const default_result, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__short_byte_array || array.bytes[15] == tid__byte_array);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64       len;
     const u8* bytes;
     bool      need_to_free;
     if (array.bytes[15] == tid__short_byte_array) {
          len = short_byte_array__get_len(array);
          if (len == 0) {
               ref_cnt__dec(thrd_data, fn);
               return default_result;
          }

          bytes        = array.bytes;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(array);
          u32 const slice_len    = slice__get_len(array);
          bytes                  = &((const u8*)slice_array__get_items(array))[slice_offset];
          u64 const ref_cnt      = get_ref_cnt(array);
          u64 const array_len    = slice_array__get_len(array);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

          need_to_free = ref_cnt == 1;
     }

     t_any result     = bool__true;
     t_any fn_args[2] = {{.structure = {.type = tid__int}}, {.structure = {.type = tid__byte}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[0].qwords[0] = idx;
          fn_args[1].bytes[0]  = bytes[idx];

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
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

     if (need_to_free) free((void*)array.qwords[0]);
     ref_cnt__dec(thrd_data, fn);

     return result;
}

static t_any byte_array__any__own(t_thrd_data* const thrd_data, t_any const array, t_any const default_result, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__short_byte_array || array.bytes[15] == tid__byte_array);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64       len;
     const u8* bytes;
     bool      need_to_free;
     if (array.bytes[15] == tid__short_byte_array) {
          len = short_byte_array__get_len(array);
          if (len == 0) {
               ref_cnt__dec(thrd_data, fn);
               return default_result;
          }

          bytes        = array.bytes;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(array);
          u32 const slice_len    = slice__get_len(array);
          bytes                  = &((const u8*)slice_array__get_items(array))[slice_offset];
          u64 const ref_cnt      = get_ref_cnt(array);
          u64 const array_len    = slice_array__get_len(array);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

          need_to_free = ref_cnt == 1;
     }

     t_any result = bool__false;
     t_any byte   = {.bytes = {[15] = tid__byte}};
     for (u64 idx = 0; idx < len; idx += 1) {
          byte.bytes[0] = bytes[idx];

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
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

     if (need_to_free) free((void*)array.qwords[0]);
     ref_cnt__dec(thrd_data, fn);

     return result;
}

static t_any byte_array__any_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const default_result, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__short_byte_array || array.bytes[15] == tid__byte_array);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u64       len;
     const u8* bytes;
     bool      need_to_free;
     if (array.bytes[15] == tid__short_byte_array) {
          len = short_byte_array__get_len(array);
          if (len == 0) {
               ref_cnt__dec(thrd_data, fn);
               return default_result;
          }

          bytes        = array.bytes;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(array);
          u32 const slice_len    = slice__get_len(array);
          bytes                  = &((const u8*)slice_array__get_items(array))[slice_offset];
          u64 const ref_cnt      = get_ref_cnt(array);
          u64 const array_len    = slice_array__get_len(array);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

          need_to_free = ref_cnt == 1;
     }

     t_any result = bool__false;
     t_any fn_args[2] = {{.structure = {.type = tid__int}}, {.structure = {.type = tid__byte}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[0].qwords[0] = idx;
          fn_args[1].bytes[0]  = bytes[idx];

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
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

     if (need_to_free) free((void*)array.qwords[0]);
     ref_cnt__dec(thrd_data, fn);

     return result;
}

static inline t_any byte_array__cmp(t_any const array_1, t_any const array_2) {
     assert(array_1.bytes[15] == tid__short_byte_array || array_1.bytes[15] == tid__byte_array);
     assert(array_2.bytes[15] == tid__short_byte_array || array_2.bytes[15] == tid__byte_array);

     const u8* bytes_1;
     const u8* bytes_2;
     u64       len_1;
     u64       len_2;

     if (array_1.bytes[15] == tid__short_byte_array) {
          bytes_1 = array_1.bytes;
          len_1   = short_byte_array__get_len(array_1);
     } else {
          u32 const slice_offset = slice__get_offset(array_1);
          u32 const slice_len    = slice__get_len(array_1);
          bytes_1                = &((const u8*)slice_array__get_items(array_1))[slice_offset];
          len_1                  = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array_1);

          assert(slice_len <= slice_max_len || slice_offset == 0);
     }

     if (array_2.bytes[15] == tid__short_byte_array) {
          bytes_2 = array_2.bytes;
          len_2   = short_byte_array__get_len(array_2);
     } else {
          u32 const slice_offset = slice__get_offset(array_2);
          u32 const slice_len    = slice__get_len(array_2);
          bytes_2                = &((const u8*)slice_array__get_items(array_2))[slice_offset];
          len_2                  = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array_2);

          assert(slice_len <= slice_max_len || slice_offset == 0);
     }

     switch (scar__memcmp(bytes_1, bytes_2, len_1 < len_2 ? len_1 : len_2)) {
     case -1: return tkn__less;
     case  0: return len_1 < len_2 ? tkn__less : (len_2 < len_1 ? tkn__great : tkn__equal);
     case  1: return tkn__great;
     default: unreachable;
     }
}

static inline t_any long_byte_array__equal(t_any const left, t_any const right) {
     assume(left.bytes[15] == tid__byte_array);
     assume(right.bytes[15] == tid__byte_array);

     u32 const left_slice_len = slice__get_len(left);
     u64 const left_len       = left_slice_len <= slice_max_len ? left_slice_len : slice_array__get_len(left);

     u32 const right_slice_len = slice__get_len(right);
     u64 const right_len       = right_slice_len <= slice_max_len ? right_slice_len : slice_array__get_len(right);

     if (left_len != right_len) return bool__false;

     u32       const left_slice_offset  = slice__get_offset(left);
     u32       const right_slice_offset = slice__get_offset(right);
     const u8* const left_bytes         = &((const u8*)slice_array__get_items(left))[left_slice_offset];
     const u8* const right_bytes        = &((const u8*)slice_array__get_items(right))[right_slice_offset];

     assert(left_slice_len  <= slice_max_len || left_slice_offset  == 0);
     assert(right_slice_len <= slice_max_len || right_slice_offset == 0);

     return scar__memcmp(left_bytes, right_bytes, left_len) == 0 ? bool__true : bool__false;
}

static t_any copy__short_byte_array__short_byte_array(t_thrd_data* const thrd_data, t_any dst, u64 const idx, t_any const src, const char* const owner) {
     assert(dst.bytes[15] == tid__short_byte_array);
     assert(src.bytes[15] == tid__short_byte_array);

     u8 const dst_len = short_byte_array__get_len(dst);
     u8 const src_len = short_byte_array__get_len(src);
     if (dst_len < idx + src_len || (i64)idx < 0) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     dst.raw_bits &= ~((((u128)1 << src_len * 8) - 1) << idx * 8);
     dst.raw_bits |= (src.raw_bits & 0xffff'ffff'ffff'ffff'ffff'ffff'ffffuwb) << idx * 8;
     return dst;
}

static t_any copy__long_byte_array__short_byte_array__own(t_thrd_data* const thrd_data, t_any const dst, u64 const idx, t_any const src, const char* const owner) {
     assume(dst.bytes[15] == tid__byte_array);
     assert(src.bytes[15] == tid__short_byte_array);

     u32 const slice_offset  = slice__get_offset(dst);
     u32 const slice_len     = slice__get_len(dst);
     u64 const dst_ref_cnt   = get_ref_cnt(dst);
     u64 const dst_array_len = slice_array__get_len(dst);
     u64 const dst_len       = slice_len <= slice_max_len ? slice_len : dst_array_len;
     u8* const bytes         = &((u8*)slice_array__get_items(dst))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u8 const src_len = short_byte_array__get_len(src);
     if (dst_len < idx + src_len || (i64)idx < 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, dst);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (dst_ref_cnt == 1) {
          *(ua_u128*)&bytes[(i64)(idx + src_len) - 16] &= (u128)-1 >> src_len * 8;
          *(ua_u128*)&bytes[(i64)(idx + src_len) - 16] |= src_len == 0 ? 0 : src.raw_bits << (16 - src_len) * 8;
          return dst;
     }
     if (dst_ref_cnt != 0) set_ref_cnt(dst, dst_ref_cnt - 1);

     t_any     new_array = long_byte_array__new(dst_len);
     slice_array__set_len(new_array, dst_len);
     new_array           = slice__set_metadata(new_array, 0, slice_len);
     u8* const new_bytes = slice_array__get_items(new_array);
     memcpy(new_bytes, bytes, idx);
     *(ua_u128*)&new_bytes[(i64)(idx + src_len) - 16] &= (u128)-1 >> src_len * 8;
     *(ua_u128*)&new_bytes[(i64)(idx + src_len) - 16] |= src_len == 0 ? 0 : src.raw_bits << (16 - src_len) * 8;
     memcpy(&new_bytes[idx + src_len], &bytes[idx + src_len], dst_len - idx - src_len);
     return new_array;
}

static t_any copy__long_byte_array__long_byte_array__own(t_thrd_data* const thrd_data, t_any const dst, u64 const idx, t_any src, const char* const owner) {
     assume(dst.bytes[15] == tid__byte_array);
     assume(src.bytes[15] == tid__byte_array);

     u32 const dst_slice_offset = slice__get_offset(dst);
     u32 const dst_slice_len    = slice__get_len(dst);
     u64 const dst_ref_cnt      = get_ref_cnt(dst);
     u64 const dst_array_len    = slice_array__get_len(dst);
     u64 const dst_len          = dst_slice_len <= slice_max_len ? dst_slice_len : dst_array_len;
     u8* const dst_bytes        = slice_array__get_items(dst);

     assert(dst_slice_len <= slice_max_len || dst_slice_offset == 0);

     u32 const src_slice_offset = slice__get_offset(src);
     u32 const src_slice_len    = slice__get_len(src);
     u64 const src_ref_cnt      = get_ref_cnt(src);
     u64 const src_array_len    = slice_array__get_len(src);
     u64 const src_len          = src_slice_len <= slice_max_len ? src_slice_len : src_array_len;
     u8*       src_bytes        = slice_array__get_items(src);

     assert(src_slice_len <= slice_max_len || src_slice_offset == 0);

     if (dst_len < idx + src_len || (i64)idx < 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, src);
          ref_cnt__dec(thrd_data, dst);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (dst_ref_cnt == 1) {
          if (src_ref_cnt > 1) set_ref_cnt(src, src_ref_cnt - 1);
          memcpy(&dst_bytes[dst_slice_offset + idx], &src_bytes[src_slice_offset], src_len);
          if (src_ref_cnt == 1) free((void*)src.qwords[0]);
          return dst;
     }
     if (dst_ref_cnt != 0) set_ref_cnt(dst, dst_ref_cnt - 1);

     if (src_ref_cnt == 1 || (src_ref_cnt == 2 && src_bytes == dst_bytes)) {
          if (src_bytes == dst_bytes) {
               memmove(&dst_bytes[dst_slice_offset + idx], &dst_bytes[src_slice_offset], src_len);
               return dst;
          }

          u64 const src_cap = slice_array__get_cap(src);
          if (src_cap < dst_len) {
               src.qwords[0] = (u64)realloc((u8*)src.qwords[0], 16 + dst_len);
               src_bytes     = slice_array__get_items(src);
          }
          slice_array__set_len(src, dst_len);
          src = slice__set_metadata(src, 0, dst_slice_len);

          if (src_slice_offset != idx) memmove(&src_bytes[idx], &src_bytes[src_slice_offset], src_len);
          memcpy(src_bytes, &dst_bytes[dst_slice_offset], idx);
          memcpy(&src_bytes[idx + src_len], &dst_bytes[dst_slice_offset + idx + src_len], dst_len - src_len - idx);
          return src;
     }
     if (src_ref_cnt != 0) set_ref_cnt(src, src_ref_cnt - (src_bytes == dst_bytes ? 2 : 1));

     t_any new_array = long_byte_array__new(dst_len);
     new_array       = slice__set_metadata(new_array, 0, dst_slice_len);
     slice_array__set_len(new_array, dst_len);

     u8* const new_bytes = slice_array__get_items(new_array);
     memcpy(new_bytes, &dst_bytes[dst_slice_offset], idx);
     memcpy(&new_bytes[idx], &src_bytes[src_slice_offset], src_len);
     memcpy(&new_bytes[idx + src_len], &dst_bytes[dst_slice_offset + idx + src_len], dst_len - src_len - idx);
     return new_array;
}

static t_any short_byte_array__delete(t_thrd_data* const thrd_data, t_any array, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(idx_from <= idx_to);
     assert(array.bytes[15] == tid__short_byte_array);

     u64 const len = short_byte_array__get_len(array);
     if (idx_to > len) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     array.raw_bits  = (array.raw_bits & (((u128)1 << idx_from * 8) - 1)) | (((array.raw_bits & 0xffff'ffff'ffff'ffff'ffff'ffff'ffffuwb) >> idx_to * 8) << idx_from * 8);
     array.bytes[15] = tid__short_byte_array;
     array           = short_byte_array__set_len(array, len - (idx_to - idx_from));
     return array;
}

static t_any long_byte_array__delete__own(t_thrd_data* const thrd_data, t_any array, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;
     u8* const bytes        = &((u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (idx_to > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const new_len = len - (idx_to - idx_from);
     if (new_len == len) return array;

     if (new_len < 15) {
          if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

          t_any           short_array;
          short_array.raw_bits  = idx_from == 0 ? 0 : (*(ua_u128*)&bytes[(i64)idx_from - 16]) >> (16 - idx_from) * 8;
          short_array.raw_bits |= idx_to == len ? 0 : ((*(ua_u128*)&bytes[(i64)len - 16]) >> (16 - (len - idx_to)) * 8) << idx_from * 8;
          short_array.bytes[15] = tid__short_byte_array;
          short_array           = short_byte_array__set_len(short_array, new_len);

          if (ref_cnt == 1) free((void*)array.qwords[0]);

          return short_array;
     }

     if (idx_from == 0 || idx_to == len) {
          u64 const slice_idx_from = idx_from == 0 ? idx_to : 0;
          u64 const slice_to_from  = idx_from == 0 ? len : idx_from;
          return long_byte_array__slice__own(thrd_data, array, slice_idx_from, slice_to_from, owner);
     }

     if (ref_cnt == 1) {
          array = slice__set_metadata(array, slice_offset, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          slice_array__set_len(array, array_len - (idx_to - idx_from));
          memmove(&bytes[idx_from], &bytes[idx_to], len - idx_to);
          return array;
     }
     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any new_array     = long_byte_array__new(new_len);
     new_array           = slice__set_metadata(new_array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
     slice_array__set_len(new_array, new_len);
     u8* const new_bytes = slice_array__get_items(new_array);
     memcpy(new_bytes, bytes, idx_from);
     memcpy(&new_bytes[idx_from], &bytes[idx_to], len - idx_to);

     return new_array;
}

static inline t_any short_byte_array__drop(t_thrd_data* const thrd_data, t_any array, u64 const n, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8 const old_len = short_byte_array__get_len(array);
     if (n > old_len) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     u8 const new_len = old_len - n;
     memset_inline(&array.bytes[14], 0, 2);
     array.raw_bits >>= n * 8;
     array.bytes[15]  = tid__short_byte_array;
     array            = short_byte_array__set_len(array, new_len);
     return array;
}

static inline t_any long_byte_array__drop__own(t_thrd_data* const thrd_data, t_any array, u64 const n, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);

     u32 const slice_len = slice__get_len(array);
     u64 const len       = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);
     if (n > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     return long_byte_array__slice__own(thrd_data, array, n, len, owner);
}

static t_any short_byte_array__drop_while__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any byte     = {.bytes = {[15] = tid__byte}};
     u8    len      = short_byte_array__get_len(array);
     u8    offset   = 0;
     for (; offset < len; offset += 1) {
          byte.bytes[0] = array.bytes[offset];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) break;
     }

     ref_cnt__dec(thrd_data, fn);
     array.qwords[1] &= 0xffff'ffff'ffffull;
     array.raw_bits >>= offset * 8;
     array.bytes[15]  = tid__short_byte_array;
     array            = short_byte_array__set_len(array, len - offset);
     return array;
}

static t_any long_byte_array__drop_while__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);
     u8* const bytes        = slice_array__get_items(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any byte = {.bytes = {[15] = tid__byte}};
     u64   n    = 0;
     for (; n < len; n += 1) {
          byte.bytes[0] = bytes[slice_offset + n];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
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

     u64 const new_len  = len - n;
     u64 const n_offset = slice_offset + n;
     if (new_len < 15) {
          t_any short_array;
          short_array.raw_bits  = new_len == 0 ? 0 : (*(ua_u128*)&bytes[(i64)(n_offset + new_len) - 16]) >> (16 - new_len) * 8;
          short_array.bytes[15] = tid__short_byte_array;
          short_array           = short_byte_array__set_len(short_array, new_len);

          ref_cnt__dec(thrd_data, array);

          return short_array;
     }

     if (new_len <= slice_max_len && n_offset <= slice_max_offset) [[clang::likely]] {
          array = slice__set_metadata(array, n_offset, new_len);
          return array;
     }

     u64 const ref_cnt = get_ref_cnt(array);
     if (ref_cnt == 1) {
          memmove(bytes, &bytes[n_offset], new_len);
          slice_array__set_len(array, new_len);
          slice_array__set_cap(array, new_len);
          array           = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          array.qwords[0] = (u64)realloc((u8*)array.qwords[0], new_len + 16);
          return array;
     }
     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any     new_array = long_byte_array__new(new_len);
     new_array           = slice__set_metadata(new_array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
     slice_array__set_len(new_array, new_len);
     u8* const new_bytes = slice_array__get_items(new_array);
     memcpy(new_bytes, &bytes[n_offset], new_len);
     return new_array;
}

static t_any short_byte_array__each_to_each__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8 const len = short_byte_array__get_len(array);
     t_any fn_args[2] = {{.bytes = {[15] = tid__byte}}, {.bytes = {[15] = tid__byte}}};
     for (u8 idx_1 = 0; (i8)idx_1 < (i8)len - (i8)1; idx_1 += 1) {
          fn_args[0].bytes[0] = array.bytes[idx_1];

          for (u8 idx_2 = idx_1 + 1; idx_2 < len; idx_2 += 1) {
               fn_args[1].bytes[0] = array.bytes[idx_2];

               t_any     const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || box_items[0].bytes[15] != tid__byte || box_items[1].bytes[15] != tid__byte) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[0].bytes[0] = box_items[0].bytes[0];
               array.bytes[idx_2] = box_items[1].bytes[0];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          array.bytes[idx_1] = fn_args[0].bytes[0];
     }
     ref_cnt__dec(thrd_data, fn);

     return array;
}

static t_any long_byte_array__each_to_each__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(array);
     u32 const slice_offset = slice__get_offset(array);
     u8*       bytes        = &((u8*)slice_array__get_items(array))[slice_offset];
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt != 1) {
          if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

          array = long_byte_array__new(len);
          array = slice__set_metadata(array, 0, slice_len);
          slice_array__set_len(array, len);
          u8* const new_bytes = slice_array__get_items(array);
          memcpy(new_bytes, bytes, len);
          bytes = new_bytes;
     }

     t_any fn_args[2] = {{.bytes = {[15] = tid__byte}}, {.bytes = {[15] = tid__byte}}};
     for (u64 idx_1 = 0; idx_1 < len - 1; idx_1 += 1) {
          fn_args[0].bytes[0]  = bytes[idx_1];

          for (u64 idx_2 = idx_1 + 1; idx_2 < len; idx_2 += 1) {
               fn_args[1].bytes[0]  = bytes[idx_2];

               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    free((void*)array.qwords[0]);

                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || box_items[0].bytes[15] != tid__byte || box_items[1].bytes[15] != tid__byte) [[clang::unlikely]] {
                    free((void*)array.qwords[0]);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[0].bytes[0] = box_items[0].bytes[0];
               bytes[idx_2]        = box_items[1].bytes[0];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          bytes[idx_1] = fn_args[0].bytes[0];
     }
     ref_cnt__dec(thrd_data, fn);

     return array;
}

static t_any short_byte_array__each_to_each_kv__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8 const len = short_byte_array__get_len(array);
     t_any fn_args[4] = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     for (u8 idx_1 = 0; (i8)idx_1 < (i8)len - (i8)1; idx_1 += 1) {
          fn_args[0].bytes[0] = idx_1;
          fn_args[1].bytes[0] = array.bytes[idx_1];

          for (u8 idx_2 = idx_1 + 1; idx_2 < len; idx_2 += 1) {
               fn_args[2].bytes[0] = idx_2;
               fn_args[3].bytes[0] = array.bytes[idx_2];

               t_any     const box = common_fn__call__half_own(thrd_data, fn, fn_args, 4, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || box_items[0].bytes[15] != tid__byte || box_items[1].bytes[15] != tid__byte) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[1].bytes[0] = box_items[0].bytes[0];
               array.bytes[idx_2] = box_items[1].bytes[0];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          array.bytes[idx_1] = fn_args[1].bytes[0];
     }
     ref_cnt__dec(thrd_data, fn);

     return array;
}

static t_any long_byte_array__each_to_each_kv__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(array);
     u32 const slice_offset = slice__get_offset(array);
     u8*       bytes        = &((u8*)slice_array__get_items(array))[slice_offset];
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt != 1) {
          if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

          array = long_byte_array__new(len);
          array = slice__set_metadata(array, 0, slice_len);
          slice_array__set_len(array, len);
          u8* const new_bytes = slice_array__get_items(array);
          memcpy(new_bytes, bytes, len);
          bytes = new_bytes;
     }

     t_any fn_args[4] = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     for (u64 idx_1 = 0; idx_1 < len - 1; idx_1 += 1) {
          fn_args[0].qwords[0] = idx_1;
          fn_args[1].bytes[0]  = bytes[idx_1];

          for (u64 idx_2 = idx_1 + 1; idx_2 < len; idx_2 += 1) {
               fn_args[2].qwords[0] = idx_2;
               fn_args[3].bytes[0]  = bytes[idx_2];

               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 4, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    free((void*)array.qwords[0]);

                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || box_items[0].bytes[15] != tid__byte || box_items[1].bytes[15] != tid__byte) [[clang::unlikely]] {
                    free((void*)array.qwords[0]);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[1].bytes[0] = box_items[0].bytes[0];
               bytes[idx_2]        = box_items[1].bytes[0];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          bytes[idx_1] = fn_args[1].bytes[0];
     }
     ref_cnt__dec(thrd_data, fn);

     return array;
}

static t_any short_byte_array__filter__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any byte       = {.bytes = {[15] = tid__byte}};
     t_any result     = {.bytes = {[15] = tid__short_byte_array}};
     u8    result_len = 0;
     u8    array_len  = short_byte_array__get_len(array);

     for (u8 idx = 0; idx < array_len; idx += 1) {
          byte.bytes[0] = array.bytes[idx];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 1) {
               result.bytes[result_len] = byte.bytes[0];
               result_len              += 1;
          }
     }

     ref_cnt__dec(thrd_data, fn);
     return short_byte_array__set_len(result, result_len);
}

static t_any long_byte_array__filter__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset   = slice__get_offset(array);
     u32       const slice_len      = slice__get_len(array);
     const u8* const original_bytes = &((const u8*)slice_array__get_items(array))[slice_offset];
     u64       const ref_cnt        = get_ref_cnt(array);
     u64       const array_len      = slice_array__get_len(array);
     u64       const len            = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64  part_lens[64];
     part_lens[0]      = 1;
     u8   current_part = 0;
     u64  odd_is_pass;

     t_any byte = {.bytes = {[15] = tid__byte}};
     {
          byte.bytes[0] = original_bytes[0];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, array);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          odd_is_pass = fn_result.bytes[0] ^ 1;
     }

     t_any     dst_array  = array;
     u8*       dst_bytes  = slice_array__get_items(dst_array);
     const u8* src_bytes  = original_bytes;
     bool      dst_is_mut = ref_cnt == 1;
     u64       dst_len    = 0;
     for (u64 idx = 1; idx < len; idx += 1) {
          byte.bytes[0] = original_bytes[idx];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_array.qwords[0] != array.qwords[0]) ref_cnt__dec(thrd_data, dst_array);
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
                    dst_array  = long_byte_array__new(len - droped_len);
                    dst_bytes  = slice_array__get_items(dst_array);
                    dst_is_mut = true;
               }

               for (u8 part_idx = 0; part_idx < 64; part_idx += 1) {
                    u64 const part_len = part_lens[part_idx];
                    if ((part_idx & 1) == odd_is_pass) {
                         memmove(&dst_bytes[dst_len], src_bytes, part_len);
                         dst_len += part_len;
                    }
                    src_bytes = &src_bytes[part_len];
               }

               current_part = 0;
               part_lens[0] = 1;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (src_bytes == original_bytes) {
          u64 from = -1;
          u64 to;

          switch (current_part) {
          case 0:
               if (odd_is_pass == 0) return array;
               ref_cnt__dec(thrd_data, array);
               return (const t_any){.bytes = {[15] = tid__short_byte_array}};
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

          if (from != (u64)-1) return long_byte_array__slice__own(thrd_data, array, from, to, owner);

          typedef u64 t_vec __attribute__((ext_vector_type(8), aligned(alignof(u64))));

          if (!dst_is_mut) {
               u64         droped_len = 0;
               u8          part_idx   = 8;
               t_vec const mask       = __builtin_convertvector(u8_to_v_8_bool(odd_is_pass ? 0x55 : 0xaa), t_vec) * -1;
               for (; part_idx <= current_part; part_idx += 8)
                    droped_len += __builtin_reduce_add(*(const t_vec*)&part_lens[part_idx - 8] & mask);
               for (part_idx -= odd_is_pass ? 8 : 7; part_idx <= current_part; droped_len += part_lens[part_idx], part_idx += 2);

               dst_array = long_byte_array__new(len - droped_len);
               dst_bytes = slice_array__get_items(dst_array);
          }
     }

     for (u8 part_idx = 0; part_idx <= current_part; part_idx += 1) {
          u64 const part_len = part_lens[part_idx];
          if ((part_idx & 1) == odd_is_pass) {
               memmove(&dst_bytes[dst_len], src_bytes, part_len);
               dst_len += part_len;
          }
          src_bytes = &src_bytes[part_len];
     }

     if (dst_array.qwords[0] != array.qwords[0]) ref_cnt__dec(thrd_data, array);

     if (dst_len < 15) {
          t_any short_array;
          short_array.raw_bits  = dst_len == 0 ? 0 : (*(ua_u128*)&dst_bytes[(i64)dst_len - 16]) >> (16 - dst_len) * 8;
          short_array.bytes[15] = tid__short_byte_array;
          short_array           = short_byte_array__set_len(short_array, dst_len);

          ref_cnt__dec(thrd_data, dst_array);
          return short_array;
     }

     slice_array__set_len(dst_array, dst_len);
     dst_array = slice__set_metadata(dst_array, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
     return dst_array;
}

static t_any short_byte_array__filter_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any kv[2]      = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     t_any result     = {.bytes = {[15] = tid__short_byte_array}};
     u8    result_len = 0;
     u8    array_len  = short_byte_array__get_len(array);

     for (u8 idx = 0; idx < array_len; idx += 1) {
          kv[0].bytes[0] = idx;
          kv[1].bytes[0] = array.bytes[idx];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 1) {
               result.bytes[result_len]  = kv[1].bytes[0];
               result_len               += 1;
          }
     }

     ref_cnt__dec(thrd_data, fn);
     return short_byte_array__set_len(result, result_len);
}

static t_any long_byte_array__filter_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset   = slice__get_offset(array);
     u32       const slice_len      = slice__get_len(array);
     const u8* const original_bytes = &((const u8*)slice_array__get_items(array))[slice_offset];
     u64       const ref_cnt        = get_ref_cnt(array);
     u64       const array_len      = slice_array__get_len(array);
     u64       const len            = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64  part_lens[64];
     part_lens[0]      = 1;
     u8   current_part = 0;
     u64  odd_is_pass;

     t_any kv[2] = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     {
          kv[1].bytes[0] = original_bytes[0];
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

     t_any     dst_array  = array;
     u8*       dst_bytes  = slice_array__get_items(dst_array);
     const u8* src_bytes  = original_bytes;
     bool      dst_is_mut = ref_cnt == 1;
     u64       dst_len    = 0;
     for (u64 idx = 1; idx < len; idx += 1) {
          kv[0].qwords[0] = idx;
          kv[1].bytes[0]  = original_bytes[idx];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_array.qwords[0] != array.qwords[0]) ref_cnt__dec(thrd_data, dst_array);
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
                    dst_array  = long_byte_array__new(len - droped_len);
                    dst_bytes  = slice_array__get_items(dst_array);
                    dst_is_mut = true;
               }

               for (u8 part_idx = 0; part_idx < 64; part_idx += 1) {
                    u64 const part_len = part_lens[part_idx];
                    if ((part_idx & 1) == odd_is_pass) {
                         memmove(&dst_bytes[dst_len], src_bytes, part_len);
                         dst_len += part_len;
                    }
                    src_bytes = &src_bytes[part_len];
               }

               current_part = 0;
               part_lens[0] = 1;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (src_bytes == original_bytes) {
          u64 from = -1;
          u64 to;

          switch (current_part) {
          case 0:
               if (odd_is_pass == 0) return array;
               ref_cnt__dec(thrd_data, array);
               return (const t_any){.bytes = {[15] = tid__short_byte_array}};
          case 1:
               from = odd_is_pass ? part_lens[0] : 0;
               to   = odd_is_pass ? len : part_lens[0];
               break;
          case 2:
               if (odd_is_pass == 0) {
                    from = part_lens[0];
                    to   = from + part_lens[1];
               }
               break;
          }

          if (from != (u64)-1) return long_byte_array__slice__own(thrd_data, array, from, to, owner);

          typedef u64 t_vec __attribute__((ext_vector_type(8), aligned(alignof(u64))));

          if (!dst_is_mut) {
               u64 droped_len = 0;
               u8  part_idx   = 8;
               for (; part_idx <= current_part; part_idx += 8)
                    droped_len += __builtin_reduce_add(
                         *(const t_vec*)&part_lens[part_idx - 8] &
                         (__builtin_convertvector(u8_to_v_8_bool(odd_is_pass ? 0x55 : 0xaa), t_vec) * -1)
                    );
               for (part_idx -= odd_is_pass ? 8 : 7; part_idx <= current_part; droped_len += part_lens[part_idx], part_idx += 2);

               dst_array = long_byte_array__new(len - droped_len);
               dst_bytes = slice_array__get_items(dst_array);
          }
     }

     for (u8 part_idx = 0; part_idx <= current_part; part_idx += 1) {
          u64 const part_len = part_lens[part_idx];
          if ((part_idx & 1) == odd_is_pass) {
               memmove(&dst_bytes[dst_len], src_bytes, part_len);
               dst_len += part_len;
          }
          src_bytes = &src_bytes[part_len];
     }

     if (dst_array.qwords[0] != array.qwords[0]) ref_cnt__dec(thrd_data, array);

     if (dst_len < 15) {
          t_any short_array;
          short_array.raw_bits  = dst_len == 0 ? 0 : (*(ua_u128*)&dst_bytes[(i64)dst_len - 16]) >> (16 - dst_len) * 8;
          short_array.bytes[15] = tid__short_byte_array;
          short_array           = short_byte_array__set_len(short_array, dst_len);

          ref_cnt__dec(thrd_data, dst_array);
          return short_array;
     }

     slice_array__set_len(dst_array, dst_len);
     dst_array = slice__set_metadata(dst_array, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
     return dst_array;
}

static t_any short_byte_array__foldl__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any fn_args[2];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};
     u8    array_len  = short_byte_array__get_len(array);
     for (u8 idx = 0; idx < array_len; idx += 1) {
          fn_args[1].bytes[0] = array.bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_byte_array__foldl__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[1].bytes[0] = bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)array.qwords[0]);
     else if (ref_cnt != 0 && state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_byte_array__foldl1__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8 len = short_byte_array__get_len(array);
     if (len == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {array.bytes[0], [15] = tid__byte}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};
     for (u8 idx = 1; idx < len; idx += 1) {
          fn_args[1].bytes[0] = array.bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_byte_array__foldl1__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (len == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {bytes[0], [15] = tid__byte}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};
     for (u64 idx = 1; idx < len; idx += 1) {
          fn_args[1].bytes[0] = bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)array.qwords[0]);
     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_byte_array__foldl_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     u8    array_len  = short_byte_array__get_len(array);
     for (u8 idx = 0; idx < array_len; idx += 1) {
          fn_args[1].bytes[0] = idx;
          fn_args[2].bytes[0] = array.bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_byte_array__foldl_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[1].qwords[0] = idx;
          fn_args[2].bytes[0]  = bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)array.qwords[0]);
     else if (ref_cnt != 0 && state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_byte_array__foldr__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any fn_args[2];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};
     u8    array_len  = short_byte_array__get_len(array);
     for (u8 idx = array_len - 1; idx != (u8)-1; idx -= 1) {
          fn_args[1].bytes[0] = array.bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_byte_array__foldr__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = state;
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};
     for (u64 idx = len - 1; idx != -1; idx -= 1) {
          fn_args[1].bytes[0] = bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)array.qwords[0]);
     else if (ref_cnt != 0 && state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_byte_array__foldr1__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8 len = short_byte_array__get_len(array);
     if (len == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {array.bytes[len - 1], [15] = tid__byte}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};
     for (u8 idx = len - 2; idx != (u8)-1; idx -= 1) {
          fn_args[1].bytes[0] = array.bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_byte_array__foldr1__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (len == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {bytes[len - 1], [15] = tid__byte}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};
     for (u64 idx = len - 2; idx != (u64)-1; idx -= 1) {
          fn_args[1].bytes[0] = bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)array.qwords[0]);
     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_byte_array__foldr_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     u8    array_len  = short_byte_array__get_len(array);
     for (u8 idx = array_len - 1; idx != (u8)-1; idx -= 1) {
          fn_args[1].bytes[0] = idx;
          fn_args[2].bytes[0] = array.bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any long_byte_array__foldr_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     for (u64 idx = len - 1; idx != (u64)-1; idx -= 1) {
          fn_args[1].qwords[0] = idx;
          fn_args[2].bytes[0]  = bytes[idx];
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) free((void*)array.qwords[0]);
     else if (ref_cnt != 0 && state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

static t_any short_byte_array__look_from_end_in(t_any const array, u8 const looked_byte) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8  const len              = short_byte_array__get_len(array);
     u16 const looked_byte_mask = v_16_bool_to_u16(__builtin_convertvector(array.vector == looked_byte, v_16_bool)) & (((u16)1 << len) - 1);
     return looked_byte_mask == 0 ? null : (const t_any){.structure = {.value = sizeof(int) * 8 - 1 - __builtin_clz(looked_byte_mask), .type = tid__int}};
}

core_basic u64 look_byte_from_end(const u8* const bytes, u64 const bytes_len, u8 const looked_byte) {
     u64 remain_bytes = bytes_len;
     if (remain_bytes >= 64) {
          typedef u8 v_64_u8 __attribute__((ext_vector_type(64), aligned(1)));

          v_64_u8 const looked_byte_vec = looked_byte;
          do {
               u64 looked_byte_mask = v_64_bool_to_u64(__builtin_convertvector(*(const v_64_u8*)&bytes[remain_bytes - 64] == looked_byte_vec, v_64_bool));
               if (looked_byte_mask != 0) return remain_bytes + sizeof(long long) * 8 - 65 - __builtin_clzll(looked_byte_mask);
               remain_bytes -= 64;
          } while (remain_bytes >= 64);
     }

     switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));

          u64 looked_byte_mask =
               v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)bytes == looked_byte, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)&bytes[remain_bytes - 32] == looked_byte, v_32_bool)) << (remain_bytes - 32);

          return looked_byte_mask == 0 ? -1 : sizeof(long long) * 8 - 1 - __builtin_clzll(looked_byte_mask);
     }
     case 5: {
          assume(remain_bytes > 16 && remain_bytes <= 32);

          typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));

          u32 looked_byte_mask =
               v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)bytes == looked_byte, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)&bytes[remain_bytes - 16] == looked_byte, v_16_bool)) << (remain_bytes - 16);

          return looked_byte_mask == 0 ? -1 : sizeof(long) * 8 - 1 - __builtin_clzl(looked_byte_mask);
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 16);

          typedef u8 v_8_u8 __attribute__((ext_vector_type(8), aligned(1)));

          u16 looked_byte_mask =
               v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)bytes == looked_byte, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)&bytes[remain_bytes - 8] == looked_byte, v_8_bool)) << (remain_bytes - 8);

          return looked_byte_mask == 0 ? -1 : sizeof(int) * 8 - 1 - __builtin_clz(looked_byte_mask);
     }
     case 3: {
          assume(remain_bytes > 4 && remain_bytes <= 8);

          u64 const delta_bits = (remain_bytes - 4) * 8;

          u64 neg_looked_byte_vec = (u8)~looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 16;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u64 looked_byte_mask = (*(const ua_u32*)bytes | (u64)*(const ua_u32*)&bytes[remain_bytes - 4] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101'0101'0101ull;

          return looked_byte_mask == 0 ? -1 : (u64)(sizeof(long long) * 8 - 1 - __builtin_clzll(looked_byte_mask)) / 8;
     }
     case 2: {
          assume(remain_bytes >= 2 && remain_bytes <= 4);

          u32 const delta_bits = (remain_bytes - 2) * 8;

          u32 neg_looked_byte_vec = (u8)~looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u32 looked_byte_mask = (*(const ua_u16*)bytes | (u32)*(const ua_u16*)&bytes[remain_bytes - 2] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101ul;

          return looked_byte_mask == 0 ? -1 : (sizeof(long) * 8 - 1 - __builtin_clzl(looked_byte_mask)) / 8;
     }
     case 1:
          return bytes[0] == looked_byte ? 0 : -1;
     case 0:
          return -1;
     default:
          unreachable;
     }
}

core_basic inline u64 look_2_bytes_from_end(const u8* const bytes, u64 const bytes_len, u16 const looked_bytes) {
     u64 remain_bytes = bytes_len;
     if (remain_bytes >= 64) {
          typedef u8 v_64_u8 __attribute__((ext_vector_type(64), aligned(1)));

          v_64_u8 const looked_first_byte_vec  = looked_bytes;
          v_64_u8 const looked_second_byte_vec = looked_bytes >> 8;
          do {
               v_64_u8 const bytes_vec         = *(const v_64_u8*)&bytes[remain_bytes - 64];
               u64           looked_bytes_mask =
                    v_64_bool_to_u64(__builtin_convertvector(bytes_vec == looked_first_byte_vec, v_64_bool)) &
                    v_64_bool_to_u64(__builtin_convertvector(bytes_vec == looked_second_byte_vec, v_64_bool)) >> 1;

               if (looked_bytes_mask != 0) return remain_bytes + sizeof(long long) * 8 - 65 - __builtin_clzll(looked_bytes_mask);

               remain_bytes -= 63;
          } while (remain_bytes >= 64);
     }
     switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));

          v_32_u8 const looked_first_byte_vec  = looked_bytes;
          v_32_u8 const looked_second_byte_vec = looked_bytes >> 8;

          v_32_u8 const low_part  = *(const v_32_u8*)bytes;
          v_32_u8 const high_part = *(const v_32_u8*)&bytes[remain_bytes - 32];

          u64 looked_bytes_mask =
          (
               v_32_bool_to_u32(__builtin_convertvector(low_part == looked_first_byte_vec, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_first_byte_vec, v_32_bool)) << (remain_bytes - 32)
          ) & (
               v_32_bool_to_u32(__builtin_convertvector(low_part == looked_second_byte_vec, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_second_byte_vec, v_32_bool)) << (remain_bytes - 32)
          ) >> 1;

          return looked_bytes_mask == 0 ? -1 : sizeof(long long) * 8 - 1 - __builtin_clzll(looked_bytes_mask);
     }
     case 5: {
          assume(remain_bytes > 16 && remain_bytes <= 32);

          typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));

          v_16_u8 const looked_first_byte_vec  = looked_bytes;
          v_16_u8 const looked_second_byte_vec = looked_bytes >> 8;

          v_16_u8 const low_part  = *(const v_16_u8*)bytes;
          v_16_u8 const high_part = *(const v_16_u8*)&bytes[remain_bytes - 16];

          u32 looked_bytes_mask =
          (
               v_16_bool_to_u16(__builtin_convertvector(low_part == looked_first_byte_vec, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_first_byte_vec, v_16_bool)) << (remain_bytes - 16)
          ) & (
               v_16_bool_to_u16(__builtin_convertvector(low_part == looked_second_byte_vec, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_second_byte_vec, v_16_bool)) << (remain_bytes - 16)
          ) >> 1;

          return looked_bytes_mask == 0 ? -1 : sizeof(long) * 8 - 1 - __builtin_clzl(looked_bytes_mask);
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 16);

          typedef u8 v_8_u8 __attribute__((ext_vector_type(8), aligned(1)));

          v_8_u8 const looked_first_byte_vec  = looked_bytes;
          v_8_u8 const looked_second_byte_vec = looked_bytes >> 8;

          v_8_u8 const low_part  = *(const v_8_u8*)bytes;
          v_8_u8 const high_part = *(const v_8_u8*)&bytes[remain_bytes - 8];

          u16 looked_bytes_mask =
          (
               v_8_bool_to_u8(__builtin_convertvector(low_part == looked_first_byte_vec, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_first_byte_vec, v_8_bool)) << (remain_bytes - 8)
          ) & (
               v_8_bool_to_u8(__builtin_convertvector(low_part == looked_second_byte_vec, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_second_byte_vec, v_8_bool)) << (remain_bytes - 8)
          ) >> 1;

          return looked_bytes_mask == 0 ? -1 : sizeof(int) * 8 - 1 - __builtin_clz(looked_bytes_mask);
     }
     case 3: {
          assume(remain_bytes > 4 && remain_bytes <= 8);

          typedef u64 v_2_u64 __attribute__((ext_vector_type(2)));

          u64 const delta_bits = (remain_bytes - 4) * 8;
          u64 const bytes_vec  = *(const ua_u32*)bytes | (u64)*(const ua_u32*)&bytes[remain_bytes - 4] << delta_bits;

          v_2_u64 neg_looked_byte_2_vecs  = (const v_2_u64){looked_bytes & 255, looked_bytes >> 8} ^ 255;
          neg_looked_byte_2_vecs         |= neg_looked_byte_2_vecs << 8;
          neg_looked_byte_2_vecs         |= neg_looked_byte_2_vecs << 16;
          neg_looked_byte_2_vecs         |= neg_looked_byte_2_vecs << delta_bits;

          v_2_u64 looked_byte_2_masks = bytes_vec ^ neg_looked_byte_2_vecs;
          looked_byte_2_masks        &= looked_byte_2_masks >> 4;
          looked_byte_2_masks        &= looked_byte_2_masks >> 2;
          looked_byte_2_masks        &= looked_byte_2_masks >> 1;
          looked_byte_2_masks        &= 0x0101'0101'0101'0101ull;

          u64 const looked_bytes_mask = __builtin_reduce_and(looked_byte_2_masks >> (const v_2_u64){0, 8});
          return looked_bytes_mask == 0 ? -1 : (u64)(sizeof(long long) * 8 - 1 - __builtin_clzll(looked_bytes_mask)) / 8;
     }
     case 2: {
          assume(remain_bytes >= 2 && remain_bytes <= 4);

          u32 const delta_bits = (remain_bytes - 2) * 8;
          u32 const bytes_vec  = *(const ua_u16*)bytes | (u32)*(const ua_u16*)&bytes[remain_bytes - 2] << delta_bits;

          u64 neg_looked_byte_2_vecs = ((looked_bytes & 255) | (u64)(looked_bytes >> 8) << 32) ^ 0x0000'00ff'0000'00ffull;
          neg_looked_byte_2_vecs    |= neg_looked_byte_2_vecs << 8;
          neg_looked_byte_2_vecs    |= neg_looked_byte_2_vecs << 16;
          neg_looked_byte_2_vecs    |= (neg_looked_byte_2_vecs & 0xffff'fffful) << delta_bits | (neg_looked_byte_2_vecs & 0xffff'ffff'0000'0000ull) << delta_bits;

          u64 looked_byte_2_masks = (bytes_vec | (u64)bytes_vec << 32) ^ neg_looked_byte_2_vecs;
          looked_byte_2_masks    &= looked_byte_2_masks >> 4;
          looked_byte_2_masks    &= looked_byte_2_masks >> 2;
          looked_byte_2_masks    &= looked_byte_2_masks >> 1;
          looked_byte_2_masks    &= 0x0101'0101'0101'0101ull;

          u32 const looked_bytes_mask = (u32)looked_byte_2_masks & (u32)(looked_byte_2_masks >> 40);
          return looked_bytes_mask == 0 ? -1 : (sizeof(long) * 8 - 1 - __builtin_clzl(looked_bytes_mask)) / 8;
     }
     case 1: case 0:
          return -1;
     default:
          unreachable;
     }
}

static t_any long_byte_array__look_from_end_in__own(t_thrd_data* const thrd_data, t_any const array, u8 const looked_byte) {
     assume(array.bytes[15] == tid__byte_array);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);
     u64 const looked_byte_idx = look_byte_from_end(bytes, len, looked_byte);
     if (ref_cnt == 1) free((void*)array.qwords[0]);

     return looked_byte_idx == (u64)-1 ? null : (const t_any){.structure = {.value = looked_byte_idx, .type = tid__int}};
}

[[clang::always_inline]]
static t_any short_byte_array__look_in(t_any const array, u8 const looked_byte) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8  const len              = short_byte_array__get_len(array);
     u16 const looked_byte_mask = v_16_bool_to_u16(__builtin_convertvector(array.vector == looked_byte, v_16_bool)) & (((u16)1 << len) - 1);
     return looked_byte_mask == 0 ? null : (const t_any){.structure = {.value = __builtin_ctz(looked_byte_mask), .type = tid__int}};
}

core_basic inline u64 look_byte_from_begin(const u8* const bytes, u64 const bytes_len, u8 const looked_byte) {
     u64 idx = 0;
     if (bytes_len - idx >= 64) {
          typedef u8 v_64_u8 __attribute__((ext_vector_type(64), aligned(1)));

          v_64_u8 const looked_byte_vec = looked_byte;
          do {
               u64 looked_byte_mask = v_64_bool_to_u64(__builtin_convertvector(*(const v_64_u8*)&bytes[idx] == looked_byte_vec, v_64_bool));
               if (looked_byte_mask != 0) return idx + __builtin_ctzll(looked_byte_mask);
               idx += 64;
          } while (bytes_len - idx >= 64);
     }

     u8 const remain_bytes = bytes_len - idx;
     switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));

          u64 looked_byte_mask =
               v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)&bytes[idx] == looked_byte, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)&bytes[bytes_len - 32] == looked_byte, v_32_bool)) << (remain_bytes - 32);

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctzll(looked_byte_mask);
     }
     case 5: {
          assume(remain_bytes > 16 && remain_bytes <= 32);

          typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));

          u32 looked_byte_mask =
               v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)&bytes[idx] == looked_byte, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)&bytes[bytes_len - 16] == looked_byte, v_16_bool)) << (remain_bytes - 16);

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctzll(looked_byte_mask);
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 16);

          typedef u8 v_8_u8 __attribute__((ext_vector_type(8), aligned(1)));

          u16 looked_byte_mask =
               v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)&bytes[idx] == looked_byte, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)&bytes[bytes_len - 8] == looked_byte, v_8_bool)) << (remain_bytes - 8);

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctz(looked_byte_mask);
     }
     case 3: {
          assume(remain_bytes > 4 && remain_bytes <= 8);

          u64 const delta_bits = (remain_bytes - 4) * 8;

          u64 neg_looked_byte_vec = (u8)~looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 16;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u64 looked_byte_mask = (*(const ua_u32*)&bytes[idx] | (u64)*(const ua_u32*)&bytes[bytes_len - 4] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101'0101'0101ull;

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctzll(looked_byte_mask) / 8;
     }
     case 2: {
          assume(remain_bytes >= 2 && remain_bytes <= 4);

          u32 const delta_bits = (remain_bytes - 2) * 8;

          u32 neg_looked_byte_vec = (u8)~looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u32 looked_byte_mask = (*(const ua_u16*)&bytes[idx] | (u32)*(const ua_u16*)&bytes[bytes_len - 2] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101ul;

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctzll(looked_byte_mask) / 8;
     }
     case 1:
          return bytes[idx] == looked_byte ? idx : -1;
     case 0:
          return -1;
     default:
          unreachable;
     }
}

core_basic inline u64 look_2_bytes_from_begin(const u8* const bytes, u64 const bytes_len, u16 const looked_bytes) {
     u64 idx = 0;
     if (bytes_len - idx >= 64) {
          typedef u8 v_64_u8 __attribute__((ext_vector_type(64), aligned(1)));

          v_64_u8 const looked_first_byte_vec  = looked_bytes;
          v_64_u8 const looked_second_byte_vec = looked_bytes >> 8;
          do {
               v_64_u8 const bytes_vec         = *(const v_64_u8*)&bytes[idx];
               u64           looked_bytes_mask =
                    v_64_bool_to_u64(__builtin_convertvector(bytes_vec == looked_first_byte_vec, v_64_bool)) &
                    v_64_bool_to_u64(__builtin_convertvector(bytes_vec == looked_second_byte_vec, v_64_bool)) >> 1;

               if (looked_bytes_mask != 0) return idx + __builtin_ctzll(looked_bytes_mask);

               idx += 63;
          } while (bytes_len - idx >= 64);
     }

     u8 const remain_bytes = bytes_len - idx;
     switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));

          v_32_u8 const looked_first_byte_vec  = looked_bytes;
          v_32_u8 const looked_second_byte_vec = looked_bytes >> 8;

          v_32_u8 const low_part  = *(const v_32_u8*)&bytes[idx];
          v_32_u8 const high_part = *(const v_32_u8*)&bytes[bytes_len - 32];

          u64 looked_bytes_mask =
               (
                    v_32_bool_to_u32(__builtin_convertvector(low_part == looked_first_byte_vec, v_32_bool)) |
                    (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_first_byte_vec, v_32_bool)) << (remain_bytes - 32)
               ) & (
                    v_32_bool_to_u32(__builtin_convertvector(low_part == looked_second_byte_vec, v_32_bool)) |
                    (u64)v_32_bool_to_u32(__builtin_convertvector(high_part == looked_second_byte_vec, v_32_bool)) << (remain_bytes - 32)
               ) >> 1;

          return looked_bytes_mask == 0 ? -1 : idx + __builtin_ctzll(looked_bytes_mask);
     }
     case 5: {
          assume(remain_bytes > 16 && remain_bytes <= 32);

          typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));

          v_16_u8 const looked_first_byte_vec  = looked_bytes;
          v_16_u8 const looked_second_byte_vec = looked_bytes >> 8;

          v_16_u8 const low_part  = *(const v_16_u8*)&bytes[idx];
          v_16_u8 const high_part = *(const v_16_u8*)&bytes[bytes_len - 16];

          u32 looked_bytes_mask =
          (
               v_16_bool_to_u16(__builtin_convertvector(low_part == looked_first_byte_vec, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_first_byte_vec, v_16_bool)) << (remain_bytes - 16)
          ) & (
               v_16_bool_to_u16(__builtin_convertvector(low_part == looked_second_byte_vec, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(high_part == looked_second_byte_vec, v_16_bool)) << (remain_bytes - 16)
          ) >> 1;

          return looked_bytes_mask == 0 ? -1 : idx + __builtin_ctzl(looked_bytes_mask);
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 16);

          typedef u8 v_8_u8 __attribute__((ext_vector_type(8), aligned(1)));

          v_8_u8 const looked_first_byte_vec  = looked_bytes;
          v_8_u8 const looked_second_byte_vec = looked_bytes >> 8;

          v_8_u8 const low_part  = *(const v_8_u8*)&bytes[idx];
          v_8_u8 const high_part = *(const v_8_u8*)&bytes[bytes_len - 8];

          u16 looked_bytes_mask =
          (
               v_8_bool_to_u8(__builtin_convertvector(low_part == looked_first_byte_vec, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_first_byte_vec, v_8_bool)) << (remain_bytes - 8)
          ) & (
               v_8_bool_to_u8(__builtin_convertvector(low_part == looked_second_byte_vec, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(high_part == looked_second_byte_vec, v_8_bool)) << (remain_bytes - 8)
          ) >> 1;

          return looked_bytes_mask == 0 ? -1 : idx + __builtin_ctz(looked_bytes_mask);
     }
     case 3: {
          assume(remain_bytes > 4 && remain_bytes <= 8);

          typedef u64 v_2_u64 __attribute__((ext_vector_type(2)));

          u64 const delta_bits = (remain_bytes - 4) * 8;
          u64 const bytes_vec  = *(const ua_u32*)&bytes[idx] | (u64)*(const ua_u32*)&bytes[bytes_len - 4] << delta_bits;

          v_2_u64 neg_looked_byte_2_vecs  = (const v_2_u64){looked_bytes & 255, looked_bytes >> 8} ^ 255;
          neg_looked_byte_2_vecs         |= neg_looked_byte_2_vecs << 8;
          neg_looked_byte_2_vecs         |= neg_looked_byte_2_vecs << 16;
          neg_looked_byte_2_vecs         |= neg_looked_byte_2_vecs << delta_bits;

          v_2_u64 looked_byte_2_masks = bytes_vec ^ neg_looked_byte_2_vecs;
          looked_byte_2_masks        &= looked_byte_2_masks >> 4;
          looked_byte_2_masks        &= looked_byte_2_masks >> 2;
          looked_byte_2_masks        &= looked_byte_2_masks >> 1;
          looked_byte_2_masks        &= 0x0101'0101'0101'0101ull;

          u64 const looked_bytes_mask = __builtin_reduce_and(looked_byte_2_masks >> (const v_2_u64){0, 8});
          return looked_bytes_mask == 0 ? -1 : idx + __builtin_ctzll(looked_bytes_mask) / 8;
     }
     case 2: {
          assume(remain_bytes >= 2 && remain_bytes <= 4);

          u32 const delta_bits = (remain_bytes - 2) * 8;
          u32 const bytes_vec  = *(const ua_u16*)&bytes[idx] | (u32)*(const ua_u16*)&bytes[bytes_len - 2] << delta_bits;

          u64 neg_looked_byte_2_vecs = ((looked_bytes & 255) | (u64)(looked_bytes >> 8) << 32) ^ 0x0000'00ff'0000'00ffull;
          neg_looked_byte_2_vecs    |= neg_looked_byte_2_vecs << 8;
          neg_looked_byte_2_vecs    |= neg_looked_byte_2_vecs << 16;
          neg_looked_byte_2_vecs    |= (neg_looked_byte_2_vecs & 0xffff'fffful) << delta_bits | (neg_looked_byte_2_vecs & 0xffff'ffff'0000'0000ull) << delta_bits;

          u64 looked_byte_2_masks = (bytes_vec | (u64)bytes_vec << 32) ^ neg_looked_byte_2_vecs;
          looked_byte_2_masks    &= looked_byte_2_masks >> 4;
          looked_byte_2_masks    &= looked_byte_2_masks >> 2;
          looked_byte_2_masks    &= looked_byte_2_masks >> 1;
          looked_byte_2_masks    &= 0x0101'0101'0101'0101ull;

          u32 const looked_bytes_mask = (u32)looked_byte_2_masks & (u32)(looked_byte_2_masks >> 40);
          return looked_bytes_mask == 0 ? -1 : idx + __builtin_ctzl(looked_bytes_mask) / 8;
     }
     case 1: case 0:
          return -1;
     default:
          unreachable;
     }
}

static t_any long_byte_array__look_in__own(t_thrd_data* const thrd_data, t_any const array, u8 const looked_byte) {
     assume(array.bytes[15] == tid__byte_array);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);
     u64 const looked_byte_idx = look_byte_from_begin(bytes, len, looked_byte);
     if (ref_cnt == 1) free((void*)array.qwords[0]);

     return looked_byte_idx == (u64)-1 ? null : (const t_any){.structure = {.value = looked_byte_idx, .type = tid__int}};
}

[[clang::always_inline]]
static t_any short_byte_array__look_other_from_end_in(t_any const array, u8 const not_looked_byte) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8  const len              = short_byte_array__get_len(array);
     u16 const looked_byte_mask = v_16_bool_to_u16(__builtin_convertvector(array.vector != not_looked_byte, v_16_bool)) & (((u16)1 << len) - 1);
     return looked_byte_mask == 0 ? null : (const t_any){.structure = {.value = sizeof(int) * 8 - 1 - __builtin_clz(looked_byte_mask), .type = tid__int}};
}

static u64 look_other_byte_from_end(const u8* const bytes, u64 const bytes_len, u8 const not_looked_byte) {
     u64 remain_bytes = bytes_len;
     if (remain_bytes >= 64) {
          typedef u8 v_64_u8 __attribute__((ext_vector_type(64), aligned(1)));

          v_64_u8 const not_looked_byte_vec = not_looked_byte;
          do {
               u64 looked_byte_mask = v_64_bool_to_u64(__builtin_convertvector(*(const v_64_u8*)&bytes[remain_bytes - 64] != not_looked_byte_vec, v_64_bool));
               if (looked_byte_mask != 0) return remain_bytes + sizeof(long long) * 8 - 65 - __builtin_clzll(looked_byte_mask);
               remain_bytes -= 64;
          } while (remain_bytes >= 64);
     }

     switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));

          u64 looked_byte_mask =
               v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)bytes != not_looked_byte, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)&bytes[remain_bytes - 32] != not_looked_byte, v_32_bool)) << (remain_bytes - 32);

          return looked_byte_mask == 0 ? -1 : sizeof(long long) * 8 - 1 - __builtin_clzll(looked_byte_mask);
     }
     case 5: {
          assume(remain_bytes > 16 && remain_bytes <= 32);

          typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));

          u32 looked_byte_mask =
               v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)bytes != not_looked_byte, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)&bytes[remain_bytes - 16] != not_looked_byte, v_16_bool)) << (remain_bytes - 16);

          return looked_byte_mask == 0 ? -1 : sizeof(long long) * 8 - 1 - __builtin_clzll(looked_byte_mask);
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 16);

          typedef u8 v_8_u8 __attribute__((ext_vector_type(8), aligned(1)));

          u16 looked_byte_mask =
               v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)bytes != not_looked_byte, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)&bytes[remain_bytes - 8] != not_looked_byte, v_8_bool)) << (remain_bytes - 8);

          return looked_byte_mask == 0 ? -1 : sizeof(int) * 8 - 1 - __builtin_clz(looked_byte_mask);
     }
     case 3: {
          assume(remain_bytes > 4 && remain_bytes <= 8);

          u64 const delta_bits = (remain_bytes - 4) * 8;

          u64 neg_looked_byte_vec = (u8)~not_looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 16;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u64 looked_byte_mask = (*(const ua_u32*)bytes | (u64)*(const ua_u32*)&bytes[remain_bytes - 4] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101'0101'0101ull;
          looked_byte_mask    ^= 0x0101'0101'0101'0101ull;
          looked_byte_mask    &= remain_bytes == 8 ? (u64)-1 : ((u64)1 << remain_bytes * 8) - 1;

          return looked_byte_mask == 0 ? -1 : (u64)(sizeof(long long) * 8 - 1 - __builtin_clzll(looked_byte_mask)) / 8;
     }
     case 2: {
          assume(remain_bytes >= 2 && remain_bytes <= 4);

          u32 const delta_bits = (remain_bytes - 2) * 8;

          u32 neg_looked_byte_vec = (u8)~not_looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u32 looked_byte_mask = (*(const ua_u16*)bytes | (u32)*(const ua_u16*)&bytes[remain_bytes - 2] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101ul;
          looked_byte_mask    ^= 0x0101'0101ul;
          looked_byte_mask    &= remain_bytes == 4 ? (u32)-1 : ((u32)1 << remain_bytes * 8) - 1;

          return looked_byte_mask == 0 ? -1 : (u64)(sizeof(long long) * 8 - 1 - __builtin_clzll(looked_byte_mask)) / 8;
     }
     case 1:
          return bytes[0] != not_looked_byte ? 0 : -1;
     case 0:
          return -1;
     default:
          unreachable;
     }
}

static t_any long_byte_array__look_other_from_end_in__own(t_thrd_data* const thrd_data, t_any const array, u8 const not_looked_byte) {
     assume(array.bytes[15] == tid__byte_array);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);
     u64 const looked_byte_idx = look_other_byte_from_end(bytes, len, not_looked_byte);
     if (ref_cnt == 1) free((void*)array.qwords[0]);

     return looked_byte_idx == (u64)-1 ? null : (const t_any){.structure = {.value = looked_byte_idx, .type = tid__int}};
}

static t_any short_byte_array__look_other_in(t_any const array, u8 const not_looked_byte) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8  const len              = short_byte_array__get_len(array);
     u16 const looked_byte_mask = v_16_bool_to_u16(__builtin_convertvector(array.vector != not_looked_byte, v_16_bool)) & (((u16)1 << len) - 1);
     return looked_byte_mask == 0 ? null : (const t_any){.structure = {.value = __builtin_ctz(looked_byte_mask), .type = tid__int}};
}

static u64 look_other_byte_from_begin(const u8* const bytes, u64 const bytes_len, u8 const not_looked_byte) {
     u64 idx = 0;
     if (bytes_len - idx >= 64) {
          typedef u8 v_64_u8 __attribute__((ext_vector_type(64), aligned(1)));

          v_64_u8 const not_looked_byte_vec = not_looked_byte;
          do {
               u64 looked_byte_mask = v_64_bool_to_u64(__builtin_convertvector(*(const v_64_u8*)&bytes[idx] != not_looked_byte_vec, v_64_bool));
               if (looked_byte_mask != 0) return idx + __builtin_ctzll(looked_byte_mask);
               idx += 64;
          } while (bytes_len - idx >= 64);
     }

     u8 const remain_bytes = bytes_len - idx;
     switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));

          u64 looked_byte_mask =
               v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)&bytes[idx] != not_looked_byte, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)&bytes[bytes_len - 32] != not_looked_byte, v_32_bool)) << (remain_bytes - 32);

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctzll(looked_byte_mask);
     }
     case 5: {
          assume(remain_bytes > 16 && remain_bytes <= 32);

          typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));

          u32 looked_byte_mask =
               v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)&bytes[idx] != not_looked_byte, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)&bytes[bytes_len - 16] != not_looked_byte, v_16_bool)) << (remain_bytes - 16);

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctzll(looked_byte_mask);
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 16);

          typedef u8 v_8_u8 __attribute__((ext_vector_type(8), aligned(1)));

          u16 looked_byte_mask =
               v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)&bytes[idx] != not_looked_byte, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)&bytes[bytes_len - 8] != not_looked_byte, v_8_bool)) << (remain_bytes - 8);

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctz(looked_byte_mask);
     }
     case 3: {
          assume(remain_bytes > 4 && remain_bytes <= 8);

          u64 const delta_bits = (remain_bytes - 4) * 8;

          u64 neg_looked_byte_vec = (u8)~not_looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 16;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u64 looked_byte_mask = (*(const ua_u32*)&bytes[idx] | (u64)*(const ua_u32*)&bytes[bytes_len - 4] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101'0101'0101ull;
          looked_byte_mask    ^= 0x0101'0101'0101'0101ull;
          looked_byte_mask    &= remain_bytes == 8 ? (u64)-1 : ((u64)1 << remain_bytes * 8) - 1;

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctzll(looked_byte_mask) / 8;
     }
     case 2: {
          assume(remain_bytes >= 2 && remain_bytes <= 4);

          u32 const delta_bits = (remain_bytes - 2) * 8;

          u32 neg_looked_byte_vec = (u8)~not_looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u32 looked_byte_mask = (*(const ua_u16*)&bytes[idx] | (u32)*(const ua_u16*)&bytes[bytes_len - 2] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101ul;
          looked_byte_mask    ^= 0x0101'0101ul;
          looked_byte_mask    &= remain_bytes == 4 ? (u32)-1 : ((u32)1 << remain_bytes * 8) - 1;

          return looked_byte_mask == 0 ? -1 : idx + __builtin_ctzll(looked_byte_mask) / 8;
     }
     case 1:
          return bytes[idx] != not_looked_byte ? idx : -1;
     case 0:
          return -1;
     default:
          unreachable;
     }
}

static t_any long_byte_array__look_other_in__own(t_thrd_data* const thrd_data, t_any const array, u8 const not_looked_byte) {
     assume(array.bytes[15] == tid__byte_array);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);
     u64 const looked_byte_idx = look_other_byte_from_begin(bytes, len, not_looked_byte);
     if (ref_cnt == 1) free((void*)array.qwords[0]);

     return looked_byte_idx == (u64)-1 ? null : (const t_any){.structure = {.value = looked_byte_idx, .type = tid__int}};
}

static t_any look_part_from_end_in__short_byte_array__short_byte_array(t_any const array, t_any const part) {
     assert(array.bytes[15] == tid__short_byte_array);
     assert(part.bytes[15] == tid__short_byte_array);

     u8 const array_len = short_byte_array__get_len(array);
     u8 const part_len  = short_byte_array__get_len(part);
     if (part_len == 0) return (const t_any){.structure = {.value = array_len, .type = tid__int}};
     if (part_len > array_len) return null;

     u16 occurrence_mask = -1;
     for (u8 idx = 0; idx < part_len; idx += 1)
          occurrence_mask &= v_16_bool_to_u16(__builtin_convertvector(array.vector == part.vector[idx], v_16_bool)) >> idx;
     occurrence_mask &= ((u16)1 << (array_len - part_len + 1)) - 1;
     return occurrence_mask == 0 ?
          null :
          (const t_any){.structure = {.value = sizeof(int) * 8 - __builtin_clz(occurrence_mask) - 1, .type = tid__int}};
}

[[clang::noinline]]
static u64 look_byte_part_from_end__rabin_karp(const u8* const array_bytes, u64 const array_len, const u8* const part_bytes, u64 const part_len) {
     u64 const rnd_num    = (static_rnd_num & 0x7fff'ffffull) + 256;
     u64       part_hash  = part_bytes[part_len - 1];
     u64       array_hash = array_bytes[array_len - 1];
     u64       power      = 1;

     for (u64 offset = 2; offset <= part_len; offset += 1) {
          part_hash  = (part_hash * rnd_num + part_bytes[part_len - offset]) % rabin_karp_q;
          array_hash = (array_hash * rnd_num + array_bytes[array_len - offset]) % rabin_karp_q;
          power      = power * rnd_num % rabin_karp_q;
     }

     for (u64 next_idx = array_len - part_len - 1;; next_idx -= 1) {
          if (part_hash == array_hash && scar__memcmp(&array_bytes[next_idx + 1], part_bytes, part_len) == 0) return next_idx + 1;
          if (next_idx == (u64)-1) return -1;

          array_hash = ((array_hash - (array_bytes[next_idx + part_len] * power) % rabin_karp_q + rabin_karp_q) * rnd_num + array_bytes[next_idx]) % rabin_karp_q;
     }
}

static t_any look_part_from_end_in__long_byte_array__byte_array__own(t_thrd_data* const thrd_data, t_any const array, t_any const part) {
     assume(array.bytes[15] == tid__byte_array);
     assume(part.bytes[15] == tid__short_byte_array || part.bytes[15] == tid__byte_array);

     u32       const array_slice_offset = slice__get_offset(array);
     u32       const array_slice_len    = slice__get_len(array);
     const u8* const array_bytes        = &((const u8*)slice_array__get_items(array))[array_slice_offset];
     u64       const array_ref_cnt      = get_ref_cnt(array);
     u64       const array_array_len    = slice_array__get_len(array);
     u64       const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     const u8* part_bytes;
     u64       part_len;
     u64       part_ref_cnt;
     if (part.bytes[15] == tid__short_byte_array) {
          part_ref_cnt = 0;
          part_len     = short_byte_array__get_len(part);

          switch (part_len) {
          default:
               part_bytes = part.bytes;
               break;
          case 1: {
               u64 const byte_idx = look_byte_from_end(array_bytes, array_len, part.bytes[0]);
               ref_cnt__dec(thrd_data, array);

               return byte_idx == (u64)-1 ? null : (const t_any){.structure = {.value = byte_idx, .type = tid__int}};
          }
          case 0:
               ref_cnt__dec(thrd_data, array);
               return (const t_any){.structure = {.value = array_len, .type = tid__int}};
          }
     } else {
          u32 const part_slice_offset = slice__get_offset(part);
          u32 const part_slice_len    = slice__get_len(part);
          part_ref_cnt                = get_ref_cnt(part);
          u64 const part_array_len    = slice_array__get_len(part);
          part_len                    = part_slice_len <= slice_max_len ? part_slice_len : part_array_len;
          part_bytes                  = &((const u8*)slice_array__get_items(part))[part_slice_offset];

          assert(part_slice_len <= slice_max_len || part_slice_offset == 0);
     }

     if (array.qwords[0] == part.qwords[0] && part.bytes[15] == tid__byte_array) {
          if (array_ref_cnt > 2) set_ref_cnt(array, array_ref_cnt - 2);
     } else {
          if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);
          if (part_ref_cnt > 1) set_ref_cnt(part, part_ref_cnt - 1);
     }

     t_any result;
     if (part_len > array_len)
          result = null;
     else if (part_len >= 1024 && array_len >= 8192) {
          u64 const idx = look_byte_part_from_end__rabin_karp(array_bytes, array_len, part_bytes, part_len);
          result = idx == (u64)-1 ? null : (const t_any){.structure = {.value = idx, .type = tid__int}};
     } else {
          u64 remain_bytes = array_len - part_len + 2;

          while (true) {
               u64 const part_in_array_idx = look_2_bytes_from_end(array_bytes, remain_bytes, *(const ua_u16*)part_bytes);

               if (part_in_array_idx == (u64)-1) {
                    result = null;
                    break;
               }

               if (scar__memcmp(&array_bytes[part_in_array_idx], part_bytes, part_len) == 0) {
                    result = (const t_any){.structure = {.value = part_in_array_idx, .type = tid__int}};
                    break;
               }

               remain_bytes = part_in_array_idx + 1;
          }
     }

     if (array.qwords[0] == part.qwords[0] && part.bytes[15] == tid__byte_array) {
          if (array_ref_cnt == 2) free((void*)array.qwords[0]);
     } else {
          if (array_ref_cnt == 1) free((void*)array.qwords[0]);
          if (part_ref_cnt == 1) free((void*)part.qwords[0]);
     }

     return result;
}

static t_any look_part_in__short_byte_array__short_byte_array(t_any const array, t_any const part) {
     assert(array.bytes[15] == tid__short_byte_array);
     assert(part.bytes[15] == tid__short_byte_array);

     u8 const array_len = short_byte_array__get_len(array);
     u8 const part_len  = short_byte_array__get_len(part);
     if (part_len == 0) return (const t_any){.structure = {.value = 0, .type = tid__int}};
     if (part_len > array_len) return null;

     u16 occurrence_mask = -1;
     for (u8 idx = 0; idx < part_len; idx += 1)
          occurrence_mask &= v_16_bool_to_u16(__builtin_convertvector(array.vector == part.vector[idx], v_16_bool)) >> idx;
     occurrence_mask &= ((u32)1 << (array_len - part_len + 1)) - 1;
     return occurrence_mask == 0 ?
          null :
          (const t_any){.structure = {.value = __builtin_ctz(occurrence_mask), .type = tid__int}};
}

[[clang::noinline]]
static u64 look_byte_part_from_begin__rabin_karp(const u8* const array_bytes, u64 const array_len, const u8* const part_bytes, u64 const part_len) {
     u64 const rnd_num    = (static_rnd_num & 0x7fff'ffffull) + 256;
     u64       part_hash  = part_bytes[0];
     u64       array_hash = array_bytes[0];
     u64       power      = 1;

     for (u64 idx = 1; idx < part_len; idx += 1) {
          part_hash  = (part_hash * rnd_num + part_bytes[idx]) % rabin_karp_q;
          array_hash = (array_hash * rnd_num + array_bytes[idx]) % rabin_karp_q;
          power      = power * rnd_num % rabin_karp_q;
     }

     u64 const end_idx = array_len - part_len;
     for (u64 idx = 0;; idx += 1) {
          if (part_hash == array_hash && scar__memcmp(&array_bytes[idx], part_bytes, part_len) == 0) return idx;
          if (idx == end_idx) return -1;

          array_hash = ((array_hash - (array_bytes[idx] * power) % rabin_karp_q + rabin_karp_q) * rnd_num + array_bytes[idx + part_len]) % rabin_karp_q;
     }
}

static t_any look_part_in__long_byte_array__byte_array__own(t_thrd_data* const thrd_data, t_any const array, t_any const part) {
     assume(array.bytes[15] == tid__byte_array);
     assume(part.bytes[15] == tid__short_byte_array || part.bytes[15] == tid__byte_array);

     u32       const array_slice_offset = slice__get_offset(array);
     u32       const array_slice_len    = slice__get_len(array);
     const u8* const array_bytes        = &((const u8*)slice_array__get_items(array))[array_slice_offset];
     u64       const array_ref_cnt      = get_ref_cnt(array);
     u64       const array_array_len    = slice_array__get_len(array);
     u64       const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     const u8* part_bytes;
     u64       part_len;
     u64       part_ref_cnt;
     if (part.bytes[15] == tid__short_byte_array) {
          part_ref_cnt = 0;
          part_len = short_byte_array__get_len(part);

          switch (part_len) {
          default:
               part_bytes = part.bytes;
               break;
          case 1:
               u64 const byte_idx = look_byte_from_begin(array_bytes, array_len, part.bytes[0]);
               ref_cnt__dec(thrd_data, array);
               return byte_idx == (u64)-1 ? null : (const t_any){.structure = {.value = byte_idx, .type = tid__int}};
          case 0:
               ref_cnt__dec(thrd_data, array);
               return (const t_any){.structure = {.value = 0, .type = tid__int}};
          }
     } else {
          u32 const part_slice_offset = slice__get_offset(part);
          u32 const part_slice_len    = slice__get_len(part);
          part_ref_cnt                = get_ref_cnt(part);
          u64 const part_array_len    = slice_array__get_len(part);
          part_len                    = part_slice_len <= slice_max_len ? part_slice_len : part_array_len;
          part_bytes                  = &((const u8*)slice_array__get_items(part))[part_slice_offset];

          assert(part_slice_len <= slice_max_len || part_slice_offset == 0);
     }

     if (array.qwords[0] == part.qwords[0] && part.bytes[15] == tid__byte_array) {
          if (array_ref_cnt > 2) set_ref_cnt(array, array_ref_cnt - 2);
     } else {
          if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);
          if (part_ref_cnt > 1) set_ref_cnt(part, part_ref_cnt - 1);
     }

     t_any result;
     if (part_len > array_len)
          result = null;
     else if (part_len >= 1024 && array_len >= 8192) {
          u64 const idx = look_byte_part_from_end__rabin_karp(array_bytes, array_len, part_bytes, part_len);
          result = idx == (u64)-1 ? null : (const t_any){.structure = {.value = idx, .type = tid__int}};
     } else {
          u64 idx  = 0;
          u64 edge = array_len - part_len + 2;
          while (true) {
               u64 const part_in_array_offset = look_2_bytes_from_begin(&array_bytes[idx], edge - idx, *(const ua_u16*)part_bytes);
               if (part_in_array_offset == (u64)-1) {
                    result = null;
                    break;
               }

               idx += part_in_array_offset;

               if (scar__memcmp(&array_bytes[idx], part_bytes, part_len) == 0) {
                    result = (const t_any){.structure = {.value = idx, .type = tid__int}};
                    break;
               }

               idx += 1;
          }
     }

     if (array.qwords[0] == part.qwords[0] && part.bytes[15] == tid__byte_array) {
          if (array_ref_cnt == 2) free((void*)array.qwords[0]);
     } else {
          if (array_ref_cnt == 1) free((void*)array.qwords[0]);
          if (part_ref_cnt == 1) free((void*)part.qwords[0]);
     }

     return result;
}

static t_any short_byte_array__map__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8 const len  = short_byte_array__get_len(array);
     t_any    byte = {.bytes = {[15] = tid__byte}};
     for (u8 idx = 0; idx < len; idx += 1) {
          byte.bytes[0] = array.bytes[idx];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
          if (fn_result.bytes[15] != tid__byte) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          array.bytes[idx] = fn_result.bytes[0];
     }
     ref_cnt__dec(thrd_data, fn);
     return array;
}

static t_any long_byte_array__map__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(array);
     u32 const slice_offset = slice__get_offset(array);
     u8* const bytes        = &((u8*)slice_array__get_items(array))[slice_offset];
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any  dst_array;
     u8*    dst_bytes;
     if (ref_cnt == 1)
          dst_bytes = bytes;
     else {
          if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

          dst_array = long_byte_array__new(len);
          dst_array = slice__set_metadata(dst_array, 0, slice_len);
          slice_array__set_len(dst_array, len);
          dst_bytes = slice_array__get_items(dst_array);
     }

     t_any  byte = {.bytes = {[15] = tid__byte}};
     for (u64 idx = 0; idx < len; idx += 1) {
          byte.bytes[0] = bytes[idx];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
          if (fn_result.bytes[15] != tid__byte) [[clang::unlikely]] {
               free((void*)(ref_cnt == 1 ? array.qwords[0] : dst_array.qwords[0]));
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          dst_bytes[idx] = fn_result.bytes[0];
     }
     ref_cnt__dec(thrd_data, fn);

     return ref_cnt == 1 ? array : dst_array;
}

static t_any short_byte_array__map_kv__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8 const len  = short_byte_array__get_len(array);
     t_any    fn_args[2]  = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     for (u8 idx = 0; idx < len; idx += 1) {
          fn_args[0].bytes[0] = idx;
          fn_args[1].bytes[0] = array.bytes[idx];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_result.bytes[15] != tid__byte) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          array.bytes[idx] = fn_result.bytes[0];
     }
     ref_cnt__dec(thrd_data, fn);
     return array;
}

static t_any long_byte_array__map_kv__own(t_thrd_data* const thrd_data, t_any const array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len    = slice__get_len(array);
     u32 const slice_offset = slice__get_offset(array);
     u8* const bytes        = &((u8*)slice_array__get_items(array))[slice_offset];
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any  dst_array;
     u8*    dst_bytes;
     if (ref_cnt == 1)
          dst_bytes = bytes;
     else {
          if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

          dst_array = long_byte_array__new(len);
          dst_array = slice__set_metadata(dst_array, 0, slice_len);
          slice_array__set_len(dst_array, len);
          dst_bytes = slice_array__get_items(dst_array);
     }

     t_any    fn_args[2]  = {{.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[0].bytes[0] = idx;
          fn_args[1].bytes[0] = bytes[idx];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_result.bytes[15] != tid__byte) [[clang::unlikely]] {
               free((void*)(ref_cnt == 1 ? array.qwords[0] : dst_array.qwords[0]));
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          dst_bytes[idx] = fn_result.bytes[0];
     }
     ref_cnt__dec(thrd_data, fn);

     return ref_cnt == 1 ? array : dst_array;
}

static t_any short_byte_array__map_with_state__own(t_thrd_data* const thrd_data, t_any array, t_any const state, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8 const array_len  = short_byte_array__get_len(array);
     t_any    fn_args[2] = {state, {.bytes = {[15] = tid__byte}}};
     for (u8 idx = 0; idx < array_len; idx += 1) {
          fn_args[1].bytes[0] = array.bytes[idx];
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || box_items[1].bytes[15] != tid__byte) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]       = box_items[0];
          array.bytes[idx] = box_items[1].bytes[0];
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[0]           = array;
     new_box_items[1]           = fn_args[0];

     return new_box;
}

static t_any long_byte_array__map_with_state__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len       = slice__get_len(array);
     u32 const slice_offset    = slice__get_offset(array);
     u8* const bytes           = &((u8*)slice_array__get_items(array))[slice_offset];
     u64 const ref_cnt         = get_ref_cnt(array);
     u64 const array_array_len = slice_array__get_len(array);
     u64 const len             = slice_len <= slice_max_len ? slice_len : array_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any dst_array;
     u8*   dst_bytes;
     if (ref_cnt == 1)
          dst_bytes = bytes;
     else {
          if (ref_cnt != 0 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

          dst_array = long_byte_array__new(len);
          dst_array = slice__set_metadata(dst_array, 0, slice_len);
          slice_array__set_len(dst_array, len);
          dst_bytes = slice_array__get_items(dst_array);
     }

     t_any fn_args[2] = {state, {.bytes = {[15] = tid__byte}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[1].bytes[0]  = bytes[idx];
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               if (ref_cnt == 1)
                    free((void*)array.qwords[0]);
               else {
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
          if (box_len < 2 || box_items[1].bytes[15] != tid__byte) [[clang::unlikely]] {
               if (ref_cnt == 1)
                    free((void*)array.qwords[0]);
               else {
                    free((void*)dst_array.qwords[0]);
                    if (state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);
               }

               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]     = box_items[0];
          dst_bytes[idx] = box_items[1].bytes[0];
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

static t_any short_byte_array__map_kv_with_state__own(t_thrd_data* const thrd_data, t_any array, t_any const state, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8 const array_len  = short_byte_array__get_len(array);
     t_any    fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     for (u8 idx = 0; idx < array_len; idx += 1) {
          fn_args[1].bytes[0] = idx;
          fn_args[2].bytes[0] = array.bytes[idx];
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || box_items[1].bytes[15] != tid__byte) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]       = box_items[0];
          array.bytes[idx] = box_items[1].bytes[0];
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[0]           = array;
     new_box_items[1]           = fn_args[0];

     return new_box;
}

static t_any long_byte_array__map_kv_with_state__own(t_thrd_data* const thrd_data, t_any const array, t_any const state, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32 const slice_len       = slice__get_len(array);
     u32 const slice_offset    = slice__get_offset(array);
     u8* const bytes           = &((u8*)slice_array__get_items(array))[slice_offset];
     u64 const ref_cnt         = get_ref_cnt(array);
     u64 const array_array_len = slice_array__get_len(array);
     u64 const len             = slice_len <= slice_max_len ? slice_len : array_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any dst_array;
     u8*   dst_bytes;
     if (ref_cnt == 1)
          dst_bytes = bytes;
     else {
          if (ref_cnt != 0 && state.qwords[0] != array.qwords[0]) set_ref_cnt(array, ref_cnt - 1);

          dst_array = long_byte_array__new(len);
          dst_array = slice__set_metadata(dst_array, 0, slice_len);
          slice_array__set_len(dst_array, len);
          dst_bytes = slice_array__get_items(dst_array);
     }

     t_any fn_args[3] = {state, {.bytes = {[15] = tid__int}}, {.bytes = {[15] = tid__byte}}};
     for (u64 idx = 0; idx < len; idx += 1) {
          fn_args[1].qwords[0] = idx;
          fn_args[2].bytes[0]  = bytes[idx];
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               if (ref_cnt == 1)
                    free((void*)array.qwords[0]);
               else {
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
          if (box_len < 2 || box_items[1].bytes[15] != tid__byte) [[clang::unlikely]] {
               if (ref_cnt == 1)
                    free((void*)array.qwords[0]);
               else {
                    free((void*)dst_array.qwords[0]);
                    if (state.qwords[0] == array.qwords[0]) ref_cnt__dec(thrd_data, array);
               }

               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]     = box_items[0];
          dst_bytes[idx] = box_items[1].bytes[0];
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

static inline t_any prefix_of__long_byte_array__byte_array__own(t_thrd_data* const thrd_data, t_any const array, t_any const prefix) {
     assume(array.bytes[15] == tid__byte_array);
     assume(prefix.bytes[15] == tid__short_byte_array || prefix.bytes[15] == tid__byte_array);

     u32       const array_slice_offset = slice__get_offset(array);
     u32       const array_slice_len    = slice__get_len(array);
     const u8* const array_bytes        = &((const u8*)slice_array__get_items(array))[array_slice_offset];
     u64       const array_ref_cnt      = get_ref_cnt(array);
     u64       const array_array_len    = slice_array__get_len(array);
     u64       const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     const u8* prefix_bytes;
     u64       prefix_len;
     u64       prefix_ref_cnt;
     if (prefix.bytes[15] == tid__short_byte_array) {
          prefix_ref_cnt = 0;
          prefix_bytes   = prefix.bytes;
          prefix_len     = short_byte_array__get_len(prefix);
     } else {
          u32 const prefix_slice_offset = slice__get_offset(prefix);
          u32 const prefix_slice_len    = slice__get_len(prefix);
          prefix_bytes                  = &((const u8*)slice_array__get_items(prefix))[prefix_slice_offset];
          prefix_ref_cnt                = get_ref_cnt(prefix);
          u64 const prefix_array_len    = slice_array__get_len(prefix);
          prefix_len                    = prefix_slice_len <= slice_max_len ? prefix_slice_len : prefix_array_len;

          assert(prefix_slice_len <= slice_max_len || prefix_slice_offset == 0);
     }

     if (array.qwords[0] == prefix.qwords[0] && prefix.bytes[15] == tid__byte_array) {
          if (array_ref_cnt > 2) set_ref_cnt(array, array_ref_cnt - 2);
     } else {
          if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);
          if (prefix_ref_cnt > 1) set_ref_cnt(prefix, prefix_ref_cnt - 1);
     }

     t_any const result = array_len >= prefix_len && scar__memcmp(array_bytes, prefix_bytes, prefix_len) == 0 ? bool__true : bool__false;

     if (array.qwords[0] == prefix.qwords[0] && prefix.bytes[15] == tid__byte_array) {
          if (array_ref_cnt == 2) free((void*)array.qwords[0]);
     } else {
          if (array_ref_cnt == 1) free((void*)array.qwords[0]);
          if (prefix_ref_cnt == 1) free((void*)prefix.qwords[0]);
     }

     return result;
}

static t_any short_byte_array__print(t_thrd_data* const thrd_data, t_any array, const char* const owner, FILE* const file) {
     assert(array.bytes[15] == tid__short_byte_array);

     typedef u8  v_32_u8 __attribute__((ext_vector_type(32)));
     typedef u64 v_4_u64 __attribute__((ext_vector_type(4)));

     u8 const str_len = short_byte_array__get_len(array) * 2;
     memset_inline(&array.bytes[14], 0, 2);

     v_32_u8 chars_vec = array.vector.s00112233445566778899aabbccddeeff >> (const v_32_u8)(const v_4_u64)0x0004'0004'0004'0004ull & 0xf;
     chars_vec        += (u8)'0';
     chars_vec        += (chars_vec > (u8)'9') & (u8)('a' - ':');

     if (fwrite(&chars_vec, 1, str_len, file) != str_len) [[clang::unlikely]]
          return error__cant_print(thrd_data, owner);

     return null;
}

[[clang::noinline]]
static t_any long_byte_array__print(t_thrd_data* const thrd_data, t_any const array, const char* const owner, FILE* const file) {
     assert(array.bytes[15] == tid__byte_array);

     typedef u8  v_16_u8_a1 __attribute__((ext_vector_type(16), aligned(1)));
     typedef u8  v_32_u8 __attribute__((ext_vector_type(32)));
     typedef u8  v_32_u8_a1 __attribute__((ext_vector_type(32), aligned(1)));
     typedef u64 v_4_u64 __attribute__((ext_vector_type(4)));

     [[gnu::aligned(alignof(v_32_u8))]]
     u8 chars[1024];

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const bytes_len    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (bytes_len == 15) {
          v_32_u8 chars_vec = (*(const v_16_u8_a1*)&bytes[-1]).s112233445566778899aabbccddeeff00 >> (const v_32_u8)(const v_4_u64)0x0004'0004'0004'0004ull & 0xf;
          chars_vec        += (u8)'0';
          chars_vec        += (chars_vec > (u8)'9') & (u8)('a' - ':');

          if (fwrite(&chars_vec, 1, 30, file) != 30) [[clang::unlikely]]
               return error__cant_print(thrd_data, owner);
          return null;
     }

     u64 chars_idx = 0;
     u64 byte_idx  = 0;
     for (; bytes_len - byte_idx >= 16; byte_idx += 16) {
          v_32_u8 chars_vec = (*(const v_16_u8_a1*)&bytes[byte_idx]).s112233445566778899aabbccddeeff00 >> (const v_32_u8)(const v_4_u64)0x0004'0004'0004'0004ull & 0xf;
          chars_vec        += (u8)'0';
          chars_vec        += (chars_vec > (u8)'9') & (u8)('a' - ':');

          *(v_32_u8*)&chars[chars_idx] = chars_vec;

          chars_idx += 32;
          if (chars_idx == 1024) {
               chars_idx = 0;
               if (fwrite(chars, 1, 1024, file) != 1024) [[clang::unlikely]]
                    return error__cant_print(thrd_data, owner);
          }
     }

     v_32_u8_a1 chars_vec = (*(const v_16_u8_a1*)&bytes[bytes_len - 16]).s112233445566778899aabbccddeeff00 >> (const v_32_u8)(const v_4_u64)0x0004'0004'0004'0004ull & 0xf;
     chars_vec           += (u8)'0';
     chars_vec           += (chars_vec > (u8)'9') & (u8)('a' - ':');

     u8  const last_bytes_len = bytes_len - byte_idx;
     u32 const last_chars_len = chars_idx + last_bytes_len * 2;
     chars_idx                = chars_idx == 0 ? 0 : chars_idx - 32 + last_bytes_len * 2;

     *(v_32_u8_a1*)&chars[chars_idx] = chars_vec;
     if (fwrite(chars, 1, last_chars_len, file) != last_chars_len) [[clang::unlikely]]
          return error__cant_print(thrd_data, owner);

     return null;
}

[[clang::always_inline]]
static t_any short_byte_array__qty(t_any const array, u8 const looked_byte) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8  const len              = short_byte_array__get_len(array);
     u16 const looked_byte_mask = v_16_bool_to_u16(__builtin_convertvector(array.vector == looked_byte, v_16_bool)) & (((u16)1 << len) - 1);
     return (const t_any){.structure = {.value = __builtin_popcount(looked_byte_mask), .type = tid__int}};
}

static u64 qty_byte(const u8* const bytes, u64 const bytes_len, u8 const looked_byte) {
     u64 idx    = 0;
     u64 result = 0;
     if (bytes_len - idx >= 64) {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          t_vec const looked_byte_vec = looked_byte;
          do {
               u64 looked_byte_mask = v_64_bool_to_u64(__builtin_convertvector(*(const t_vec*)&bytes[idx] == looked_byte_vec, v_64_bool));
               result += __builtin_popcountll(looked_byte_mask);
               idx    += 64;
          } while (bytes_len - idx >= 64);
     }

     u8 const remain_bytes = bytes_len - idx;
     switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));

          u64 looked_byte_mask =
               v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)&bytes[idx] == looked_byte, v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(*(const v_32_u8*)&bytes[bytes_len - 32] == looked_byte, v_32_bool)) << (remain_bytes - 32);

          return result + __builtin_popcountll(looked_byte_mask);
     }
     case 5: {
          assume(remain_bytes > 16 && remain_bytes <= 32);

          typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));

          u32 looked_byte_mask =
               v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)&bytes[idx] == looked_byte, v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(*(const v_16_u8*)&bytes[bytes_len - 16] == looked_byte, v_16_bool)) << (remain_bytes - 16);

          return result + __builtin_popcountll(looked_byte_mask);
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 16);

          typedef u8 v_8_u8 __attribute__((ext_vector_type(8), aligned(1)));

          u16 looked_byte_mask =
               v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)&bytes[idx] == looked_byte, v_8_bool)) |
               (u16)v_8_bool_to_u8(__builtin_convertvector(*(const v_8_u8*)&bytes[bytes_len - 8] == looked_byte, v_8_bool)) << (remain_bytes - 8);

          return result + __builtin_popcount(looked_byte_mask);
     }
     case 3: {
          assume(remain_bytes > 4 && remain_bytes <= 8);

          u64 const delta_bits = (remain_bytes - 4) * 8;

          u64 neg_looked_byte_vec = (u8)~looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 16;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u64 looked_byte_mask = (*(const ua_u32*)&bytes[idx] | (u64)*(const ua_u32*)&bytes[bytes_len - 4] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101'0101'0101ull;

          return result + __builtin_popcountll(looked_byte_mask);
     }
     case 2: {
          assume(remain_bytes >= 2 && remain_bytes <= 4);

          u32 const delta_bits = (remain_bytes - 2) * 8;

          u32 neg_looked_byte_vec = (u8)~looked_byte;
          neg_looked_byte_vec    |= neg_looked_byte_vec << 8;
          neg_looked_byte_vec    |= neg_looked_byte_vec << delta_bits;

          u32 looked_byte_mask = (*(const ua_u16*)&bytes[idx] | (u32)*(const ua_u16*)&bytes[bytes_len - 2] << delta_bits) ^ neg_looked_byte_vec;
          looked_byte_mask    &= looked_byte_mask >> 4;
          looked_byte_mask    &= looked_byte_mask >> 2;
          looked_byte_mask    &= looked_byte_mask >> 1;
          looked_byte_mask    &= 0x0101'0101ul;

          return result + __builtin_popcountll(looked_byte_mask);
     }
     case 1:
          return result + (bytes[idx] == looked_byte ? 1 : 0);
     case 0:
          return result;
     default:
          unreachable;
     }
}

static t_any long_byte_array__qty__own(t_thrd_data* const thrd_data, t_any const array, u8 const looked_byte) {
     assume(array.bytes[15] == tid__byte_array);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     u64 const result = qty_byte(bytes, len, looked_byte);

     if (ref_cnt == 1) free((void*)array.qwords[0]);

     return (const t_any){.structure = {.value = result, .type = tid__int}};
}

static t_any short_byte_array__repeat(t_thrd_data* const thrd_data, t_any const part, u64 const count, const char* const owner) {
     assert(part.bytes[15] == tid__short_byte_array);

     u64        array_len;
     u64  const part_len = short_byte_array__get_len(part);
     bool const overflow = __builtin_mul_overflow(part_len, count, &array_len);
     if (array_len > array_max_len || overflow) [[clang::unlikely]]
          return error__out_of_bounds(thrd_data, owner);

     if (part_len == 0 || count == 0) return (const t_any){.bytes = {[15] = tid__short_byte_array}};

     if (array_len <= 14) {
          t_any array = part;
          for (u64 counter = 1; counter < count; counter += 1) {
               array.raw_bits <<= part_len * 8;
               array.raw_bits  |= part.raw_bits;
          }
          array.bytes[15] = tid__short_byte_array;
          array           = short_byte_array__set_len(array, array_len);
          return array;
     }

     t_any     array       = long_byte_array__new(array_len);
     slice_array__set_len(array, array_len);
     array                 = slice__set_metadata(array, 0, array_len <= slice_max_len ? array_len : slice_max_len + 1);
     u8* const array_bytes = slice_array__get_items(array);

     if (part_len == 1)
          memset(array_bytes, part.bytes[0], array_len);
     else {
          u128 const part_mask = (u128)-1 >> part_len * 8;
          u128 const part_bits = part.raw_bits << (16 - part_len) * 8;
          for (u64 part_idx = 0; part_idx < count; part_idx += 1) {
               *(ua_u128*)&array_bytes[(i64)(part_len * (part_idx + 1)) - 16] &= part_mask;
               *(ua_u128*)&array_bytes[(i64)(part_len * (part_idx + 1)) - 16] |= part_bits;
          }
     }

     return array;
}

static t_any long_byte_array__repeat__own(t_thrd_data* const thrd_data, t_any const part, u64 const count, const char* const owner) {
     assert(part.bytes[15] == tid__byte_array);

     u32 const slice_offset   = slice__get_offset(part);
     u32 const slice_len      = slice__get_len(part);
     u64 const ref_cnt        = get_ref_cnt(part);
     u64 const part_array_len = slice_array__get_len(part);
     u64 const part_len       = slice_len <= slice_max_len ? slice_len : part_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64        result_array_len;
     bool const overflow   = __builtin_mul_overflow(part_len, count, &result_array_len);
     if (result_array_len > array_max_len || overflow) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, part);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (count < 2) {
          if (count == 0) {
               ref_cnt__dec(thrd_data, part);
               return (const t_any){.bytes = {[15] = tid__short_byte_array}};
          }

          return part;
     }

     t_any result;
     if (ref_cnt == 1) {
          u64 const part_array_cap = slice_array__get_cap(part);
          result                    = part;
          if (part_array_cap < result_array_len) {
               result.qwords[0]  = (u64)realloc((u8*)result.qwords[0], result_array_len + 16);
               slice_array__set_cap(result, result_array_len);
          }

          u8* const bytes = slice_array__get_items(result);
          if (slice_offset != 0) memmove(bytes, &bytes[slice_offset], part_len);
     } else {
          if (ref_cnt != 0) set_ref_cnt(part, ref_cnt - 1);

          result = long_byte_array__new(result_array_len);
          memcpy(slice_array__get_items(result), &((const u8*)slice_array__get_items(part))[slice_offset], part_len);
     }

     slice_array__set_len(result, result_array_len);
     result                = slice__set_metadata(result, 0, result_array_len <= slice_max_len ? result_array_len : slice_max_len + 1);
     u8* const array_bytes = slice_array__get_items(result);

     for (
          u64 part_idx = 1;
          part_idx < count;

          memcpy(&array_bytes[part_len * part_idx], &array_bytes[part_len * (part_idx - 1)], part_len),
          part_idx += 1
     );

     return result;
}

[[clang::always_inline]]
static t_any short_byte_array__replace(t_any array, u8 const old_byte, u8 const new_byte) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8 const    len = short_byte_array__get_len(array);
     array.chars_vec = (array.chars_vec != old_byte) & array.chars_vec | ((array.chars_vec == old_byte) & new_byte);
     array.raw_bits &= ((u128)1 << len * 8) - 1;
     array.bytes[15] = tid__short_byte_array;
     array           = short_byte_array__set_len(array, len);
     return array;
}

static inline t_any long_byte_array__replace__own(t_thrd_data* const thrd_data, t_any const array, u8 const old_byte, u8 const new_byte) {
     assume(array.bytes[15] == tid__byte_array);

     if (old_byte == new_byte) return array;

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;
     u8* const src_bytes    = &((u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any      dst;
     u8*        dst_bytes = src_bytes;
     bool const is_mut    = ref_cnt == 1;
     u64        idx       = 0;
     if (len - idx >= 64) {
          typedef u8 t_vec __attribute__((ext_vector_type(64), aligned(1)));

          t_vec const old_byte_vec = old_byte;
          do {
               t_vec const bytes_vec = *(const t_vec*)&src_bytes[idx];

               if (__builtin_reduce_or(bytes_vec == old_byte_vec) != 0) {
                    if (!is_mut && dst_bytes == src_bytes) {
                         if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

                         dst       = long_byte_array__new(len);
                         slice_array__set_len(dst, len);
                         dst       = slice__set_metadata(dst, 0, slice_len);
                         dst_bytes = slice_array__get_items(dst);
                         memcpy(dst_bytes, src_bytes, idx);
                    }

                    *(t_vec*)&dst_bytes[idx] = bytes_vec & (bytes_vec != old_byte_vec) | (bytes_vec == old_byte_vec) & new_byte;
               } else if (dst_bytes != src_bytes) *(t_vec*)&dst_bytes[idx] = bytes_vec;

               idx += 64;
          } while (len - idx >= 64);
     }

     u8 const remain_bytes = len - idx;
     switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assume(remain_bytes > 32 && remain_bytes < 64);

          typedef u8   v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));
          typedef char v_32_char __attribute__((ext_vector_type(32)));

          v_32_u8   const low_vec   = *(const v_32_u8*)&src_bytes[idx];
          v_32_u8   const high_vec  = *(const v_32_u8*)&src_bytes[len - 32];
          v_32_char const low_mask  = low_vec == old_byte;
          v_32_char const high_mask = high_vec == old_byte;

          if (__builtin_reduce_or(low_mask | high_mask) != 0) {
               if (!is_mut && dst_bytes == src_bytes) {
                    if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

                    dst       = long_byte_array__new(len);
                    slice_array__set_len(dst, len);
                    dst       = slice__set_metadata(dst, 0, slice_len);
                    dst_bytes = slice_array__get_items(dst);
                    memcpy(dst_bytes, src_bytes, idx);
               }

               *(v_32_u8*)&dst_bytes[idx]      = low_vec & ~low_mask | low_mask & new_byte;
               *(v_32_u8*)&dst_bytes[len - 32] = high_vec & ~high_mask | high_mask & new_byte;
          } else if (dst_bytes != src_bytes) memcpy_le_64(&dst_bytes[idx], &src_bytes[idx], remain_bytes);

          break;
     }
     case 5: {
          assume(remain_bytes > 16 && remain_bytes <= 32);

          typedef u8   v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));
          typedef char v_16_char __attribute__((ext_vector_type(16)));

          v_16_u8   const low_vec   = *(const v_16_u8*)&src_bytes[idx];
          v_16_u8   const high_vec  = *(const v_16_u8*)&src_bytes[len - 16];
          v_16_char const low_mask  = low_vec == old_byte;
          v_16_char const high_mask = high_vec == old_byte;

          if (__builtin_reduce_or(low_mask | high_mask) != 0) {
               if (!is_mut && dst_bytes == src_bytes) {
                    if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

                    dst       = long_byte_array__new(len);
                    slice_array__set_len(dst, len);
                    dst       = slice__set_metadata(dst, 0, slice_len);
                    dst_bytes = slice_array__get_items(dst);
                    memcpy(dst_bytes, src_bytes, idx);
               }

               *(v_16_u8*)&dst_bytes[idx]      = low_vec & ~low_mask | low_mask & new_byte;
               *(v_16_u8*)&dst_bytes[len - 16] = high_vec & ~high_mask | high_mask & new_byte;
          } else if (dst_bytes != src_bytes) memcpy_le_32(&dst_bytes[idx], &src_bytes[idx], remain_bytes);

          break;
     }
     case 4: {
          assume(remain_bytes > 8 && remain_bytes <= 16);

          typedef u8   v_8_u8 __attribute__((ext_vector_type(8), aligned(1)));
          typedef char v_8_char __attribute__((ext_vector_type(8)));

          v_8_u8   const low_vec   = *(const v_8_u8*)&src_bytes[idx];
          v_8_u8   const high_vec  = *(const v_8_u8*)&src_bytes[len - 8];
          v_8_char const low_mask  = low_vec == old_byte;
          v_8_char const high_mask = high_vec == old_byte;

          if (__builtin_reduce_or(low_mask | high_mask) != 0) {
               if (!is_mut && dst_bytes == src_bytes) {
                    if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

                    dst       = long_byte_array__new(len);
                    slice_array__set_len(dst, len);
                    dst       = slice__set_metadata(dst, 0, slice_len);
                    dst_bytes = slice_array__get_items(dst);
                    memcpy(dst_bytes, src_bytes, idx);
               }

               *(v_8_u8*)&dst_bytes[idx]     = low_vec & ~low_mask | low_mask & new_byte;
               *(v_8_u8*)&dst_bytes[len - 8] = high_vec & ~high_mask | high_mask & new_byte;
          } else if (dst_bytes != src_bytes) memcpy_le_16(&dst_bytes[idx], &src_bytes[idx], remain_bytes);

          break;
     }
     case 3: {
          assume(remain_bytes > 4 && remain_bytes <= 8);

          u32 neg_old_byte_vec = (u8)~old_byte;
          neg_old_byte_vec    |= neg_old_byte_vec << 8;
          neg_old_byte_vec    |= neg_old_byte_vec << 16;

          u32 new_byte_vec = (u8)new_byte;
          new_byte_vec    |= new_byte_vec << 8;
          new_byte_vec    |= new_byte_vec << 16;

          u32 const low_vec  = *(const ua_u32*)&src_bytes[idx];
          u32       low_mask = low_vec ^ neg_old_byte_vec;
          low_mask          &= low_mask >> 4;
          low_mask          &= low_mask >> 2;
          low_mask          &= low_mask >> 1;
          low_mask          &= 0x0101'0101ul;
          low_mask          |= low_mask << 1;
          low_mask          |= low_mask << 2;
          low_mask          |= low_mask << 4;

          u32 const high_vec = *(const ua_u32*)&src_bytes[len - 4];
          u32       high_mask = high_vec ^ neg_old_byte_vec;
          high_mask          &= high_mask >> 4;
          high_mask          &= high_mask >> 2;
          high_mask          &= high_mask >> 1;
          high_mask          &= 0x0101'0101ul;
          high_mask          |= high_mask << 1;
          high_mask          |= high_mask << 2;
          high_mask          |= high_mask << 4;

          if ((low_mask | high_mask) != 0) {
               if (!is_mut && dst_bytes == src_bytes) {
                    if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

                    dst       = long_byte_array__new(len);
                    slice_array__set_len(dst, len);
                    dst       = slice__set_metadata(dst, 0, slice_len);
                    dst_bytes = slice_array__get_items(dst);
                    memcpy(dst_bytes, src_bytes, idx);
               }

               *(ua_u32*)&dst_bytes[idx]     = low_vec & ~low_mask | new_byte_vec & low_mask;
               *(ua_u32*)&dst_bytes[len - 4] = high_vec & ~high_mask | new_byte_vec & high_mask;
          } else if (dst_bytes != src_bytes) memcpy_le_16(&dst_bytes[idx], &src_bytes[idx], remain_bytes);

          break;
     }
     case 2: {
          assume(remain_bytes >= 2 && remain_bytes <= 4);

          u16 neg_old_byte_vec = (u8)~old_byte;
          neg_old_byte_vec    |= neg_old_byte_vec << 8;

          u16 new_byte_vec = (u8)new_byte;
          new_byte_vec    |= new_byte_vec << 8;

          u16 const low_vec  = *(const ua_u16*)&src_bytes[idx];
          u16       low_mask = low_vec ^ neg_old_byte_vec;
          low_mask          &= low_mask >> 4;
          low_mask          &= low_mask >> 2;
          low_mask          &= low_mask >> 1;
          low_mask          &= 0x0101;
          low_mask          |= low_mask << 1;
          low_mask          |= low_mask << 2;
          low_mask          |= low_mask << 4;

          u16 const high_vec  = *(const ua_u16*)&src_bytes[len - 2];
          u16       high_mask = high_vec ^ neg_old_byte_vec;
          high_mask          &= high_mask >> 4;
          high_mask          &= high_mask >> 2;
          high_mask          &= high_mask >> 1;
          high_mask          &= 0x0101;
          high_mask          |= high_mask << 1;
          high_mask          |= high_mask << 2;
          high_mask          |= high_mask << 4;

          if ((low_mask | high_mask) != 0) {
               if (!is_mut && dst_bytes == src_bytes) {
                    if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

                    dst       = long_byte_array__new(len);
                    slice_array__set_len(dst, len);
                    dst       = slice__set_metadata(dst, 0, slice_len);
                    dst_bytes = slice_array__get_items(dst);
                    memcpy(dst_bytes, src_bytes, idx);
               }

               *(ua_u16*)&dst_bytes[idx]     = low_vec & ~low_mask | new_byte_vec & low_mask;
               *(ua_u16*)&dst_bytes[len - 2] = high_vec & ~high_mask | new_byte_vec & high_mask;
          } else if (dst_bytes != src_bytes) memcpy_le_16(&dst_bytes[idx], &src_bytes[idx], remain_bytes);

          break;
     }
     case 1:
          if (src_bytes[idx] == old_byte) {
               if (!is_mut && dst_bytes == src_bytes) {
                    if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

                    dst       = long_byte_array__new(len);
                    slice_array__set_len(dst, len);
                    dst       = slice__set_metadata(dst, 0, slice_len);
                    dst_bytes = slice_array__get_items(dst);
                    memcpy(dst_bytes, src_bytes, idx);
               }

               dst_bytes[idx] = new_byte;
          } else if (dst_bytes != src_bytes) dst_bytes[idx] = src_bytes[idx];

          break;
     case 0:
          break;
     default:
          unreachable;
     }

     return dst_bytes == src_bytes ? array : dst;
}

static t_any byte_array__replace_part__own(t_thrd_data* const thrd_data, t_any const array, t_any const old_part, t_any const new_part, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array || array.bytes[15] == tid__byte_array);
     assert(old_part.bytes[15] == tid__short_byte_array || old_part.bytes[15] == tid__byte_array);
     assert(new_part.bytes[15] == tid__short_byte_array || new_part.bytes[15] == tid__byte_array);

     if ((array.bytes[15] == tid__short_byte_array && old_part.bytes[15] == tid__short_byte_array && new_part.bytes[15] == tid__short_byte_array)) {
          u8 const array_len    = short_byte_array__get_len(array);
          u8 const old_part_len = short_byte_array__get_len(old_part);
          u8 const new_part_len = short_byte_array__get_len(new_part);
          if (old_part.raw_bits == new_part.raw_bits || old_part_len > array_len) return array;

          if (old_part_len == 1 && new_part_len == 1) return short_byte_array__replace(array, old_part.bytes[0], new_part.bytes[0]);

          if (old_part_len == 0 && (array_len + 1) * new_part_len + array_len < 15) {
               t_any dst_array  = new_part;
               t_any array_byte = {.bytes = {[14] = 1, [15] = tid__short_byte_array}};
               for (u64 idx = 0; idx < array_len; idx += 1) {
                    array_byte.bytes[0] = array.bytes[idx];
                    dst_array           = concat__short_byte_array__short_byte_array(dst_array, array_byte);
                    dst_array           = concat__short_byte_array__short_byte_array(dst_array, new_part);
               }

               return dst_array;
          }
     }

     u8    old_part_buffer[32];
     u8    new_part_buffer[32];
     t_any old_part_as_long;
     t_any new_part_as_long;
     if (old_part.bytes[15] == tid__short_byte_array) {
          if (old_part.raw_bits == new_part.raw_bits) return array;

          u8 const len = short_byte_array__get_len(old_part);
          memcpy_inline(&old_part_buffer[16], old_part.bytes, 16);
          old_part_as_long.qwords[0] = (u64)old_part_buffer;
          old_part_as_long.bytes[15] = tid__byte_array;
          old_part_as_long           = slice__set_metadata(old_part_as_long, 0, len);
#ifndef NDEBUG
          set_ref_cnt(old_part_as_long, 1);
#endif
          slice_array__set_len(old_part_as_long, len);
          slice_array__set_cap(old_part_as_long, 15);
          set_ref_cnt(old_part_as_long, 0);
     } else old_part_as_long = old_part;

     if (new_part.bytes[15] == tid__short_byte_array) {
          u8 const len = short_byte_array__get_len(new_part);
          memcpy_inline(&new_part_buffer[16], new_part.bytes, 16);
          new_part_as_long.qwords[0] = (u64)new_part_buffer;
          new_part_as_long.bytes[15] = tid__byte_array;
          new_part_as_long           = slice__set_metadata(new_part_as_long, 0, len);
#ifndef NDEBUG
          set_ref_cnt(new_part_as_long, 1);
#endif
          slice_array__set_len(new_part_as_long, len);
          slice_array__set_cap(new_part_as_long, 15);
          set_ref_cnt(new_part_as_long, 0);
     } else new_part_as_long = new_part;

     u32       const old_part_slice_offset = slice__get_offset(old_part_as_long);
     u32       const old_part_slice_len    = slice__get_len(old_part_as_long);
     const u8* const old_part_bytes        = &((const u8*)slice_array__get_items(old_part_as_long))[old_part_slice_offset];
     u64       const old_part_len          = old_part_slice_len <= slice_max_len ? old_part_slice_len : slice_array__get_len(old_part_as_long);

     assert(old_part_slice_len <= slice_max_len || old_part_slice_offset == 0);

     u32       const new_part_slice_offset = slice__get_offset(new_part_as_long);
     u32       const new_part_slice_len    = slice__get_len(new_part_as_long);
     const u8* const new_part_bytes        = &((const u8*)slice_array__get_items(new_part_as_long))[new_part_slice_offset];
     u64       const new_part_len          = new_part_slice_len <= slice_max_len ? new_part_slice_len : slice_array__get_len(new_part_as_long);

     assert(new_part_slice_len <= slice_max_len || new_part_slice_offset == 0);

     if (old_part_len == 1 && new_part_len == 1)
          return long_byte_array__replace__own(thrd_data, array, old_part_bytes[0], new_part_bytes[0]);

     u8    array_buffer[32];
     t_any array_as_long;
     if (array.bytes[15] == tid__short_byte_array) {
          u8 const len = short_byte_array__get_len(array);
          memcpy_inline(&array_buffer[16], array.bytes, 16);
          array_as_long.qwords[0] = (u64)array_buffer;
          array_as_long.bytes[15] = tid__byte_array;
          array_as_long           = slice__set_metadata(array_as_long, 0, len);
#ifndef NDEBUG
          set_ref_cnt(array_as_long, 1);
#endif
          slice_array__set_len(array_as_long, len);
          slice_array__set_cap(array_as_long, 15);
          set_ref_cnt(array_as_long, 0);
     } else array_as_long = array;

     u32       const array_slice_offset = slice__get_offset(array_as_long);
     u32       const array_slice_len    = slice__get_len(array_as_long);
     const u8* const array_bytes        = &((u8*)slice_array__get_items(array_as_long))[array_slice_offset];
     u64       const array_len          = array_slice_len <= slice_max_len ? array_slice_len : slice_array__get_len(array_as_long);

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     if (old_part_len == 0) [[clang::unlikely]] {
          ref_cnt__add(thrd_data, new_part, array_len + 1, owner);
          u64 const dst_len       = (array_len + 1) * new_part_len + array_len;
          t_any     dst_array     = long_byte_array__new(dst_len);
          dst_array               = concat__long_byte_array__long_byte_array__own(thrd_data, dst_array, new_part_as_long, owner);
          t_any     array_byte    = {.bytes = {[14] = 1, [15] = tid__short_byte_array}};
          for (u64 idx = 0; idx < array_len; idx += 1) {
               array_byte.bytes[0] = array_bytes[idx];
               dst_array           = concat__long_byte_array__short_byte_array__own(thrd_data, dst_array, array_byte, owner);
               dst_array           = concat__long_byte_array__long_byte_array__own(thrd_data, dst_array, new_part_as_long, owner);
          }

          ref_cnt__dec(thrd_data, old_part);
          ref_cnt__dec(thrd_data, new_part);
          ref_cnt__dec(thrd_data, array);

          return dst_array;
     }

     u64   dst_len;
     u64   dst_cap;
     u8*   dst_bytes;
     t_any dst_array    = array_as_long;
     bool  first_search = true;

     u64 array_idx = 0;
     while (array_idx + old_part_len <= array_len) {
          u64 occurrence_offset;
          u64 const tail_len = array_len - array_idx;
          if (old_part_len == 1) {
               u64       array_idx_offset = 0;
               u64 const edge             = tail_len - old_part_len + 1;
               while (true) {
                    occurrence_offset = look_byte_from_begin(&array_bytes[array_idx + array_idx_offset], edge - array_idx_offset, old_part_bytes[0]);
                    if (occurrence_offset == (u64)-1) break;

                    occurrence_offset += array_idx_offset;
                    array_idx_offset   = occurrence_offset + 1;

                    break;
               }
          } else if (old_part_len >= 1024 && tail_len >= 8192)
               occurrence_offset = look_byte_part_from_begin__rabin_karp(&array_bytes[array_idx], tail_len, old_part_bytes, old_part_len);
          else {
               u64       array_idx_offset = 0;
               u64 const edge             = tail_len - old_part_len + 2;
               while (true) {
                    occurrence_offset = look_2_bytes_from_begin(&array_bytes[array_idx + array_idx_offset], edge - array_idx_offset, *(const ua_u16*)old_part_bytes);
                    if (occurrence_offset == (u64)-1) break;

                    occurrence_offset += array_idx_offset;
                    array_idx_offset   = occurrence_offset + 1;

                    if (scar__memcmp(&array_bytes[array_idx + occurrence_offset], old_part_bytes, old_part_len) == 0) break;
               }
          }

          if (occurrence_offset == -1) break;
          if (old_part_len == new_part_len) {
               ref_cnt__inc(thrd_data, new_part, owner);
               dst_array = copy__long_byte_array__long_byte_array__own(thrd_data, dst_array, array_idx + occurrence_offset, new_part_as_long, owner);
          } else {
               if (first_search) {
                    dst_array = long_byte_array__new(array_len - old_part_len + new_part_len);
                    dst_bytes = slice_array__get_items(dst_array);
                    dst_len   = 0;
                    dst_cap   = slice_array__get_cap(dst_array);
               } else if (dst_len + occurrence_offset + new_part_len > dst_cap) {
                    dst_cap = dst_cap + occurrence_offset + new_part_len;
                    if (dst_cap > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

                    dst_cap = dst_cap * 3 / 2;
                    dst_cap = dst_cap > array_max_len ? array_max_len : dst_cap;

                    dst_array.qwords[0] = (u64)realloc((u8*)dst_array.qwords[0], dst_cap + 16);
                    dst_bytes           = slice_array__get_items(dst_array);
               }

               memcpy(&dst_bytes[dst_len], &array_bytes[array_idx], occurrence_offset);
               dst_len += occurrence_offset;
               memcpy(&dst_bytes[dst_len], new_part_bytes, new_part_len);
               dst_len += new_part_len;
          }

          array_idx   += occurrence_offset + old_part_len;
          first_search = false;
     }

     ref_cnt__dec(thrd_data, old_part);
     ref_cnt__dec(thrd_data, new_part);
     if (first_search) return array;

     if (old_part_len == new_part_len) {
          dst_len   = array_len;
          dst_bytes = &((u8*)slice_array__get_items(dst_array))[slice__get_offset(dst_array)];
     } else {
          u64 const last_part_len = array_len - array_idx;
          if (last_part_len != 0) {
               if (dst_len + last_part_len > dst_cap) {
                    dst_cap = dst_len + last_part_len;
                    if (dst_cap > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

                    dst_array.qwords[0] = (u64)realloc((u8*)dst_array.qwords[0], dst_cap + 16);
                    dst_bytes           = slice_array__get_items(dst_array);
               }

               memcpy(&dst_bytes[dst_len], &array_bytes[array_idx], last_part_len);
               dst_len += last_part_len;
          }

          ref_cnt__dec(thrd_data, array);
          dst_array = slice__set_metadata(dst_array, 0, dst_len <= slice_max_len ? dst_len : slice_max_len + 1);
          slice_array__set_len(dst_array, dst_len);
          slice_array__set_cap(dst_array, dst_cap);
     }

     if (dst_len < 15) {
          t_any short_array;
          short_array.raw_bits  = dst_len == 0 ? 0 : (*(ua_u128*)&dst_bytes[(i64)dst_len - 16]) >> (16 - dst_len) * 8;
          short_array.bytes[15] = tid__short_byte_array;
          short_array           = short_byte_array__set_len(short_array, dst_len);

          free((void*)dst_array.qwords[0]);
          return short_array;
     }

     return dst_array;
}

static t_any long_byte_array__reserve__own(t_thrd_data* const thrd_data, t_any array, u64 const new_items_qty, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);

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
          array.qwords[0] = (u64)realloc((void*)array.qwords[0], new_cap + 16);
          return array;
     }

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any new_array = long_byte_array__new(new_cap);
     slice_array__set_len(new_array, len);
     new_array       = slice__set_metadata(new_array, 0, slice_len);
     memcpy(slice_array__get_items(new_array), &((const u8*)slice_array__get_items(array))[slice_offset], len);

     return new_array;
}

[[clang::always_inline]]
static t_any short_byte_array__reverse(t_any array) {
     assert(array.bytes[15] == tid__short_byte_array);

     u16 const len    = short_byte_array__get_len(array);
     array.vector     = array.vector.sfedcba9876543210;
     array.raw_bits >>= len == 0 ? 16 : (16 - len) * 8;
     array.bytes[15]  = tid__short_byte_array;
     array            = short_byte_array__set_len(array, len);
     return array;
}

static t_any long_byte_array__reverse__own(t_thrd_data* const thrd_data, t_any const array) {
     assume(array.bytes[15] == tid__byte_array);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     typedef u8 v_64_u8 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(1)));
     typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(1)));

     u64 idx = 0;
     if (ref_cnt == 1) {
          for (; len > 64 + idx * 2; idx += 64) {
               v_64_u8* const part_from_begin_ptr = (v_64_u8*)&bytes[idx];
               v_64_u8  const part_from_begin     = *part_from_begin_ptr;
               v_64_u8* const part_from_end_ptr   = (v_64_u8*)&bytes[len - idx - 64];
               v_64_u8  const part_from_end       = *part_from_end_ptr;

               *part_from_begin_ptr = __builtin_shufflevector(part_from_end, part_from_end, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
               *part_from_end_ptr   = __builtin_shufflevector(part_from_begin, part_from_begin, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
          }

          u8 const remain_bytes = len - idx * 2;
          switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
          case 6: {
               assert(remain_bytes > 32 && remain_bytes <= 64);

               v_32_u8* const part_from_begin_ptr = (v_32_u8*)&bytes[idx];
               v_32_u8  const part_from_begin     = *part_from_begin_ptr;
               v_32_u8* const part_from_end_ptr   = (v_32_u8*)&bytes[len - idx - 32];
               v_32_u8  const part_from_end       = *part_from_end_ptr;

               *part_from_begin_ptr = __builtin_shufflevector(part_from_end, part_from_end, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
               *part_from_end_ptr   = __builtin_shufflevector(part_from_begin, part_from_begin, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
               return array;
          }
          case 5: {
               assert(remain_bytes > 16 && remain_bytes <= 32);

               v_16_u8* const part_from_begin_ptr = (v_16_u8*)&bytes[idx];
               v_16_u8  const part_from_begin     = *part_from_begin_ptr;
               v_16_u8* const part_from_end_ptr   = (v_16_u8*)&bytes[len - idx - 16];
               v_16_u8  const part_from_end       = *part_from_end_ptr;

               *part_from_begin_ptr = part_from_end.sfedcba9876543210;
               *part_from_end_ptr   = part_from_begin.sfedcba9876543210;
               return array;
          }
          case 4: {
               assert(remain_bytes > 8 && remain_bytes <= 16);

               ua_u64* const part_from_begin_ptr = (ua_u64*)&bytes[idx];
               u64     const part_from_begin     = *part_from_begin_ptr;
               ua_u64* const part_from_end_ptr   = (ua_u64*)&bytes[len - idx - 8];
               u64     const part_from_end       = *part_from_end_ptr;

               *part_from_begin_ptr = __builtin_bswap64(part_from_end);
               *part_from_end_ptr   = __builtin_bswap64(part_from_begin);
               return array;
          }
          case 3: {
               assert(remain_bytes > 4 && remain_bytes <= 8);

               ua_u32* const part_from_begin_ptr = (ua_u32*)&bytes[idx];
               u32     const part_from_begin     = *part_from_begin_ptr;
               ua_u32* const part_from_end_ptr   = (ua_u32*)&bytes[len - idx - 4];
               u32     const part_from_end       = *part_from_end_ptr;

               *part_from_begin_ptr = __builtin_bswap32(part_from_end);
               *part_from_end_ptr   = __builtin_bswap32(part_from_begin);
               return array;
          }
          case 2: {
               assert(remain_bytes >= 2 && remain_bytes <= 4);

               ua_u16* const part_from_begin_ptr = (ua_u16*)&bytes[idx];
               u16     const part_from_begin     = *part_from_begin_ptr;
               ua_u16* const part_from_end_ptr   = (ua_u16*)&bytes[len - idx - 2];
               u16     const part_from_end       = *part_from_end_ptr;

               *part_from_begin_ptr = __builtin_bswap16(part_from_end);
               *part_from_end_ptr   = __builtin_bswap16(part_from_begin);
               return array;
          }
          default:
               assert((i8)remain_bytes < 2);

               return array;
          }
     }

     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any reversed_array     = long_byte_array__new(len);
     reversed_array           = slice__set_metadata(reversed_array, 0, slice_len);
     slice_array__set_len(reversed_array, len);
     u8* const reversed_bytes = slice_array__get_items(reversed_array);
     for (;
          len - idx >= 64;

          *(v_64_u8*)&reversed_bytes[len - idx - 64] = __builtin_shufflevector(*(const v_64_u8*)&bytes[idx], (const v_64_u8){}, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),
          idx += 64
     );

     u64 const remain_bytes = len - idx;
     switch (remain_bytes < 3 ? remain_bytes : sizeof(long long) * 8 - __builtin_clzll(remain_bytes - 1)) {
     case 6:
          *(v_32_u8*)&reversed_bytes[len - idx - 32] = __builtin_shufflevector(*(const v_32_u8*)&bytes[idx], (const v_32_u8){}, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
          *(v_32_u8*)reversed_bytes                  = __builtin_shufflevector(*(const v_32_u8*)&bytes[len - 32], (const v_32_u8){}, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
          break;
     case 5:
          *(v_16_u8*)&reversed_bytes[len - idx - 16] = __builtin_shufflevector(*(const v_16_u8*)&bytes[idx], (const v_16_u8){}, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
          *(v_16_u8*)reversed_bytes                  = __builtin_shufflevector(*(const v_16_u8*)&bytes[len - 16], (const v_16_u8){}, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
          break;
     case 4:
          *(ua_u64*)&reversed_bytes[len - idx - 8] = __builtin_bswap64(*(const ua_u64*)&bytes[idx]);
          *(ua_u64*)reversed_bytes                 = __builtin_bswap64(*(const ua_u64*)&bytes[len - 8]);
          break;
     case 3:
          *(ua_u32*)&reversed_bytes[len - idx - 4] = __builtin_bswap32(*(const ua_u32*)&bytes[idx]);
          *(ua_u32*)reversed_bytes                 = __builtin_bswap32(*(const ua_u32*)&bytes[len - 4]);
          break;
     case 2:
          *(ua_u16*)&reversed_bytes[len - idx - 2] = __builtin_bswap16(*(const ua_u16*)&bytes[idx]);
          *(ua_u16*)reversed_bytes                 = __builtin_bswap16(*(const ua_u16*)&bytes[len - 2]);
          break;
     case 1:
          reversed_bytes[0] = bytes[idx];
          break;
     case 0:
          break;
     default:
          unreachable;
     }

     return reversed_array;
}

static t_any short_byte_array__self_copy(t_thrd_data* const thrd_data, t_any array, u64 const idx_from, u64 const idx_to, u64 const part_len, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8 const array_len = short_byte_array__get_len(array);
     if (idx_from > array_len || idx_to > array_len || part_len > array_len || idx_from + part_len > array_len || idx_to + part_len > array_len) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     memmove_le_16(&array.bytes[idx_to], &array.bytes[idx_from], part_len);
     return array;
}

static t_any long_byte_array__dup__own(t_thrd_data* const thrd_data, t_any const array) {
     assume(array.bytes[15] == tid__byte_array);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     u64       const ref_cnt      = get_ref_cnt(array);
     u64       const array_len    = slice_array__get_len(array);
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;
     const u8* const old_bytes    = &((const u8*)slice_array__get_items(array))[slice_offset];

     assert(ref_cnt != 1);

     if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);

     t_any     new_array = long_byte_array__new(len);
     slice_array__set_len(new_array, len);
     new_array           = slice__set_metadata(new_array, 0, slice_len);
     u8* const new_bytes = slice_array__get_items(new_array);
     memcpy(new_bytes, old_bytes, len);

     return new_array;
}

static t_any long_byte_array__self_copy__own(t_thrd_data* const thrd_data, t_any array, u64 const idx_from, u64 const idx_to, u64 const part_len, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);

     u32 const slice_len = slice__get_len(array);
     u64 const array_len = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     if (idx_from > array_len || idx_to > array_len || part_len > array_len || idx_from + part_len > array_len || idx_to + part_len > array_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (part_len == 0 || idx_from == idx_to) return array;
     if (get_ref_cnt(array) != 1) array = long_byte_array__dup__own(thrd_data, array);

     u8* const bytes = &((u8*)slice_array__get_items(array))[slice__get_offset(array)];
     memmove(&bytes[idx_to], &bytes[idx_from], part_len);

     return array;
}

static t_any short_byte_array__sort(t_any array, bool const is_asc_order) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8 const array_len  = short_byte_array__get_len(array);
     if (array_len < 2) return array;

     typedef u16 v_8_u16 __attribute__((ext_vector_type(8)));

     memset_inline(&array.bytes[14], 0, 2);
     v_8_u16 shifts = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);
     // array.vector    = array.vector.s0132457689bacdfe;
     // array.vector    = array.vector.s021375648a9bfdec;
     array.vector   = array.vector.s031265748b9aedfc;
     shifts         = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);
     array.vector   = array.vector.s021375648a9bfdec;
     shifts         = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);
     // array.vector    = array.vector.s0123547689abdcfe;
     // array.vector    = array.vector.s04152637fbead9c8;
     array.vector   = array.vector.s05142736ebfac9d8;
     shifts         = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);
     // array.vector    = array.vector.s02461357fdb9eca8;
     // array.vector    = array.vector.s02134657b9a8fdec;
     array.vector   = array.vector.s042615379dbf8cae;
     shifts         = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);
     array.vector   = array.vector.s02134657b9a8fdec;
     shifts         = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);
     // array.vector    = array.vector.s0123456798badcfe;
     // array.vector    = array.vector.s08192a3b4c5d6e7f;
     array.vector   = array.vector.s09182b3a4d5c6f7e;
     shifts         = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);
     // array.vector    = array.vector.s02468ace13579bdf;
     // array.vector    = array.vector.s041526378c9daebf;
     array.vector   = array.vector.s082a4c6e193b5d7f;
     shifts         = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);
     // array.vector    = array.vector.s024613578ace9bdf;
     // array.vector    = array.vector.s021346578a9bcedf;
     array.vector   = array.vector.s042615378cae9dbf;
     shifts         = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);
     array.vector   = array.vector.s021346578a9bcedf;
     shifts         = __builtin_convertvector((array.vector.s02468ace > array.vector.s13579bdf) & 8, v_8_u16);
     array.vector   = (const v_any_u8)((const v_8_u16)array.vector << shifts | (const v_8_u16)array.vector >> shifts);

     if (is_asc_order)
          array.raw_bits >>= (16 - array_len) * 8;
     else {
          array.vector    = array.vector.sfedcba9876543210;
          array.raw_bits &= ((u128)1 << array_len * 8) - 1;
     }

     array.bytes[15] = tid__short_byte_array;
     array           = short_byte_array__set_len(array, array_len);
     return array;
}

[[clang::noinline]]
static t_any long_byte_array__sort__own(t_thrd_data* const thrd_data, t_any array, bool const is_asc_order) {
     assume(array.bytes[15] == tid__byte_array);

     u32 const slice_offset = slice__get_offset(array);
     u32 const slice_len    = slice__get_len(array);
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;
     u8* bytes              = &((u8*)slice_array__get_items(array))[slice_offset];

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64 bytes_table[256] = {};
     for (u64 idx = 0; idx < len; bytes_table[bytes[idx]] += 1, idx += 1);

     if (ref_cnt != 1) {
          if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

          array = long_byte_array__new(len);
          array = slice__set_metadata(array, 0, slice_len);
          slice_array__set_len(array, len);
          bytes = slice_array__get_items(array);
     }

     u64 array_idx = 0;
     if (is_asc_order)
          for (int table_idx = 0; table_idx < 256; table_idx += 1) {
               u64 const qty = bytes_table[table_idx];
               if (qty < 257)
                    memset_le_256(&bytes[array_idx], table_idx, qty);
               else memset(&bytes[array_idx], table_idx, qty);

               array_idx += qty;
          }
     else
          for (int table_idx = 255; table_idx != -1; table_idx -= 1) {
               u64 const qty = bytes_table[table_idx];
               if (qty < 257)
                    memset_le_256(&bytes[array_idx], table_idx, qty);
               else memset(&bytes[array_idx], table_idx, qty);

               array_idx += qty;
          }

     return array;
}

[[clang::always_inline]]
static void bytes__swap(u8* const bytes, u64 const idx_1, u64 const idx_2) {
     u8 const buffer = bytes[idx_1];
     bytes[idx_1]    = bytes[idx_2];
     bytes[idx_2]    = buffer;
}

static t_any bytes__heap_sort_fn__to_heap(t_thrd_data* const thrd_data, u8* const bytes, t_any const fn, u64 currend_idx, u64 const begin_idx, u64 const end_edge, const char* const owner) {
     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {[15] = tid__byte}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};

     while (true) {
          u64       largest_idx = currend_idx;
          u64 const left_idx    = 2 * currend_idx - begin_idx + 1;
          u64 const right_idx   = left_idx + 1;
          if (right_idx < end_edge) {
               fn_args[0].bytes[0] = bytes[largest_idx];
               fn_args[1].bytes[0] = bytes[right_idx];
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

               fn_args[0].bytes[0] = bytes[largest_idx];
               fn_args[1].bytes[0] = bytes[left_idx];
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
               fn_args[0].bytes[0] = bytes[largest_idx];
               fn_args[1].bytes[0] = bytes[left_idx];
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

          bytes__swap(bytes, currend_idx, largest_idx);
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

static t_any bytes__heap_sort_fn(t_thrd_data* const thrd_data, u8* const bytes, t_any const fn, u64 const begin_idx, u64 const end_edge, const char* const owner) {
     for (u64 currend_idx = (end_edge - begin_idx) / 2 + begin_idx - 1; (i64)currend_idx >= (i64)begin_idx; currend_idx -= 1) {
          t_any const to_heap_result = bytes__heap_sort_fn__to_heap(thrd_data, bytes, fn, currend_idx, begin_idx, end_edge, owner);
          if (to_heap_result.bytes[15] == tid__error) [[clang::unlikely]] return to_heap_result;
     }

     for (u64 current_edge = end_edge - 1; (i64)current_edge >= (i64)begin_idx; current_edge -= 1) {
          bytes__swap(bytes, begin_idx, current_edge);
          t_any const to_heap_result = bytes__heap_sort_fn__to_heap(thrd_data, bytes, fn, begin_idx, begin_idx, current_edge, owner);
          if (to_heap_result.bytes[15] == tid__error) [[clang::unlikely]] return to_heap_result;
     }

     return null;
}

[[clang::noinline]]
static t_any bytes__quick_sort_fn(t_thrd_data* const thrd_data, u8* const bytes, t_any const fn, u64 const begin_idx, u64 const end_edge, u64 const rnd_num, const char* const owner) {
     t_any fn_args[2];
     fn_args[0] = (const t_any){.bytes = {[15] = tid__byte}};
     fn_args[1] = (const t_any){.bytes = {[15] = tid__byte}};

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
               bytes__swap(bytes, left_idx, left_idx + rnd_offset);

               u8 const current_byte = bytes[left_idx];
               u64      currend_idx  = left_idx;
               u64      opposite_idx = right_edge - 1;
               u64      direction    = -1;
               bool     cmp_modifier = true;
               while (true) {
                    u8 const opposite_byte = bytes[opposite_idx];
                    fn_args[0].bytes[0]    = current_byte;
                    fn_args[1].bytes[0]    = opposite_byte;
                    t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

                    switch (cmp_result.raw_bits) {
                    case comptime_tkn__less: case comptime_tkn__great:
                         if ((cmp_result.bytes[0] == less_id) == cmp_modifier) break;
                    case comptime_tkn__equal: {
                         bytes__swap(bytes, currend_idx, opposite_idx);

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
               t_any const sort_result = bytes__quick_sort_fn(thrd_data, bytes, fn, small_left_idx, small_right_edge, rnd_num, owner);
               if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;
          } else if (small_range_len > 2) {
               t_any const sort_result = bytes__heap_sort_fn(thrd_data, bytes, fn, small_left_idx, small_right_edge, owner);
               if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] return sort_result;
          } else if (small_range_len == 2) {
               fn_args[0].bytes[0] = bytes[small_left_idx];
               fn_args[1].bytes[0] = bytes[small_left_idx + 1];
               t_any const cmp_result = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);

               switch (cmp_result.raw_bits) {
               case comptime_tkn__great:
                    bytes__swap(bytes, small_left_idx, small_left_idx + 1);
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

static t_any byte_array__sort_fn__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array || array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8* bytes;
     u64 len;
     if (array.bytes[15] == tid__short_byte_array) {
          len = short_byte_array__get_len(array);

          if (len < 2) {
               ref_cnt__dec(thrd_data, fn);
               return array;
          }

          bytes = array.bytes;
     } else {
          if (get_ref_cnt(array) != 1) array = long_byte_array__dup__own(thrd_data, array);

          u32 const slice_offset = slice__get_offset(array);
          u32 const slice_len    = slice__get_len(array);
          bytes                  = &((u8*)slice_array__get_items(array))[slice_offset];
          len                    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

          assert(slice_len <= slice_max_len || slice_offset == 0);
     }

     t_any const sort_result = bytes__quick_sort_fn(thrd_data, bytes, fn, 0, len, static_rnd_num, owner);
     ref_cnt__dec(thrd_data, fn);
     if (sort_result.bytes[15] == tid__error) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return sort_result;
     }

     return array;
}

static t_any short_byte_array__split(t_thrd_data* const thrd_data, t_any array, u8 const separator, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8       remain_bytes   = short_byte_array__get_len(array);
     u16      separator_mask = v_16_bool_to_u16(__builtin_convertvector(array.vector == separator, v_16_bool)) & (((u16)1 << remain_bytes) - 1);
     memset_inline(&array.bytes[14], 0, 2);

     u8 const     result_len    = __builtin_popcount(separator_mask) + 1;
     t_any        result        = array__init(thrd_data, result_len, owner);
     result                     = slice__set_metadata(result, 0, result_len);
     slice_array__set_len(result, result_len);
     t_any* const parts         = slice_array__get_items(result);
     t_any        part;
     for (u8 part_idx = 0; part_idx < result_len; part_idx += 1) {
          u8 const separator_idx = separator_mask == 0 ? remain_bytes : __builtin_ctz(separator_mask);
          part.raw_bits          = array.raw_bits & (((u128)1 << separator_idx * 8) - 1);
          part.bytes[15]         = tid__short_byte_array;
          part                   = short_byte_array__set_len(part, separator_idx);
          parts[part_idx]        = part;
          u8 const shift         = separator_idx + 1;
          array.raw_bits       >>= shift * 8;
          separator_mask       >>= shift;
          remain_bytes          -= shift;
     }

     return result;
}

static inline t_any array__append__own(t_thrd_data* const thrd_data, t_any const array, t_any const new_item, const char* const owner);

static t_any long_byte_array__split__own(t_thrd_data* const thrd_data, t_any const array, u8 const separator, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);

     u32       const slice_offset = slice__get_offset(array);
     u32       const slice_len    = slice__get_len(array);
     const u8* const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];
     u64       const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any result         = array__init(thrd_data, 1, owner);
     u64   part_first_idx = 0;
     while (true) {
          u64 separator_idx = look_byte_from_begin(&bytes[part_first_idx], len - part_first_idx, separator);
          separator_idx     = separator_idx == (u64)-1 ? len : part_first_idx + separator_idx;

          ref_cnt__inc(thrd_data, array, owner);
          t_any const part = long_byte_array__slice__own(thrd_data, array, part_first_idx, separator_idx, owner);
          part_first_idx   = separator_idx + 1;

          assert(part.bytes[15] == tid__short_byte_array || part.bytes[15] == tid__byte_array);

          result = array__append__own(thrd_data, result, part, owner);
          if (separator_idx == len) break;
     }

     ref_cnt__dec(thrd_data, array);
     return result;
}

static t_any short_byte_array__split_by_part__own(t_thrd_data* const thrd_data, t_any array, t_any const separator, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assert(separator.bytes[15] == tid__short_byte_array || separator.bytes[15] == tid__byte_array);

     u8 array_len = short_byte_array__get_len(array);
     memset_inline(&array.bytes[14], 0, 2);
     if (separator.raw_bits == (const t_any){.bytes = {[15] = tid__short_byte_array}}.raw_bits) {
          if (array_len == 0) return empty_array;

          t_any        result = array__init(thrd_data, array_len, owner);
          result              = slice__set_metadata(result, 0, array_len);
          slice_array__set_len(result, array_len);
          t_any* const parts  = slice_array__get_items(result);
          t_any        part   = (const t_any){.bytes = {[14] = 1, [15] = tid__short_byte_array}};
          for (u8 part_idx = 0; part_idx < array_len; part_idx += 1) {
               part.bytes[0]    = array.bytes[part_idx];
               parts[part_idx]  = part;
          }

          return result;
     }

     t_any result = array__init(thrd_data, 1, owner);
     if (separator.bytes[15] == tid__short_byte_array) {
          u8 const separator_len   = short_byte_array__get_len(separator);
          u16      occurrence_mask = -1;
          for (u8 idx = 0; idx < separator_len; idx += 1)
               occurrence_mask &= v_16_bool_to_u16(__builtin_convertvector(array.vector == separator.vector[idx], v_16_bool)) >> idx;
          occurrence_mask &= separator_len > array_len ? 0 : ((u16)1 << (array_len - separator_len + 1)) - 1;

          t_any part;
          while (occurrence_mask != 0) {
               u8 const separator_offset = __builtin_ctz(occurrence_mask);
               part.raw_bits             = array.raw_bits & (((u128)1 << separator_offset * 8) - 1);
               part.bytes[15]            = tid__short_byte_array;
               part                      = short_byte_array__set_len(part, separator_offset);
               result                    = array__append__own(thrd_data, result, part, owner);

               u8 const shift    = separator_offset + separator_len;
               array.raw_bits  >>= shift * 8;
               occurrence_mask >>= shift;
               array_len        -= shift;
          }
     } else ref_cnt__dec(thrd_data, separator);

     array.bytes[15] = tid__short_byte_array;
     array           = short_byte_array__set_len(array, array_len);
     result          = array__append__own(thrd_data, result, array, owner);
     return result;
}

static t_any long_byte_array__split_by_part__own(t_thrd_data* const thrd_data, t_any const array, t_any const separator, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(separator.bytes[15] == tid__short_byte_array || separator.bytes[15] == tid__byte_array);

     u32       const array_slice_offset = slice__get_offset(array);
     u32       const array_slice_len    = slice__get_len(array);
     const u8* const array_bytes        = &((const u8*)slice_array__get_items(array))[array_slice_offset];
     u64       const array_len          = array_slice_len <= slice_max_len ? array_slice_len : slice_array__get_len(array);

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     const u8* separator_bytes;
     u64       separator_len;
     if (separator.bytes[15] == tid__short_byte_array) {
          separator_len   = short_byte_array__get_len(separator);
          separator_bytes = separator.bytes;

          if (separator_len == 0) {
               t_any        result = array__init(thrd_data, array_len, owner);
               result              = slice__set_metadata(result, 0, array_slice_len);
               slice_array__set_len(result, array_len);
               t_any* const parts  = slice_array__get_items(result);
               for (u64 part_idx = 0; part_idx < array_len; part_idx += 1) {
                    parts[part_idx].qwords[0] = array_bytes[part_idx];
                    parts[part_idx].qwords[1] = (u64)tid__short_byte_array << 56 | (u64)1 << 48;
               }

               ref_cnt__dec(thrd_data, array);
               return result;
          }
     } else {
          u32 const separator_slice_offset = slice__get_offset(separator);
          u32 const separator_slice_len    = slice__get_len(separator);
          separator_bytes                  = &((const u8*)slice_array__get_items(separator))[separator_slice_offset];
          separator_len                    = separator_slice_len <= slice_max_len ? separator_slice_len : slice_array__get_len(separator);

          assert(separator_slice_len <= slice_max_len || separator_slice_offset == 0);
     }

     t_any       result               = array__init(thrd_data, 1, owner);
     u64         part_first_idx       = 0;
     u64   const edge                 = array_len - separator_len + (separator_len == 1 ? 1 : 2);
     while (true) {
          u64 const array_remain_len = array_len - part_first_idx;
          u64       separator_idx;
          if (separator_len > array_remain_len) separator_idx = array_len;
          else if (separator_len == 1) {
               u64 array_idx = part_first_idx;
               while (true) {
                    separator_idx = look_byte_from_begin(&array_bytes[array_idx], edge - array_idx, separator_bytes[0]);
                    if (separator_idx == (u64)-1) {
                         separator_idx = array_len;
                         break;
                    }

                    separator_idx += array_idx;
                    break;
               }
          } else if (separator_len >= 1024 && array_remain_len >= 8192) {
               separator_idx = look_byte_part_from_begin__rabin_karp(&array_bytes[part_first_idx], array_remain_len, separator_bytes, separator_len);
               separator_idx = separator_idx == -1 ? array_len : part_first_idx + separator_idx;
          } else {
               u64 array_idx = part_first_idx;
               while (true) {
                    separator_idx = look_2_bytes_from_begin(&array_bytes[array_idx], edge - array_idx, *(const ua_u16*)separator_bytes);
                    if (separator_idx == (u64)-1) {
                         separator_idx = array_len;
                         break;
                    }

                    array_idx += separator_idx;

                    if (scar__memcmp(&array_bytes[array_idx], separator_bytes, separator_len) == 0) {
                         separator_idx = array_idx;
                         break;
                    }

                    array_idx += 1;
               }
          }

          ref_cnt__inc(thrd_data, array, owner);
          t_any const part = long_byte_array__slice__own(thrd_data, array, part_first_idx, separator_idx, owner);
          part_first_idx   = separator_idx + separator_len;

          assert(part.bytes[15] == tid__short_byte_array || part.bytes[15] == tid__byte_array);

          result = array__append__own(thrd_data, result, part, owner);
          if (separator_idx == array_len) break;
     }

     ref_cnt__dec(thrd_data, array);
     ref_cnt__dec(thrd_data, separator);

     return result;
}

static t_any suffix_of__long_byte_array__byte_array__own(t_thrd_data* const thrd_data, t_any const array, t_any const suffix) {
     assume(array.bytes[15] == tid__byte_array);
     assume(suffix.bytes[15] == tid__short_byte_array || suffix.bytes[15] == tid__byte_array);

     u32       const array_slice_offset = slice__get_offset(array);
     u32       const array_slice_len    = slice__get_len(array);
     const u8* const array_bytes        = &((const u8*)slice_array__get_items(array))[array_slice_offset];
     u64       const array_ref_cnt      = get_ref_cnt(array);
     u64       const array_array_len    = slice_array__get_len(array);
     u64       const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);

     const u8* suffix_bytes;
     u64       suffix_len;
     u64       suffix_ref_cnt;
     if (suffix.bytes[15] == tid__short_byte_array) {
          suffix_ref_cnt = 0;
          suffix_bytes   = suffix.bytes;
          suffix_len     = short_byte_array__get_len(suffix);
     } else {
          u32 const suffix_slice_offset = slice__get_offset(suffix);
          u32 const suffix_slice_len    = slice__get_len(suffix);
          suffix_bytes                  = &((const u8*)slice_array__get_items(suffix))[suffix_slice_offset];
          suffix_ref_cnt                = get_ref_cnt(suffix);
          u64 const suffix_array_len    = slice_array__get_len(suffix);
          suffix_len                    = suffix_slice_len <= slice_max_len ? suffix_slice_len : suffix_array_len;

          assert(suffix_slice_len <= slice_max_len || suffix_slice_offset == 0);
     }

     if (array.qwords[0] == suffix.qwords[0] && suffix.bytes[15] == tid__byte_array) {
          if (array_ref_cnt > 2) set_ref_cnt(array, array_ref_cnt - 2);
     } else {
          if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);
          if (suffix_ref_cnt > 1) set_ref_cnt(suffix, suffix_ref_cnt - 1);
     }

     t_any const result = array_len >= suffix_len && scar__memcmp(&array_bytes[array_len - suffix_len], suffix_bytes, suffix_len) == 0 ? bool__true : bool__false;

     if (array.qwords[0] == suffix.qwords[0] && suffix.bytes[15] == tid__byte_array) {
          if (array_ref_cnt == 2) free((void*)array.qwords[0]);
     } else {
          if (array_ref_cnt == 1) free((void*)array.qwords[0]);
          if (suffix_ref_cnt == 1) free((void*)suffix.qwords[0]);
     }

     return result;
}

static t_any short_byte_array__take_while__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     t_any byte     = {.bytes = {[15] = tid__byte}};
     u8    len      = short_byte_array__get_len(array);
     u8    offset   = 0;
     for (; offset < len; offset += 1) {
          byte.bytes[0] = array.bytes[offset];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) break;
     }
     ref_cnt__dec(thrd_data, fn);

     array.raw_bits  &= ((u128)1 <<offset * 8) - 1;
     array.bytes[15]  = tid__short_byte_array;
     array            = short_byte_array__set_len(array, offset);
     return array;
}

static t_any long_byte_array__take_while__own(t_thrd_data* const thrd_data, t_any array, t_any const fn, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u32        const slice_offset = slice__get_offset(array);
     u32        const slice_len    = slice__get_len(array);
     const u8*  const bytes        = &((const u8*)slice_array__get_items(array))[slice_offset];
     u64        const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     t_any byte = {.bytes = {[15] = tid__byte}};
     u64   n    = 0;
     for (; n < len; n += 1) {
          byte.bytes[0] = bytes[n];
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &byte, 1, owner);
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

     if (n < 15) {
          t_any short_array;
          short_array.raw_bits  = n == 0 ? 0 : (*(ua_u128*)&bytes[(i64)(n) - 16]) >> (16 - n) * 8;
          short_array.bytes[15] = tid__short_byte_array;
          short_array           = short_byte_array__set_len(short_array, n);

          ref_cnt__dec(thrd_data, array);
          return short_array;
     }

     if (n <= slice_max_len) [[clang::likely]] {
          array = slice__set_metadata(array, slice_offset, n);
          return array;
     }

     assert(slice_offset == 0);

     u64 const ref_cnt = get_ref_cnt(array);
     if (ref_cnt == 1) {
          slice_array__set_len(array, n);
          slice_array__set_cap(array, n);
          array           = slice__set_metadata(array, 0, slice_max_len + 1);
          array.qwords[0] = (u64)realloc((u8*)array.qwords[0], n + 16);

          return array;
     }
     if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

     t_any     new_array = long_byte_array__new(n);
     new_array           = slice__set_metadata(new_array, 0, slice_max_len + 1);
     slice_array__set_len(new_array, n);
     u8* const new_bytes = slice_array__get_items(new_array);
     memcpy(new_bytes, bytes, n);

     return new_array;
}

static t_any byte_array__to_array__own(t_thrd_data* const thrd_data, t_any const array) {
     assert(array.bytes[15] == tid__short_byte_array || array.bytes[15] == tid__byte_array);

     u64       len;
     const u8* bytes;
     bool      need_to_free;
     if (array.bytes[15] == tid__short_byte_array) {
          len = short_byte_array__get_len(array);
          if (len == 0) return empty_array;
          bytes = array.bytes;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(array);
          u32 const slice_len    = slice__get_len(array);
          bytes                  = &((const u8*)slice_array__get_items(array))[slice_offset];
          u64 const ref_cnt      = get_ref_cnt(array);
          u64 const array_len    = slice_array__get_len(array);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);
          need_to_free = ref_cnt == 1;
     }

     t_any        result       = array__init(thrd_data, len, nullptr);
     result                    = slice__set_metadata(result, 0, len <= slice_max_len ? len : slice_max_len + 1);
     slice_array__set_len(result, len);
     t_any* const result_items = slice_array__get_items(result);
     t_any        byte         = {.bytes = {[15] = tid__byte}};
     for (u64 idx = 0; idx < len; idx += 1) {
          byte.bytes[0] = bytes[idx];
          result_items[idx] = byte;
     }

     if (need_to_free) free((void*)array.qwords[0]);
     return result;
}

static t_any byte_array__to_box__own(t_thrd_data* const thrd_data, t_any const array, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array | array.bytes[15] == tid__byte_array);

     u64       len;
     const u8* bytes;
     bool      need_to_free;
     if (array.bytes[15] == tid__short_byte_array) {
          len = short_byte_array__get_len(array);
          if (len == 0) return empty_box;
          bytes = array.bytes;
          need_to_free = false;
     } else {
          u32 const slice_offset = slice__get_offset(array);
          u32 const slice_len    = slice__get_len(array);
          bytes                  = &((const u8*)slice_array__get_items(array))[slice_offset];
          u64 const ref_cnt      = get_ref_cnt(array);
          u64 const array_len    = slice_array__get_len(array);
          len                    = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (len > 16) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, array);
               return error__out_of_bounds(thrd_data, owner);
          }

          if (ref_cnt > 1) set_ref_cnt(array, ref_cnt - 1);
          need_to_free = ref_cnt == 1;
     }

     t_any        box       = box__new(thrd_data, len, owner);
     t_any* const box_items = box__get_items(box);
     for (u8 idx = 0; idx < len; idx += 1)
          box_items[idx] = (const t_any){.bytes = {bytes[idx], [15] = tid__byte}};

     if (need_to_free) free((void*)array.qwords[0]);
     return box;
}

static t_any long_byte_array__unslice__own(t_thrd_data* const thrd_data, t_any array) {
     assume(array.bytes[15] == tid__byte_array);

     u32 const slice_len = slice__get_len(array);
     u64 const ref_cnt   = get_ref_cnt(array);
     u64 const array_len = slice_array__get_len(array);
     u64 const array_cap = slice_array__get_cap(array);
     u64 const len       = slice_len <= slice_max_len ? slice_len : array_len;

     if ((len == array_len && array_cap == array_len) || ref_cnt == 0) return array;

     u32 const slice_offset = slice__get_offset(array);

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u8* const bytes = slice_array__get_items(array);
     if (ref_cnt == 1) {
          if (slice_offset != 0) memmove(bytes, &bytes[slice_offset], len);
          slice_array__set_len(array, len);
          slice_array__set_cap(array, len);
          array           = slice__set_metadata(array, 0, slice_len);
          array.qwords[0] = (u64)realloc((u8*)array.qwords[0], len + 16);
          return array;
     }
     set_ref_cnt(array, ref_cnt - 1);

     t_any     new_array = long_byte_array__new(len);
     new_array           = slice__set_metadata(new_array, 0, slice_len);
     slice_array__set_len(new_array, len);
     u8* const new_bytes = slice_array__get_items(new_array);
     memcpy(new_bytes, &bytes[slice_offset], len);

     return new_array;
}

static void byte_array__zip_init_read(const t_any* const array, t_zip_read_state* const state, t_any* key, t_any* value, u64* const result_len, t_any* const) {
     assert(array->bytes[15] == tid__short_byte_array || array->bytes[15] == tid__byte_array );

     if (array->bytes[15] == tid__short_byte_array) {
          state->byte_position = array->bytes;
          *result_len          = short_byte_array__get_len(*array);
     } else {
          state->byte_position = &((const u8*)slice_array__get_items(*array))[slice__get_offset(*array)];
          u32 const slice_len  = slice__get_len(*array);
          *result_len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(*array);
     }

     *key   = (const t_any){.structure = {.value = -1, .type = tid__int}};
     *value = (const t_any){.bytes = {[15] = tid__byte}};
}

static void byte_array__zip_read(t_thrd_data* const, t_zip_read_state* const state, t_any* const key, t_any* const value, const char* const) {
     key->qwords[0]      += 1;
     value->bytes[0]      = *state->byte_position;
     state->byte_position = &state->byte_position[1];
}

static void byte_array__zip_init_search(const t_any* const array, t_zip_search_state* const state, t_any* value, u64* const result_len, t_any* const) {
     assert(array->bytes[15] == tid__short_byte_array || array->bytes[15] == tid__byte_array );

     if (array->bytes[15] == tid__short_byte_array) {
          state->byte_position = array->bytes;
          u8 const array_len   = short_byte_array__get_len(*array);
          *result_len          = *result_len > array_len ? array_len : *result_len;
     } else {
          state->byte_position = &((const u8*)slice_array__get_items(*array))[slice__get_offset(*array)];
          u32 const slice_len  = slice__get_len(*array);
          u64 const array_len  = slice_len <= slice_max_len ? slice_len : slice_array__get_len(*array);
          *result_len          = *result_len > array_len ? array_len : *result_len;
     }

     *value = (const t_any){.bytes = {[15] = tid__byte}};
}

static bool byte_array__zip_search(t_thrd_data* const, t_zip_search_state* const state, t_any const, t_any* const value, const char* const) {
     value->bytes[0]      = *state->byte_position;
     state->byte_position = &state->byte_position[1];
     return true;
}

static t_any byte_array__join(
     t_thrd_data* const thrd_data,
     u64          const parts_len,
     const t_any* const parts,
     u64          const separator_len,
     const u8*          separator_bytes,
     u64          const result_len,
     u8           const step_size,
     const char*  const owner
) {
     if (parts_len == 0) return (const t_any){.bytes = {[15] = tid__short_byte_array}};

     u64 idx = 0;
     for (; parts[idx].bytes[15] == tid__null || parts[idx].bytes[15] == tid__holder; idx += step_size);

     if (parts_len == 1) {
          t_any const result = parts[idx];
          ref_cnt__inc(thrd_data, result, owner);
          return result;
     }

     u64 remain_items_len = parts_len;

     if (result_len < 15) {
          t_any result             = parts[idx];
          u8    current_result_len = short_byte_array__get_len(result);
          remain_items_len        -= 1;
          for (idx += step_size; remain_items_len != 0; idx += step_size) {
               const t_any* const part_ptr = &parts[idx];
               if (part_ptr->bytes[15] == tid__null || part_ptr->bytes[15] == tid__holder) continue;

               result.raw_bits    |= *(const u128*)separator_bytes << current_result_len * 8;
               current_result_len += separator_len;
               result.raw_bits    |= part_ptr->raw_bits << current_result_len * 8;
               current_result_len += short_byte_array__get_len(*part_ptr);

               remain_items_len -= 1;
          }

          result.bytes[15] = tid__short_byte_array;
          result           = short_byte_array__set_len(result, result_len);
          return result;
     }

     t_any     result       = long_byte_array__new(result_len);
     slice_array__set_len(result, result_len);
     result                 = slice__set_metadata(result, 0, result_len <= slice_max_len ? result_len : slice_max_len + 1);
     u8* const result_bytes = slice_array__get_items(result);
     u64 current_result_len = 0;

     for (;; idx += step_size) {
          const t_any* const part_ptr = &parts[idx];
          switch (part_ptr->bytes[15]) {
          case tid__null: case tid__holder:
               continue;
          case tid__short_byte_array:
               u8 const part_len = short_byte_array__get_len(*part_ptr);
               memcpy_le_16(&result_bytes[current_result_len], part_ptr->bytes, part_len);
               current_result_len += part_len;
               break;
          case tid__byte_array: {
               t_any     const part              = *part_ptr;
               u32       const part_slice_offset = slice__get_offset(part);
               u32       const part_slice_len    = slice__get_len(part);
               u64       const part_len          = part_slice_len <= slice_max_len ? part_slice_len : slice_array__get_len(part);
               const u8* const part_bytes        = &((const u8*)slice_array__get_items(part))[part_slice_offset];

               memcpy(&result_bytes[current_result_len], part_bytes, part_len);
               current_result_len += part_len;
               break;
          }
          default:
               unreachable;
          }

          remain_items_len -= 1;
          if (remain_items_len == 0) break;

          memcpy(&result_bytes[current_result_len], separator_bytes, separator_len);
          current_result_len += separator_len;
     }

     return result;
}

static t_any short_byte_array__insert_to(t_thrd_data* const thrd_data, t_any array, u8 const byte, u64 const position, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8 const len = short_byte_array__get_len(array);
     if (position > len) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     u8 const new_len = len + 1;
     if (new_len < 15) {
          u128 const first_part_mask = ((u128)1 << position * 8) - 1;
          u128 const last_part       = (array.raw_bits & ~first_part_mask) << 8;
          array.raw_bits            &= first_part_mask;
          array.bytes[position]      = byte;
          array.raw_bits            |= last_part;
          array.bytes[15]            = tid__short_byte_array;
          array                      = short_byte_array__set_len(array, new_len);
          return array;
     }

     t_any    result       = long_byte_array__new(new_len);
     result                = slice__set_metadata(result, 0, new_len);
     slice_array__set_len(result, new_len);
     u8*      result_bytes = slice_array__get_items(result);
     u8       old_idx      = 0;
     for (u8 new_idx = 0; new_idx < new_len; new_idx += 1) {
          if (new_idx == (u8)position) result_bytes[new_idx] = byte;
          else {
               result_bytes[new_idx] = array.bytes[old_idx];
               old_idx += 1;
          }
     }

     return result;
}

static t_any long_byte_array__insert_to__own(t_thrd_data* const thrd_data, t_any const array, u8 const byte, u64 const position, const char* const owner) {
     assert(array.bytes[15] == tid__byte_array);

     u32 const slice_len    = slice__get_len(array);
     u32 const slice_offset = slice__get_offset(array);
     u64 const ref_cnt      = get_ref_cnt(array);
     u64 const array_len    = slice_array__get_len(array);
     u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (position > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const new_len = len + 1;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     t_any result;
     if (ref_cnt == 1) {
          u64 const cap = slice_array__get_cap(array);
          result        = array;
          slice_array__set_len(result, new_len);
          result        = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (cap < new_len) {
               result.qwords[0] = (u64)realloc((u8*)result.qwords[0], new_cap + 16);
               slice_array__set_cap(result, new_cap);
          }

          u8* const bytes = slice_array__get_items(result);
          if (slice_offset == 0) {
               memmove(&bytes[position + 1], &bytes[position], len - position);
               bytes[position] = byte;
          } else {
               memmove(bytes, &bytes[slice_offset], position);
               bytes[position] = byte;
               memmove(&bytes[position + 1], &bytes[slice_offset + position], len - position);
          }
     } else {
          if (ref_cnt != 0) set_ref_cnt(array, ref_cnt - 1);

          result                    = long_byte_array__new(new_len);
          slice_array__set_len(result, new_len);
          result                    = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          const u8* const old_bytes = &((const u8*)slice_array__get_items(array))[slice_offset];
          u8*       const new_bytes = slice_array__get_items(result);
          memcpy(new_bytes, old_bytes, position);
          new_bytes[position] = byte;
          memcpy(&new_bytes[position + 1], &old_bytes[position], len - position);
     }

     return result;
}

static t_any insert_part_to__short_byte_array__short_byte_array(t_thrd_data* const thrd_data, t_any array, t_any const part, u64 const position, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assert(part.bytes[15] == tid__short_byte_array);

     u8 const array_len = short_byte_array__get_len(array);
     if (position > array_len) [[clang::unlikely]] return error__out_of_bounds(thrd_data, owner);

     u8 const part_len = short_byte_array__get_len(part);
     u8 const new_len  = array_len + part_len;
     if (new_len < 15) {
          u128 const first_part_mask = ((u128)1 << position * 8) - 1;
          u128 const last_part       = (array.raw_bits & ~first_part_mask) << part_len * 8;
          array.raw_bits            &= first_part_mask;
          array.raw_bits            |= part.raw_bits << position * 8;
          array.raw_bits            |= last_part;
          array.bytes[15]            = tid__short_byte_array;
          array                      = short_byte_array__set_len(array, new_len);
          return array;
     }

     t_any result       = long_byte_array__new(new_len);
     result             = slice__set_metadata(result, 0, new_len);
     slice_array__set_len(result, new_len);
     u8*   result_bytes = slice_array__get_items(result);

     *(ua_u128*)&result_bytes[(i64)position - 16]               &= (u128)-1 >> position * 8;
     *(ua_u128*)&result_bytes[(i64)position - 16]               |= position == 0 ? 0 : (array.raw_bits & ((u128)1 << position * 8) - 1) << (16 - position) * 8;
     *(ua_u128*)&result_bytes[(i64)(position + part_len) - 16]  &= (u128)-1 >> part_len * 8;
     *(ua_u128*)&result_bytes[(i64)(position + part_len) - 16]  |= part.raw_bits << (16 - part_len) * 8;
     u8 const tail_len                                           = array_len - position;
     *(ua_u128*)&result_bytes[(i64)(part_len + array_len) - 16] &= (u128)-1 >> tail_len * 8;
     *(ua_u128*)&result_bytes[(i64)(part_len + array_len) - 16] |= tail_len == 0 ? 0 : (array.raw_bits >> position * 8) << (16 - tail_len) * 8;
     return result;
}

static t_any insert_part_to__short_byte_array__long_byte_array__own(t_thrd_data* const thrd_data, t_any const array, t_any const part, u64 const position, const char* const owner) {
     assert(array.bytes[15] == tid__short_byte_array);
     assume(part.bytes[15] == tid__byte_array);

     u8 const array_len = short_byte_array__get_len(array);
     if (position > array_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, part);
          return error__out_of_bounds(thrd_data, owner);
     }

     if (array_len == 0) return part;

     u32 const slice_len      = slice__get_len(part);
     u32 const slice_offset   = slice__get_offset(part);
     u64 const part_ref_cnt   = get_ref_cnt(part);
     u64 const part_array_len = slice_array__get_len(part);
     u64 const part_len       = slice_len <= slice_max_len ? slice_len : part_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     u64 const new_len = array_len + part_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     t_any result;
     if (part_ref_cnt == 1) {
          u64 const cap = slice_array__get_cap(part);
          result        = part;
          slice_array__set_len(result, new_len);
          result        = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (cap < new_len) {
               result.qwords[0] = (u64)realloc((u8*)result.qwords[0], new_cap + 16);
               slice_array__set_cap(result, new_cap);
          }

          u8* const result_bytes = slice_array__get_items(result);

          if (slice_offset != position) memmove(&result_bytes[position], &result_bytes[slice_offset], part_len);

          *(ua_u128*)&result_bytes[(i64)position - 16]               &= (u128)-1 >> position * 8;
          *(ua_u128*)&result_bytes[(i64)position - 16]               |= position == 0 ? 0 : (array.raw_bits & ((u128)1 << position * 8) - 1) << (16 - position) * 8;
          u8 const tail_len                                           = array_len - position;
          *(ua_u128*)&result_bytes[(i64)(part_len + array_len) - 16] &= (u128)-1 >> tail_len * 8;
          *(ua_u128*)&result_bytes[(i64)(part_len + array_len) - 16] |= tail_len == 0 ? 0 : (array.raw_bits >> position * 8) << (16 - tail_len) * 8;
     } else {
          if (part_ref_cnt != 0) set_ref_cnt(part, part_ref_cnt - 1);

          result                       = long_byte_array__new(new_len);
          slice_array__set_len(result, new_len);
          result                       = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          const u8* const part_bytes   = &((const u8*)slice_array__get_items(part))[slice_offset];
          u8*       const result_bytes = slice_array__get_items(result);
          *(ua_u128*)&result_bytes[(i64)position - 16] &= (u128)-1 >> position * 8;
          *(ua_u128*)&result_bytes[(i64)position - 16] |= position == 0 ? 0 : (array.raw_bits & ((u128)1 << position * 8) - 1) << (16 - position) * 8;
          memcpy(&result_bytes[position], part_bytes, part_len);
          u8 const tail_len                                           = array_len - position;
          *(ua_u128*)&result_bytes[(i64)(part_len + array_len) - 16] &= (u128)-1 >> tail_len * 8;
          *(ua_u128*)&result_bytes[(i64)(part_len + array_len) - 16] |= tail_len == 0 ? 0 : (array.raw_bits >> position * 8) << (16 - tail_len) * 8;
     }

     return result;
}

static t_any insert_part_to__long_byte_array__short_byte_array__own(t_thrd_data* const thrd_data, t_any const array, t_any const part, u64 const position, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assert(part.bytes[15] == tid__short_byte_array);

     u32 const slice_len       = slice__get_len(array);
     u32 const slice_offset    = slice__get_offset(array);
     u64 const array_ref_cnt   = get_ref_cnt(array);
     u64 const array_array_len = slice_array__get_len(array);
     u64 const array_len       = slice_len <= slice_max_len ? slice_len : array_array_len;

     assert(slice_len <= slice_max_len || slice_offset == 0);

     if (position > array_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          return error__out_of_bounds(thrd_data, owner);
     }

     u8 const part_len = short_byte_array__get_len(part);
     if (part_len == 0) return array;

     u64 const new_len = array_len + part_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     t_any result;
     if (array_ref_cnt == 1) {
          u64 const cap = slice_array__get_cap(array);
          result        = array;
          slice_array__set_len(result, new_len);
          result        = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (cap < new_len) {
               result.qwords[0] = (u64)realloc((u8*)result.qwords[0], new_cap + 16);
               slice_array__set_cap(result, new_cap);
          }

          u8* const result_bytes = slice_array__get_items(result);
          if (slice_offset != 0) memmove(result_bytes, &result_bytes[slice_offset], position);
          if (slice_offset != part_len) memmove(&result_bytes[position + part_len], &result_bytes[slice_offset + position], array_len - position);
          *(ua_u128*)&result_bytes[(i64)(position + part_len) - 16] &= (u128)-1 >> part_len * 8;
          *(ua_u128*)&result_bytes[(i64)(position + part_len) - 16] |= part.raw_bits << (16 - part_len) * 8;
     } else {
          if (array_ref_cnt != 0) set_ref_cnt(array, array_ref_cnt - 1);

          result                       = long_byte_array__new(new_len);
          slice_array__set_len(result, new_len);
          result                       = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          const u8* const array_bytes  = &((const u8*)slice_array__get_items(array))[slice_offset];
          u8*       const result_bytes = slice_array__get_items(result);
          memcpy(result_bytes, array_bytes, position);
          *(ua_u128*)&result_bytes[(i64)(position + part_len) - 16] &= (u128)-1 >> part_len * 8;
          *(ua_u128*)&result_bytes[(i64)(position + part_len) - 16] |= part.raw_bits << (16 - part_len) * 8;
          memcpy(&result_bytes[position + part_len], &array_bytes[position], array_len - position);
     }

     return result;
}

static t_any insert_part_to__long_byte_array__long_byte_array__own(t_thrd_data* const thrd_data, t_any array, t_any part, u64 const position, const char* const owner) {
     assume(array.bytes[15] == tid__byte_array);
     assume(part.bytes[15] == tid__byte_array);

     u32 const array_slice_offset = slice__get_offset(array);
     u32 const array_slice_len    = slice__get_len(array);
     u64 const array_array_len    = slice_array__get_len(array);
     u64 const array_array_cap    = slice_array__get_cap(array);
     u8*       array_bytes        = slice_array__get_items(array);
     u64 const array_len          = array_slice_len <= slice_max_len ? array_slice_len : array_array_len;
     u64 const array_ref_cnt      = get_ref_cnt(array);

     assert(array_slice_len <= slice_max_len || array_slice_offset == 0);
     assert(array_array_cap >= array_array_len);

     if (position > array_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, array);
          ref_cnt__dec(thrd_data, part);
          return error__out_of_bounds(thrd_data, owner);
     }

     u32 const part_slice_offset = slice__get_offset(part);
     u32 const part_slice_len    = slice__get_len(part);
     u64 const part_array_len    = slice_array__get_len(part);
     u64 const part_array_cap    = slice_array__get_cap(part);
     u8*       part_bytes        = slice_array__get_items(part);
     u64 const part_len          = part_slice_len <= slice_max_len ? part_slice_len : part_array_len;
     u64 const part_ref_cnt      = get_ref_cnt(part);

     assert(part_slice_len <= slice_max_len || part_slice_offset == 0);
     assert(part_array_cap >= part_array_len);

     u64 const new_len = array_len + part_len;
     if (new_len > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

     u64 new_cap = new_len * 3 / 2;
     new_cap     = new_cap <= array_max_len ? new_cap : new_len;

     if (array_bytes == part_bytes) {
          if (array_ref_cnt > 2) set_ref_cnt(array, array_ref_cnt - 2);
     } else {
          if (array_ref_cnt > 1) set_ref_cnt(array, array_ref_cnt - 1);
          if (part_ref_cnt > 1) set_ref_cnt(part, part_ref_cnt - 1);
     }

     int const array_is_mut  = array_ref_cnt == 1;
     int const part_is_mut = part_ref_cnt == 1;

     u8 const scenario =
          array_is_mut                                       |
          part_is_mut * 2                                    |
          (array_is_mut  && array_array_cap  >= new_len) * 4 |
          (part_is_mut && part_array_cap >= new_len) * 8     ;

     switch (scenario) {
     case 1: case 3:
          array.qwords[0] = (u64)realloc((u8*)array.qwords[0], new_cap + 16);
          slice_array__set_cap(array, new_cap);
          array_bytes     = slice_array__get_items(array);
     case 5: case 7: case 15:
          slice_array__set_len(array, new_len);
          array = slice__set_metadata(array, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (array_slice_offset != 0) memmove(array_bytes, &array_bytes[array_slice_offset], position);
          if (array_slice_offset != part_len) memmove(&array_bytes[position + part_len], &array_bytes[array_slice_offset + position], array_len - position);
          memcpy(&array_bytes[position], &part_bytes[part_slice_offset], part_len);

          if (part_is_mut) free((void*)part.qwords[0]);

          return array;
     case 2:
          part.qwords[0] = (u64)realloc((u8*)part.qwords[0], new_cap + 16);
          slice_array__set_cap(part, new_cap);
          part_bytes     = slice_array__get_items(part);
     case 10: case 11:
          slice_array__set_len(part, new_len);
          part = slice__set_metadata(part, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);

          if (part_slice_offset != position) memmove(&part_bytes[position], &part_bytes[part_slice_offset], part_len);
          memcpy(part_bytes, &array_bytes[array_slice_offset], position);
          memcpy(&part_bytes[position + part_len], &array_bytes[array_slice_offset + position], array_len - position);

          if (array_is_mut) free((void*)array.qwords[0]);

          return part;
     case 0: {
          t_any     result       = long_byte_array__new(new_len);
          slice_array__set_len(result, new_len);
          result                 = slice__set_metadata(result, 0, new_len <= slice_max_len ? new_len : slice_max_len + 1);
          u8* const result_bytes = slice_array__get_items(result);
          memcpy(result_bytes, &array_bytes[array_slice_offset], position);
          memcpy(&result_bytes[position], &part_bytes[part_slice_offset], part_len);
          memcpy(&result_bytes[position + part_len], &array_bytes[array_slice_offset + position], array_len - position);

          if (array_bytes == part_bytes) {
               if (array_ref_cnt == 2) free((void*)array.qwords[0]);
          } else {
               if (array_is_mut) free((void*)array.qwords[0]);
               if (part_is_mut) free((void*)part.qwords[0]);
          }

          return result;
     }
     default:
          unreachable;
     }
}
