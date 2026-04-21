#pragma once

#include "../common/fn.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"

[[clang::noinline]]
core t_any McoreFNexpected_error(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.expected_error";

     t_any const id   = args[0];
     t_any const msg  = args[1];
     t_any const data = args[2];

     switch (id.bytes[15]) {
     [[clang::likely]]   case tid__token:
          break;
     [[clang::unlikely]] case tid__error:
          ref_cnt__dec(thrd_data, msg);
          ref_cnt__dec(thrd_data, data);
          return id;
     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     switch (msg.bytes[15]) {
     [[clang::likely]]   case tid__short_string:
     [[clang::likely]]   case tid__string:
          break;
     [[clang::unlikely]] case tid__error:
          ref_cnt__dec(thrd_data, data);
          return msg;
     [[clang::unlikely]] default:
          goto invalid_type_label;
     }

     if (!type_is_common_or_null(data.bytes[15])) [[clang::unlikely]] {
          if (data.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, msg);
               return data;
          }

          goto invalid_type_label;
     }

     t_error* const error = aligned_alloc(16, sizeof(t_error));
     *error = (const t_error) {
          .ref_cnt  = 1,
          .owner      = ptr_to_ptr64(owner),
          .id         = id,
          .msg        = msg,
          .data       = data,
     };

#ifdef CALL_STACK
     error->call_stack__len = thrd_data->call_stack.len;
     error->call_stack      = malloc(error->call_stack__len * sizeof(char*));
     memcpy(error->call_stack, thrd_data->call_stack.stack, thrd_data->call_stack.len * sizeof(char*));
#endif

     return (const t_any){.structure = {.value = (u64)error, .type = tid__error}};

     invalid_type_label:
     ref_cnt__dec(thrd_data, id);
     ref_cnt__dec(thrd_data, msg);
     ref_cnt__dec(thrd_data, data);

     call_stack__push(thrd_data, owner);
     t_any const result = error__invalid_type(thrd_data, owner);
     call_stack__pop(thrd_data);

     return result;
}
