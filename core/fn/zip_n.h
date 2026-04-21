#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/box/basic.h"
#include "../struct/byte_array/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"

[[gnu::hot, clang::noinline]]
core t_any McoreFNzip_n(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.zip_n";

     t_any const box = args[0];
     t_any const fn  = args[1];
     if (box.bytes[15] != tid__box || !(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn) || box__get_len(box) < 2) [[clang::unlikely]] {
          if (box.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, fn);
               return box;
          }
          ref_cnt__dec(thrd_data, box);

          if (fn.bytes[15] == tid__error) return fn;
          ref_cnt__dec(thrd_data, fn);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (box.bytes[15] != tid__box || !(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn))
               result = error__invalid_type(thrd_data, owner);
          else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     void (*init_read)(const t_any* const container, t_zip_read_state* const state, t_any* key, t_any* value, u64* const result_len, t_any* const result);
     void (*read)(t_thrd_data* const thrd_data, t_zip_read_state* const state, t_any* const key, t_any* const value, const char* const owner);
     void (*init_search[15])(const t_any* const container, t_zip_search_state* const state, t_any* value, u64* const result_len, t_any* const result);
     bool (*search[15])(t_thrd_data* const thrd_data, t_zip_search_state* const state, t_any const key, t_any* const value, const char* const owner);

     u8     const containers_len    = box__get_len(box);
     u8     const tail_len          = containers_len - 1;
     t_any* const containers_in_box = box__get_items(box);
     t_any* const tail_in_box       = &containers_in_box[1];
     t_type_id    result_type;
     switch (containers_in_box->bytes[15]) {
     case tid__short_string:
          init_read   = short_string__zip_init_read;
          read        = short_string__zip_read;
          result_type = tid__array;
          break;
     case tid__short_byte_array: case tid__byte_array:
          init_read   = byte_array__zip_init_read;
          read        = byte_array__zip_read;
          result_type = tid__array;

          break;
     case tid__obj: {
          bool called;

          call_stack__push(thrd_data, owner);
          t_any const call_result = obj__try_call__any_result__own(thrd_data, mtd__core_zip_n, containers_in_box[0], args, 2, owner, &called);
          call_stack__pop(thrd_data);

          if (called) return call_result;
          init_read   = obj__zip_init_read;
          read        = obj__zip_read;
          result_type = tid__obj;
          break;
     }
     case tid__table:
          init_read   = table__zip_init_read;
          read        = table__zip_read;
          result_type = tid__table;
          break;
     case tid__set:
          init_read   = set__zip_init_read;
          read        = set__zip_read;
          result_type = tid__table;
          break;
     case tid__string:
          init_read   = long_string__zip_init_read;
          read        = long_string__zip_read;
          result_type = tid__array;
          break;
     case tid__array:
          init_read   = array__zip_init_read;
          read        = array__zip_read;
          result_type = tid__array;
          break;
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__any_result__own(thrd_data, mtd__core_zip_n, containers_in_box[0], args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]]
     default:
          goto invalid_type_label;
     }

     for (u8 idx = 0; idx < tail_len; idx += 1)
          switch (tail_in_box[idx].bytes[15]) {
          case tid__short_string:
               init_search[idx] = short_string__zip_init_search;
               search[idx]      = short_string__zip_search;
               if (result_type != tid__array) goto invalid_type_label;
               break;
          case tid__short_byte_array: case tid__byte_array:
               init_search[idx] = byte_array__zip_init_search;
               search[idx]      = byte_array__zip_search;
               if (result_type != tid__array) goto invalid_type_label;
               break;
          case tid__obj: {
               init_search[idx] = obj__zip_init_search;
               search[idx]      = obj__zip_search;
               if (result_type == tid__array) goto invalid_type_label;
               break;
          }
          case tid__table:
               init_search[idx] = table__zip_init_search;
               search[idx]      = table__zip_search;
               if (result_type == tid__array) goto invalid_type_label;
               result_type = tid__table;
               break;
          case tid__set:
               init_search[idx] = set__zip_init_search;
               search[idx]      = set__zip_search;
               if (result_type == tid__array) goto invalid_type_label;
               result_type = tid__table;
               break;
          case tid__string:
               init_search[idx] = long_string__zip_init_search;
               search[idx]      = long_string__zip_search;
               if (result_type != tid__array) goto invalid_type_label;
               break;
          case tid__array:
               init_search[idx] = array__zip_init_search;
               search[idx]      = array__zip_search;
               if (result_type != tid__array) goto invalid_type_label;
               break;

          [[clang::unlikely]]
          default:
               goto invalid_type_label;
          }

     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     t_any  containers[16];
     t_any* tail = &containers[1];
     memcpy_le_256(containers, containers_in_box, 16 * containers_len);
     if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

     t_zip_read_state   read_state;
     t_zip_search_state search_states[15];
     t_any              key;
     u64                result_len;
     t_any              fn_args[16];
     t_any              result = null;
     init_read(&containers[0], &read_state, &key, &fn_args[0], &result_len, &result);
     u64 need_to_read_len = result_len;
     for (u8 idx = 0; idx < tail_len; idx += 1)
          init_search[idx](&tail[idx], &search_states[idx], &fn_args[idx + 1], &result_len, &result);

     if (result_len == 0) {
          for (u8 idx = 0; idx < containers_len; ref_cnt__dec(thrd_data, containers[idx]), idx += 1);
          ref_cnt__dec(thrd_data, fn);
          ref_cnt__dec(thrd_data, result);

          switch (result_type) {
          case tid__array:
               return empty_array;
          case tid__table:
               return empty_table;
          case tid__obj:
               return empty_obj;
          default:
               unreachable;
          }
     }

     if (result_type == tid__array) need_to_read_len = result_len;

     call_stack__push(thrd_data, owner);

     void (*write)(t_thrd_data* const thrd_data, t_any* const container, t_any* const result_array_items, bool const is_new_array, t_any const key, t_any const value, const char* const owner);
     bool const is_new_array = result_type == tid__array && result.bytes[15] == tid__null;
     t_any*     result_array_items;
     switch (result_type) {
     case tid__array:
          if (result.bytes[15] == tid__null) result = array__init(thrd_data, result_len, owner);
          result_array_items = &((t_any*)slice_array__get_items(result))[slice__get_offset(result)];
          write              = array__zip_write;
          need_to_read_len   = result_len;
          break;
     case tid__table:
          result = table__init(thrd_data, result_len, owner);
          write  = table__zip_write;
          break;
     case tid__obj:
          result = obj__init(thrd_data, result_len, owner);
          write  = obj__zip_write;
          break;
     default:
          unreachable;
     }

     for (u64 items_idx = 0; items_idx < need_to_read_len; items_idx += 1) {
          read(thrd_data, &read_state, &key, &fn_args[0], owner);
          for (u8 tail_idx = 0; tail_idx < tail_len; tail_idx += 1)
               if (!search[tail_idx](thrd_data, &search_states[tail_idx], key, &fn_args[tail_idx + 1], owner)) {
                    ref_cnt__dec(thrd_data, key);
                    for (u8 free_idx = 0; free_idx <= tail_idx; ref_cnt__dec(thrd_data, fn_args[free_idx]), free_idx += 1);
                    goto double_continue;
               }

          t_any const result_item = common_fn__call__half_own(thrd_data, fn, fn_args, containers_len, owner);
          if (result_item.bytes[15] == tid__error) [[clang::unlikely]] {
               for (u8 free_idx = 0; free_idx < containers_len; ref_cnt__dec(thrd_data, containers[free_idx]), free_idx += 1);
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, result);

               call_stack__pop(thrd_data);

               return result_item;
          }

          write(thrd_data, &result, result_array_items, is_new_array, key, result_item, owner);
          double_continue:
     }

     for (u8 idx = 0; idx < containers_len; ref_cnt__dec(thrd_data, containers[idx]), idx += 1);
     ref_cnt__dec(thrd_data, fn);

     if (result.bytes[15] == tid__array && !is_new_array)
          result = array__slice__own(thrd_data, result, 0, result_len, owner);

     call_stack__pop(thrd_data);

     return result;

     invalid_type_label:
     ref_cnt__dec(thrd_data, box);
     ref_cnt__dec(thrd_data, fn);

     call_stack__push(thrd_data, owner);
     t_any const error = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return error;
}
