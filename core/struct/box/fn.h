#pragma once

#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../array/basic.h"
#include "../bool/basic.h"
#include "../error/basic.h"
#include "../null/basic.h"
#include "basic.h"

static inline void box__unpack__own(t_thrd_data* const thrd_data, t_any box, u8 const unpacking_items_len, u8 const max_box_item_idx, t_any* const unpacking_items, const char* const owner) {
     if (box.bytes[15] != tid__box) {
          if (!(box.bytes[15] == tid__error || box.bytes[15] == tid__null)) {
               ref_cnt__dec(thrd_data, box);
               box = error__invalid_type(thrd_data, owner);
          }

          ref_cnt__add(thrd_data, box, unpacking_items_len - 1, owner);
          for (u8 idx = 0; idx < unpacking_items_len; unpacking_items[idx] = box, idx += 1);
          return;
     }

     u8 const box_len = box__get_len(box);
     if (box_len < max_box_item_idx) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, box);
          t_any const error = error__unpacking_not_enough_items(thrd_data, owner);
          set_ref_cnt(error, unpacking_items_len);

          for (u8 idx = 0; idx < unpacking_items_len; unpacking_items[idx] = error, idx += 1);
          return;
     }

     u16                used_items = 0;
     const t_any* const box_items  = box__get_items(box);
     for (u8 unpacking_item_idx = 0; unpacking_item_idx < unpacking_items_len; unpacking_item_idx += 1) {
          u8 const box_item_idx               = unpacking_items[unpacking_item_idx].bytes[0];
          unpacking_items[unpacking_item_idx] = box_items[box_item_idx];
          used_items |= 1 << box_item_idx;
     }

     if (!box__is_global(box)) {
          for (u16 unused_items = ~used_items & ((u16)1 << box_len) - 1; unused_items != 0;) {
               u16 const idx = __builtin_ctzs(unused_items);
               ref_cnt__dec(thrd_data, box_items[idx]);
               unused_items ^= (u16)1 << idx;
          }

          thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);
     }
}

[[gnu::hot, clang::noinline]]
static t_any equal(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner);

[[clang::noinline]]
static t_any box__equal__own(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner) {
     assume(left.bytes[15] == tid__box);
     assume(right.bytes[15] == tid__box);

     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     t_any left_items_buffer[16];

     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     t_any right_items_buffer[16];

     u8 const left_len  = box__get_len(left);
     u8 const right_len = box__get_len(right);

     if (left_len != right_len) {
          ref_cnt__dec(thrd_data, left);
          ref_cnt__dec(thrd_data, right);
          return bool__false;
     }

     if (left_len == 0) return bool__true;

     const t_any* left_items  = box__get_items(left);
     const t_any* right_items = box__get_items(right);
     if (!box__is_global(left)) {
          memcpy_le_256(left_items_buffer, left_items, left_len * 16);
          left_items             = left_items_buffer;
          thrd_data->free_boxes ^= (u32)1 << box__get_idx(left);
     }
     if (!box__is_global(right)) {
          memcpy_le_256(right_items_buffer, right_items, left_len * 16);
          right_items            = right_items_buffer;
          thrd_data->free_boxes ^= (u32)1 << box__get_idx(right);
     }

     t_any result;
     for (u8 idx = 0; idx < left_len; idx += 1) {
          t_any const left_item  = left_items[idx];
          t_any const right_item = right_items[idx];

          if (left_item.bytes[15] != right_item.bytes[15]) {
               result = bool__false;
               ref_cnt__dec(thrd_data, left_item);
               ref_cnt__dec(thrd_data, right_item);
          } else if (left_item.bytes[15] != tid__box) {
               result = equal(thrd_data, left_item, right_item, owner);
               ref_cnt__dec(thrd_data, left_item);
               ref_cnt__dec(thrd_data, right_item);
          } else result = box__equal__own(thrd_data, left_item, right_item, owner);

          assert(result.bytes[15] == tid__bool);

          if (result.bytes[0] == 0) {
               for (u8 free_idx = idx + 1; free_idx < left_len; ref_cnt__dec(thrd_data, left_items[free_idx]), ref_cnt__dec(thrd_data, right_items[free_idx]), free_idx += 1);
               break;
          }
     }

     return result;
}

static t_any box__get_item__own(t_thrd_data* const thrd_data, t_any const box, t_any const idx, const char* const owner) {
     assume(box.bytes[15] == tid__box);
     assert(idx.bytes[15] == tid__int);

     if (idx.qwords[0] >= box__get_len(box)) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, box);
          return error__out_of_bounds(thrd_data, owner);
     }

     t_any* const item_ptr = &box__get_items(box)[idx.qwords[0]];
     t_any  const item     = *item_ptr;
     if (!box__is_global(box)) {
          item_ptr->bytes[15] = tid__null;
          ref_cnt__dec(thrd_data, box);
     }

     return item;
}

static t_any box__slice__own(t_thrd_data* const thrd_data, t_any const box, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(idx_from <= idx_to);
     assume(box.bytes[15] == tid__box);

     u8 const len = box__get_len(box);
     if (idx_to > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, box);
          return error__out_of_bounds(thrd_data, owner);
     }

     u8 const new_len = idx_to - idx_from;
     if (new_len == 0) {
          ref_cnt__dec(thrd_data, box);
          return empty_box;
     }

     if (new_len == len) return box;

     t_any* const items = box__get_items(box);
     if (box__is_global(box)) {
          t_any new_box = box__new(thrd_data, new_len, owner);
          memcpy_le_256(box__get_items(new_box), &items[idx_from], new_len * 16);
          return new_box;
     }

     for (u8 idx = 0; idx < idx_from; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     for (u8 idx = idx_to; idx != len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
     memmove_le_256(items, &items[idx_from], new_len * 16);
     return box__set_len(box, new_len);
}

[[gnu::hot, clang::noinline]]
static t_any cmp(t_thrd_data* const thrd_data, t_any const arg_1, t_any const arg_2, const char* const owner);

[[clang::noinline]]
static t_any box__cmp__own(t_thrd_data* const thrd_data, t_any const arg_1, t_any const arg_2, const char* const owner) {
     assert(arg_1.bytes[15] == tid__box);
     assert(arg_2.bytes[15] == tid__box);

     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     t_any items_1_buffer[16];

     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     t_any items_2_buffer[16];

     u8 const len_1     = box__get_len(arg_1);
     u8 const len_2     = box__get_len(arg_2);
     u8 const short_len = len_1 < len_2 ? len_1 : len_2;

     const t_any* items_1 = box__get_items(arg_1);
     const t_any* items_2 = box__get_items(arg_2);
     if (!box__is_global(arg_1)) {
          memcpy_le_256(items_1_buffer, items_1, len_1 * 16);
          items_1                = items_1_buffer;
          thrd_data->free_boxes ^= (u32)1 << box__get_idx(arg_1);
     }
     if (!box__is_global(arg_2)) {
          memcpy_le_256(items_2_buffer, items_2, len_2 * 16);
          items_2                = items_2_buffer;
          thrd_data->free_boxes ^= (u32)1 << box__get_idx(arg_2);
     }

     t_any result = tkn__equal;
     for (u8 idx = 0; idx < short_len; idx += 1) {
          t_any const item_1 = items_1[idx];
          t_any const item_2 = items_2[idx];

          if (item_1.bytes[15] != item_2.bytes[15]) {
               result = tkn__not_equal;
               ref_cnt__dec(thrd_data, item_1);
               ref_cnt__dec(thrd_data, item_2);
          } else if (item_1.bytes[15] != tid__box) {
               result = cmp(thrd_data, item_1, item_2, owner);
               ref_cnt__dec(thrd_data, item_1);
               ref_cnt__dec(thrd_data, item_2);
          } else result = box__cmp__own(thrd_data, item_1, item_2, owner);

          assert(result.bytes[15] == tid__token);

          if (result.bytes[0] != equal_id) {
               for (u8 free_idx = idx + 1; free_idx < len_1; ref_cnt__dec(thrd_data, items_1[free_idx]), free_idx += 1);
               for (u8 free_idx = idx + 1; free_idx < len_2; ref_cnt__dec(thrd_data, items_2[free_idx]), free_idx += 1);
               break;
          }
     }

     if (len_1 != len_2 && result.bytes[0] == equal_id) result = len_1 < len_2 ? tkn__less : tkn__great;

     return result;
}

static t_any box__delete__own(t_thrd_data* const thrd_data, t_any box, u64 const idx_from, u64 const idx_to, const char* const owner) {
     assume(idx_from <= idx_to);
     assert(box.bytes[15] == tid__box);

     u64 const len = box__get_len(box);
     if (idx_to > len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, box);
          return error__out_of_bounds(thrd_data, owner);
     }

     u8 const new_len = len - (idx_to - idx_from);
     if (new_len == 0) {
          ref_cnt__dec(thrd_data, box);
          return empty_box;
     }

     if (new_len == len) return box;

     t_any* const items = box__get_items(box);
     if (box__is_global(box)) {
          t_any        new_box   = box__new(thrd_data, new_len, owner);
          t_any* const new_items = box__get_items(new_box);
          memcpy_le_256(new_items, items, idx_from * 16);
          memcpy_le_256(&new_items[idx_from], &items[idx_to], (len - idx_to) * 16);
          return new_box;
     }

     box = box__set_len(box, new_len);
     for (u64 box_idx = idx_from; box_idx < idx_to; ref_cnt__dec(thrd_data, items[box_idx]), box_idx += 1);
     memmove_le_256(&items[idx_from], &items[idx_to], (len - idx_to) * 16);
     return box;
}

[[clang::noinline]]
static t_any common_print(t_thrd_data* const thrd_data, t_any const arg, const char* const owner);

static t_any box__print(t_thrd_data* const thrd_data, t_any box, const char* const owner) {
     assert(box.bytes[15] == tid__box);

     u8 len = box__get_len(box);

     assume(len <= 16);

     if (len == 0) {
          if (fwrite("[box]", 1, 5, stdout) != 5) [[clang::unlikely]] goto cant_print_label;
          return null;
     }

     if (fwrite("[box ", 1, 5, stdout) != 5) [[clang::unlikely]] goto cant_print_label;

     const t_any* const items = box__get_items(box);
     for (u8 idx = 0; idx < len; idx += 1) {
          t_any const print_result = common_print(thrd_data, items[idx], owner);
          if (print_result.bytes[15] == tid__error) [[clang::unlikely]] return print_result;

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

static t_any box__to_array__own(t_thrd_data* const thrd_data, t_any const box, const char* const owner) {
     assume(box.bytes[15] == tid__box);

     u8 const len = box__get_len(box);
     if (len == 0) return empty_array;

     t_any* const box_items   = box__get_items(box);
     t_any        array       = array__init(thrd_data, len, owner);
     array                    = slice__set_metadata(array, 0, len);
     slice_array__set_len(array, len);
     t_any* const array_items = slice_array__get_items(array);
     for (u8 idx = 0; idx < len; idx += 1) {
          t_any const item = box_items[idx];
          if (!type_is_common(item.bytes[15])) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, box);
               free((void*)array.qwords[0]);
               return error__invalid_type(thrd_data, owner);
          }
          array_items[idx] = item;
     }

     if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);
     return array;
}