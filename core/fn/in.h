#pragma once

#include "look_in.h"

[[gnu::hot]]
core t_any McoreFNinB(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.in?";

     call_stack__push(thrd_data, owner);
     t_any const result = look_in(thrd_data, args, owner);
     call_stack__pop(thrd_data);

     switch (result.bytes[15]) {
     default: {
          ref_cnt__dec(thrd_data, result);
          return bool__true;
     }
     case tid__null:
          return bool__false;

     [[clang::unlikely]]
     case tid__error:
          return result;
     }
}