#pragma once

#include "../fn/hash.h"
#include "../fn/__equal__.h"

static t_any check_switch_operand__own(t_thrd_data* const thrd_data, t_any const operand, const char* const owner) {
     switch (operand.bytes[15]) {
     case tid__short_string: case tid__token: case tid__bool: case tid__byte: case tid__char: case tid__int: case tid__time: case tid__short_byte_array:
          return (const t_any){.bytes = {[15] = tid__null}};
     case tid__byte_array: case tid__string:
          ref_cnt__dec(thrd_data, operand);
          return (const t_any){.bytes = {[15] = tid__null}};
     case tid__error:
          return operand;
     case tid__obj: case tid__custom: {
          u64 const ref_cnt = get_ref_cnt(operand);
          if (ref_cnt > 1) set_ref_cnt(operand, ref_cnt - 1);

          t_any const operand_hash = hash(thrd_data, operand, 0, owner);
          bool  const correct_type = operand_hash.bytes[15] == tid__int && equal(thrd_data, operand, operand, owner).bytes[0] == 1;

          if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, operand);

          if (!correct_type) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, operand_hash);
               goto invalid_type_label;
          }

          return (const t_any){.bytes = {[15] = tid__null}};
     }
     case tid__table: {
          u64 const ref_cnt = get_ref_cnt(operand);
          if (ref_cnt > 1) set_ref_cnt(operand, ref_cnt - 1);

          u64          remain_qty = hash_map__get_len(operand);
          t_any* const kvs        = table__get_kvs(operand);

          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               t_any const key = kvs[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;

               if (ref_cnt > 1)
                    ref_cnt__inc(thrd_data, key, owner);

               t_any const key_check_result = check_switch_operand__own(thrd_data, key, owner);
               if (key_check_result.bytes[15] == tid__error) [[clang::unlikely]] {
                    if (ref_cnt == 1) {
                         ref_cnt__dec(thrd_data, kvs[idx + 1]);

                         for (idx += 2; remain_qty != 0; idx += 2) {
                              if (kvs[idx].bytes[15] == tid__null || kvs[idx].bytes[15] == tid__holder) continue;

                              remain_qty -= 1;

                              ref_cnt__dec(thrd_data, kvs[idx]);
                              ref_cnt__dec(thrd_data, kvs[idx + 1]);
                         }

                         free((void*)operand.qwords[0]);
                    }

                    return key_check_result;
               }

               t_any const value = kvs[idx + 1];
               if (ref_cnt > 1)
                    ref_cnt__inc(thrd_data, value, owner);

               t_any const value_check_result = check_switch_operand__own(thrd_data, value, owner);
               if (value_check_result.bytes[15] == tid__error) [[clang::unlikely]] {
                    if (ref_cnt == 1) {
                         for (idx += 2; remain_qty != 0; idx += 2) {
                              if (kvs[idx].bytes[15] == tid__null || kvs[idx].bytes[15] == tid__holder) continue;

                              remain_qty -= 1;

                              ref_cnt__dec(thrd_data, kvs[idx]);
                              ref_cnt__dec(thrd_data, kvs[idx + 1]);
                         }

                         free((void*)operand.qwords[0]);
                    }

                    return value_check_result;
               }
          }

          if (ref_cnt == 1) free((void*)operand.qwords[0]);

          return (const t_any){.bytes = {[15] = tid__null}};
     }
     case tid__set: {
          u64    const ref_cnt    = get_ref_cnt(operand);
          if (ref_cnt > 1) set_ref_cnt(operand, ref_cnt - 1);

          u64          remain_qty = hash_map__get_len(operand);
          t_any* const items      = set__get_items(operand);

          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               t_any item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

               remain_qty -= 1;

               if (ref_cnt > 1) ref_cnt__inc(thrd_data, item, owner);

               t_any const item_check_result = check_switch_operand__own(thrd_data, item, owner);
               if (item_check_result.bytes[15] == tid__error) [[clang::unlikely]] {
                    if (ref_cnt == 1) {
                         for (idx += 1; remain_qty != 0; idx += 1) {
                              item = items[idx];
                              if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

                              remain_qty -= 1;
                              ref_cnt__dec(thrd_data, item);
                         }

                         free((void*)operand.qwords[0]);
                    }

                    return item_check_result;
               }
          }

          if (ref_cnt == 1) free((void*)operand.qwords[0]);

          return (const t_any){.bytes = {[15] = tid__null}};
     }
     case tid__array: {
          u32    const slice_offset      = slice__get_offset(operand);
          u32    const slice_len         = slice__get_len(operand);
          t_any* const items             = slice_array__get_items(operand);
          t_any* const items_from_offset = &items[slice_offset];
          u64    const ref_cnt           = get_ref_cnt(operand);
          u64    const array_len         = slice_array__get_len(operand);
          u64    const len               = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt == 1)
               for (u32 idx = 0; idx < slice_offset; ref_cnt__dec(thrd_data, items[idx]), idx += 1);
          else if (ref_cnt != 0) set_ref_cnt(operand, ref_cnt - 1);

          for (u64 idx = 0; idx < len; idx += 1) {
               t_any const item = items_from_offset[idx];

               if (ref_cnt > 1) ref_cnt__inc(thrd_data, item, owner);

               t_any const item_check_result = check_switch_operand__own(thrd_data, item, owner);
               if (item_check_result.bytes[15] == tid__error) [[clang::unlikely]] {
                    if (ref_cnt == 1) {
                         for (idx += slice_offset + 1; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

                         free((void*)operand.qwords[0]);
                    }

                    return item_check_result;
               }
          }

          if (ref_cnt == 1) {
               for (u64 idx = slice_offset + len; idx < array_len; ref_cnt__dec(thrd_data, items[idx]), idx += 1);

               free((void*)operand.qwords[0]);
          }

          return (const t_any){.bytes = {[15] = tid__null}};
     }

     [[clang::unlikely]] default:
          ref_cnt__dec(thrd_data, operand);
          goto invalid_type_label;
     }

     invalid_type_label:
     return error__invalid_type(thrd_data, owner);
}

static inline t_any const check_switch_table(t_thrd_data* const thrd_data, t_any const table, u32 const expected_len) {
     if (table.bytes[15] == tid__error) [[clang::unlikely]] return table;

     assume(table.bytes[15] == tid__table);

     if (hash_map__get_len(table) != expected_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, table);

          t_error* const error = aligned_alloc(16, sizeof(t_error));
          *error               = (const t_error) {
               .ref_cnt = 1,
               .id        = token_from_ascii_chars(9, "same_case"),
               .msg       = string_from_ascii("In the 'switch' operator the same case is specified several times."),
               .data      = null
          };
          return (const t_any){.structure = {.value = (u64)error, .type = tid__error}};
     }

     return table;
}
