#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/byte_array/basic.h"
#include "../struct/error/basic.h"
#include "../struct/null/basic.h"

[[clang::always_inline]]
core t_any McoreFNwrite_to_memAAA(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.write_to_mem!!!";

     t_any const address = args[0];
     t_any const offset  = args[1];
     t_any const bytes   = args[2];
     if (offset.bytes[15] != tid__int || !(bytes.bytes[15] == tid__short_byte_array || bytes.bytes[15] == tid__byte_array)) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, address);

          if (offset.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, bytes);
               return offset;
          }

          ref_cnt__dec(thrd_data, offset);
          if (bytes.bytes[15] == tid__error)
               return bytes;

          ref_cnt__dec(thrd_data, bytes);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     if (bytes.bytes[15] == tid__short_byte_array) {
          memcpy_le_16((void*)(address.qwords[0] + offset.qwords[0]), bytes.bytes, short_byte_array__get_len(bytes));

          ref_cnt__dec(thrd_data, address);
          return null;
     }

     u64 const ref_cnt   = get_ref_cnt(bytes);
     u64 const array_len = slice_array__get_len(bytes);
     if (ref_cnt > 1) set_ref_cnt(bytes, ref_cnt - 1);

     u32       const slice_offset = slice__get_offset(bytes);
     u32       const slice_len    = slice__get_len(bytes);
     const u8* const raw_bytes    = &((const u8*)slice_array__get_items(bytes))[slice_offset];
     u64       const len          = slice_len <= slice_max_len ? slice_len : array_len;

     u64 const address_with_offset = address.qwords[0] + offset.qwords[0];
     if ((address_with_offset >= (u64)raw_bytes && address_with_offset < (u64)raw_bytes + len) || (address_with_offset + len >= (u64)raw_bytes && address_with_offset < (u64)raw_bytes))
          memmove((void*)address_with_offset, raw_bytes, len);
     else memcpy((void*)address_with_offset, raw_bytes, len);

     if (ref_cnt == 1) free((void*)bytes.qwords[0]);
     ref_cnt__dec(thrd_data, address);

     return null;
}