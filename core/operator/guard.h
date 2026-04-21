#pragma once

#include "../common/include.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/bool/basic.h"

static inline t_any Oguard(t_thrd_data* const thrd_data, t_any const guard_exp, const char* const file, u64 const line, u64 const row) {
     if (guard_exp.bytes[15] != tid__bool || guard_exp.bytes[0] == 0) [[clang::unlikely]] {
#ifdef CALL_STACK
          call_stack__show(thrd_data->call_stack.len, thrd_data->call_stack.stack, stderr);
#endif
          fprintf(stderr, "GUARD!!!\nfile: %s\nline:%"PRIu64"\nrow:%"PRIu64"\n", file, line, row);
          exit(EXIT_FAILURE);
     }

     return bool__true;
}
