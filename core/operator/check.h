#pragma once

#include "../common/include.h"
#include "../common/type.h"
#include "../struct/error/fn.h"

static inline t_any Ocheck(t_thrd_data* const thrd_data, t_any const exp, const char* const file, u64 const line, u64 const row) {
     if (exp.bytes[15] == tid__error) [[clang::unlikely]] {
          fprintf(stderr, "CHECK!!!\nfile: %s\nline:%"PRIu64"\nrow:%"PRIu64"\n\n", file, line, row);
          error__show_and_exit(thrd_data, exp);
     }

     return exp;
}