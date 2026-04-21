#pragma once

#include "../../common/corsar.h"
#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../string/fn.h"
#include "../token/fn.h"
#include "basic.h"

static void error__unpack__own(t_thrd_data* const thrd_data, t_any error, u8 const unpacking_items_len, u8 const, t_any* const unpacking_items, const char* const owner) {
     if (error.bytes[15] != tid__error) {
          if (error.bytes[15] != tid__null) {
               ref_cnt__dec(thrd_data, error);
               error = error__invalid_type(thrd_data, owner);
               ref_cnt__add(thrd_data, error, unpacking_items_len - 1, owner);
          }

          for (u8 idx = 0; idx < unpacking_items_len; unpacking_items[idx] = error, idx += 1);
          return;
     }

     const t_any* const error_items = &((t_error*)error.qwords[0])->id;
     for (u8 unpacking_item_idx = 0; unpacking_item_idx < unpacking_items_len; unpacking_item_idx += 1) {
          u8    const error_item_idx          = unpacking_items[unpacking_item_idx].bytes[0];
          t_any const item                    = error_items[error_item_idx];
          unpacking_items[unpacking_item_idx] = item;
          ref_cnt__inc(thrd_data, item, owner);
     }

     ref_cnt__dec(thrd_data, error);
}

[[gnu::cold, clang::noinline]]
core_error t_any error__new__own(t_thrd_data* const thrd_data, const char* const owner, t_any const id, t_any const msg, t_any const data) {
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
     return error__invalid_type(thrd_data, owner);
}

[[gnu::cold, clang::noinline, noreturn]]
core_error void error__show_and_exit(t_thrd_data* const thrd_data, t_any const error_arg) {
     assert(error_arg.bytes[15] == tid__error);

     const t_error* const error = (const t_error*)error_arg.qwords[0];

     assert(error->msg.bytes[15] == tid__string || error->msg.bytes[15] == tid__short_string);

#ifdef CALL_STACK
     call_stack__show(error->call_stack__len, error->call_stack, stderr);
#endif

     fwrite("[ERROR]: ", 1, 9, stderr);
     t_any const print_result = string__print(thrd_data, error->msg, "", stderr);
     if (print_result.bytes[15] == tid__null) {
          fprintf(stderr, " (%s)\n", (const char*)error->owner.pointer);
     } else {
          assert(print_result.bytes[15] == tid__error);

          bool const fail_text_recoding = ((t_error*)print_result.qwords[0])->id.raw_bits == token_from_ascii_chars(13, "text_recoding").raw_bits;

          if (fail_text_recoding) {
               fprintf(stderr, "The application finished executing with an error, but cannot display a message about the error. (id = :");
               token__print(thrd_data, error->id, "", stderr);
               fprintf(stderr, ")\n");
          }
     }

     exit(EXIT_FAILURE);
}
