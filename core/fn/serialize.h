#pragma once

#include "../common/const.h"
#include "../common/fn.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/char/basic.h"
#include "../struct/custom/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/string/basic.h"
#include "../struct/string/cvt.h"

enum : u8 {
     st__non_neg_num__0__ff                                   = 0,
     st__non_neg_num__100__1_00ff                             = 1,
     st__non_neg_num__1_0100__101_00ff                        = 2,
     st__non_neg_num__101_0100__1_0101_00ff                   = 3,
     st__non_neg_num__1_0101_0100__101_0101_00ff              = 4,
     st__non_neg_num__101_0101_0100__1_0101_0101_00ff         = 5,
     st__non_neg_num__1_0101_0101_0100__101_0101_0101_00ff    = 6,
     st__non_neg_num__101_0101_0101_0100__7fff_ffff_ffff_ffff = 7,
     st__neg_num__1__100                                      = 8,
     st__neg_num__101__1_0100                                 = 9,
     st__neg_num__1_0101__101_0100                            = 10,
     st__neg_num__101_0101__1_0101_0100                       = 11,
     st__neg_num__1_0101_0101__101_0101_0100                  = 12,
     st__neg_num__101_0101_0101__1_0101_0101_0100             = 13,
     st__neg_num__1_0101_0101_0101__101_0101_0101_0100        = 14,
     st__neg_num__101_0101_0101_0101__8000_0000_0000_0000     = 15,
     st__string                                               = 16,
     st__token                                                = 17,
     st__false                                                = 18,
     st__true                                                 = 19,
     st__byte                                                 = 20,
     st__char__1__100                                         = 21,
     st__char__101__1_0100                                    = 22,
     st__char__1_0101__3f_ffff                                = 23,
     st__short_time                                           = 24,
     st__full_time                                            = 25,
     st__float                                                = 26,
     st__byte_array                                           = 27,
     st__obj                                                  = 28,
     st__table                                                = 29,
     st__set                                                  = 30,
     st__array                                                = 31,
     st__fix_obj                                              = 32,
} typedef t_serialize_type;

struct {
     u8* bytes;
     u64 len;
     u64 cap;
} typedef t_serialized_bytes;

[[clang::always_inline]]
static void bytes__prepare_to_write(t_thrd_data* const thrd_data, t_serialized_bytes* bytes, u64 const added_size, const char* const owner) {
     u64 const max_len = bytes->len + added_size;
     if (max_len > array_max_len + 16) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum byte array size has been exceeded.", owner);

     if (max_len > bytes->cap) {
          bytes->cap   = max_len * 3 / 2;
          bytes->cap   = bytes->cap > array_max_len + 16 ? max_len : bytes->cap;
          bytes->bytes = realloc(bytes->bytes, bytes->cap);
     }
}

[[clang::always_inline]]
static void bytes__write_len(t_serialized_bytes* bytes, u64 const len) {
     assume(len <= 0xff'ffff'ffff'ffffull);
     assume(bytes->cap - bytes->len >= 8);

     typedef u8  v_8_u8  __attribute__((ext_vector_type(8)));
     typedef u64 v_8_u64 __attribute__((ext_vector_type(8)));

     [[gnu::aligned(alignof(v_8_u8))]]
     u64      len_bytes;
     *(v_8_u8*)&len_bytes = __builtin_convertvector((const v_8_u64)len >> (const v_8_u64){0, 7, 14, 21, 28, 35, 42, 49}, v_8_u8) & 0x7f;
     u8 const len_size    = (sizeof(long long) * 8 - 1 - __builtin_clzll(len | 1)) / 7 + 1;
     len_bytes           |= (u64)1 << (len_size * 8 - 1);
     memcpy_inline(&bytes->bytes[bytes->len], &len_bytes, 8);
     bytes->len += len_size;
}

[[gnu::hot, clang::noinline]]
static bool serialize(t_thrd_data* const thrd_data, t_serialized_bytes* bytes, t_any const arg, const char* const owner) {
     [[gnu::aligned(32)]]
     char token_chars[32];
     u8   token_len;

     switch (arg.bytes[15]) {
     case tid__short_string:
          bytes__prepare_to_write(thrd_data, bytes, 17, owner);
          bytes->bytes[bytes->len] = st__string;
          bytes->len              += 1;
          memcpy_inline(&bytes->bytes[bytes->len], arg.bytes, 16);
          bytes->len += short_string__get_size(arg) + 1;
          return true;
     case tid__token:
          bytes__prepare_to_write(thrd_data, bytes, 33, owner);
          bytes->bytes[bytes->len] = st__token;
          bytes->len              += 1;

          token_to_ascii_chars(arg, &token_len, token_chars);
          token_chars[token_len - 1] |= 128;
          memcpy_inline(&bytes->bytes[bytes->len], token_chars, 32);
          bytes->len += token_len;

          return true;
     case tid__bool:
          bytes__prepare_to_write(thrd_data, bytes, 1, owner);
          bytes->bytes[bytes->len] = arg.bytes[0] == 1 ? st__true : st__false;
          bytes->len              += 1;
          return true;
     case tid__byte:
          bytes__prepare_to_write(thrd_data, bytes, 2, owner);
          bytes->bytes[bytes->len] = st__byte;
          bytes->len              += 1;
          bytes->bytes[bytes->len] = arg.bytes[0];
          bytes->len              += 1;
          return true;
     case tid__char: {
          bytes__prepare_to_write(thrd_data, bytes, 5, owner);
          u32       code           = char_to_code(arg.bytes);
          u8  const char_size      = 1 + (code > 0x100) + (code > 0x1'0100ull);
          code                    -= ((char_size == 2) * 0x100 | (char_size == 3) * 0x1'0100ull) + 1;
          bytes->bytes[bytes->len] = st__char__1__100 + char_size - 1;
          bytes->len              += 1;

          memcpy_inline(&bytes->bytes[bytes->len], &code, 4);
          bytes->len += char_size;
          return true;
     }
     case tid__int: {
          typedef i64 v_8_i64 __attribute__((ext_vector_type(8)));

          [[gnu::aligned(alignof(v_8_i64))]]
          i64 const first_ints[8] = {0, 0x100, 0x1'0100ll, 0x101'0100ll, 0x1'0101'0100ll, 0x101'0101'0100ll, 0x1'0101'0101'0100ll, 0x101'0101'0101'0100ll};

          bytes__prepare_to_write(thrd_data, bytes, 9, owner);

          i64 num = arg.qwords[0];
          u8  int_size;
          if (num >= 0) {
               int_size                 = -__builtin_reduce_add(num >= *(const v_8_i64*)first_ints);
               bytes->bytes[bytes->len] = st__non_neg_num__0__ff + int_size - 1;
               num                     -= first_ints[int_size - 1];
          } else {
               int_size                 = -__builtin_reduce_add(num < -*(const v_8_i64*)first_ints);
               bytes->bytes[bytes->len] = st__neg_num__1__100 + int_size - 1;
               num                      = (num + first_ints[int_size - 1] + 1) * -1;
          }

          bytes->len += 1;
          memcpy_inline(&bytes->bytes[bytes->len], &num, 8);
          bytes->len += int_size;

          return true;
     }
     case tid__time: {
          u64 all_seconds = arg.qwords[0];
          if (all_seconds >= 1'893'456'000ull && all_seconds < 6'188'423'296ull) {
               bytes__prepare_to_write(thrd_data, bytes, 5, owner);
               bytes->bytes[bytes->len] = st__short_time;
               bytes->len              += 1;
               all_seconds             -= 1'893'456'000ull;
               memcpy_inline(&bytes->bytes[bytes->len], &all_seconds, 4);
               bytes->len += 4;
          } else {
               bytes__prepare_to_write(thrd_data, bytes, 9, owner);
               bytes->bytes[bytes->len] = st__full_time;
               bytes->len              += 1;
               memcpy_inline(&bytes->bytes[bytes->len], &all_seconds, 8);
               bytes->len += 8;
          }

          return true;
     }
     case tid__float:
          bytes__prepare_to_write(thrd_data, bytes, 9, owner);
          bytes->bytes[bytes->len] = st__float;
          bytes->len              += 1;
          memcpy_inline(&bytes->bytes[bytes->len], &arg.qwords[0], 8);
          bytes->len += 8;
          return true;
     case tid__short_byte_array: {
          bytes__prepare_to_write(thrd_data, bytes, 18, owner);
          u8 const array_len       = short_byte_array__get_len(arg);
          bytes->bytes[bytes->len] = st__byte_array;
          bytes->len              += 1;
          bytes->bytes[bytes->len] = array_len | 0x80;
          bytes->len              += 1;
          memcpy_inline(&bytes->bytes[bytes->len], arg.bytes, 16);
          bytes->len += array_len;
          return true;
     }
     case tid__obj: {
          u64                deleted_len    = 0;
          u8           const fix_idx_offset = obj__get_fix_idx_offset(arg);
          const t_any* const items          = obj__get_fields(arg);
          u64          const items_len      = (hash_map__get_last_idx(arg) + 1) * hash_map__get_chunk_size(arg) *  2;
          u64          const fields_len = obj__get_fields_len(arg);
          if (fix_idx_offset != 120) for (u64 idx = 0; idx < items_len; deleted_len += items[idx].bytes[15] == tid__holder, idx += 2);

          bytes__prepare_to_write(thrd_data, bytes, fields_len * 16 + deleted_len * 15 + 11, owner);
          if (fix_idx_offset == 120 || (fields_len | deleted_len) == 0) {
               bytes->bytes[bytes->len] = st__obj;
               bytes->len              += 1;
          } else {
               *(ua_u32*)&bytes->bytes[bytes->len] = st__fix_obj | (u32)fix_idx_offset << 8 | (u32)obj__get_fix_idx_size(arg) << 16;
               bytes->len                         += 3;
          }
          bytes__write_len(bytes, fields_len + deleted_len);

          for (u64 idx = 0; idx < items_len; idx += 2) {
               t_any key     = items[idx];
               bool  deleted = false;
               switch (key.bytes[15]) {
               case tid__holder:
                    if (fix_idx_offset == 120) continue;
                    deleted = true;
               case tid__token:
                    bytes__prepare_to_write(thrd_data, bytes, 32, owner);

                    key.bytes[15] = tid__token;
                    token_to_ascii_chars(key, &token_len, token_chars);
                    if (deleted) {
                         token_chars[token_len] = (u8)128;
                         token_len              += 1;
                    } else token_chars[token_len - 1] |= 128;
                    memcpy_inline(&bytes->bytes[bytes->len], token_chars, 32);
                    bytes->len += token_len;

                    if (!(deleted || serialize(thrd_data, bytes, items[idx + 1], owner))) [[clang::unlikely]] return false;

                    break;
               case tid__null:
                    continue;
               default:
                    unreachable;
               }
          }

          return true;
     }
     case tid__table: {
          u64 remain_qty = hash_map__get_len(arg);

          bytes__prepare_to_write(thrd_data, bytes, remain_qty * 2 + 9, owner);
          bytes->bytes[bytes->len] = st__table;
          bytes->len              += 1;
          bytes__write_len(bytes, remain_qty);
          remain_qty *= 2;

          const t_any* const items = table__get_kvs(arg);
          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) {
                    idx += 1;
                    continue;
               }

               remain_qty -= 1;
               if (!serialize(thrd_data, bytes, item, owner)) [[clang::unlikely]] return false;
          }

          return true;
     }
     case tid__set: {
          u64 remain_qty = hash_map__get_len(arg);

          bytes__prepare_to_write(thrd_data, bytes, remain_qty + 9, owner);
          bytes->bytes[bytes->len] = st__set;
          bytes->len              += 1;
          bytes__write_len(bytes, remain_qty);

          const t_any* const items = set__get_items(arg);
          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               if (!serialize(thrd_data, bytes, item, owner)) [[clang::unlikely]] return false;
          }

          return true;
     }
     case tid__byte_array: {
          u32       const slice_offset = slice__get_offset(arg);
          u32       const slice_len    = slice__get_len(arg);
          u64       const array_len    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(arg);
          const u8* const array_bytes  = &((const u8*)slice_array__get_items(arg))[slice_offset];

          assert(slice_len <= slice_max_len || slice_offset == 0);

          bytes__prepare_to_write(thrd_data, bytes, array_len + 9, owner);
          bytes->bytes[bytes->len] = st__byte_array;
          bytes->len              += 1;
          bytes__write_len(bytes, array_len);
          memcpy(&bytes->bytes[bytes->len], array_bytes, array_len);
          bytes->len += array_len;
          return true;
     }
     case tid__string: {
          u32       const slice_offset = slice__get_offset(arg);
          u32       const slice_len    = slice__get_len(arg);
          u64       const string_len   = slice_len <= slice_max_len ? slice_len : slice_array__get_len(arg);
          u64             string_size  = string_len * 3;
          const u8*       chars        = &((const u8*)slice_array__get_items(arg))[slice_offset * 3];

          assert(slice_len <= slice_max_len || slice_offset == 0);

          bytes__prepare_to_write(thrd_data, bytes, string_len + 2, owner);
          bytes->bytes[bytes->len] = st__string;
          bytes->len              += 1;

          while (true) {
               t_str_cvt_result const cvt_result = corsar_chars_to_ctf8_chars(string_size, chars, bytes->cap - bytes->len, &bytes->bytes[bytes->len]);
               assert((i64)cvt_result.src_offset >= 0);

               string_size -= cvt_result.src_offset;
               bytes->len  += cvt_result.dst_offset;

               if (string_size == 0) {
                    bytes__prepare_to_write(thrd_data, bytes, 1, owner);
                    bytes->bytes[bytes->len] = 0;
                    bytes->len              += 1;
                    return true;
               }

               chars              = &chars[cvt_result.src_offset];
               u8 const char_size = char_size_in_ctf8(char_to_code(chars));
               bytes__prepare_to_write(thrd_data, bytes, char_size * 3 >= string_size ? char_size : string_size / 3, owner);
          }
     }
     case tid__array: {
          u32          const slice_offset = slice__get_offset(arg);
          u32          const slice_len    = slice__get_len(arg);
          u64          const array_len    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(arg);
          const t_any* const items        = &((const t_any*)slice_array__get_items(arg))[slice_offset];

          bytes__prepare_to_write(thrd_data, bytes, array_len + 9, owner);
          bytes->bytes[bytes->len] = st__array;
          bytes->len              += 1;
          bytes__write_len(bytes, array_len);

          for (u64 idx = 0; idx < array_len; idx += 1)
               if (!serialize(thrd_data, bytes, items[idx], owner)) [[clang::unlikely]] return false;

          return true;
     }

     [[clang::unlikely]]
     default:
          return false;
     }
}

core t_any McoreFNserialize(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.serialize";

     t_any const arg   = args[0];
     t_any const bytes = args[1];
     if (arg.bytes[15] == tid__error || !(bytes.bytes[15] == tid__short_byte_array || bytes.bytes[15] == tid__byte_array)) [[clang::unlikely]] {
          if (arg.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, bytes);
               return arg;
          }

          if (bytes.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, arg);
               return bytes;
          }

          goto invalid_type_label;
     }

     t_serialized_bytes serialized_bytes;
     if (bytes.bytes[15] == tid__short_byte_array) {
          u8 const len = short_byte_array__get_len(bytes);
          serialized_bytes.len   = 16 + len;
          serialized_bytes.cap   = 64;
          serialized_bytes.bytes = malloc(64);
          memcpy_inline(&serialized_bytes.bytes[16], bytes.bytes, 16);
     } else if (get_ref_cnt(bytes) == 1) {
          serialized_bytes.len   = slice_array__get_len(bytes) + 16;
          serialized_bytes.cap   = slice_array__get_cap(bytes) + 16;
          serialized_bytes.bytes = (u8*)bytes.qwords[0];
     } else {
          u32       const slice_offset = slice__get_offset(bytes);
          u32       const slice_len    = slice__get_len(bytes);
          u64       const array_len    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(bytes);
          const u8* const array_bytes  = &((const u8*)slice_array__get_items(bytes))[slice_offset];

          serialized_bytes.len   = array_len + 16;
          serialized_bytes.cap   = array_len * 3 / 2 + 16;
          serialized_bytes.bytes = malloc(serialized_bytes.cap);

          memcpy(&serialized_bytes.bytes[16], array_bytes, array_len);
     }

     call_stack__push(thrd_data, owner);
     bool const is_serialized = serialize(thrd_data, &serialized_bytes, arg, owner);
     call_stack__pop(thrd_data);

     ref_cnt__dec(thrd_data, arg);

     if (!is_serialized) [[clang::unlikely]] {
          free(serialized_bytes.bytes);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     if (serialized_bytes.len < 31) {
          t_any result;
          memcpy_inline(result.bytes, &serialized_bytes.bytes[serialized_bytes.len - 16], 16);
          free(serialized_bytes.bytes);
          result.raw_bits >>= (32 - serialized_bytes.len) * 8;
          result.bytes[15]  = tid__short_byte_array;
          result            = short_byte_array__set_len(result, serialized_bytes.len - 16);
          return result;
     }

     u64 const result_len = serialized_bytes.len - 16;
     t_any     result     = {.structure = {.value = (u64)serialized_bytes.bytes, .type = tid__byte_array}};
     result               = slice__set_metadata(result, 0, result_len <= slice_max_len ? result_len : slice_max_len + 1);
     set_ref_cnt(result, 1);
     slice_array__set_len(result, result_len);
     slice_array__set_cap(result, serialized_bytes.cap - 16);
     return result;

     invalid_type_label:
     ref_cnt__dec(thrd_data, arg);
     ref_cnt__dec(thrd_data, bytes);

     call_stack__push(thrd_data, owner);
     t_any const error = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return error;
}