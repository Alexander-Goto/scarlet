#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/error/fn.h"

core t_any McoreFNshuffle_box(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.shuffle_box";

     t_any       box   = args[0];
     t_any const idxes = args[1];
     if (box.bytes[15] != tid__box || idxes.bytes[15] != tid__box || box__get_len(box) != box__get_len(idxes)) [[clang::unlikely]] {
          if (box.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, idxes);
               return box;
          }
          ref_cnt__dec(thrd_data, box);

          if (idxes.bytes[15] == tid__error) return idxes;
          ref_cnt__dec(thrd_data, idxes);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (box.bytes[15] != tid__box || idxes.bytes[15] != tid__box)
               result = error__invalid_type(thrd_data, owner);
          else result = error__invalid_items_num(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     u8 const box_len = box__get_len(box);
     if (box_len == 0) return box;

     u16                already_in_new = 0;
     t_any*             box_items      = box__get_items(box);
     const t_any* const idxs_items     = box__get_items(idxes);

     t_box new_box;
     for (u8 item_idx = 0; item_idx < box_len; item_idx += 1) {
          t_any const idx_from_idxs = idxs_items[item_idx];
          if (
               idx_from_idxs.bytes[15] != tid__int ||
               idx_from_idxs.qwords[0] >= box_len  ||
               (already_in_new & 1 << idx_from_idxs.qwords[0]) != 0
          ) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, box);
               ref_cnt__dec(thrd_data, idxes);

               t_any result;
               call_stack__push(thrd_data, owner);
               if (idx_from_idxs.bytes[15] != tid__int)
                    result = error__invalid_type(thrd_data, owner);
               else if (idx_from_idxs.qwords[0] >= box_len)
                    result = error__out_of_bounds(thrd_data, owner);
               else result = error__shuffle_dup(thrd_data, owner);
               call_stack__pop(thrd_data);

               return result;
          }

          already_in_new         |= 1 << idx_from_idxs.qwords[0];
          new_box.items[item_idx] = box_items[idx_from_idxs.qwords[0]];
     }

     ref_cnt__dec(thrd_data, idxes);
     if (box__is_global(box)) {
          call_stack__push(thrd_data, owner);
          box = box__new(thrd_data, box_len, owner);
          call_stack__pop(thrd_data);

          box_items = box__get_items(box);
     }

     assume_aligned(box_items, ALIGN_FOR_CACHE_LINE);

     memcpy_inline(box_items, new_box.items, 256);
     return box;
}