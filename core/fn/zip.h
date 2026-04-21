#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/byte_array/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"

[[gnu::hot, clang::noinline]]
core t_any McoreFNzip(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.zip";

     t_any const container_1      = args[0];
     t_any const container_2      = args[1];
     t_any const fn               = args[2];
     u8    const container_1_type = container_1.bytes[15];
     u8    const container_2_type = container_2.bytes[15];

     if (container_1_type == tid__error || container_2_type == tid__error || !(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn)) [[clang::unlikely]] {
          if (container_1_type == tid__error) {
               ref_cnt__dec(thrd_data, container_2);
               ref_cnt__dec(thrd_data, fn);
               return container_1;
          }

          if (container_2_type == tid__error) {
               ref_cnt__dec(thrd_data, container_1);
               ref_cnt__dec(thrd_data, fn);
               return container_2;
          }

          if (fn.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, container_1);
               ref_cnt__dec(thrd_data, container_2);
               return fn;
          }

          goto invalid_type_label;
     }

     void (*init_read)(const t_any* const container, t_zip_read_state* const state, t_any* key, t_any* value, u64* const result_len, t_any* const result);
     void (*read)(t_thrd_data* const thrd_data, t_zip_read_state* const state, t_any* const key, t_any* const value, const char* const owner);
     void (*init_search)(const t_any* const container, t_zip_search_state* const state, t_any* value, u64* const result_len, t_any* const result);
     bool (*search)(t_thrd_data* const thrd_data, t_zip_search_state* const state, t_any const key, t_any* const value, const char* const owner);

     t_type_id result_type;
     switch (container_1_type) {
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
          t_any const call_result = obj__try_call__any_result__own(thrd_data, mtd__core_zip, container_1, args, 3, owner, &called);
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
          t_any const result = custom__call__any_result__own(thrd_data, mtd__core_zip, container_1, args, 3, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]]
     default:
          goto invalid_type_label;
     }

     switch (container_2_type) {
     case tid__short_string:
          init_search = short_string__zip_init_search;
          search      = short_string__zip_search;
          if (result_type != tid__array) goto invalid_type_label;
          break;
     case tid__short_byte_array: case tid__byte_array:
          init_search = byte_array__zip_init_search;
          search      = byte_array__zip_search;
          if (result_type != tid__array) goto invalid_type_label;
          break;
     case tid__obj: {
          init_search = obj__zip_init_search;
          search      = obj__zip_search;
          if (result_type == tid__array) goto invalid_type_label;
          break;
     }
     case tid__table:
          init_search = table__zip_init_search;
          search      = table__zip_search;
          if (result_type == tid__array) goto invalid_type_label;
          result_type = tid__table;
          break;
     case tid__set:
          init_search = set__zip_init_search;
          search      = set__zip_search;
          if (result_type == tid__array) goto invalid_type_label;
          result_type = tid__table;
          break;
     case tid__string:
          init_search = long_string__zip_init_search;
          search      = long_string__zip_search;
          if (result_type != tid__array) goto invalid_type_label;
          break;
     case tid__array:
          init_search = array__zip_init_search;
          search      = array__zip_search;
          if (result_type != tid__array) goto invalid_type_label;
          break;

     [[clang::unlikely]]
     default:
          goto invalid_type_label;
     }

     t_zip_read_state   read_state;
     t_zip_search_state search_state;
     t_any              key;
     u64                result_len;
     t_any              fn_args[2];
     t_any              result = null;
     init_read(&container_1, &read_state, &key, &fn_args[0], &result_len, &result);
     u64 need_to_read_len = result_len;
     init_search(&container_2, &search_state, &fn_args[1], &result_len, &result);

     if (result_len == 0) {
          ref_cnt__dec(thrd_data, container_1);
          ref_cnt__dec(thrd_data, container_2);
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

     call_stack__push(thrd_data, owner);

     void (*write)(t_thrd_data* const thrd_data, t_any* const container, t_any* const result_array_items, bool const is_new_array, t_any const key, t_any const value, const char* const owner);
     bool const is_new_array = result_type == tid__array && result.bytes[15] == tid__null;
     t_any*     result_array_items;
     switch (result_type) {
     case tid__array:
          if (is_new_array) result = array__init(thrd_data, result_len, owner);
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

     for (u64 idx = 0; idx < need_to_read_len; idx += 1) {
          read(thrd_data, &read_state, &key, &fn_args[0], owner);
          if (!search(thrd_data, &search_state, key, &fn_args[1], owner)) {
               ref_cnt__dec(thrd_data, key);
               ref_cnt__dec(thrd_data, fn_args[0]);
               continue;
          }

          t_any const result_item = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (result_item.bytes[15] == tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, container_1);
               ref_cnt__dec(thrd_data, container_2);
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, result);

               call_stack__pop(thrd_data);

               return result_item;
          }

          write(thrd_data, &result, result_array_items, is_new_array, key, result_item, owner);
     }

     ref_cnt__dec(thrd_data, container_1);
     ref_cnt__dec(thrd_data, container_2);
     ref_cnt__dec(thrd_data, fn);

     if (result.bytes[15] == tid__array && !is_new_array)
          result = array__slice__own(thrd_data, result, 0, result_len, owner);

     call_stack__pop(thrd_data);

     return result;

     invalid_type_label:
     ref_cnt__dec(thrd_data, container_1);
     ref_cnt__dec(thrd_data, container_2);
     ref_cnt__dec(thrd_data, fn);

     call_stack__push(thrd_data, owner);
     t_any const error = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return error;
}
