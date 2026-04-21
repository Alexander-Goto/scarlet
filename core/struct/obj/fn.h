#pragma once

#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../common/xxh3.h"
#include "../array/basic.h"
#include "../common/fn_struct.h"
#include "../common/hash_map.h"
#include "../error/fn.h"
#include "../holder/basic.h"
#include "../token/fn.h"
#include "basic.h"
#include "mtd.h"

[[clang::always_inline]]
static t_any* obj__key_ptr(t_any const obj, t_any const key) {
     assume(obj.bytes[15] == tid__obj);
     assume(key.bytes[15] == tid__token);
     assert(key.raw_bits != mtds_token.raw_bits);

     u8     const fix_idx_offset = obj__get_fix_idx_offset(obj);
     t_any* const items          = obj__get_fields(obj);

     if (fix_idx_offset != 120) {
          u8     const fix_idx_size = obj__get_fix_idx_size(obj);
          u8     const idx          = (u64)(key.raw_bits >> fix_idx_offset) & (((u64)1 << fix_idx_size) - 1);
          t_any* const key_ptr      = &items[idx * 2];
          return (key_ptr->raw_bits & (((u128)1 << 120) - 1)) == (key.raw_bits & (((u128)1 << 120) - 1)) ? key_ptr : nullptr;
     }

     u64 const last_idx     = hash_map__get_last_idx(obj);
     u16 const chunk_size   = hash_map__get_chunk_size(obj);
     u64 const items_len    = (last_idx + 1) * chunk_size * 2;
     u64 const first_idx    = (token__hash(key, hash_map__get_hash_seed(obj)) & last_idx) * chunk_size * 2;
     u64       remain_steps = items_len / 2;

     u64 idx = first_idx;

     assert(idx < items_len);

     t_any* result = nullptr;
     while (true) {
          t_any* const key_ptr = &items[idx];
          if (key_ptr->bytes[15] == tid__null) {
               if (result == nullptr)
                    return items_len != (hash_map_max_chunks * hash_map_max_chunk_size * 2) && items_len > 8 && (obj__get_fields_len(obj) * 5 / 4) * 2 > items_len ? nullptr : key_ptr;

               return result;
          }
          if (key_ptr->raw_bits == key.raw_bits) return key_ptr;
          if (key_ptr->bytes[15] == tid__holder && result == nullptr) result = key_ptr;

          remain_steps -= 1;
          idx          += 2;
          idx           = idx == items_len ? 0 : idx;

          if (remain_steps == 0) return result;
     }
}

static void obj__unpack__own(t_thrd_data* const thrd_data, t_any obj, u64 const unpacking_items_len, t_any* const unpacking_items, const char* const owner) {
     if (obj.bytes[15] != tid__obj) {
          if (!(obj.bytes[15] == tid__error || obj.bytes[15] == tid__null)) {
               ref_cnt__dec(thrd_data, obj);
               obj = error__invalid_type(thrd_data, owner);
          }

          for (u64 idx = 0; idx < unpacking_items_len; idx += 1) {
               t_any const key = unpacking_items[idx];
               if (key.bytes[15] != tid__error) [[clang::likely]] {
                    ref_cnt__inc(thrd_data, obj, owner);
                    ref_cnt__dec(thrd_data, key);
                    unpacking_items[idx] = obj;
               }
          }

          ref_cnt__dec(thrd_data, obj);
          return;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     for (u64 idx = 0; idx < unpacking_items_len; idx += 1) {
          t_any const key = unpacking_items[idx];
          if (unpacking_items[idx].bytes[15] == tid__token) [[clang::likely]] {
               const t_any* const key_in_obj_ptr = obj__key_ptr(obj, key);
               if (key_in_obj_ptr == nullptr || key_in_obj_ptr->bytes[15] != tid__token) unpacking_items[idx].bytes[15] = tid__null;
               else {
                    t_any const value = key_in_obj_ptr[1];
                    ref_cnt__inc(thrd_data, value, owner);
                    unpacking_items[idx] = value;
               }
          } else if (unpacking_items[idx].bytes[15] != tid__error) {
               ref_cnt__dec(thrd_data, key);
               unpacking_items[idx] = error__invalid_type(thrd_data, owner);
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, obj);
}

[[clang::noinline]]
static t_any obj__try_call__own(t_thrd_data* const thrd_data, t_any const mtd_tkn, u8 const result_type, t_any const obj, const t_any* const args, u8 const args_len, const char* const owner, bool* const called) {
     assert(mtd_tkn.bytes[15] == tid__token);
     assert(type_is_correct(result_type));
     assert(obj.bytes[15] == tid__obj);

     *called          = false;
     t_any const mtds = *obj__get_mtds(obj);
     if (mtds.bytes[15] != tid__obj) return null;

     const t_any* const mtd_key_ptr = obj__key_ptr(mtds, mtd_tkn);
     if (mtd_key_ptr != nullptr) {
          t_any const mtd = mtd_key_ptr[1];
          if (mtd.bytes[15] == tid__short_fn || mtd.bytes[15] == tid__fn) {
               t_any const mtd_result = common_fn__call__half_own(thrd_data, mtd, args, args_len, owner);
               *called                = true;
               if (mtd_result.bytes[15] == result_type) [[clang::likely]] return mtd_result;
               if (mtd_result.bytes[15] == tid__error) return mtd_result;

               ref_cnt__dec(thrd_data, mtd_result);
               return error__invalid_type(thrd_data, owner);
          }
     }

     return null;
}

[[clang::noinline]]
static t_any obj__try_call__any_result__own(t_thrd_data* const thrd_data, t_any const mtd_tkn, t_any const obj, const t_any* const args, u8 const args_len, const char* const owner, bool* const called) {
     assert(mtd_tkn.bytes[15] == tid__token);
     assert(obj.bytes[15] == tid__obj);

     *called          = false;
     t_any const mtds = *obj__get_mtds(obj);
     if (mtds.bytes[15] != tid__obj) return null;

     const t_any* const mtd_key_ptr = obj__key_ptr(mtds, mtd_tkn);
     if (mtd_key_ptr != nullptr) {
          t_any const mtd = mtd_key_ptr[1];
          if (mtd.bytes[15] == tid__short_fn || mtd.bytes[15] == tid__fn) {
               *called = true;
               return common_fn__call__half_own(thrd_data, mtd, args, args_len, owner);
          }
     }

     return null;
}

[[clang::noinline]]
static t_any obj__call__own(t_thrd_data* const thrd_data, t_any const mtd_tkn, u8 const result_type, t_any const obj, const t_any* const args, u8 const args_len, const char* const owner) {
     assert(mtd_tkn.bytes[15] == tid__token);
     assert(type_is_correct(result_type));
     assert(obj.bytes[15] == tid__obj);

     t_any const mtds = *obj__get_mtds(obj);
     if (mtds.bytes[15] == tid__obj) [[clang::likely]] {
          const t_any* const mtd_key_ptr = obj__key_ptr(mtds, mtd_tkn);
          if (mtd_key_ptr != nullptr) [[clang::likely]] {
               t_any const mtd = mtd_key_ptr[1];
               if (mtd.bytes[15] == tid__short_fn || mtd.bytes[15] == tid__fn) [[clang::likely]] {
                    t_any const mtd_result = common_fn__call__half_own(thrd_data, mtd, args, args_len, owner);
                    if (mtd_result.bytes[15] == result_type) [[clang::likely]] return mtd_result;
                    if (mtd_result.bytes[15] == tid__error) return mtd_result;

                    ref_cnt__dec(thrd_data, mtd_result);
                    goto invalid_type_label;
               }
          }
     }

     for (u8 idx = 0; idx < args_len; ref_cnt__dec(thrd_data, args[idx]), idx += 1);
     invalid_type_label:
     return error__invalid_type(thrd_data, owner);
}

[[clang::noinline]]
static t_any obj__call__any_result__own(t_thrd_data* const thrd_data, t_any const mtd_tkn, t_any const obj, const t_any* const args, u8 const args_len, const char* const owner) {
     assert(mtd_tkn.bytes[15] == tid__token);
     assert(obj.bytes[15] == tid__obj);

     t_any const mtds = *obj__get_mtds(obj);
     if (mtds.bytes[15] == tid__obj) [[clang::likely]] {
          const t_any* const mtd_key_ptr = obj__key_ptr(mtds, mtd_tkn);
          if (mtd_key_ptr != nullptr) [[clang::likely]] {
               t_any const mtd = mtd_key_ptr[1];
               if (mtd.bytes[15] == tid__short_fn || mtd.bytes[15] == tid__fn) [[clang::likely]]
                    return common_fn__call__half_own(thrd_data, mtd, args, args_len, owner);
          }
     }

     for (u8 idx = 0; idx < args_len; ref_cnt__dec(thrd_data, args[idx]), idx += 1);
     return error__invalid_type(thrd_data, owner);
}

[[clang::always_inline]]
static t_any obj__get_field__own(t_thrd_data* const thrd_data, t_any const obj, t_any const name, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assert(name.bytes[15] == tid__token);

     if (name.raw_bits == mtds_token.raw_bits) {
          t_any const mtds = *obj__get_mtds(obj);
          ref_cnt__inc(thrd_data, mtds, owner);
          ref_cnt__dec(thrd_data, obj);
          return mtds;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     const t_any* const kv_ptr = obj__key_ptr(obj, name);
     t_any              value;
     if (kv_ptr == nullptr || kv_ptr->bytes[15] != tid__token) value.bytes[15] = tid__null;
     else {
          value = kv_ptr[1];
          ref_cnt__inc(thrd_data, value, owner);
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, obj);

     return value;
}

static t_any obj__get_item__own(t_thrd_data* const thrd_data, t_any const obj, t_any const key, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = key;

          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core___get_item__, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) {
               if (!type_is_common_or_null_or_error(result.bytes[15])) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, result);
                    return error__invalid_type(thrd_data, owner);
               }

               return result;
          }
     }

     if (key.bytes[15] != tid__token) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, key);
          return error__invalid_type(thrd_data, owner);
     }

     return obj__get_field__own(thrd_data, obj, key, owner);
}

[[clang::noinline]]
static t_any obj__all__own(t_thrd_data* const thrd_data, t_any const obj, t_any const default_result, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = default_result;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_all, tid__bool, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0) set_ref_cnt(obj, ref_cnt - 1);

     t_any              result     = bool__true;
     const t_any* const fields = obj__get_fields(obj);
     u64                idx        = 0;
     for (; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty -= 1;

          t_any const field = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, field, owner);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &field, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 0) {
               result = bool__false;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;

               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

[[clang::noinline]]
static t_any obj__all_kv__own(t_thrd_data* const thrd_data, t_any const obj, t_any const default_result, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = default_result;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_all_kv, tid__bool, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0) set_ref_cnt(obj, ref_cnt - 1);

     const t_any* const fields = obj__get_fields(obj);
     t_any              result     = bool__true;
     u64                idx        = 0;
     for (; remain_qty != 0; idx += 2) {
          const t_any* const kv_ptr = &fields[idx];
          if (kv_ptr->bytes[15] != tid__token) continue;

          remain_qty -= 1;

          if (ref_cnt > 1) ref_cnt__inc(thrd_data, kv_ptr[1], owner);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv_ptr, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 0) {
               result = bool__false;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;

               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

[[clang::noinline]]
static t_any obj__any__own(t_thrd_data* const thrd_data, t_any const obj, t_any const default_result, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = default_result;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_any, tid__bool, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0) set_ref_cnt(obj, ref_cnt - 1);

     t_any              result     = bool__false;
     const t_any* const fields = obj__get_fields(obj);
     u64                idx        = 0;
     for (; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty -= 1;

          t_any const field = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, field, owner);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &field, 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 1) {
               result = bool__true;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;

               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

[[clang::noinline]]
static t_any obj__any_kv__own(t_thrd_data* const thrd_data, t_any const obj, t_any const default_result, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assert(default_result.bytes[15]==tid__bool);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = default_result;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_any_kv, tid__bool, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, fn);
          return default_result;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0) set_ref_cnt(obj, ref_cnt - 1);

     t_any              result     = bool__false;
     const t_any* const fields = obj__get_fields(obj);
     u64                idx        = 0;
     for (; remain_qty != 0; idx += 2) {
          const t_any* const kv_ptr = &fields[idx];
          if (kv_ptr->bytes[15] != tid__token) continue;

          remain_qty -= 1;

          if (ref_cnt > 1) ref_cnt__inc(thrd_data, kv_ptr[1], owner);

          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv_ptr, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (fn_result.bytes[15] == tid__error)
                    result = fn_result;
               else {
                    ref_cnt__dec(thrd_data, fn_result);
                    result = error__invalid_type(thrd_data, owner);
               }

               break;
          }

          if (fn_result.bytes[0] == 1) {
               result = bool__true;
               break;
          }
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;

               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return result;
}

[[clang::noinline]]
static t_any obj__clear__own(t_thrd_data* const thrd_data, t_any const obj, const char* const owner) {
     assert(obj.bytes[15] == tid__obj);

     {
          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_clear, tid__obj, obj, &obj, 1, owner, &called);
          if (called) return result;
     }

     t_any* const mtds             = obj__get_mtds(obj);
     bool   const new_hash_map_len = mtds->bytes[15] == tid__null ? 0 : 1;
     t_any* const items            = obj__get_fields(obj);
     u64    const ref_cnt          = get_ref_cnt(obj);
     if (ref_cnt == 1) {
          u64 remain_qty = obj__get_fields_len(obj);
          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               t_any* const kv_ptr = &items[idx];
               if (kv_ptr->bytes[15] == tid__null || kv_ptr->bytes[15] == tid__holder) continue;

               remain_qty -= 1;

               ref_cnt__dec(thrd_data, kv_ptr[1]);
               kv_ptr[0].bytes[15] = tid__null;
               kv_ptr[1].bytes[15] = tid__null;
          }

          return hash_map__set_len(obj, new_hash_map_len);
     }

     if (ref_cnt != 0) set_ref_cnt(obj, ref_cnt - 1);
     u64 const items_len = (hash_map__get_last_idx(obj) + 1) * hash_map__get_chunk_size(obj);

     ref_cnt__inc(thrd_data, *mtds, owner);
     u8    const fix_idx_offset = obj__get_fix_idx_offset(obj);
     t_any       new_obj;
     if (fix_idx_offset == 120) {
          new_obj                 = obj__init(thrd_data, items_len, owner);
          *obj__get_mtds(new_obj) = *mtds;
     } else {
          new_obj                 = obj__init_fix(fix_idx_offset, obj__get_fix_idx_size(obj));
          *obj__get_mtds(new_obj) = *mtds;
          t_any* const new_items  = obj__get_fields(new_obj);
          for (u64 idx = 0; idx < items_len * 2; idx += 2) {
               t_any key                    = items[idx];
               key.bytes[15]                = tid__null;
               new_items[idx]               = key;
               new_items[idx + 1].bytes[15] = tid__null;
          }
     }

     return hash_map__set_len(new_obj, new_hash_map_len);
}

[[gnu::hot, clang::noinline]]
static t_any equal(t_thrd_data* const thrd_data, t_any const left, t_any const right, const char* const owner);

static t_any obj__cmp(t_thrd_data* const thrd_data, t_any const obj_1,  t_any const obj_2, const char* const owner) {
     assert(obj_1.bytes[15] == tid__obj);
     assert(obj_2.bytes[15] == tid__obj);

     u64 remain_qty = hash_map__get_len(obj_1);

     t_any const mtds = *obj__get_mtds(obj_1);
     if (mtds.bytes[15] == tid__obj) {
          remain_qty -= 1;
          const t_any* const mtd_key_ptr = obj__key_ptr(mtds, mtd__core_cmp);
          if (mtd_key_ptr != nullptr) {
               t_any const mtd = mtd_key_ptr[1];
               if (mtd.bytes[15] == tid__short_fn || mtd.bytes[15] == tid__fn) {
                    ref_cnt__inc(thrd_data, obj_1, owner);
                    ref_cnt__inc(thrd_data, obj_2, owner);

                    thrd_data->fns_args[0] = obj_1;
                    thrd_data->fns_args[1] = obj_2;

                    t_any const mtd_result = common_fn__call__half_own(thrd_data, mtd, thrd_data->fns_args, 2, owner);

                    if (
                         !(
                              mtd_result.raw_bits == tkn__equal.raw_bits     ||
                              mtd_result.raw_bits == tkn__great.raw_bits     ||
                              mtd_result.raw_bits == tkn__less.raw_bits      ||
                              mtd_result.raw_bits == tkn__nan_cmp.raw_bits   ||
                              mtd_result.raw_bits == tkn__not_equal.raw_bits
                         )
                    ) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The 'core.cmp' function returned a value other than ':equal', ':not_equal', ':less', ':great', ':nan_cmp'.", owner);

                    return mtd_result;
               }
          }
     }

     if (remain_qty != obj__get_fields_len(obj_2)) return tkn__not_equal;
     if (remain_qty == 0) return tkn__equal;

     t_any small;
     t_any big;

     if ((hash_map__get_last_idx(obj_1) + 1) * hash_map__get_chunk_size(obj_1) < (hash_map__get_last_idx(obj_2) + 1) * hash_map__get_chunk_size(obj_2)) {
          small = obj_1;
          big   = obj_2;
     } else {
          small = obj_2;
          big   = obj_1;
     }

     const t_any* const small_fields = obj__get_fields(small);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const field_name = small_fields[idx];
          if (field_name.bytes[15] != tid__token) continue;

          remain_qty -= 1;
          const t_any* const key_ptr = obj__key_ptr(big, field_name);
          if (key_ptr == nullptr || key_ptr->bytes[15] != tid__token) return tkn__not_equal;
          t_any const eq_result = equal(thrd_data, small_fields[idx + 1], key_ptr[1], owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) return tkn__not_equal;
     }

     return tkn__equal;
}

static t_any obj__equal(t_thrd_data* const thrd_data, t_any const left,  t_any const right, const char* const owner) {
     assert(left.bytes[15] == tid__obj);
     assert(right.bytes[15] == tid__obj);

     u64 remain_qty = hash_map__get_len(left);

     t_any const mtds = *obj__get_mtds(left);
     if (mtds.bytes[15] == tid__obj) {
          remain_qty                    -= 1;
          const t_any* const mtd_key_ptr = obj__key_ptr(mtds, mtd__core___equal__);
          if (mtd_key_ptr != nullptr) {
               t_any const mtd = mtd_key_ptr[1];
               if (mtd.bytes[15] == tid__short_fn || mtd.bytes[15] == tid__fn) {
                    ref_cnt__inc(thrd_data, left, owner);
                    ref_cnt__inc(thrd_data, right, owner);

                    thrd_data->fns_args[0] = left;
                    thrd_data->fns_args[1] = right;

                    t_any const mtd_result = common_fn__call__half_own(thrd_data, mtd, thrd_data->fns_args, 2, owner);

                    if (mtd_result.bytes[15] != tid__bool) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The 'core.(===)' function returned a value other than 'true', 'false'.", owner);

                    return mtd_result;
               }
          }
     }

     if (remain_qty != obj__get_fields_len(right)) return bool__false;
     if (remain_qty == 0) return bool__true;

     t_any small;
     t_any big;

     if ((hash_map__get_last_idx(left) + 1) * hash_map__get_chunk_size(left) < (hash_map__get_last_idx(right) + 1) * hash_map__get_chunk_size(right)) {
          small = left;
          big   = right;
     } else {
          small = right;
          big   = left;
     }

     const t_any* const small_fields = obj__get_fields(small);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const field_name = small_fields[idx];
          if (field_name.bytes[15] != tid__token) continue;

          remain_qty                -= 1;
          const t_any* const key_ptr = obj__key_ptr(big, field_name);
          if (key_ptr == nullptr || key_ptr->bytes[15] != tid__token) return bool__false;
          t_any const eq_result = equal(thrd_data, small_fields[idx + 1], key_ptr[1], owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) return eq_result;
     }

     return bool__true;
}

[[clang::noinline]]
static t_any obj__enlarge__own(t_thrd_data* const thrd_data, t_any const obj, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     t_any*       const mtds       = obj__get_mtds(obj);
     const t_any* const fields = &mtds[1];
     u64                remain_qty = obj__get_fields_len(obj);
     t_any              new_obj    = obj__init(thrd_data, (hash_map__get_last_idx(obj) + 1) * hash_map__get_chunk_size(obj) + 1, owner);
     hash_map__set_hash_seed(new_obj, hash_map__get_hash_seed(obj));

     if (mtds->bytes[15] != tid__null) {
          *obj__get_mtds(new_obj) = *mtds;
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, *mtds, owner);
          new_obj = hash_map__set_len(new_obj, 1);
     }

     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const name = fields[idx];
          if (name.bytes[15] != tid__token) continue;

          remain_qty         -= 1;
          t_any* const kv_ptr = obj__key_ptr(new_obj, name);

          assume(kv_ptr != nullptr);
          assert(kv_ptr->bytes[15] == tid__null);

          t_any const field = fields[idx + 1];
          kv_ptr[0]            = name;
          kv_ptr[1]            = field;
          new_obj              = hash_map__set_len(new_obj, hash_map__get_len(new_obj) + 1);
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, field, owner);
     }

     if (ref_cnt == 1) free((void*)obj.qwords[0]);

     return new_obj;
}

[[clang::noinline]]
static t_any obj__dup__own(t_thrd_data* const thrd_data, t_any const obj, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     u64 const items_len = (hash_map__get_last_idx(obj) + 1) * hash_map__get_chunk_size(obj) * 2 + 1;
     u64 const mem_size  = (items_len + 1) * 16;
     t_any     new_obj   = obj;

     new_obj.qwords[0]     = (u64)aligned_alloc(16, mem_size);
     const t_any* old_data = (const t_any*)obj.qwords[0];
     t_any*       new_data = (t_any*)new_obj.qwords[0];

     new_data->raw_bits = old_data->raw_bits;
     set_ref_cnt(new_obj, 1);

     const t_any* old_items = &old_data[1];
     t_any*       new_items = &new_data[1];

     u64 const ref_cnt = get_ref_cnt(obj);

     assert(ref_cnt != 1);

     if (ref_cnt == 0)
          memcpy(new_items, old_items, mem_size - 16);
     else {
          set_ref_cnt(obj, ref_cnt - 1);

          for (u64 idx = 0; idx < items_len; idx += 1) {
               t_any const item = old_items[idx];
               new_items[idx]   = item;
               ref_cnt__inc(thrd_data, item, owner);
          }
     }

     return new_obj;
}

[[clang::noinline]]
static t_any obj__each_to_each__own(t_thrd_data* const thrd_data, t_any obj, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_each_to_each, tid__obj, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 remain_qty_1 = obj__get_fields_len(obj);
     if (remain_qty_1 < 2) {
          ref_cnt__dec(thrd_data, fn);
          return obj;
     }

     if (get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);

     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[2];
     for (u64 idx_1 = 0; remain_qty_1 != 1; idx_1 += 2) {
          t_any const name_1 = fields[idx_1];
          if (name_1.bytes[15] != tid__token) continue;

          fn_args[0] = fields[idx_1 + 1];
          remain_qty_1 -= 1;

          u64 remain_qty_2 = remain_qty_1;
          for (u64 idx_2 = idx_1 + 2; remain_qty_2 != 0; idx_2 += 2) {
               t_any const name_2 = fields[idx_2];
               if (name_2.bytes[15] != tid__token) continue;

               fn_args[1] = fields[idx_2 + 1];
               remain_qty_2 -= 1;

               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    fields[idx_1 + 1].raw_bits = 0;
                    fields[idx_2 + 1].raw_bits = 0;
                    ref_cnt__dec(thrd_data, obj);
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || !type_is_common(box_items[0].bytes[15]) || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
                    fields[idx_1 + 1].raw_bits = 0;
                    fields[idx_2 + 1].raw_bits = 0;
                    ref_cnt__dec(thrd_data, obj);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[0]            = box_items[0];
               fields[idx_2 + 1] = box_items[1];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          fields[idx_1 + 1] = fn_args[0];
     }
     ref_cnt__dec(thrd_data, fn);

     return obj;
}

[[clang::noinline]]
static t_any obj__each_to_each_kv__own(t_thrd_data* const thrd_data, t_any obj, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_each_to_each_kv, tid__obj, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 remain_qty_1 = obj__get_fields_len(obj);
     if (remain_qty_1 < 2) {
          ref_cnt__dec(thrd_data, fn);
          return obj;
     }

     if (get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);

     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[4];
     for (u64 idx_1 = 0; remain_qty_1 != 1; idx_1 += 2) {
          fn_args[0] = fields[idx_1];
          if (fn_args[0].bytes[15] != tid__token) continue;

          fn_args[1] = fields[idx_1 + 1];
          remain_qty_1 -= 1;

          u64 remain_qty_2 = remain_qty_1;
          for (u64 idx_2 = idx_1 + 2; remain_qty_2 != 0; idx_2 += 2) {
               fn_args[2] = fields[idx_2];
               if (fn_args[2].bytes[15] != tid__token) continue;

               fn_args[3] = fields[idx_2 + 1];
               remain_qty_2 -= 1;

               t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 4, owner);
               if (box.bytes[15] != tid__box) [[clang::unlikely]] {
                    fields[idx_1 + 1].raw_bits = 0;
                    fields[idx_2 + 1].raw_bits = 0;
                    ref_cnt__dec(thrd_data, obj);
                    ref_cnt__dec(thrd_data, fn);
                    if (box.bytes[15] == tid__error) return box;

                    ref_cnt__dec(thrd_data, box);
                    return error__invalid_type(thrd_data, owner);
               }

               u8           const box_len   = box__get_len(box);
               const t_any* const box_items = box__get_items(box);
               if (box_len < 2 || !type_is_common(box_items[0].bytes[15]) || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
                    fields[idx_1 + 1].raw_bits = 0;
                    fields[idx_2 + 1].raw_bits = 0;
                    ref_cnt__dec(thrd_data, obj);
                    ref_cnt__dec(thrd_data, fn);
                    ref_cnt__dec(thrd_data, box);
                    return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
               }

               if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

               fn_args[1]            = box_items[0];
               fields[idx_2 + 1] = box_items[1];
               for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
          }

          fields[idx_1 + 1] = fn_args[1];
     }
     ref_cnt__dec(thrd_data, fn);

     return obj;
}

static t_any obj__builtin_unsafe_add_kv__own(t_thrd_data* const thrd_data, t_any obj, t_any const field_name, t_any const field, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(field_name.bytes[15] == tid__token);
     assert(type_is_common(field.bytes[15]));

     if (get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);

     if (field_name.raw_bits == mtds_token.raw_bits) {
          assume(field.bytes[15] == tid__obj);

          t_any* const mtds = obj__get_mtds(obj);
          if (mtds->bytes[15] != tid__null) [[clang::unlikely]] goto not_uniq_key__label;

          *mtds = field;
          obj   = hash_map__set_len(obj, hash_map__get_len(obj) + 1);

          return obj;
     }

#ifndef NDEBUG
     bool first_time = true;
#endif
     while (true) {
          t_any* const kv_ptr = obj__key_ptr(obj, field_name);
          if (kv_ptr != nullptr) {
               if (kv_ptr->bytes[15] == tid__token) [[clang::unlikely]] goto not_uniq_key__label;

               kv_ptr[0] = field_name;
               kv_ptr[1] = field;
               obj       = hash_map__set_len(obj, hash_map__get_len(obj) + 1);

               return obj;
          }

          obj = obj__enlarge__own(thrd_data, obj, owner);

#ifndef NDEBUG
          assert(first_time);
          first_time = false;
#endif
     }

     not_uniq_key__label:
     ref_cnt__dec__noinlined_part(thrd_data, obj);
     ref_cnt__dec(thrd_data, field);
     return error__not_uniq_key(thrd_data, owner);
}

static t_any obj__builtin_add_kv__own(t_thrd_data* const thrd_data, t_any obj, t_any const field_name, t_any const field, const char* const owner) {
     if (obj.bytes[15] != tid__obj || field_name.bytes[15]!=tid__token || !type_is_common(field.bytes[15]) || (field_name.raw_bits == mtds_token.raw_bits && field.bytes[15] != tid__obj)) [[clang::unlikely]] {
          if (obj.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, field_name);
               ref_cnt__dec(thrd_data, field);
               return obj;
          }
          ref_cnt__dec(thrd_data, obj);

          if (field_name.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, field);
               return field_name;
          }
          ref_cnt__dec(thrd_data, field_name);

          if (field.bytes[15] == tid__error) return field;
          ref_cnt__dec(thrd_data, field);

          return error__invalid_type(thrd_data, owner);
     }

     return obj__builtin_unsafe_add_kv__own(thrd_data, obj, field_name, field, owner);
}

[[clang::noinline]]
static t_any obj__fit__own(t_thrd_data* const thrd_data, t_any const obj, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assert(obj__get_fix_idx_offset(obj) == 120);

     u64    const ref_cnt    = get_ref_cnt(obj);
     t_any* const mtds       = obj__get_mtds(obj);
     t_any* const fields     = &mtds[1];
     u64          remain_qty = obj__get_fields_len(obj);

     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     if (remain_qty == 0) {
          t_any new_obj;
          if (mtds->bytes[15] == tid__null) new_obj = empty_obj;
          else {
               new_obj = obj__init(thrd_data, 1, owner);
               new_obj = hash_map__set_len(new_obj, 1);
               *obj__get_mtds(new_obj) = *mtds;
               if (ref_cnt > 1) ref_cnt__inc(thrd_data, *mtds, owner);
          }

          if (ref_cnt == 1) free((void*)obj.qwords[0]);

          return new_obj;
     }

     t_any new_obj = obj__init(thrd_data, remain_qty, owner);
     if (mtds->bytes[15] != tid__null) {
          *obj__get_mtds(new_obj) = *mtds;
          new_obj                 = hash_map__set_len(new_obj, 1);
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, *mtds, owner);
     }

     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const name = fields[idx];
          if (name.bytes[15] != tid__token) continue;

          remain_qty         -= 1;
          t_any* const kv_ptr = obj__key_ptr(new_obj, name);

          assume(kv_ptr != nullptr);
          assert(kv_ptr->bytes[15] == tid__null);

          new_obj = hash_map__set_len(new_obj, hash_map__get_len(new_obj) + 1);

          t_any const field = fields[idx + 1];
          kv_ptr[0]            = name;
          kv_ptr[1]            = field;

          if (ref_cnt > 1) ref_cnt__inc(thrd_data, field, owner);
     }

     if (ref_cnt == 1) free((void*)obj.qwords[0]);

     return new_obj;
}

[[clang::noinline]]
static t_any obj__filter__own(t_thrd_data* const thrd_data, t_any obj, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_filter, tid__obj, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64       remain_qty = obj__get_fields_len(obj);
     t_any*    fields = obj__get_fields(obj);
     u64 const len        = hash_map__get_len(obj);
     u64       new_len    = len;
     bool      dst_is_mut = get_ref_cnt(obj) == 1;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any* kv_ptr = &fields[idx];
          if (kv_ptr->bytes[15] != tid__token) continue;

          remain_qty -= 1;
          ref_cnt__inc(thrd_data, kv_ptr[1], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, &kv_ptr[1], 1, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_is_mut) obj = hash_map__set_len(obj, new_len);
               ref_cnt__dec(thrd_data, obj);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) {
               if (!dst_is_mut) {
                    obj        = obj__dup__own(thrd_data, obj, owner);
                    fields = obj__get_fields(obj);
                    dst_is_mut = true;
               }

               kv_ptr = &fields[idx];
               ref_cnt__dec(thrd_data, kv_ptr[1]);
               new_len            -= 1;
               kv_ptr[0].bytes[15] = tid__holder;
               kv_ptr[1].bytes[15] = tid__holder;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (new_len == len) return obj;
     obj = hash_map__set_len(obj, new_len);

     if (obj__need_to_fit(obj)) obj = obj__fit__own(thrd_data, obj, owner);
     return obj;
}

[[clang::noinline]]
static t_any obj__filter_kv__own(t_thrd_data* const thrd_data, t_any obj, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_filter_kv, tid__obj, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64       remain_qty = obj__get_fields_len(obj);
     t_any*    fields = obj__get_fields(obj);
     u64 const len        = hash_map__get_len(obj);
     u64       new_len    = len;
     bool      dst_is_mut = get_ref_cnt(obj) == 1;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any* kv_ptr = &fields[idx];
          if (kv_ptr->bytes[15] != tid__token) continue;

          remain_qty -= 1;
          ref_cnt__inc(thrd_data, kv_ptr[1], owner);
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv_ptr, 2, owner);
          if (fn_result.bytes[15] != tid__bool) [[clang::unlikely]] {
               if (dst_is_mut) obj = hash_map__set_len(obj, new_len);
               ref_cnt__dec(thrd_data, obj);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          if (fn_result.bytes[0] == 0) {
               if (!dst_is_mut) {
                    obj        = obj__dup__own(thrd_data, obj, owner);
                    fields = obj__get_fields(obj);
                    dst_is_mut = true;
               }

               kv_ptr = &fields[idx];
               ref_cnt__dec(thrd_data, kv_ptr[1]);
               kv_ptr[0].bytes[15] = tid__holder;
               kv_ptr[1].bytes[15] = tid__holder;
               new_len            -= 1;
          }
     }
     ref_cnt__dec(thrd_data, fn);

     if (new_len == len) return obj;
     obj = hash_map__set_len(obj, new_len);

     if (obj__need_to_fit(obj)) obj = obj__fit__own(thrd_data, obj, owner);
     return obj;
}

[[clang::noinline]]
static t_any obj__foldl__own(t_thrd_data* const thrd_data, t_any const obj, t_any const state, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = state;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_foldl, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0 && state.qwords[0] != obj.qwords[0]) set_ref_cnt(obj, ref_cnt - 1);

     u64          remain_qty = obj__get_fields_len(obj);
     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[2];
     fn_args[0]              = state;
     u64          idx        = 0;
     for (; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty -= 1;

          fn_args[1]  = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == obj.qwords[0]) ref_cnt__dec(thrd_data, obj);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

[[clang::noinline]]
static t_any obj__foldl1__own(t_thrd_data* const thrd_data, t_any const obj, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = fn;

          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_foldl1, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0) set_ref_cnt(obj, ref_cnt - 1);

     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[2];
     u64          idx        = 0;
     for (;; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty -= 1;
          fn_args[0]  = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[0], owner);
          break;
     }

     for (idx += 2; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty -= 1;

          fn_args[1] = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

[[clang::noinline]]
static t_any obj__foldl_kv__own(t_thrd_data* const thrd_data, t_any const obj, t_any const state, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = state;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_foldl_kv, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0 && state.qwords[0] != obj.qwords[0]) set_ref_cnt(obj, ref_cnt - 1);

     u64          remain_qty = obj__get_fields_len(obj);
     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[3];
     fn_args[0]              = state;
     u64          idx        = 0;
     for (; remain_qty != 0; idx += 2) {
          fn_args[1] = fields[idx];
          if (fn_args[1].bytes[15] != tid__token) continue;

          remain_qty -= 1;

          fn_args[2]  = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[2], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == obj.qwords[0]) ref_cnt__dec(thrd_data, obj);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

[[clang::noinline]]
static t_any obj__foldr__own(t_thrd_data* const thrd_data, t_any const obj, t_any const state, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = state;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_foldr, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0 && state.qwords[0] != obj.qwords[0]) set_ref_cnt(obj, ref_cnt - 1);

     u64          remain_qty = obj__get_fields_len(obj);
     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[2];
     fn_args[0]              = state;
     u64          idx        = 0;
     for (; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty -= 1;

          fn_args[1]  = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == obj.qwords[0]) ref_cnt__dec(thrd_data, obj);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

[[clang::noinline]]
static t_any obj__foldr1__own(t_thrd_data* const thrd_data, t_any const obj, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = fn;

          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_foldr1, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, fn);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0) set_ref_cnt(obj, ref_cnt - 1);

     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[2];
     u64          idx        = 0;
     for (;; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty -= 1;
          fn_args[0]  = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[0], owner);
          break;
     }

     for (idx += 2; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty -= 1;

          fn_args[1] = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[1], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     }

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

[[clang::noinline]]
static t_any obj__foldr_kv__own(t_thrd_data* const thrd_data, t_any const obj, t_any const state, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = state;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_foldr_kv, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));
     else if (ref_cnt != 0 && state.qwords[0] != obj.qwords[0]) set_ref_cnt(obj, ref_cnt - 1);

     u64          remain_qty = obj__get_fields_len(obj);
     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[3];
     fn_args[0]              = state;
     u64          idx        = 0;
     for (; remain_qty != 0; idx += 2) {
          fn_args[1] = fields[idx];
          if (fn_args[1].bytes[15] != tid__token) continue;

          remain_qty -= 1;

          fn_args[2]  = fields[idx + 1];
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, fn_args[2], owner);
          fn_args[0] = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (fn_args[0].bytes[15] == tid__breaker) {
               fn_args[0] = breaker__get_result__own(thrd_data, fn_args[0]);
               break;
          }
          if (fn_args[0].bytes[15] == tid__error) [[clang::unlikely]] break;
     }

     if (ref_cnt == 1) {
          for (idx += 2; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               remain_qty -= 1;
               ref_cnt__dec(thrd_data, fields[idx + 1]);
          }

          free((void*)obj.qwords[0]);
     } else if (ref_cnt != 0 && state.qwords[0] == obj.qwords[0]) ref_cnt__dec(thrd_data, obj);

     ref_cnt__dec(thrd_data, fn);
     return fn_args[0];
}

[[gnu::hot, clang::noinline]]
static t_any hash(t_thrd_data* const thrd_data, t_any const arg, u64 seed, const char* const owner);

static t_any obj__hash(t_thrd_data* const thrd_data, t_any const obj, u64 const seed, const char* const owner) {
     assert(obj.bytes[15] == tid__obj);

     {
          ref_cnt__inc(thrd_data, obj, owner);

          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = (const t_any){.structure = {.value = seed, .type = tid__int}};

          bool called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_hash, tid__int, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;

          ref_cnt__dec(thrd_data, obj);
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) return (const t_any){.structure = {.value = xxh3_get_hash(nullptr, 0, seed ^ obj.bytes[15]), .type = tid__int}};

     const t_any* const items    = obj__get_fields(obj);
     u64                obj_hash = 0;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const field_name = items[idx];
          if (field_name.bytes[15] != tid__token) continue;

          remain_qty                    -= 1;
          u64   const field_name_hash = token__hash(field_name, seed);
          t_any const field_hash      = hash(thrd_data, items[idx + 1], field_name_hash, owner);
          if (field_hash.bytes[15] == tid__error) [[clang::unlikely]] return field_hash;

          assert(field_hash.bytes[15] == tid__int);

          obj_hash ^= field_hash.qwords[0];
     }

     return (const t_any){.structure = {.value = obj_hash, .type = tid__int}};
}

[[clang::noinline]]
static t_any obj__look_in__own(t_thrd_data* const thrd_data, t_any const obj, t_any const looked_item, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          thrd_data->fns_args[0] = looked_item;
          thrd_data->fns_args[1] = obj;

          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_look_in, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, looked_item);
          return null;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     t_any* const fields = obj__get_fields(obj);
     t_any        result     = null;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty           -= 1;
          t_any const eq_result = equal(thrd_data, fields[idx + 1], looked_item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) {
               result = fields[idx];
               break;
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, obj);
     ref_cnt__dec(thrd_data, looked_item);

     return result;
}

[[clang::noinline]]
static t_any obj__look_other_in__own(t_thrd_data* const thrd_data, t_any const obj, t_any const not_looked_item, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          thrd_data->fns_args[0] = not_looked_item;
          thrd_data->fns_args[1] = obj;

          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_look_other_in, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, not_looked_item);
          return null;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     t_any* const fields = obj__get_fields(obj);
     t_any        result     = null;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty           -= 1;
          t_any const eq_result = equal(thrd_data, fields[idx + 1], not_looked_item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 0) {
               result = fields[idx];
               break;
          }
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, obj);
     ref_cnt__dec(thrd_data, not_looked_item);

     return result;
}

[[clang::noinline]]
static t_any obj__map__own(t_thrd_data* const thrd_data, t_any obj, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_map, tid__obj, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, fn);
          return obj;
     }

     if (get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);

     t_any* const fields = obj__get_fields(obj);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty               -= 1;

          t_any* const field_ptr = &fields[idx + 1];
          t_any const fn_result     = common_fn__call__half_own(thrd_data, fn, field_ptr, 1, owner);
          if (!type_is_common(fn_result.bytes[15])) [[clang::unlikely]] {
               field_ptr->raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, obj);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          *field_ptr = fn_result;
     }

     ref_cnt__dec(thrd_data, fn);

     return obj;
}

[[clang::noinline]]
static t_any obj__map_kv__own(t_thrd_data* const thrd_data, t_any obj, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_map_kv, tid__obj, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, fn);
          return obj;
     }

     if (get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);

     t_any* const fields = obj__get_fields(obj);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any* const kv_ptr = &fields[idx];
          if (kv_ptr->bytes[15] != tid__token) continue;

          remain_qty           -= 1;
          t_any const fn_result = common_fn__call__half_own(thrd_data, fn, kv_ptr, 2, owner);
          if (!type_is_common(fn_result.bytes[15])) [[clang::unlikely]] {
               kv_ptr[1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, obj);
               ref_cnt__dec(thrd_data, fn);
               if (fn_result.bytes[15] == tid__error) return fn_result;

               ref_cnt__dec(thrd_data, fn_result);
               return error__invalid_type(thrd_data, owner);
          }

          kv_ptr[1] = fn_result;
     }

     ref_cnt__dec(thrd_data, fn);

     return obj;
}

[[clang::noinline]]
static t_any obj__map_with_state__own(t_thrd_data* const thrd_data, t_any obj, t_any const state, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = state;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_map_with_state, tid__box, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty != 0 && get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);

     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[2];
     fn_args[0]              = state;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const name = fields[idx];
          if (name.bytes[15] != tid__token) continue;

          remain_qty -= 1;

          fn_args[1]      = fields[idx + 1];
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 2, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               fields[idx + 1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, obj);
               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
               fields[idx + 1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, obj);
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]          = box_items[0];
          fields[idx + 1] = box_items[1];
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[0]           = obj;
     new_box_items[1]           = fn_args[0];

     return new_box;
}

[[clang::noinline]]
static t_any obj__map_kv_with_state__own(t_thrd_data* const thrd_data, t_any obj, t_any const state, t_any const fn, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);
     assume(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = state;
          thrd_data->fns_args[2] = fn;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_map_kv_with_state, tid__box, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty != 0 && get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);

     t_any* const fields = obj__get_fields(obj);
     t_any        fn_args[3];
     fn_args[0]              = state;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          fn_args[1] = fields[idx];
          if (fn_args[1].bytes[15] != tid__token) continue;

          remain_qty -= 1;

          fn_args[2]      = fields[idx + 1];
          t_any const box = common_fn__call__half_own(thrd_data, fn, fn_args, 3, owner);
          if (box.bytes[15] != tid__box) [[clang::unlikely]] {
               fields[idx + 1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, obj);
               ref_cnt__dec(thrd_data, fn);
               if (box.bytes[15] == tid__error) return box;

               ref_cnt__dec(thrd_data, box);
               return error__invalid_type(thrd_data, owner);
          }

          u8           const box_len   = box__get_len(box);
          const t_any* const box_items = box__get_items(box);
          if (box_len < 2 || !type_is_common(box_items[1].bytes[15])) [[clang::unlikely]] {
               fields[idx + 1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, obj);
               ref_cnt__dec(thrd_data, fn);
               ref_cnt__dec(thrd_data, box);
               return box_len >= 2 ? error__invalid_type(thrd_data, owner) : error__unpacking_not_enough_items(thrd_data, owner);
          }

          if (!box__is_global(box)) thrd_data->free_boxes ^= (u32)1 << box__get_idx(box);

          fn_args[0]          = box_items[0];
          fields[idx + 1] = box_items[1];
          for (u8 free_idx = 2; free_idx < box_len; ref_cnt__dec(thrd_data, box_items[free_idx]), free_idx += 1);
     }
     ref_cnt__dec(thrd_data, fn);

     t_any  const new_box       = box__new(thrd_data, 2, owner);
     t_any* const new_box_items = box__get_items(new_box);
     new_box_items[0]           = obj;
     new_box_items[1]           = fn_args[0];

     return new_box;
}

[[clang::noinline]]
static t_any common_print(t_thrd_data* const thrd_data, t_any const arg, const char* const owner);

[[clang::noinline]]
static t_any obj__print(t_thrd_data* const thrd_data, t_any const obj, const char* const owner) {
     assert(obj.bytes[15] == tid__obj);

     {
          bool called;
          ref_cnt__inc(thrd_data, obj, owner);
          t_any const result = obj__try_call__own(thrd_data, mtd__core_print, tid__null, obj, &obj, 1, owner, &called);
          if (called) return result;
          ref_cnt__dec(thrd_data, obj);
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          if (fwrite("[obj]", 1, 5, stdout) != 5) [[clang::unlikely]] goto cant_print_label;
          return null;
     }

     if (fwrite("[obj ", 1, 5, stdout) != 5) [[clang::unlikely]] goto cant_print_label;

     const t_any* const items = obj__get_fields(obj);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const field_name = items[idx];
          if (field_name.bytes[15] != tid__token) continue;

          remain_qty                            -= 1;
          t_any const print_field_name_result = token__print(thrd_data, field_name, owner, stdout);
          if (print_field_name_result.bytes[15] == tid__error) [[clang::unlikely]] return print_field_name_result;

          if (fwrite(" = ", 1, 3, stdout) != 3) [[clang::unlikely]] goto cant_print_label;

          t_any const print_field_result = common_print(thrd_data, items[idx + 1], owner);
          if (print_field_result.bytes[15] == tid__error) [[clang::unlikely]] return print_field_result;

          const char* str;
          int         str_len;
          if (remain_qty == 0) {
               str     = "]";
               str_len = 1;
          } else {
               str         = ", ";
               str_len     = 2;
          }

          if (fwrite(str, 1, str_len, stdout) != str_len) [[clang::unlikely]] goto cant_print_label;
     }

     return null;

     cant_print_label:
     return error__cant_print(thrd_data, owner);
}

[[clang::noinline]]
static t_any obj__qty__own(t_thrd_data* const thrd_data, t_any const obj, t_any const looked_item, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          thrd_data->fns_args[0] = looked_item;
          thrd_data->fns_args[1] = obj;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_qty, tid__int, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, obj);
          ref_cnt__dec(thrd_data, looked_item);
          return (const t_any){.bytes = {[15] = tid__int}};
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     t_any* const fields = obj__get_fields(obj);
     u64          result     = 0;
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty           -= 1;
          t_any const eq_result = equal(thrd_data, fields[idx + 1], looked_item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          result += eq_result.bytes[0];
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, obj);
     ref_cnt__dec(thrd_data, looked_item);

     return (const t_any){.structure = {.value = result, .type = tid__int}};
}

[[clang::noinline]]
static t_any obj__replace__own(t_thrd_data* const thrd_data, t_any obj, t_any const old_item, t_any const new_item, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = old_item;
          thrd_data->fns_args[2] = new_item;

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_replace, tid__obj, obj, thrd_data->fns_args, 3, owner, &called);
          if (called) return result;
     }

     u64 remain_qty = obj__get_fields_len(obj);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, old_item);
          ref_cnt__dec(thrd_data, new_item);
          return obj;
     }

     bool   is_mut     = get_ref_cnt(obj) == 1;
     t_any* fields = obj__get_fields(obj);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          if (fields[idx].bytes[15] != tid__token) continue;

          remain_qty           -= 1;
          t_any const eq_result = equal(thrd_data, fields[idx + 1], old_item, owner);

          assert(eq_result.bytes[15] == tid__bool);

          if (eq_result.bytes[0] == 1) {
               if (!is_mut) {
                    obj        = obj__dup__own(thrd_data, obj, owner);
                    fields = obj__get_fields(obj);
                    is_mut = true;
               }

               ref_cnt__inc(thrd_data, new_item, owner);
               ref_cnt__dec(thrd_data, fields[idx + 1]);
               fields[idx + 1] = new_item;
          }
     }

     ref_cnt__dec(thrd_data, old_item);
     ref_cnt__dec(thrd_data, new_item);
     return obj;
}

[[clang::noinline]]
static t_any obj__reserve__own(t_thrd_data* const thrd_data, t_any obj, u64 const new_items_qty, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          thrd_data->fns_args[0] = obj;
          thrd_data->fns_args[1] = (const t_any){.structure = {.value = new_items_qty, .type = tid__int}};

          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_reserve, tid__obj, obj, thrd_data->fns_args, 2, owner, &called);
          if (called) return result;
     }

     u64 const fields_len = obj__get_fields_len(obj);
     u64 const new_cap        = fields_len + new_items_qty;
     if (new_cap > hash_map_max_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, obj);
          return error__out_of_bounds(thrd_data, owner);
     }

     u64 const current_cap = (hash_map__get_last_idx(obj) + 1) * hash_map__get_chunk_size(obj);
     if (current_cap >= new_cap) return obj;

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     t_any*       const mtds       = obj__get_mtds(obj);
     const t_any* const fields = &mtds[1];
     u64                remain_qty = fields_len;
     t_any              new_obj    = obj__init(thrd_data, new_cap, owner);
     hash_map__set_hash_seed(new_obj, hash_map__get_hash_seed(obj));

     if (mtds->bytes[15] != tid__null) {
          *obj__get_mtds(new_obj) = *mtds;
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, *mtds, owner);
          new_obj = hash_map__set_len(new_obj, 1);
     }

     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const name = fields[idx];
          if (name.bytes[15] != tid__token) continue;

          remain_qty         -= 1;
          t_any* const kv_ptr = obj__key_ptr(new_obj, name);

          assume(kv_ptr != nullptr);
          assert(kv_ptr->bytes[15] == tid__null);

          t_any const field = fields[idx + 1];
          kv_ptr[0]            = name;
          kv_ptr[1]            = field;
          new_obj              = hash_map__set_len(new_obj, hash_map__get_len(new_obj) + 1);
          if (ref_cnt > 1) ref_cnt__inc(thrd_data, field, owner);
     }

     if (ref_cnt == 1) free((void*)obj.qwords[0]);

     return new_obj;
}

[[clang::noinline]]
static t_any obj__take_one__own(t_thrd_data* const thrd_data, t_any const obj, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_take_one, obj, &obj, 1, owner, &called);
          if (called) {
               if (!type_is_common_or_null_or_error(result.bytes[15])) [[clang::unlikely]] {
                    ref_cnt__dec(thrd_data, result);
                    return error__invalid_type(thrd_data, owner);
               }

               return result;
          }
     }

     if (obj__get_fields_len(obj) == 0) {
          ref_cnt__dec(thrd_data, obj);
          return null;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     t_any* fields = obj__get_fields(obj);
     while (true) {
          if (!(fields->bytes[15] != tid__token)) {
               t_any const result = fields[1];
               ref_cnt__inc(thrd_data, result, owner);
               if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, obj);
               return result;
          }

          fields = &fields[2];
     }
}

[[clang::noinline]]
static t_any obj__take_one_kv__own(t_thrd_data* const thrd_data, t_any const obj, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          bool        called;
          t_any const result = obj__try_call__any_result__own(thrd_data, mtd__core_take_one_kv, obj, &obj, 1, owner, &called);
          if (called) {
               if (result.bytes[15] != tid__box && result.bytes[15] != tid__null) [[clang::unlikely]] {
                    if (result.bytes[15] == tid__error) return result;

                    ref_cnt__dec(thrd_data, result);
                    return error__invalid_type(thrd_data, owner);
               }

               return result;
          }
     }

     if (obj__get_fields_len(obj) == 0) {
          ref_cnt__dec(thrd_data, obj);
          return null;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     t_any* fields = obj__get_fields(obj);
     while (true) {
          if (fields->bytes[15] == tid__token) {
               t_any  const result = box__new(thrd_data, 2, owner);
               t_any* const items  = box__get_items(result);
               memcpy_inline(items, fields, 32);
               ref_cnt__inc(thrd_data, items[1], owner);
               if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, obj);
               return result;
          }

          fields = &fields[2];
     }
}

[[clang::noinline]]
static t_any obj__to_array__own(t_thrd_data* const thrd_data, t_any const obj, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_to_array, tid__array, obj, &obj, 1, owner, &called);
          if (called) return result;
     }

     u64 const len = obj__get_fields_len(obj);
     if (len == 0) {
          ref_cnt__dec(thrd_data, obj);
          return empty_array;
     }

     t_any        array       = array__init(thrd_data, len, owner);
     array                    = slice__set_metadata(array, 0, len <= slice_max_len ? len : slice_max_len + 1);
     slice_array__set_len(array, len);
     t_any* const array_items = slice_array__get_items(array);
     u64    const ref_cnt     = get_ref_cnt(obj);
     t_any* const fields  = obj__get_fields(obj);
     u64          remain_qty  = len;
     if (ref_cnt > 1) {
          set_ref_cnt(obj, ref_cnt - 1);

          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               t_any const field = fields[idx + 1];
               ref_cnt__inc(thrd_data, field, owner);
               array_items[len - remain_qty] = field;
               remain_qty                   -= 1;
          }
     } else {
          if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));

          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               array_items[len - remain_qty] = fields[idx + 1];
               remain_qty                   -= 1;
          }

          if (ref_cnt == 1) free((void*)obj.qwords[0]);
     }

     return array;
}

[[clang::noinline]]
static t_any obj__to_box__own(t_thrd_data* const thrd_data, t_any const obj, const char* const owner) {
     assume(obj.bytes[15] == tid__obj);

     {
          bool        called;
          t_any const result = obj__try_call__own(thrd_data, mtd__core_to_box, tid__box, obj, &obj, 1, owner, &called);
          if (called) return result;
     }

     u64 const len = obj__get_fields_len(obj);
     if (len == 0) {
          ref_cnt__dec(thrd_data, obj);
          return empty_box;
     }

     if (len > 16) {
          ref_cnt__dec(thrd_data, obj);
          return error__out_of_bounds(thrd_data, owner);
     }

     t_any        box        = box__new(thrd_data, len, owner);
     t_any* const box_items  = box__get_items(box);
     u64    const ref_cnt     = get_ref_cnt(obj);
     t_any* const fields = obj__get_fields(obj);
     u64          remain_qty = len;
     if (ref_cnt > 1) {
          set_ref_cnt(obj, ref_cnt - 1);

          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               t_any const field = fields[idx + 1];
               ref_cnt__inc(thrd_data, field, owner);
               box_items[len - remain_qty] = field;
               remain_qty                   -= 1;
          }
     } else {
          if (ref_cnt == 1) ref_cnt__dec(thrd_data, *obj__get_mtds(obj));

          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               if (fields[idx].bytes[15] != tid__token) continue;

               box_items[len - remain_qty] = fields[idx + 1];
               remain_qty                   -= 1;
          }

          if (ref_cnt == 1) free((void*)obj.qwords[0]);
     }

     return box;
}

static void obj__zip_init_read(const t_any* const obj, t_zip_read_state* const state, t_any*, t_any*, u64* const result_len, t_any* const) {
     assert(obj->bytes[15] == tid__obj);

     *result_len                = obj__get_fields_len(*obj);
     state->state.item_position = obj__get_fields(*obj);
     state->state.is_mut        = get_ref_cnt(*obj) == 1;
}

static void obj__zip_read(t_thrd_data* const thrd_data, t_zip_read_state* const state, t_any* const key, t_any* const value, const char* const owner) {
     for (;
          state->state.item_position->bytes[15] != tid__token;
          state->state.item_position = &state->state.item_position[2]
     );

     *key   = state->state.item_position[0];
     *value = state->state.item_position[1];
     if (state->state.is_mut) memset_inline(state->state.item_position, tid__token, 32);
     else ref_cnt__inc(thrd_data, *value, owner);

     state->state.item_position = &state->state.item_position[2];
}

static void obj__zip_init_search(const t_any* const obj, t_zip_search_state* const state, t_any*, u64* const result_len, t_any* const) {
     assert(obj->bytes[15] == tid__obj);

     u8 const fix_idx_offset = obj__get_fix_idx_offset(*obj);
     if (fix_idx_offset == 120) {
          u64 const last_idx   = hash_map__get_last_idx(*obj);
          u16 const chunk_size = hash_map__get_chunk_size(*obj);
          u64 const items_len  = (last_idx + 1) * chunk_size * 2;

          state->hash_map_state.is_fix     = false;
          state->hash_map_state.chunk_size = chunk_size;
          state->hash_map_state.hash_seed  = hash_map__get_hash_seed(*obj);
          state->hash_map_state.last_idx   = last_idx;
          state->hash_map_state.items_len  = items_len;
          state->hash_map_state.steps      = items_len / 2;
     } else {
          state->fix_obj_state.is_fix         = true;
          state->fix_obj_state.fix_idx_offset = fix_idx_offset;
          state->fix_obj_state.fix_idx_size   = obj__get_fix_idx_size(*obj);
     }

     state->hash_map_state.items  = obj__get_fields(*obj);
     state->hash_map_state.is_mut = get_ref_cnt(*obj) == 1;

     u64 const fields_len = obj__get_fields_len(*obj);
     *result_len              = *result_len < fields_len ? *result_len : fields_len;
}

static bool obj__zip_search(t_thrd_data* const thrd_data, t_zip_search_state* const state, t_any const key, t_any* const value, const char* const owner) {
     if (key.bytes[15] != tid__token) return false;

     if (state->hash_map_state.is_fix) {
          u8     const idx          = (key.raw_bits >> state->fix_obj_state.fix_idx_offset) & (((u64)1 << state->fix_obj_state.fix_idx_size) - 1);
          t_any* const key_ptr      = &state->fix_obj_state.items[idx * 2];
          if (key_ptr->raw_bits == key.raw_bits) {
               *value = key_ptr[1];
               if (state->fix_obj_state.is_mut)
                    memset_inline(key_ptr, tid__token, 32);
               else ref_cnt__inc(thrd_data, *value, owner);

               return true;
          }
          return false;
     }

     u64    const first_idx    = (token__hash(key, state->hash_map_state.hash_seed) & state->hash_map_state.last_idx) * state->hash_map_state.chunk_size * 2;
     u64          remain_steps = state->hash_map_state.steps;
     u64          idx          = first_idx;
     t_any* const items        = state->hash_map_state.items;
     u64    const items_len    = state->hash_map_state.items_len;

     assert(idx < items_len);

     while (true) {
          t_any* const key_ptr = &items[idx];

          if (key_ptr->bytes[15] == tid__null) return false;
          if (key_ptr->raw_bits == key.raw_bits) {
               *value = key_ptr[1];
               if (state->hash_map_state.is_mut)
                    memset_inline(key_ptr, tid__token, 32);
               else ref_cnt__inc(thrd_data, *value, owner);

               return true;
          }

          remain_steps -= 1;
          idx          += 2;
          idx           = idx == items_len ? 0 : idx;

          if (remain_steps == 0) return false;
     }
}

static void obj__zip_write(t_thrd_data* const thrd_data, t_any* const obj, t_any* const, bool const, t_any const key, t_any const value, const char* const owner) {
     *obj = obj__builtin_add_kv__own(thrd_data, *obj, key, value, owner);
}
