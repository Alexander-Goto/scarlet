#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/byte_array/basic.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"

core t_any McoreFNmove(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.move";

     t_any       container    = args[0];
     t_any const from_arg     = args[1];
     t_any const to_arg       = args[2];
     t_any const part_len_arg = args[3];

     if (container.bytes[15] == tid__error || from_arg.bytes[15] != tid__int || to_arg.bytes[15] != tid__int || part_len_arg.bytes[15] != tid__int) [[clang::unlikely]] {
          if (container.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, from_arg);
               ref_cnt__dec(thrd_data, to_arg);
               ref_cnt__dec(thrd_data, part_len_arg);
               return container;
          }

          if (from_arg.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, to_arg);
               ref_cnt__dec(thrd_data, part_len_arg);
               return from_arg;
          }

          if (to_arg.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, from_arg);
               ref_cnt__dec(thrd_data, part_len_arg);
               return to_arg;
          }

          if (part_len_arg.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container);
               ref_cnt__dec(thrd_data, from_arg);
               ref_cnt__dec(thrd_data, to_arg);
               return part_len_arg;
          }

          goto invalid_type_label;
     }

     bool in_place;
     u8*  container_mem;
     u64  from;
     u64  to;
     u64  part_len;
     u64  container_len;

     t_any new_container;
     u8*   new_container_mem;

     switch (container.bytes[15]) {
     case tid__short_string: {
          u8 const string_len = short_string__get_len(container);
          if (
               from_arg.qwords[0]                          > string_len ||
               to_arg.qwords[0]                            > string_len ||
               part_len_arg.qwords[0]                      > string_len ||
               from_arg.qwords[0] + part_len_arg.qwords[0] > string_len ||
               to_arg.qwords[0] + part_len_arg.qwords[0]   > string_len
          ) [[clang::unlikely]] goto out_of_bounds_label;

          if (part_len_arg.bytes[0] == 0 || from_arg.bytes[0] == to_arg.bytes[0]) return container;

          bool const from_before_to = from_arg.bytes[0] < to_arg.bytes[0];
          from                      = short_string__offset_of_idx(container, from_arg.qwords[0], true);
          part_len                  = short_string__offset_of_idx(container, from_arg.qwords[0] + part_len_arg.qwords[0], true) - from;
          to                        = short_string__offset_of_idx(container, to_arg.qwords[0] + part_len_arg.qwords[0] * from_before_to, true) - part_len * from_before_to;

          in_place      = true;
          container_mem = container.bytes;
          container_len = short_string__get_size(container);
          break;
     }
     case tid__short_byte_array:
          container_len = short_byte_array__get_len(container);
          if (
               from_arg.qwords[0]                          > container_len ||
               to_arg.qwords[0]                            > container_len ||
               part_len_arg.qwords[0]                      > container_len ||
               from_arg.qwords[0] + part_len_arg.qwords[0] > container_len ||
               to_arg.qwords[0] + part_len_arg.qwords[0]   > container_len
          ) [[clang::unlikely]] goto out_of_bounds_label;

          if (part_len_arg.bytes[0] == 0 || from_arg.bytes[0] == to_arg.bytes[0]) return container;

          in_place      = true;
          container_mem = container.bytes;
          from          = from_arg.qwords[0];
          to            = to_arg.qwords[0];
          part_len      = part_len_arg.qwords[0];
          break;
     case tid__byte_array: {
          u32 const slice_offset = slice__get_offset(container);
          u32 const slice_len    = slice__get_len(container);
          container_len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(container);
          if (
               from_arg.qwords[0]                          > container_len ||
               to_arg.qwords[0]                            > container_len ||
               part_len_arg.qwords[0]                      > container_len ||
               from_arg.qwords[0] + part_len_arg.qwords[0] > container_len ||
               to_arg.qwords[0] + part_len_arg.qwords[0]   > container_len
          ) [[clang::unlikely]] goto out_of_bounds_label;

          if (part_len_arg.qwords[0] == 0 || from_arg.qwords[0] == to_arg.qwords[0]) return container;

          in_place      = get_ref_cnt(container) == 1;
          container_mem = &((u8*)slice_array__get_items(container))[slice_offset];
          from          = from_arg.qwords[0];
          to            = to_arg.qwords[0];
          part_len      = part_len_arg.qwords[0];

          if (!in_place) {
               new_container     = long_byte_array__new(container_len);
               new_container     = slice__set_metadata(new_container, 0, slice_len);
               slice_array__set_len(new_container, container_len);
               new_container_mem = slice_array__get_items(new_container);
          }
          break;
     }
     case tid__string: {
          u32 const slice_offset = slice__get_offset(container);
          u32 const slice_len    = slice__get_len(container);
          u64 const string_len   = slice_len <= slice_max_len ? slice_len : slice_array__get_len(container);
          if (
               from_arg.qwords[0]                          > string_len ||
               to_arg.qwords[0]                            > string_len ||
               part_len_arg.qwords[0]                      > string_len ||
               from_arg.qwords[0] + part_len_arg.qwords[0] > string_len ||
               to_arg.qwords[0] + part_len_arg.qwords[0]   > string_len
          ) [[clang::unlikely]] goto out_of_bounds_label;

          if (part_len_arg.qwords[0] == 0 || from_arg.qwords[0] == to_arg.qwords[0]) return container;

          in_place      = get_ref_cnt(container) == 1;
          container_mem = &((u8*)slice_array__get_items(container))[slice_offset * 3];
          from          = from_arg.qwords[0] * 3;
          to            = to_arg.qwords[0] * 3;
          part_len      = part_len_arg.qwords[0] * 3;
          container_len = string_len * 3;

          if (!in_place) {
               new_container     = long_string__new(string_len);
               new_container     = slice__set_metadata(new_container, 0, slice_len);
               slice_array__set_len(new_container, string_len);
               new_container_mem = slice_array__get_items(new_container);
          }
          break;
     }
     case tid__array: {
          u32 const slice_offset = slice__get_offset(container);
          u32 const slice_len    = slice__get_len(container);
          u64 const array_len    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(container);
          if (
               from_arg.qwords[0]                          > array_len ||
               to_arg.qwords[0]                            > array_len ||
               part_len_arg.qwords[0]                      > array_len ||
               from_arg.qwords[0] + part_len_arg.qwords[0] > array_len ||
               to_arg.qwords[0] + part_len_arg.qwords[0]   > array_len
          ) [[clang::unlikely]] goto out_of_bounds_label;

          if (part_len_arg.qwords[0] == 0 || from_arg.qwords[0] == to_arg.qwords[0]) return container;

          t_any* const items   = &((t_any*)slice_array__get_items(container))[slice_offset];
          u64    const ref_cnt = get_ref_cnt(container);
          if (ref_cnt == 1) {
               in_place      = true;
               container_mem = (u8*)items;
               from          = from_arg.qwords[0] * 16;
               to            = to_arg.qwords[0] * 16;
               part_len      = part_len_arg.qwords[0] * 16;
               container_len = array_len * 16;
               break;
          }
          if (ref_cnt != 0) set_ref_cnt(container, ref_cnt - 1);

          call_stack__push(thrd_data, owner);

          new_container     = array__init(thrd_data, array_len, owner);
          new_container     = slice__set_metadata(new_container, 0, slice_len);
          slice_array__set_len(new_container, array_len);
          new_container_mem = slice_array__get_items(new_container);

          from             = from_arg.qwords[0];
          to               = to_arg.qwords[0];
          part_len         = part_len_arg.qwords[0];

          u64 const overlap_size = part_len - (from < to ? to - from : from - to);
          if ((i64)overlap_size > 0) {
               u64 const old_to = to;
               to               = from + (from > old_to) * overlap_size;
               from             = old_to + !(from > old_to) * overlap_size;
               part_len        -= overlap_size;
          }

          bool const from_before_to    = from < to;
          u64  const first_changed_idx = from_before_to ? from : to;
          for (u64 idx = 0; idx < first_changed_idx; idx += 1) {
               t_any const item                 = items[idx];
               ((t_any*)new_container_mem)[idx] = item;
               ref_cnt__inc(thrd_data, item, owner);
          }

          u64 new_idx = first_changed_idx;
          u64 idx     = from + from_before_to * part_len;
          u64 edge    = (from_before_to ? to : from) + part_len;
          for (; idx < edge; idx += 1, new_idx += 1) {
               t_any const item                     = items[idx];
               ((t_any*)new_container_mem)[new_idx] = item;
               ref_cnt__inc(thrd_data, item, owner);
          }

          idx  = from_before_to ? from : to;
          edge = from + from_before_to * part_len;
          for (; idx < edge; idx += 1, new_idx += 1) {
               t_any const item                     = items[idx];
               ((t_any*)new_container_mem)[new_idx] = item;
               ref_cnt__inc(thrd_data, item, owner);
          }

          for (u64 idx = (from_before_to ? to : from) + part_len; idx < array_len; idx += 1) {
               t_any const item                 = items[idx];
               ((t_any*)new_container_mem)[idx] = item;
               ref_cnt__inc(thrd_data, item, owner);
          }

          call_stack__pop(thrd_data);

          return new_container;
     }
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_move, tid__obj, container, args, 4, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_move, tid__custom, container, args, 4, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     if (in_place) {
          scar__move(container_mem, from, to, part_len);
          return container;
     }

     u64 const ref_cnt = get_ref_cnt(container);
     if (ref_cnt != 0) set_ref_cnt(container, ref_cnt - 1);

     u64 const overlap_size = part_len - (from < to ? to - from : from - to);
     if ((i64)overlap_size > 0) {
          u64 const old_to = to;
          to               = from + (from > old_to) * overlap_size;
          from             = old_to + !(from > old_to) * overlap_size;
          part_len        -= overlap_size;
     }

     bool const from_before_to    = from < to;
     u64  const first_changed_idx = from_before_to ? from : to;
     memcpy(new_container_mem, container_mem, first_changed_idx);

     u64 idx  = from + from_before_to * part_len;
     u64 edge = (from_before_to ? to : from) + part_len;
     memcpy(&new_container_mem[first_changed_idx], &container_mem[idx], edge - idx);

     u64 new_idx = first_changed_idx + edge - idx;
     idx         = from_before_to ? from : to;
     edge        = from + from_before_to * part_len;
     memcpy(&new_container_mem[new_idx], &container_mem[idx], edge - idx);

     new_idx += edge - idx;
     idx      = (from_before_to ? to : from) + part_len;
     memcpy(&new_container_mem[new_idx], &container_mem[idx], container_len - idx);
     return new_container;

     invalid_type_label: {
          ref_cnt__dec(thrd_data, container);
          ref_cnt__dec(thrd_data, from_arg);
          ref_cnt__dec(thrd_data, to_arg);
          ref_cnt__dec(thrd_data, part_len_arg);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     out_of_bounds_label:
     ref_cnt__dec(thrd_data, container);

     call_stack__push(thrd_data, owner);
     t_any const result = error__out_of_bounds(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}