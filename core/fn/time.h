#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/type.h"
#include "../struct/error/fn.h"

core inline t_any McoreFNtime(t_thrd_data* const thrd_data, const t_any*) {
     const char* const owner = "function - core.time";

     time_t const utc_time = time(nullptr);

     if (utc_time == (const time_t)-1) [[clang::unlikely]] {
          call_stack__push(thrd_data, owner);
          t_any const result = error__cant_get_current_time(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     return (const t_any){.structure = {.value = utc_time, .type = tid__time}};
}
