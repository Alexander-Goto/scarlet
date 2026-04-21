#pragma once

#include "../common/include.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"

struct {
     const char* file;
     u64         line;
     u64         row;
} typedef t_co_ords;

static t_mtx       tests_mtx;
static _Atomic u64 tests_qty          = 0;
static u64         tests_fail_qty     = 0;
static t_co_ords*  tests_fail_co_ords = nullptr;

[[clang::noinline]]
static t_any Otest(t_thrd_data* const thrd_data, t_any const value, const char* const file, u64 const line, u64 const row) {
     tests_qty += 1;
     if (value.bytes[15] != tid__bool || value.bytes[0] == 0) {
          ref_cnt__dec(thrd_data, value);

          mtx_lock(&tests_mtx.mtx);
          tests_fail_qty                        += 1;
          tests_fail_co_ords                     = realloc(tests_fail_co_ords, tests_fail_qty * sizeof(t_co_ords));
          tests_fail_co_ords[tests_fail_qty - 1] = (const t_co_ords){.file = file, .line = line, .row = row};
          mtx_unlock(&tests_mtx.mtx);
     }

     return null;
}
