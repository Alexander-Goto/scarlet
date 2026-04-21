#pragma once

#include "../../common/include.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../../struct/error/fn.h"

static void check_app_result(t_thrd_data* const thrd_data, t_any const result) {
     if (result.bytes[15] != tid__error) [[clang::likely]] {
          ref_cnt__dec(thrd_data, result);
          return;
     }

     error__show_and_exit(thrd_data, result);
}