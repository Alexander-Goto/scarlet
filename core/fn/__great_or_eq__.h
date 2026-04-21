#pragma once

#include "__less__.h"

core inline t_any McoreFN__great_or_eq__(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.(>=)";

     call_stack__push(thrd_data, owner);
     t_any result = less__own(thrd_data, args, owner);
     call_stack__pop(thrd_data);

     if (result.bytes[15] == tid__error) [[clang::unlikely]] return result;

     assert(result.bytes[15] == tid__bool);

     result.bytes[0] ^= 1;
     return result;
}
