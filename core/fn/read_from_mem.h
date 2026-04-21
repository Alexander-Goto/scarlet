#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/byte_array/basic.h"
#include "../struct/error/basic.h"

[[clang::always_inline]]
core t_any McoreFNread_from_memAAA(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.read_from_mem!!!";

     t_any const address = args[0];
     t_any const offset  = args[1];
     t_any const count   = args[2];
     if (offset.bytes[15] != tid__int || count.bytes[15] != tid__int || count.qwords[0] > array_max_len) [[clang::unlikely]] {
          ref_cnt__dec(thrd_data, address);

          if (offset.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, count);
               return offset;
          }

          if (count.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, offset);
               return count;
          }

          ref_cnt__dec(thrd_data, offset);
          ref_cnt__dec(thrd_data, count);

          t_any result;
          call_stack__push(thrd_data, owner);
          if (offset.bytes[15] != tid__int || count.bytes[15] != tid__int)
               result = error__invalid_type(thrd_data, owner);
          else result = error__out_of_bounds(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     if (count.qwords[0] < 15) {
          t_any result = {.bytes = {[14] = count.bytes[0], [15] = tid__short_byte_array}};
          memcpy_le_16(result.bytes, (void*)(address.qwords[0] + offset.qwords[0]), count.qwords[0]);

          ref_cnt__dec(thrd_data, address);

          return result;
     }

     t_any result = long_byte_array__new(count.qwords[0]);
     slice_array__set_len(result, count.qwords[0]);
     result       = slice__set_metadata(result, 0, count.qwords[0] <= slice_max_len ? count.qwords[0] : slice_max_len + 1);
     memcpy(slice_array__get_items(result), (void*)(address.qwords[0] + offset.qwords[0]), count.qwords[0]);

     ref_cnt__dec(thrd_data, address);

     return result;
}