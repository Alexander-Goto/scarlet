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
#include "../struct/string/fn.h"

[[gnu::hot, clang::noinline]]
core t_any McoreFNjoin(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.join";

     t_any     const parts          = args[0];
     t_any     const separator      = args[1];
     t_type_id const separator_type = separator.bytes[15];
     if (parts.bytes[15] == tid__error || separator_type == tid__error) [[clang::unlikely]] {
          if (parts.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, separator);
               return parts;
          }

          ref_cnt__dec(thrd_data, parts);
          return separator;
     }

     u64         separator_len;
     const void* separator_items;
     u8          separator_short_string_size = 16;

     switch (separator_type) {
     case tid__short_string:
          separator_len               = short_string__get_len(separator);
          separator_items             = separator.bytes;
          separator_short_string_size = short_string__get_size(separator);
          break;
     case tid__short_byte_array:
          separator_len   = short_byte_array__get_len(separator);
          separator_items = separator.bytes;
          break;
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_join, tid__obj, separator, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__byte_array: case tid__string: case tid__array: {
          u32 const slice_offset = slice__get_offset(separator);
          u32 const slice_len    = slice__get_len(separator);
          u64 const item_size    = separator_type == tid__string ? 3 : (separator_type == tid__array ? 16 : 1);
          separator_len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(separator);
          separator_items        = &((const u8*)slice_array__get_items(separator))[slice_offset * item_size];
          break;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_join, tid__custom, separator, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]]
     default:
          goto invalid_type_label;
     }

     const t_any* parts_items;
     u64          parts_len;
     u8           step_size;

     switch (parts.bytes[15]) {
     case tid__obj:
          parts_items = obj__get_fields(parts);
          parts_items = &parts_items[1];
          parts_len   = obj__get_fields_len(parts);
          step_size   = 2;
          break;
     case tid__table:
          parts_items = table__get_kvs(parts);
          parts_items = &parts_items[1];
          parts_len   = hash_map__get_len(parts);
          step_size   = 2;
          break;
     case tid__set:
          parts_items = set__get_items(parts);
          parts_len   = hash_map__get_len(parts);
          step_size   = 1;
          break;
     case tid__array:
          u32       const slice_offset = slice__get_offset(parts);
          u32       const slice_len    = slice__get_len(parts);
          parts_items                  = &((const t_any*)slice_array__get_items(parts))[slice_offset];
          parts_len                    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(parts);
          step_size                    = 1;
          break;

     [[clang::unlikely]]
     default:
          goto invalid_type_label;
     }

     u64 result_len               = 0;
     u8  result_short_string_size = 0;

     u64 remain_items_len = parts_len;
     for (u64 idx = 0; remain_items_len != 0; idx += step_size) {
          t_any const part = parts_items[idx];
          switch (part.bytes[15]) {
          case tid__short_string:
               if (!(separator_type == tid__short_string || separator_type == tid__string)) [[clang::unlikely]] goto invalid_type_label;

               result_len               += short_string__get_len(part);
               result_short_string_size += result_short_string_size < 16 ? short_string__get_size(part) : 0;
               break;
          case tid__string: {
               if (!(separator_type == tid__short_string || separator_type == tid__string)) [[clang::unlikely]] goto invalid_type_label;

               u32 const part_slice_len = slice__get_len(part);
               result_len              += part_slice_len <= slice_max_len ? part_slice_len : slice_array__get_len(part);
               result_short_string_size = 16;
               break;
          }
          case tid__short_byte_array:
               if (!(separator_type == tid__short_byte_array || separator_type == tid__byte_array)) [[clang::unlikely]] goto invalid_type_label;

               result_len += short_byte_array__get_len(part);
               break;
          case tid__byte_array: {
               if (!(separator_type == tid__short_byte_array || separator_type == tid__byte_array)) [[clang::unlikely]] goto invalid_type_label;

               u32 const part_slice_len = slice__get_len(part);
               result_len              += part_slice_len <= slice_max_len ? part_slice_len : slice_array__get_len(part);
               break;
          }
          case tid__array: {
               if (separator_type != tid__array) [[clang::unlikely]] goto invalid_type_label;

               u32 const part_slice_len = slice__get_len(part);
               result_len              += part_slice_len <= slice_max_len ? part_slice_len : slice_array__get_len(part);
               break;
          }
          case tid__null: case tid__holder:
               continue;

          [[clang::unlikely]]
          default:
               goto invalid_type_label;
          }

          remain_items_len -= 1;

          if (remain_items_len != 0) {
               result_len               += separator_len;
               result_short_string_size += result_short_string_size < 16 ? separator_short_string_size : 0;
          }
     }

     if (result_len > array_max_len) [[clang::unlikely]] {
          call_stack__push(thrd_data, owner);

          switch (separator_type) {
          case tid__short_string: case tid__string:
               fail_with_call_stack(thrd_data, "The maximum string size has been exceeded.", owner);
          case tid__short_byte_array: case tid__byte_array:
               fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);
          case tid__array:
               fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);
          default:
               unreachable;
          }
     }

     u64 const parts_ref_cnt = get_ref_cnt(parts);
     if (parts_ref_cnt > 1) set_ref_cnt(parts, parts_ref_cnt - 1);

     {
          t_any result;
          call_stack__push(thrd_data, owner);
          switch (separator_type) {
          case tid__short_string: case tid__string:
               result = string__join(thrd_data, parts_len, parts_items, separator_len, separator_items, separator_short_string_size, result_len, result_short_string_size, step_size, owner);
               break;
          case tid__short_byte_array: case tid__byte_array:
               result = byte_array__join(thrd_data, parts_len, parts_items, separator_len, separator_items, result_len, step_size, owner);
               break;
          case tid__array:
               result = array__join(thrd_data, parts_len, parts_items, separator_len, separator_items, get_ref_cnt(separator) == 0, result_len, step_size, owner);
               break;
          default:
               unreachable;
          }
          call_stack__pop(thrd_data);

          if (parts_ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, parts);
          ref_cnt__dec(thrd_data, separator);

          return result;
     }

     invalid_type_label:
     ref_cnt__dec(thrd_data, parts);
     ref_cnt__dec(thrd_data, separator);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}