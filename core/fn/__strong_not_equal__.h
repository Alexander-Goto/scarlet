#pragma once

#include "__strong_equal__.h"

core inline t_any McoreFN__strong_not_equal__(t_thrd_data* const thrd_data, const t_any* const args) {
     #ifdef CALL_STACK
     const char* const owner = "function - core.(/=)";
     #endif

     call_stack__push(thrd_data, owner);
     t_any result = McoreFN__strong_equal__(thrd_data, args);
     call_stack__pop(thrd_data);

     if (result.bytes[15] == tid__error) [[clang::unlikely]] return result;

     assert(result.bytes[15] == tid__bool);

     result.bytes[0] ^= 1;
     return result;
}
