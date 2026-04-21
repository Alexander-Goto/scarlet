#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/type.h"
#include "../common/type.h"
#include "../struct/string/fn.h"

[[gnu::cold, clang::noinline, noreturn]]
core t_any McoreFNfail(t_thrd_data* const thrd_data, const t_any* args) {
     const char* const owner = "function - core.fail";

     t_any msg = args[0];
     switch (msg.bytes[15]) {
     case tid__short_string: case tid__string: {
#ifdef DEBUG
          call_stack__show(thrd_data->call_stack.len, thrd_data->call_stack.stack, stderr);
#endif
          call_stack__push(thrd_data, owner);
          t_any print_result = string__print(thrd_data, msg, owner, stderr);
          fwrite("\n", 1, 1, stderr);
          call_stack__pop(thrd_data);

          if (print_result.bytes[15] == tid__error) [[clang::unlikely]] {
               const t_error* const error = (const t_error*)print_result.qwords[0];

               fwrite("[ERROR]: ", 1, 9, stderr);
               print_result = string__print(thrd_data, error->msg, "", stderr);
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
          }

          exit(EXIT_FAILURE);
     }
     case tid__error:
          error__show_and_exit(thrd_data, msg);

     [[clang::unlikely]] default: {
          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          error__show_and_exit(thrd_data, result);
     }}
}
