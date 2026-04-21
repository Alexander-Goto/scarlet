#pragma once

#include "../app/env/var.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../common/xxh3.h"
#include "../struct/array/fn.h"
#include "../struct/common/slice.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/obj/mtd.h"
#include "../struct/set/fn.h"
#include "../struct/table/fn.h"

[[gnu::hot, clang::noinline]]
static t_any hash(t_thrd_data* const thrd_data, t_any const arg, u64 const seed, const char* const owner) {
     switch (arg.bytes[15]) {
     case tid__short_string: case tid__short_byte_array:
          return (const t_any){.structure = {.value = xxh3_get_hash(arg.bytes, 16, seed ^ arg.bytes[15]), .type = tid__int}};
     case tid__token:
          return (const t_any){.structure = {.value = token__hash(arg, seed), .type = tid__int}};
     case tid__bool:
          return (const t_any){.structure = {.value = __builtin_rotateleft64(arg.bytes[0] == 1 ? seed : (u64)~seed, (seed & 7) | 1), .type = tid__int}};
     case tid__byte:
          return (const t_any){.structure = {.value = xxh3_get_hash(arg.bytes, 1, seed), .type = tid__int}};
     case tid__char:
          return (const t_any){.structure = {.value = xxh3_get_hash(arg.bytes, 3, seed), .type = tid__int}};
     case tid__int: case tid__time:
          return (const t_any){.structure = {.value = xxh3_get_hash(arg.bytes, 8, seed ^ arg.bytes[15]), .type = tid__int}};
     case tid__obj:
          return obj__hash(thrd_data, arg, seed, owner);
     case tid__table:
          return table__hash(thrd_data, arg, seed, owner);
     case tid__set:
          return set__hash(thrd_data, arg, seed, owner);
     case tid__byte_array: case tid__string: {
          u32 const slice_offset = slice__get_offset(arg);
          u32 const slice_len    = slice__get_len(arg);
          u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(arg);

          assert(slice_len <= slice_max_len || slice_offset == 0);

          u8* const bytes     = &((u8*)slice_array__get_items(arg))[arg.bytes[15] == tid__string ? slice_offset * 3 : slice_offset];
          u64 const bytes_len = arg.bytes[15] == tid__string ? len * 3 : len;
          return (const t_any){.structure = {.value = xxh3_get_hash(bytes, bytes_len, seed), .type = tid__int}};
     }
     case tid__array:
          return array__hash(thrd_data, arg, seed, owner);
     case tid__custom:
          ref_cnt__inc(thrd_data, arg, owner);

          thrd_data->fns_args[0] = arg;
          thrd_data->fns_args[1] = (const t_any){.structure = {.value = seed, .type = tid__int}};

          return custom__call__own(thrd_data, mtd__core_hash, tid__int, arg, thrd_data->fns_args, 2, owner);

     [[clang::unlikely]] default:
          return error__invalid_type(thrd_data, owner);
     }
}

core t_any McoreFNhash(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.hash";
     t_any const arg  = args[0];
     t_any const seed = args[1];

     if (arg.bytes[15] == tid__error || seed.bytes[15] != tid__int) [[clang::unlikely]] {
          if (arg.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, seed);
               return arg;
          }

          if (seed.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, arg);
               return seed;
          }

          goto invalid_type_label;
     }

     t_any result;
     switch (arg.bytes[15]) {
     case tid__short_string: case tid__short_byte_array:
          return (const t_any){.structure = {.value = xxh3_get_hash(arg.bytes, 16, seed.qwords[0] ^ arg.bytes[15]), .type = tid__int}};
     case tid__token:
          return (const t_any){.structure = {.value = token__hash(arg, seed.qwords[0]), .type = tid__int}};
     case tid__bool:
          return (const t_any){.structure = {.value = __builtin_rotateleft64(arg.bytes[0] == 1 ? seed.qwords[0] : (u64)~seed.qwords[0], (seed.qwords[0] & 7) | 1), .type = tid__int}};
     case tid__byte:
          return (const t_any){.structure = {.value = xxh3_get_hash(arg.bytes, 1, seed.qwords[0]), .type = tid__int}};
     case tid__char:
          return (const t_any){.structure = {.value = xxh3_get_hash(arg.bytes, 3, seed.qwords[0]), .type = tid__int}};
     case tid__int: case tid__time:
          return (const t_any){.structure = {.value = xxh3_get_hash(arg.bytes, 8, seed.qwords[0] ^ arg.bytes[15]), .type = tid__int}};
     case tid__obj: {
          u64 const ref_cnt = get_ref_cnt(arg);
          if (ref_cnt > 1) set_ref_cnt(arg, ref_cnt - 1);

          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = obj__hash(thrd_data, arg, seed.qwords[0], owner);
          call_stack__pop(thrd_data);

          if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, arg);
          break;
     }
     case tid__table: {
          u64 const ref_cnt = get_ref_cnt(arg);
          if (ref_cnt > 1) set_ref_cnt(arg, ref_cnt - 1);

          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = table__hash(thrd_data, arg, seed.qwords[0], owner);
          call_stack__pop(thrd_data);

          if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, arg);
          break;
     }
     case tid__set: {
          u64 const ref_cnt = get_ref_cnt(arg);
          if (ref_cnt > 1) set_ref_cnt(arg, ref_cnt - 1);

          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = set__hash(thrd_data, arg, seed.qwords[0], owner);
          call_stack__pop(thrd_data);

          if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, arg);
          break;
     }
     case tid__byte_array: case tid__string: {
          u32 const slice_offset = slice__get_offset(arg);
          u32 const slice_len    = slice__get_len(arg);
          u64 const ref_cnt      = get_ref_cnt(arg);
          u64 const array_len    = slice_array__get_len(arg);
          u64 const len          = slice_len <= slice_max_len ? slice_len : array_len;

          assert(slice_len <= slice_max_len || slice_offset == 0);

          if (ref_cnt > 1) set_ref_cnt(arg, ref_cnt - 1);

          u8* const bytes     = &((u8*)slice_array__get_items(arg))[arg.bytes[15] == tid__string ? slice_offset * 3 : slice_offset];
          u64 const bytes_len = arg.bytes[15] == tid__string ? len * 3 : len;
          result = (const t_any){.structure = {.value = xxh3_get_hash(bytes, bytes_len, seed.qwords[0]), .type = tid__int}};

          if (ref_cnt == 1) free((void*)arg.qwords[0]);
          break;
     }
     case tid__array: {
          u64 const ref_cnt = get_ref_cnt(arg);
          if (ref_cnt > 1) set_ref_cnt(arg, ref_cnt - 1);

          call_stack__push(thrd_data, owner);
          [[clang::noinline]] result = array__hash(thrd_data, arg, seed.qwords[0], owner);
          call_stack__pop(thrd_data);

          if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, arg);

          break;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          result = custom__call__own(thrd_data, mtd__core_hash, tid__int, arg, args, 2, owner);
          call_stack__pop(thrd_data);

          break;
     }

     case tid__error:
          unreachable;
     [[clang::unlikely]]
     default:
          goto invalid_type_label;
     }

     return result;

     invalid_type_label:
     ref_cnt__dec(thrd_data, arg);
     ref_cnt__dec(thrd_data, seed);

     call_stack__push(thrd_data, owner);
     result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
