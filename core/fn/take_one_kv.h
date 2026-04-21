#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/fn.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/null/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"
#include "../struct/table/fn.h"

core t_any McoreFNtake_one_kv(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.take_one_kv";

     t_any container = args[0];
     switch (container.bytes[15]) {
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__take_one_kv__own(thrd_data, container, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__table: {
          call_stack__push(thrd_data, owner);
          t_any const result = table__take_one_kv__own(thrd_data, container, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any result = custom__call__any_result__own(thrd_data, mtd__core_take_one_kv, container, args, 1, owner);

          if (result.bytes[15] != tid__box && result.bytes[15] != tid__null) [[clang::unlikely]] {
               if (result.bytes[15] == tid__error) {
                    call_stack__pop(thrd_data);
                    return result;
               }

               ref_cnt__dec(thrd_data, result);
               result = error__invalid_type(thrd_data, owner);
          }
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]]case tid__error:
          return container;
     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, container);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}