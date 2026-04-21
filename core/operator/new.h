#pragma once

#include "../common/corsar.h"
#include "../common/fn.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/type.h"
#include "../struct/array/fn.h"
#include "../struct/box/basic.h"
#include "../struct/byte_array/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/string/fn.h"
#include "../struct/table/fn.h"

[[gnu::hot, clang::noinline]]
static t_any hash(t_thrd_data* const thrd_data, t_any const arg, u64 seed, const char* const owner);

[[gnu::hot, clang::noinline]]
core_basic t_any Onew__set(t_thrd_data* const thrd_data, t_any container, u64 const keys_len, const t_any* const keys, t_any const value, bool const return_old_value, const char* const owner) {
     assume(keys_len != 0);

     t_any  error;
     t_any* value_in_container_ptr = &container;
     u64    remain_keys            = keys_len;
     for (;;remain_keys -= 1) {
          t_any const key = keys[keys_len - remain_keys];
          switch (value_in_container_ptr->bytes[15]) {
          case tid__short_string: {
               t_any const string     = *value_in_container_ptr;
               u8    const string_len = short_string__get_len(string);
               if (!(key.bytes[15] == tid__int && value.bytes[15] == tid__char && remain_keys == 1 && key.qwords[0] < string_len)) [[clang::unlikely]] {
                    if (key.bytes[15] == tid__int && key.qwords[0] >= string_len) goto out_of_bounds_label;

                    goto invalid_type_label;
               }

               u8    const first_part_size = short_string__offset_of_idx(string, key.bytes[0], false);
               u32         old_char_code;
               u8    const old_char_size   = ctf8_char_to_corsar_code(&string.bytes[first_part_size], &old_char_code);
               u8    const new_char_size   = char_size_in_ctf8(char_to_code(value.bytes));
               if (short_string__get_size(string) + new_char_size - old_char_size < 16) {
                    t_any       first_part      = {.raw_bits = string.raw_bits & (((u128)1 << first_part_size * 8) - 1)};
                    t_any const last_part       = {.raw_bits = string.raw_bits >> (first_part_size + old_char_size) * 8};
                    corsar_code_to_ctf8_char(char_to_code(value.bytes), &first_part.bytes[first_part_size]);
                    first_part.raw_bits    |= last_part.raw_bits << (first_part_size + new_char_size) * 8;
                    *value_in_container_ptr = first_part;

                    if (return_old_value) {
                         t_any  const box       = box__new(thrd_data, 2, owner);
                         t_any* const box_items = box__get_items(box);
                         box_items[0]           = container;
                         box_items[1]           = (const t_any){.bytes = {[15] = tid__char}};
                         char_from_code(box_items[1].bytes, old_char_code);
                         return box;
                    }

                    return container;
               }

               *value_in_container_ptr = long_string__new(string_len);
               *value_in_container_ptr = slice__set_metadata(*value_in_container_ptr, 0, string_len);
               slice_array__set_len(*value_in_container_ptr, string_len);
               u8* const result_chars = slice_array__get_items(*value_in_container_ptr);

               ctf8_str_ze_lt16_to_corsar_chars(string.bytes, result_chars);
               memcpy_inline(&result_chars[key.bytes[0] * 3], value.bytes, 3);

               if (return_old_value) {
                    t_any  const box       = box__new(thrd_data, 2, owner);
                    t_any* const box_items = box__get_items(box);
                    box_items[0]           = container;
                    box_items[1]           = (const t_any){.bytes = {[15] = tid__char}};
                    char_from_code(box_items[1].bytes, old_char_code);
                    return box;
               }

               return container;
          }
          case tid__short_byte_array: {
               u8 const array_len = short_byte_array__get_len(*value_in_container_ptr);
               if (!(key.bytes[15] == tid__int && value.bytes[15] == tid__byte && remain_keys == 1 && key.qwords[0] < array_len)) [[clang::unlikely]] {
                    if (key.bytes[15] == tid__int && key.qwords[0] >= array_len) goto out_of_bounds_label;

                    goto invalid_type_label;
               }

               u8 const old_byte                           = value_in_container_ptr->bytes[key.bytes[0]];
               value_in_container_ptr->bytes[key.bytes[0]] = value.bytes[0];

               if (return_old_value) {
                    t_any  const box       = box__new(thrd_data, 2, owner);
                    t_any* const box_items = box__get_items(box);
                    box_items[0]           = container;
                    box_items[1]           = (const t_any){.bytes = {old_byte, [15] = tid__byte}};
                    return box;
               }
               return container;
          }
          case tid__obj: {
               if (key.bytes[15] != tid__token) [[clang::unlikely]] goto invalid_type_label;

               t_any obj = *value_in_container_ptr;
               if (get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);
               if (key.raw_bits == mtds_token.raw_bits) {
                    t_any* mtds_ptr = obj__get_mtds(obj);
                    if (remain_keys == 1) {
                         t_any const old_value = *mtds_ptr;
                         if (value.bytes[15] == tid__null) {
                              if (mtds_ptr->bytes[15] != tid__null) {
                                   mtds_ptr->bytes[15] = tid__null;
                                   obj                 = hash_map__set_len(obj, hash_map__get_len(obj) - 1);
                              }
                         } else {
                              if (value.bytes[15] != tid__obj) [[clang::unlikely]] {
                                   *value_in_container_ptr = obj;
                                   goto invalid_type_label;
                              }

                              if (mtds_ptr->bytes[15] != tid__null)
                                   *mtds_ptr = value;
                              else {
                                   *mtds_ptr = value;
                                   obj = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
                              }
                         }
                         *value_in_container_ptr = obj;

                         if (return_old_value) {
                              t_any  const box       = box__new(thrd_data, 2, owner);
                              t_any* const box_items = box__get_items(box);
                              box_items[0]           = container;
                              box_items[1]           = old_value;
                              return box;
                         }

                         ref_cnt__dec(thrd_data, old_value);
                         return container;
                    }

                    if (mtds_ptr->bytes[15] != tid__obj) {
                         *mtds_ptr = obj__init(thrd_data, 1, owner);
                         obj       = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
                    }

                    *value_in_container_ptr = obj;
                    value_in_container_ptr  = mtds_ptr;
               } else {
                    t_any* key_in_obj_ptr = obj__key_ptr(obj, key);
                    if (key_in_obj_ptr == nullptr) {
                         obj            = obj__enlarge__own(thrd_data, obj, owner);
                         key_in_obj_ptr = obj__key_ptr(obj, key);
                    }

                    assume(key_in_obj_ptr != nullptr);

                    if (remain_keys == 1) {
                         if (!type_is_common_or_null(value.bytes[15])) [[clang::unlikely]] {
                              *value_in_container_ptr = obj;
                              goto invalid_type_label;
                         }

                         t_any old_value = key_in_obj_ptr->bytes[15] == tid__token ? key_in_obj_ptr[1] : null;
                         if (value.bytes[15] == tid__null) {
                              if (key_in_obj_ptr->bytes[15] == tid__token) {
                                   key_in_obj_ptr[0].bytes[15] = tid__holder;
                                   key_in_obj_ptr[1].bytes[15] = tid__holder;
                                   obj = hash_map__set_len(obj, hash_map__get_len(obj) - 1);
                                   if (obj__need_to_fit(obj)) obj = obj__fit__own(thrd_data, obj, owner);
                              }
                         } else if (key_in_obj_ptr->bytes[15] == tid__token)
                              key_in_obj_ptr[1] = value;
                         else {
                              key_in_obj_ptr[0] = key;
                              key_in_obj_ptr[1] = value;
                              obj               = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
                         }
                         *value_in_container_ptr = obj;

                         if (return_old_value) {
                              t_any  const box       = box__new(thrd_data, 2, owner);
                              t_any* const box_items = box__get_items(box);
                              box_items[0]           = container;
                              box_items[1]           = old_value;
                              return box;
                         }

                         ref_cnt__dec(thrd_data, old_value);
                         return container;
                    }

                    if (key_in_obj_ptr->bytes[15] != tid__token) {
                         key_in_obj_ptr[0] = key;
                         key_in_obj_ptr[1] = keys[keys_len - remain_keys + 1].bytes[15] == tid__token ? obj__init(thrd_data, 1, owner) : table__init(thrd_data, 1, owner);
                         obj               = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
                    }

                    *value_in_container_ptr = obj;
                    value_in_container_ptr  = &key_in_obj_ptr[1];
               }

               break;
          }
          case tid__table: {
               t_any       table    = *value_in_container_ptr;
               u64   const seed     = hash_map__get_hash_seed(table);
               t_any const key_hash = hash(thrd_data, key, seed, owner);
               if (key_hash.bytes[15] == tid__error) [[clang::unlikely]] {
                    error = key_hash;
                    goto error_label;
               }

               if (get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);
               t_any* key_in_table_ptr = table__key_ptr(thrd_data, table, key, key_hash.qwords[0], owner);
               if (key_in_table_ptr == nullptr) {
                    table            = table__enlarge__own(thrd_data, table, owner);
                    key_in_table_ptr = table__key_ptr(thrd_data, table, key, key_hash.qwords[0], owner);
               }

               assume(key_in_table_ptr != nullptr);

               if (remain_keys == 1) {
                    if (!type_is_common_or_null(value.bytes[15])) [[clang::unlikely]] {
                         *value_in_container_ptr = table;
                         goto invalid_type_label;
                    }

                    t_any old_value = key_in_table_ptr->bytes[15] == key.bytes[15] ? key_in_table_ptr[1] : null;
                    if (value.bytes[15] == tid__null) {
                         ref_cnt__dec(thrd_data, key);
                         if (key_in_table_ptr->bytes[15] == key.bytes[15]) {
                              ref_cnt__dec(thrd_data, key_in_table_ptr[0]);
                              memset_inline(key_in_table_ptr, tid__holder, 32);
                              table = hash_map__set_len(table, hash_map__get_len(table) - 1);
                              if (table__need_to_fit(table)) table = table__fit__own(thrd_data, table, owner);
                         }
                    } else if (key_in_table_ptr->bytes[15] == key.bytes[15]) {
                         ref_cnt__dec(thrd_data, key_in_table_ptr[0]);
                         key_in_table_ptr[0] = key;
                         key_in_table_ptr[1] = value;
                    } else {
                         key_in_table_ptr[0] = key;
                         key_in_table_ptr[1] = value;
                         table               = hash_map__set_len(table, hash_map__get_len(table) + 1);
                    }
                    *value_in_container_ptr = table;

                    if (return_old_value) {
                         t_any  const box       = box__new(thrd_data, 2, owner);
                         t_any* const box_items = box__get_items(box);
                         box_items[0]           = container;
                         box_items[1]           = old_value;
                         return box;
                    }

                    ref_cnt__dec(thrd_data, old_value);
                    return container;
               }

               if (key_in_table_ptr->bytes[15] == key.bytes[15]) {
                    ref_cnt__dec(thrd_data, key_in_table_ptr[0]);
                    key_in_table_ptr[0]     = key;
                    *value_in_container_ptr = table;
                    value_in_container_ptr  = &key_in_table_ptr[1];
               } else {
                    key_in_table_ptr[0] = key;
                    key_in_table_ptr[1] = keys[keys_len - remain_keys + 1].bytes[15] == tid__token ? obj__init(thrd_data, 1, owner) : table__init(thrd_data, 1, owner);
                    table               = hash_map__set_len(table, hash_map__get_len(table) + 1);

                    *value_in_container_ptr = table;
                    value_in_container_ptr  = &key_in_table_ptr[1];
               }

               break;
          }
          case tid__byte_array: {
               t_any     array     = *value_in_container_ptr;
               u32 const slice_len = slice__get_len(array);
               u64 const array_len = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);
               if (!(key.bytes[15] == tid__int && value.bytes[15] == tid__byte && remain_keys == 1 && key.qwords[0] < array_len)) [[clang::unlikely]] {
                    if (key.bytes[15] == tid__int && key.qwords[0] >= array_len) goto out_of_bounds_label;

                    goto invalid_type_label;
               }

               if (get_ref_cnt(array) != 1) array = long_byte_array__dup__own(thrd_data, array);

               u8* const old_byte_ptr  = &((u8*)slice_array__get_items(array))[slice__get_offset(array) + key.qwords[0]];
               u8  const old_byte      = *old_byte_ptr;
               *old_byte_ptr           = value.bytes[0];
               *value_in_container_ptr = array;

               if (return_old_value) {
                    t_any  const box       = box__new(thrd_data, 2, owner);
                    t_any* const box_items = box__get_items(box);
                    box_items[0]           = container;
                    box_items[1]           = (const t_any){.bytes = {old_byte, [15] = tid__byte}};
                    return box;
               }

               return container;
          }
          case tid__string: {
               t_any     string     = *value_in_container_ptr;
               u32 const slice_len  = slice__get_len(string);
               u64 const string_len = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);
               if (!(key.bytes[15] == tid__int && value.bytes[15] == tid__char && remain_keys == 1 && key.qwords[0] < string_len)) [[clang::unlikely]] {
                    if (key.bytes[15] == tid__int && key.qwords[0] >= string_len) goto out_of_bounds_label;

                    goto invalid_type_label;
               }

               const u8* const original_chars = &((const u8*)slice_array__get_items(string))[slice__get_offset(string) * 3];
               t_any           old_char       = {.bytes = {[15] = tid__char}};
               memcpy_inline(old_char.bytes, &original_chars[key.qwords[0] * 3], 3);
               if (string_len < 16) {
                    t_any           short_string = {};
                    u8              offset       = 0;
                    for (u64 idx = 0; idx < string_len; idx += 1) {
                         u32 const code = char_to_code(idx != key.qwords[0] ? &original_chars[idx * 3] : value.bytes);
                         if (offset + char_size_in_ctf8(code) > 15) goto not_short_label;

                         offset += corsar_code_to_ctf8_char(code, &short_string.bytes[offset]);
                    }
                    *value_in_container_ptr = short_string;
                    ref_cnt__dec(thrd_data, string);

                    if (return_old_value) {
                         t_any  const box       = box__new(thrd_data, 2, owner);
                         t_any* const box_items = box__get_items(box);
                         box_items[0]           = container;
                         box_items[1]           = old_char;
                         return box;
                    }

                    return container;
               }
               not_short_label:

               if (get_ref_cnt(string) != 1) string = long_string__dup__own(thrd_data, string);
               memcpy_inline(&((u8*)slice_array__get_items(string))[(slice__get_offset(string) + key.qwords[0]) * 3], value.bytes, 3);
               *value_in_container_ptr = string;

               if (return_old_value) {
                    t_any  const box       = box__new(thrd_data, 2, owner);
                    t_any* const box_items = box__get_items(box);
                    box_items[0]           = container;
                    box_items[1]           = old_char;
                    return box;
               }
               return container;
          }
          case tid__array: {
               t_any     array     = *value_in_container_ptr;
               u32 const slice_len = slice__get_len(array);
               u64 const array_len = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);
               if (key.bytes[15] != tid__int || key.qwords[0] >= array_len) [[clang::unlikely]] {
                    if (key.bytes[15] != tid__int) goto invalid_type_label;

                    goto out_of_bounds_label;
               }

               if (get_ref_cnt(array) != 1) array = array__dup__own(thrd_data, array, owner);

               t_any* const item_in_array_ptr = &((t_any*)slice_array__get_items(array))[slice__get_offset(array) + key.qwords[0]];
               if (remain_keys == 1) {
                    if (!type_is_common(value.bytes[15])) [[clang::unlikely]] {
                         *value_in_container_ptr = array;
                         goto invalid_type_label;
                    }

                    t_any const old_value   = *item_in_array_ptr;
                    *item_in_array_ptr      = value;
                    *value_in_container_ptr = array;

                    if (return_old_value) {
                         t_any  const box       = box__new(thrd_data, 2, owner);
                         t_any* const box_items = box__get_items(box);
                         box_items[0]           = container;
                         box_items[1]           = old_value;
                         return box;
                    }

                    ref_cnt__dec(thrd_data, old_value);
                    return container;
               }

               *value_in_container_ptr = array;
               value_in_container_ptr  = item_in_array_ptr;
               break;
          }

          [[clang::unlikely]]
          default:
               goto invalid_type_label;
          }
     }

     out_of_bounds_label:
     error = error__out_of_bounds(thrd_data, owner);
     goto error_label;

     invalid_type_label:
     error = error__invalid_type(thrd_data, owner);

     error_label:
     if (value.bytes[15] == tid__error) {
          ref_cnt__dec(thrd_data, error);
          error = value;
     } else ref_cnt__dec(thrd_data, value);

     for (u64 idx = keys_len - 1; remain_keys != 0; idx -= 1, remain_keys -= 1) {
          t_any const key = keys[idx];
          if (key.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, error);
               error = key;
          } else ref_cnt__dec(thrd_data, key);
     }

     if (container.bytes[15] == tid__error) {
          ref_cnt__dec(thrd_data, error);
          error = container;
     } else ref_cnt__dec(thrd_data, container);

     return error;
}

[[gnu::hot, clang::noinline]]
core_basic t_any Onew__apply(t_thrd_data* const thrd_data, t_any container, u64 const keys_len, const t_any* const keys, t_any const fn, bool const return_old_value, const char* const owner) {
     assume(keys_len != 0);

     t_any  error;
     t_any* value_in_container_ptr = &container;
     u64    remain_keys            = keys_len;
     for (;;remain_keys -= 1) {
          t_any const key = keys[keys_len - remain_keys];
          switch (value_in_container_ptr->bytes[15]) {
          case tid__short_string: {
               t_any const string     = *value_in_container_ptr;
               u8    const string_len = short_string__get_len(string);
               if (!(key.bytes[15] == tid__int && (fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn) && remain_keys == 1 && key.qwords[0] < string_len)) [[clang::unlikely]]{
                    if (key.bytes[15] == tid__int && key.qwords[0] >= string_len) goto out_of_bounds_label;

                    goto invalid_type_label;
               }

               u8 const first_part_size = short_string__offset_of_idx(string, key.bytes[0], false);
               u32      old_char_code;
               u8 const old_char_size   = ctf8_char_to_corsar_code(&string.bytes[first_part_size], &old_char_code);
               t_any    old_char        = {.bytes = {[15] = tid__char}};
               char_from_code(old_char.bytes, old_char_code);
               t_any const new_char = common_fn__call__own(thrd_data, fn, &old_char, 1, owner);
               if (new_char.bytes[15] != tid__char) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, container);
                    if (new_char.bytes[15] == tid__error) return new_char;

                    ref_cnt__dec(thrd_data, new_char);
                    return error__invalid_type(thrd_data, owner);
               }

               u8 const new_char_size = char_size_in_ctf8(char_to_code(new_char.bytes));
               if (short_string__get_size(string) + new_char_size - old_char_size < 16) {
                    t_any       first_part      = {.raw_bits = string.raw_bits & (((u128)1 << first_part_size * 8) - 1)};
                    t_any const last_part       = {.raw_bits = string.raw_bits >> (first_part_size + old_char_size) * 8};
                    corsar_code_to_ctf8_char(char_to_code(new_char.bytes), &first_part.bytes[first_part_size]);
                    first_part.raw_bits    |= last_part.raw_bits << (first_part_size + new_char_size) * 8;
                    *value_in_container_ptr = first_part;

                    if (return_old_value) {
                         t_any  const box       = box__new(thrd_data, 2, owner);
                         t_any* const box_items = box__get_items(box);
                         box_items[0]           = container;
                         box_items[1]           = old_char;
                         return box;
                    }

                    return container;
               }

               *value_in_container_ptr = long_string__new(string_len);
               *value_in_container_ptr = slice__set_metadata(*value_in_container_ptr, 0, string_len);
               slice_array__set_len(*value_in_container_ptr, string_len);
               u8* const result_chars = slice_array__get_items(*value_in_container_ptr);

               ctf8_str_ze_lt16_to_corsar_chars(string.bytes, result_chars);
               memcpy_inline(&result_chars[key.bytes[0] * 3], new_char.bytes, 3);

               if (return_old_value) {
                    t_any  const box       = box__new(thrd_data, 2, owner);
                    t_any* const box_items = box__get_items(box);
                    box_items[0]           = container;
                    box_items[1]           = old_char;
                    return box;
               }

               return container;
          }
          case tid__short_byte_array: {
               u8 const array_len = short_byte_array__get_len(*value_in_container_ptr);
               if (!(key.bytes[15] == tid__int && (fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn) && remain_keys == 1 && key.qwords[0] < array_len)) [[clang::unlikely]]{
                    if (key.bytes[15] == tid__int && key.qwords[0] >= array_len) goto out_of_bounds_label;

                    goto invalid_type_label;
               }

               t_any const old_byte = (const t_any){.bytes = {value_in_container_ptr->bytes[key.bytes[0]], [15] = tid__byte}};
               t_any const new_byte = common_fn__call__own(thrd_data, fn, &old_byte, 1, owner);
               if (new_byte.bytes[15] != tid__byte) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, container);
                    if (new_byte.bytes[15] == tid__error) return new_byte;

                    ref_cnt__dec(thrd_data, new_byte);
                    return error__invalid_type(thrd_data, owner);
               }
               value_in_container_ptr->bytes[key.bytes[0]] = new_byte.bytes[0];

               if (return_old_value) {
                    t_any  const box       = box__new(thrd_data, 2, owner);
                    t_any* const box_items = box__get_items(box);
                    box_items[0]           = container;
                    box_items[1]           = old_byte;
                    return box;
               }

               return container;
          }
          case tid__obj: {
               if (key.bytes[15] != tid__token) [[clang::unlikely]] goto invalid_type_label;

               t_any obj = *value_in_container_ptr;
               if (get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);
               if (key.raw_bits == mtds_token.raw_bits) {
                    t_any* mtds_ptr = obj__get_mtds(obj);
                    if (remain_keys == 1) {
                         t_any const old_mtds = *mtds_ptr;
                         if (return_old_value) ref_cnt__inc(thrd_data, old_mtds, owner);
                         t_any const new_mtds = common_fn__call__own(thrd_data, fn, mtds_ptr, 1, owner);
                         if (!(new_mtds.bytes[15] == tid__obj || new_mtds.bytes[15] == tid__null)) [[clang::unlikely]] {
                              if (mtds_ptr->bytes[15] != tid__null) {
                                   if (return_old_value) ref_cnt__dec(thrd_data, old_mtds);
                                   mtds_ptr->bytes[15] = tid__null;
                                   obj                 = hash_map__set_len(obj, hash_map__get_len(obj) - 1);
                              }
                              *value_in_container_ptr = obj;
                              ref_cnt__dec(thrd_data, container);
                              if (new_mtds.bytes[15] == tid__error) return new_mtds;

                              ref_cnt__dec(thrd_data, new_mtds);
                              return error__invalid_type(thrd_data, owner);
                         }

                         if (new_mtds.bytes[15] == tid__null) {
                              if (mtds_ptr->bytes[15] != tid__null) {
                                   mtds_ptr->bytes[15] = tid__null;
                                   obj                 = hash_map__set_len(obj, hash_map__get_len(obj) - 1);
                              }
                         } else {
                              if (mtds_ptr->bytes[15] == tid__null) obj = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
                              *mtds_ptr = new_mtds;
                         }
                         *value_in_container_ptr = obj;

                         if (return_old_value) {
                              t_any  const box       = box__new(thrd_data, 2, owner);
                              t_any* const box_items = box__get_items(box);
                              box_items[0]           = container;
                              box_items[1]           = old_mtds;
                              return box;
                         }

                         return container;
                    }

                    if (mtds_ptr->bytes[15] != tid__obj) {
                         *mtds_ptr = obj__init(thrd_data, 1, owner);
                         obj       = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
                    }
                    *value_in_container_ptr = obj;
                    value_in_container_ptr  = mtds_ptr;
               } else {
                    t_any* key_in_obj_ptr = obj__key_ptr(obj, key);
                    if (key_in_obj_ptr == nullptr) {
                         obj            = obj__enlarge__own(thrd_data, obj, owner);
                         key_in_obj_ptr = obj__key_ptr(obj, key);
                    }

                    assume(key_in_obj_ptr != nullptr);

                    if (remain_keys == 1) {
                         t_any const old_value = key_in_obj_ptr->bytes[15] == tid__token ? key_in_obj_ptr[1] : null;
                         if (return_old_value) ref_cnt__inc(thrd_data, old_value, owner);
                         t_any const new_value = common_fn__call__own(thrd_data, fn, &old_value, 1, owner);
                         if (!type_is_common_or_null(new_value.bytes[15])) [[clang::unlikely]] {
                              if (return_old_value) ref_cnt__dec(thrd_data, old_value);
                              memset_inline(&key_in_obj_ptr[1], 0, 16);
                              *value_in_container_ptr = obj;
                              ref_cnt__dec(thrd_data, container);
                              if (new_value.bytes[15] == tid__error) return new_value;

                              ref_cnt__dec(thrd_data, new_value);
                              return error__invalid_type(thrd_data, owner);
                         }

                         if (new_value.bytes[15] == tid__null) {
                              if (key_in_obj_ptr->bytes[15] == tid__token) {
                                   key_in_obj_ptr[0].bytes[15] = tid__holder;
                                   key_in_obj_ptr[1].bytes[15] = tid__holder;
                                   obj = hash_map__set_len(obj, hash_map__get_len(obj) - 1);
                                   if (obj__need_to_fit(obj)) obj = obj__fit__own(thrd_data, obj, owner);
                              }
                         } else if (key_in_obj_ptr->bytes[15] == tid__token) {
                              key_in_obj_ptr[1] = new_value;
                         } else {
                              key_in_obj_ptr[0] = key;
                              key_in_obj_ptr[1] = new_value;
                              obj               = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
                         }
                         *value_in_container_ptr = obj;

                         if (return_old_value) {
                              t_any  const box       = box__new(thrd_data, 2, owner);
                              t_any* const box_items = box__get_items(box);
                              box_items[0]           = container;
                              box_items[1]           = old_value;
                              return box;
                         }

                         return container;
                    }

                    if (key_in_obj_ptr->bytes[15] != tid__token) {
                         key_in_obj_ptr[0] = key;
                         key_in_obj_ptr[1] = keys[keys_len - remain_keys + 1].bytes[15] == tid__token ? obj__init(thrd_data, 1, owner) : table__init(thrd_data, 1, owner);
                         obj               = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
                    }

                    *value_in_container_ptr = obj;
                    value_in_container_ptr  = &key_in_obj_ptr[1];
               }

               break;
          }
          case tid__table: {
               t_any       table    = *value_in_container_ptr;
               u64   const seed     = hash_map__get_hash_seed(table);
               t_any const key_hash = hash(thrd_data, key, seed, owner);
               if (key_hash.bytes[15] == tid__error) [[clang::unlikely]] {
                    error = key_hash;
                    goto error_label;
               }

               if (get_ref_cnt(table) != 1) table = table__dup__own(thrd_data, table, owner);
               t_any* key_in_table_ptr = table__key_ptr(thrd_data, table, key, key_hash.qwords[0], owner);
               if (key_in_table_ptr == nullptr) {
                    table            = table__enlarge__own(thrd_data, table, owner);
                    key_in_table_ptr = table__key_ptr(thrd_data, table, key, key_hash.qwords[0], owner);
               }

               assume(key_in_table_ptr != nullptr);

               if (remain_keys == 1) {
                    t_any const old_value = key_in_table_ptr->bytes[15] == key.bytes[15] ? key_in_table_ptr[1] : null;
                    if (return_old_value) ref_cnt__inc(thrd_data, old_value, owner);
                    t_any const new_value = common_fn__call__own(thrd_data, fn, &old_value, 1, owner);
                    if (!type_is_common_or_null(new_value.bytes[15])) [[clang::unlikely]] {
                         if (return_old_value) ref_cnt__dec(thrd_data, old_value);
                         memset_inline(&key_in_table_ptr[1], 0, 16);
                         *value_in_container_ptr = table;
                         ref_cnt__dec(thrd_data, container);
                         ref_cnt__dec(thrd_data, key);
                         if (new_value.bytes[15] == tid__error) return new_value;

                         ref_cnt__dec(thrd_data, new_value);
                         return error__invalid_type(thrd_data, owner);
                    }

                    if (new_value.bytes[15] == tid__null) {
                         ref_cnt__dec(thrd_data, key);
                         if (key_in_table_ptr->bytes[15] == key.bytes[15]) {
                              ref_cnt__dec(thrd_data, key_in_table_ptr[0]);
                              memset_inline(key_in_table_ptr, tid__holder, 32);
                              table = hash_map__set_len(table, hash_map__get_len(table) - 1);
                              if (table__need_to_fit(table)) table = table__fit__own(thrd_data, table, owner);
                         }
                    } else if (key_in_table_ptr->bytes[15] == key.bytes[15]) {
                         ref_cnt__dec(thrd_data, key_in_table_ptr[0]);
                         key_in_table_ptr[0] = key;
                         key_in_table_ptr[1] = new_value;
                    } else {
                         key_in_table_ptr[0] = key;
                         key_in_table_ptr[1] = new_value;
                         table               = hash_map__set_len(table, hash_map__get_len(table) + 1);
                    }
                    *value_in_container_ptr = table;

                    if (return_old_value) {
                         t_any  const box       = box__new(thrd_data, 2, owner);
                         t_any* const box_items = box__get_items(box);
                         box_items[0]           = container;
                         box_items[1]           = old_value;
                         return box;
                    }

                    return container;
               }

               if (key_in_table_ptr->bytes[15] == key.bytes[15]) {
                    ref_cnt__dec(thrd_data, key_in_table_ptr[0]);
                    key_in_table_ptr[0]     = key;
                    *value_in_container_ptr = table;
                    value_in_container_ptr  = &key_in_table_ptr[1];
               } else {
                    key_in_table_ptr[0]     = key;
                    key_in_table_ptr[1]     = keys[keys_len - remain_keys + 1].bytes[15] == tid__token ? obj__init(thrd_data, 1, owner) : table__init(thrd_data, 1, owner);
                    table                   = hash_map__set_len(table, hash_map__get_len(table) + 1);

                    *value_in_container_ptr = table;
                    value_in_container_ptr  = &key_in_table_ptr[1];
               }

               break;
          }
          case tid__byte_array: {
               t_any     array     = *value_in_container_ptr;
               u32 const slice_len = slice__get_len(array);
               u64 const array_len = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);
               if (!(key.bytes[15] == tid__int && (fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn) && remain_keys == 1 && key.qwords[0] < array_len)) [[clang::unlikely]]{
                    if (key.bytes[15] == tid__int && key.qwords[0] >= array_len) goto out_of_bounds_label;

                    goto invalid_type_label;
               }

               if (get_ref_cnt(array) != 1) array = long_byte_array__dup__own(thrd_data, array);
               u8* const byte_in_array_ptr = &((u8*)slice_array__get_items(array))[slice__get_offset(array) + key.qwords[0]];

               t_any const old_byte = (const t_any){.bytes = {*byte_in_array_ptr, [15] = tid__byte}};
               t_any const new_byte = common_fn__call__own(thrd_data, fn, &old_byte, 1, owner);
               if (new_byte.bytes[15] != tid__byte) [[clang::unlikely]] {
                    *value_in_container_ptr = array;
                    ref_cnt__dec(thrd_data, container);
                    if (new_byte.bytes[15] == tid__error) return new_byte;

                    ref_cnt__dec(thrd_data, new_byte);
                    return error__invalid_type(thrd_data, owner);
               }
               *byte_in_array_ptr      = new_byte.bytes[0];
               *value_in_container_ptr = array;

               if (return_old_value) {
                    t_any  const box       = box__new(thrd_data, 2, owner);
                    t_any* const box_items = box__get_items(box);
                    box_items[0]           = container;
                    box_items[1]           = old_byte;
                    return box;
               }

               return container;
          }
          case tid__string: {
               t_any     string     = *value_in_container_ptr;
               u32 const slice_len  = slice__get_len(string);
               u64 const string_len = slice_len <= slice_max_len ? slice_len : slice_array__get_len(string);
               if (!(key.bytes[15] == tid__int && (fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn) && remain_keys == 1 && key.qwords[0] < string_len)) [[clang::unlikely]]{
                    if (!(key.bytes[15] == tid__int && (fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn) && remain_keys == 1)) goto invalid_type_label;

                    ref_cnt__dec(thrd_data, container);
                    ref_cnt__dec(thrd_data, fn);
                    return error__out_of_bounds(thrd_data, owner);
               }

               t_any old_char = {.bytes = {[15] = tid__char}};
               memcpy_inline(old_char.bytes, &((const u8*)slice_array__get_items(string))[(slice__get_offset(string) + key.qwords[0]) * 3], 3);
               t_any const new_char = common_fn__call__own(thrd_data, fn, &old_char, 1, owner);
               if (new_char.bytes[15] != tid__char) [[clang::unlikely]] {
                    *value_in_container_ptr = string;
                    ref_cnt__dec(thrd_data, container);
                    if (new_char.bytes[15] == tid__error) return new_char;

                    ref_cnt__dec(thrd_data, new_char);
                    return error__invalid_type(thrd_data, owner);
               }

               if (string_len < 16) {
                    const u8* const chars        = &((const u8*)slice_array__get_items(string))[slice__get_offset(string) * 3];
                    t_any           short_string = {};
                    u8              offset       = 0;
                    for (u64 idx = 0; idx < string_len; idx += 1) {
                         u32 const code = char_to_code(idx != key.qwords[0] ? &chars[idx * 3] : new_char.bytes);
                         if (offset + char_size_in_ctf8(code) > 15) goto not_short_label;

                         offset += corsar_code_to_ctf8_char(code, &short_string.bytes[offset]);
                    }
                    *value_in_container_ptr = short_string;
                    ref_cnt__dec(thrd_data, string);

                    if (return_old_value) {
                         t_any  const box       = box__new(thrd_data, 2, owner);
                         t_any* const box_items = box__get_items(box);
                         box_items[0]           = container;
                         box_items[1]           = old_char;
                         return box;
                    }

                    return container;
               }
               not_short_label:

               if (get_ref_cnt(string) != 1) string = long_string__dup__own(thrd_data, string);
               u8* const char_in_array_ptr = &((u8*)slice_array__get_items(string))[(slice__get_offset(string) + key.qwords[0]) * 3];
               memcpy_inline(char_in_array_ptr, new_char.bytes, 3);
               *value_in_container_ptr = string;

               if (return_old_value) {
                    t_any  const box       = box__new(thrd_data, 2, owner);
                    t_any* const box_items = box__get_items(box);
                    box_items[0]           = container;
                    box_items[1]           = old_char;
                    return box;
               }

               return container;
          }
          case tid__array: {
               t_any     array     = *value_in_container_ptr;
               u32 const slice_len = slice__get_len(array);
               u64 const array_len = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);
               if (key.bytes[15] != tid__int || key.qwords[0] >= array_len) [[clang::unlikely]] {
                    if (key.bytes[15] != tid__int) goto invalid_type_label;

                    goto out_of_bounds_label;
               }

               if (get_ref_cnt(array) != 1) array = array__dup__own(thrd_data, array, owner);

               t_any* const item_in_array_ptr = &((t_any*)slice_array__get_items(array))[slice__get_offset(array) + key.qwords[0]];
               if (remain_keys == 1) {
                    t_any const old_value = *item_in_array_ptr;
                    if (return_old_value) ref_cnt__inc(thrd_data, old_value, owner);
                    t_any const new_item  = common_fn__call__own(thrd_data, fn, item_in_array_ptr, 1, owner);
                    if (!type_is_common(new_item.bytes[15])) [[clang::unlikely]] {
                         if (return_old_value) ref_cnt__dec(thrd_data, old_value);
                         memset_inline(item_in_array_ptr, 0, 16);
                         *value_in_container_ptr = array;
                         ref_cnt__dec(thrd_data, container);
                         if (new_item.bytes[15] == tid__error) return new_item;

                         ref_cnt__dec(thrd_data, new_item);
                         return error__invalid_type(thrd_data, owner);
                    }
                    *item_in_array_ptr      = new_item;
                    *value_in_container_ptr = array;

                    if (return_old_value) {
                         t_any  const box       = box__new(thrd_data, 2, owner);
                         t_any* const box_items = box__get_items(box);
                         box_items[0]           = container;
                         box_items[1]           = old_value;
                         return box;
                    }

                    return container;
               }

               *value_in_container_ptr = array;
               value_in_container_ptr  = item_in_array_ptr;
               break;
          }

          [[clang::unlikely]]
          default:
               goto invalid_type_label;
          }
     }

     out_of_bounds_label:
     error = error__out_of_bounds(thrd_data, owner);
     goto error_label;

     invalid_type_label:
     error = error__invalid_type(thrd_data, owner);

     error_label:
     if (fn.bytes[15] == tid__error) {
          ref_cnt__dec(thrd_data, error);
          error = fn;
     } else ref_cnt__dec(thrd_data, fn);

     for (u64 idx = keys_len - 1; remain_keys != 0; idx -= 1, remain_keys -= 1) {
          t_any const key = keys[idx];
          if (key.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, error);
               error = key;
          } else ref_cnt__dec(thrd_data, key);
     }

     if (container.bytes[15] == tid__error) {
          ref_cnt__dec(thrd_data, error);
          error = container;
     } else ref_cnt__dec(thrd_data, container);

     return error;
}