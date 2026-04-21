#pragma once

#include "../common/fn.h"
#include "../common/include.h"
#include "../common/type.h"

[[gnu::cold, noreturn]]
static void Ounreachable(t_thrd_data* const thrd_data, const char* const file, u64 const line, u64 const row) {
#ifdef CALL_STACK
     call_stack__show(thrd_data->call_stack.len, thrd_data->call_stack.stack, stderr);
#endif
     fprintf(stderr, "UNREACHABLE!!!\nfile: %s\nline: %"PRIu64"\nrow: %"PRIu64"\n", file, line, row);
     exit(EXIT_FAILURE);
}
