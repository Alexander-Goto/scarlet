#pragma once

#include "../../common/const.h"
#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"

static t_any const empty_box = {.structure = {.data = {32}, .type = tid__box}};

[[clang::always_inline]]
static u8 box__get_len(t_any const box) {
     assume(box.bytes[15] == tid__box);

     u8 const len = box.bytes[8] & 0x1f;

     assume(len <= 16);

     return len;
}

[[clang::always_inline]]
static t_any box__set_len(t_any box, u8 const len) {
     assume(box.bytes[15] == tid__box);
     assume(len <= 16);

     box.bytes[8] = box.bytes[8] & 0xe0 | len;
     return box;
}

[[clang::always_inline]]
static bool box__is_global(t_any const box) {
     assume(box.bytes[15] == tid__box);

     return (box.bytes[8] & 0x20) == 0x20;
}

[[clang::always_inline]]
static t_any box__set_global_flag(t_any box, bool const flag) {
     assume(box.bytes[15] == tid__box);

     box.bytes[8] = (box.bytes[8] | 0x20) ^ (flag ? 0 : 0x20);
     return box;
}

[[clang::always_inline]]
static u8 box__get_idx(t_any const box) {
     assume(box.bytes[15] == tid__box);

     u8 const idx = *(u16*)&box.bytes[8] >> 6 & 0x3f;

     assume(idx < boxes_max_qty);

     return idx;
}

[[clang::always_inline]]
static t_any box__set_idx(t_any box, u8 const idx) {
     assume(box.bytes[15] == tid__box);
     assume(idx < boxes_max_qty);

     u16* const metadata_2_bytes = (u16*)&box.bytes[8];
     *metadata_2_bytes           = *metadata_2_bytes & 0xf03f | (u16)idx << 6;
     return box;
}

[[clang::always_inline]]
static u8 box__is_shared(t_any const box) {
     assume(box.bytes[15] == tid__box);
     return (box.bytes[9] & 16) == 16;
}

[[clang::always_inline]]
static t_any box__set_shared_flag(t_any box, bool const flag) {
     assume(box.bytes[15] == tid__box);

     box.bytes[9] = (box.bytes[9] | 16) ^ (flag ? 0 : 16);
     return box;
}

[[clang::always_inline]]
static t_any box__set_metadata(t_any box, u8 const len, bool const global_flag, u8 const idx, bool const shared_flag) {
     assume(box.bytes[15] == tid__box);
     assume(len <= 16);
     assume(idx < boxes_max_qty);

     *(u32*)&box.bytes[8] = (u32)len | (global_flag ? (u32)1 : 0) << 5 | (u32)idx << 6 | (shared_flag ? (u32)1 : 0) << 12;
     return box;
}

[[clang::always_inline]]
static t_any* box__get_items(t_any const box) {
     assume(box.bytes[15] == tid__box);

     t_any* const items = (t_any*)box.qwords[0];

     assume_aligned(items, 16);

     return items;
}

core_basic inline t_any box__new(t_thrd_data* const thrd_data, u8 const len, const char* const owner) {
     if (len == 0) return empty_box;
     if (thrd_data->free_boxes == 0) [[clang::unlikely]] fail_with_call_stack(thrd_data, "Number of boxes exceeded.", owner);

     u8 const idx           = __builtin_ctzl(thrd_data->free_boxes);
     thrd_data->free_boxes ^= (u32)1 << idx;
     t_any    box           = {.structure = {.value = (u64)thrd_data->boxes[idx].items, .type = tid__box}};
     box                    = box__set_metadata(box, len, false, idx, false);
     return box;
}

static t_any box__unshare__own(t_thrd_data* const thrd_data, t_any const box, const char* const owner) {
     assume(box.bytes[15] == tid__box);
     assert(box__is_global(box) || box__is_shared(box));

     if (box__is_global(box)) return box;

     u8     const len          = box__get_len(box);
     t_any  const result       = box__new(thrd_data, len, owner);
     t_any* const box_items    = box__get_items(box);
     t_any* const result_items = box__get_items(result);
     for (u8 idx = 0; idx < len; idx += 1) {
          t_any item = box_items[idx];
          if (item.bytes[15] == tid__box) item = box__unshare__own(thrd_data, item, owner);

          result_items[idx] = item;
     }

     free(box_items);
     return result;
}
