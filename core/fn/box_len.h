#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/error/fn.h"

core t_any McoreFNbox_len(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.box_len";

     t_any const box = args[0];
     if (box.bytes[15] != tid__box) [[clang::unlikely]] {
          if (box.bytes[15] == tid__error) return box;

          ref_cnt__dec(thrd_data, box);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     call_stack__push(thrd_data, owner);
     t_any const result_box = box__new(thrd_data, 2, owner);
     call_stack__pop(thrd_data);

     t_any* const result_items = box__get_items(result_box);
     result_items[0]           = (const t_any){.structure = {.value = box__get_len(box), .type = tid__int}};
     result_items[1]           = box;
     return result_box;
}