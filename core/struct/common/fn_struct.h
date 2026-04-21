#pragma once

#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../struct/error/fn.h"
#include "../../struct/fn/basic.h"

[[clang::always_inline]]
static u8 common_fn__get_params_len(t_any const fn) {
     assert(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);

     u8 const len = fn.bytes[8] & 0x1f;

     assume(len <= 16);

     return len;
}

[[clang::always_inline]]
static t_any common_fn__set_params_len(t_any fn, u8 const len) {
     assert(fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn);
     assume(len <= 16);

     fn.bytes[8] = fn.bytes[8] & 0xe0 | len;
     return fn;
}

core_basic t_any common_fn__partial_app__own(t_thrd_data* const thrd_data, t_any const fn, const t_any* const args, u8 const args_len, const char* const owner) {
     assume(args_len <= 16);

     if (!((fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn) && common_fn__get_params_len(fn) == args_len)) [[clang::unlikely]] {
          for (u8 idx = 0; idx < args_len; ref_cnt__dec(thrd_data, args[idx]), idx += 1);

          if (fn.bytes[15] == tid__error)
               return fn;

          ref_cnt__dec(thrd_data, fn);

          if (fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn)
               return error__wrong_num_fn_args(thrd_data, owner);

          return error__invalid_type(thrd_data, owner);
     }

     u16 args_holders_mask = 0;
     for (u8 idx = 0; idx < args_len; idx += 1) {
          if (!type_is_common_or_null(args[idx].bytes[15])) [[clang::unlikely]] {
               if (args[idx].bytes[15] == tid__error) {
                    for (u8 free_idx = 0; free_idx < idx; ref_cnt__dec(thrd_data, args[free_idx]), free_idx += 1);
                    for (u8 free_idx = idx + 1; free_idx < args_len; ref_cnt__dec(thrd_data, args[free_idx]), free_idx += 1);
                    return args[idx];
               }

               for (u8 free_idx = 0; free_idx < args_len; ref_cnt__dec(thrd_data, args[free_idx]), free_idx += 1);
               return error__invalid_type(thrd_data, owner);
          }

          args_holders_mask |= (u16)(args[idx].bytes[15] == tid__holder) << idx;
     }

     u8 holders_qty = __builtin_popcount(args_holders_mask);
     if (holders_qty == args_len) return fn;

     if (fn.bytes[15] == tid__short_fn) {
          t_any* const result_data = aligned_alloc(16, (args_len + 1) *  16);
          *result_data             = (const t_any){.qwords = {1, fn.qwords[0]}};
          memcpy_le_256(&result_data[1], args, args_len * 16);
          t_any result = (const t_any){.structure = {.value = (u64)result_data, .type = tid__fn}};
          result       = fn__set_metadata(result, holders_qty, args_len, 0, false, args_holders_mask);
          return result;
     }

     u8     const params_cap = fn__get_params_cap(fn);
     u64    const ref_cnt    = fn__get_ref_cnt(thrd_data, fn);
     t_any* const args_in_fn = fn__get_args(thrd_data, fn);
     if (ref_cnt == 1) {
          u16 holders_mask = fn__get_holders_mask(fn);
          u8  arg_idx      = 0;
          for (u8 arg_in_fn_idx = 0; arg_in_fn_idx < params_cap; arg_in_fn_idx += 1) {
               if (args_in_fn[arg_in_fn_idx].bytes[15] == tid__holder) {
                    t_any const arg           = args[arg_idx];
                    args_in_fn[arg_in_fn_idx] = arg;
                    arg_idx                  += 1;
                    holders_mask             ^= (u16)(arg.bytes[15] != tid__holder) << arg_in_fn_idx;
               }
          }

          return common_fn__set_params_len(fn__set_holders_mask(fn, holders_mask), holders_qty);
     }

     if (ref_cnt != 0) fn__set_ref_cnt(thrd_data, fn, ref_cnt - 1);

     u32    const borrowed_len = fn__get_borrowed_len(fn);
     t_any* const result_data  = aligned_alloc(16, (params_cap + borrowed_len + 1) *  16);
     *result_data              = (const t_any){.qwords = {1, fn__get_address(thrd_data, fn)->bits}};

     u8  arg_idx      = 0;
     u16 holders_mask = 0;
     for (u8 arg_in_fn_idx = 0; arg_in_fn_idx < params_cap; arg_in_fn_idx += 1) {
          t_any const arg_in_fn = args_in_fn[arg_in_fn_idx];
          if (arg_in_fn.bytes[15] == tid__holder) {
               t_any const arg                = args[arg_idx];
               result_data[arg_in_fn_idx + 1] = arg;
               arg_idx                       += 1;
               holders_mask                  |= (u16)(arg.bytes[15] == tid__holder) << arg_in_fn_idx;
          } else {
               result_data[arg_in_fn_idx + 1] = arg_in_fn;
               ref_cnt__inc(thrd_data, arg_in_fn, owner);
          }
     }

     const t_any* const borrowed_in_fn     = fn__get_borrowed(thrd_data, fn);
     t_any*       const borrowed_in_result = &result_data[params_cap + 1];
     for (u32 idx = 0; idx < borrowed_len; idx += 1) {
          t_any const borrowed_item = borrowed_in_fn[idx];
          borrowed_in_result[idx]   = borrowed_item;
          ref_cnt__inc(thrd_data, borrowed_item, owner);
     }

     t_any result = (const t_any){.structure = {.value = (u64)result_data, .type = tid__fn}};
     result       = fn__set_metadata(result, holders_qty, params_cap, borrowed_len, false, holders_mask);
     return result;
}

[[clang::always_inline]]
core_basic t_any common_fn__call__own(t_thrd_data* const thrd_data, t_any const fn, const t_any* const args, u8 const args_len, const char* const owner) {
     assume(args_len <= 16);

     if (!((fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn) && common_fn__get_params_len(fn) == args_len)) [[clang::unlikely]] {
          for (u8 idx = 0; idx < args_len; ref_cnt__dec(thrd_data, args[idx]), idx += 1);

          if (fn.bytes[15] == tid__error)
               return fn;

          ref_cnt__dec(thrd_data, fn);

          if (fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn)
               return error__wrong_num_fn_args(thrd_data, owner);

          return error__invalid_type(thrd_data, owner);
     }

     if (fn.bytes[15] == tid__short_fn)
          return ((t_any (*)(t_thrd_data* const, const t_any* const))fn.qwords[0])(thrd_data, args);

     t_any              args_buffer[16];
     u64          const ref_cnt      = fn__get_ref_cnt(thrd_data, fn);
     const t_any* const borrowed     = fn__get_borrowed(thrd_data, fn);
     void*        const fn_address   = fn__get_address(thrd_data, fn)->pointer;
     u32          const borrowed_len = fn__get_borrowed_len(fn);
     u8           const params_cap   = fn__get_params_cap(fn);

     thrd_data->borrowed_data = borrowed;

     if (params_cap == args_len) {
          assume(borrowed_len != 0);

          if (ref_cnt > 1) {
               for (u32 idx = 0; idx < borrowed_len; ref_cnt__inc(thrd_data, borrowed[idx], owner), idx += 1);

               t_any const call_result = ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, args);
               ref_cnt__dec(thrd_data, fn);

               return call_result;
          }

          if (ref_cnt == 0)
               return ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, args);

          t_any const call_result = ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, args);
          if (fn__is_linear_alloc(fn))
               linear__free(&thrd_data->linear_allocator, fn.qwords[0]);
          else free((void*)fn.qwords[0]);

          return call_result;
     }

     const t_any* const args_in_fn = fn__get_args(thrd_data, fn);
     if (params_cap == 2 && args_len == 1 && borrowed_len == 0) {
          t_any const first_in_fn = args_in_fn[0];
          if (ref_cnt > 1) {
               if (first_in_fn.bytes[15] == tid__holder) {
                    t_any const second_arg = args_in_fn[1];
                    thrd_data->fns_args[0] = args[0];
                    thrd_data->fns_args[1] = second_arg;
                    ref_cnt__inc(thrd_data, second_arg, owner);
               } else {
                    thrd_data->fns_args[1] = args[0];
                    thrd_data->fns_args[0] = first_in_fn;
                    ref_cnt__inc(thrd_data, first_in_fn, owner);
               }

               fn__set_ref_cnt(thrd_data, fn, ref_cnt - 1);
          } else if (ref_cnt == 0) {
               if (first_in_fn.bytes[15] == tid__holder) {
                    t_any const second_arg = args_in_fn[1];
                    thrd_data->fns_args[0] = args[0];
                    thrd_data->fns_args[1] = second_arg;
               } else {
                    thrd_data->fns_args[1] = args[0];
                    thrd_data->fns_args[0] = first_in_fn;
               }
          } else {
               if (first_in_fn.bytes[15] == tid__holder) {
                    t_any const second_arg = args_in_fn[1];
                    thrd_data->fns_args[0] = args[0];
                    thrd_data->fns_args[1] = second_arg;
               } else {
                    thrd_data->fns_args[1] = args[0];
                    thrd_data->fns_args[0] = first_in_fn;
               }

               if (fn__is_linear_alloc(fn))
                    linear__free(&thrd_data->linear_allocator, fn.qwords[0]);
               else free((void*)fn.qwords[0]);
          }

          return ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, thrd_data->fns_args);
     }

     if (ref_cnt > 1) {
          for (u32 idx = 0; idx < borrowed_len; ref_cnt__inc(thrd_data, borrowed[idx], owner), idx += 1);

          u8 arg_idx = 0;
          for (u8 arg_in_fn_idx = 0; arg_in_fn_idx < params_cap; arg_in_fn_idx += 1) {
               t_any const arg_in_fn = args_in_fn[arg_in_fn_idx];
               if (arg_in_fn.bytes[15] == tid__holder) {
                    args_buffer[arg_in_fn_idx] = args[arg_idx];
                    arg_idx                   += 1;
               } else {
                    args_buffer[arg_in_fn_idx] = arg_in_fn;
                    ref_cnt__inc(thrd_data, arg_in_fn, owner);
               }
          }

          t_any const call_result = ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, args_buffer);
          ref_cnt__dec(thrd_data, fn);

          return call_result;
     }

     if (ref_cnt == 0) {
          u8 arg_idx = 0;
          for (u8 arg_in_fn_idx = 0; arg_in_fn_idx < params_cap; arg_in_fn_idx += 1) {
               t_any const arg_in_fn = args_in_fn[arg_in_fn_idx];
               if (arg_in_fn.bytes[15] == tid__holder) {
                    args_buffer[arg_in_fn_idx] = args[arg_idx];
                    arg_idx                   += 1;
               } else args_buffer[arg_in_fn_idx] = arg_in_fn;
          }

          return ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, args_buffer);
     }

     u16 holders_mask = fn__get_holders_mask(fn);
     for (u8 arg_idx = 0; arg_idx < args_len; arg_idx += 1) {
          u8 const holder_offset              = __builtin_ctzs(holders_mask);
          holders_mask                       ^= (u16)1 << holder_offset;
          ((t_any*)args_in_fn)[holder_offset] = args[arg_idx];
     }

     t_any const call_result = ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, args_in_fn);
     if (fn__is_linear_alloc(fn))
          linear__free(&thrd_data->linear_allocator, fn.qwords[0]);
     else free((void*)fn.qwords[0]);

     return call_result;
}

[[clang::always_inline]]
core_basic t_any common_fn__call__half_own(t_thrd_data* const thrd_data, t_any const fn, const t_any* const args, u8 const args_len, const char* const owner) {
     assume(args_len <= 16);

     if (!((fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn) && common_fn__get_params_len(fn) == args_len)) [[clang::unlikely]] {
          for (u8 idx = 0; idx < args_len; ref_cnt__dec(thrd_data, args[idx]), idx += 1);

          if (fn.bytes[15] == tid__error)
               return fn;

          if (fn.bytes[15] == tid__short_fn || fn.bytes[15] == tid__fn)
               return error__wrong_num_fn_args(thrd_data, owner);

          return error__invalid_type(thrd_data, owner);
     }

     if (fn.bytes[15] == tid__short_fn)
          return ((t_any (*)(t_thrd_data* const, const t_any* const))fn.qwords[0])(thrd_data, args);

     t_any              args_buffer[16];
     u64          const ref_cnt      = fn__get_ref_cnt(thrd_data, fn);
     const t_any* const borrowed     = fn__get_borrowed(thrd_data, fn);
     void*        const fn_address   = fn__get_address(thrd_data, fn)->pointer;
     u32          const borrowed_len = fn__get_borrowed_len(fn);
     u8           const params_cap   = fn__get_params_cap(fn);

     thrd_data->borrowed_data = borrowed;

     if (params_cap == args_len) {
          assume(borrowed_len != 0);

          if (ref_cnt != 0) for (u32 idx = 0; idx < borrowed_len; ref_cnt__inc(thrd_data, borrowed[idx], owner), idx += 1);

          return ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, args);
     }

     const t_any* const args_in_fn = fn__get_args(thrd_data, fn);
     if (params_cap == 2 && args_len == 1 && borrowed_len == 0) {
          t_any const first_in_fn = args_in_fn[0];
          if (ref_cnt != 0) {
               if (first_in_fn.bytes[15] == tid__holder) {
                    t_any const second_arg = args_in_fn[1];
                    thrd_data->fns_args[0] = args[0];
                    thrd_data->fns_args[1] = second_arg;
                    ref_cnt__inc(thrd_data, second_arg, owner);
               } else {
                    thrd_data->fns_args[1] = args[0];
                    thrd_data->fns_args[0] = first_in_fn;
                    ref_cnt__inc(thrd_data, first_in_fn, owner);
               }
          } else if (first_in_fn.bytes[15] == tid__holder) {
               t_any const second_arg = args_in_fn[1];
               thrd_data->fns_args[0] = args[0];
               thrd_data->fns_args[1] = second_arg;
          } else {
               thrd_data->fns_args[1] = args[0];
               thrd_data->fns_args[0] = first_in_fn;
          }

          return ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, thrd_data->fns_args);
     }

     if (ref_cnt != 0) {
          for (u32 idx = 0; idx < borrowed_len; ref_cnt__inc(thrd_data, borrowed[idx], owner), idx += 1);

          u8 arg_idx = 0;
          for (u8 arg_in_fn_idx = 0; arg_in_fn_idx < params_cap; arg_in_fn_idx += 1) {
               t_any const arg_in_fn = args_in_fn[arg_in_fn_idx];
               if (arg_in_fn.bytes[15] == tid__holder) {
                    args_buffer[arg_in_fn_idx] = args[arg_idx];
                    arg_idx                   += 1;
               } else {
                    args_buffer[arg_in_fn_idx] = arg_in_fn;
                    ref_cnt__inc(thrd_data, arg_in_fn, owner);
               }
          }
     } else {
          u8 arg_idx = 0;
          for (u8 arg_in_fn_idx = 0; arg_in_fn_idx < params_cap; arg_in_fn_idx += 1) {
               t_any const arg_in_fn = args_in_fn[arg_in_fn_idx];
               if (arg_in_fn.bytes[15] == tid__holder) {
                    args_buffer[arg_in_fn_idx] = args[arg_idx];
                    arg_idx                   += 1;
               } else args_buffer[arg_in_fn_idx] = arg_in_fn;
          }
     }

     return ((t_any (*)(t_thrd_data* const, const t_any* const))fn_address)(thrd_data, args_buffer);
}
