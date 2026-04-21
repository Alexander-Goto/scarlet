#pragma once

#include "../common/fn.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/obj/fn.h"

core inline t_any McoreFNtype(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.type";

     t_any const arg = args[0];
     switch (arg.bytes[15]) {
     case tid__short_string:
          return (const t_any){.bytes = {"core.string"}};
     case tid__null:
          return (const t_any){.bytes = {"core.null"}};
     case tid__token:
          return (const t_any){.bytes = {"core.token"}};
     case tid__bool:
          return (const t_any){.bytes = {"core.bool"}};
     case tid__byte:
          return (const t_any){.bytes = {"core.byte"}};
     case tid__char:
          return (const t_any){.bytes = {"core.char"}};
     case tid__int:
          return (const t_any){.bytes = {"core.int"}};
     case tid__time:
          return (const t_any){.bytes = {"core.time"}};
     case tid__float:
          return (const t_any){.bytes = {"core.float"}};
     case tid__short_fn:
          return (const t_any){.bytes = {"core.fn"}};
     case tid__short_byte_array:
          return (const t_any){.bytes = {"core.byte_array"}};
     case tid__box:
          ref_cnt__dec__noinlined_part(thrd_data, arg);
          return (const t_any){.bytes = {"core.box"}};
     case tid__breaker:
          ref_cnt__dec__noinlined_part(thrd_data, arg);
          return (const t_any){.bytes = {"core.breaker"}};
     case tid__obj: {
          bool called;

          call_stack__push(thrd_data, owner);
          t_any call_result = obj__try_call__any_result__own(thrd_data, mtd__core_type, arg, args, 1, owner, &called);
          if (called) {
               if (!(call_result.bytes[15] == tid__short_string || call_result.bytes[15] == tid__string)) [[clang::unlikely]]
                    fail_with_call_stack(thrd_data, "The 'core.type' function returned a value with a type different from 'core.string'.", owner);

               call_stack__pop(thrd_data);
               return call_result;
          }
          call_stack__pop(thrd_data);

          ref_cnt__dec(thrd_data, arg);
          return (const t_any){.bytes = {"core.obj"}};
     }
     case tid__table: {
          ref_cnt__dec(thrd_data, arg);
          return (const t_any){.bytes = {"core.table"}};
     }
     case tid__set: {
          ref_cnt__dec(thrd_data, arg);
          return (const t_any){.bytes = {"core.set"}};
     }
     case tid__byte_array: {
          ref_cnt__dec(thrd_data, arg);
          return (const t_any){.bytes = {"core.byte_array"}};
     }
     case tid__string: {
          ref_cnt__dec(thrd_data, arg);
          return (const t_any){.bytes = {"core.string"}};
     }
     case tid__array: {
          ref_cnt__dec(thrd_data, arg);
          return (const t_any){.bytes = {"core.array"}};
     }
     case tid__fn: {
          ref_cnt__dec(thrd_data, arg);
          return (const t_any){.bytes = {"core.fn"}};
     }
     case tid__thread:
          ref_cnt__dec__noinlined_part(thrd_data, arg);
          return (const t_any){.bytes = {"core.thread"}};
     case tid__channel: {
          ref_cnt__dec(thrd_data, arg);
          return (const t_any){.bytes = {"core.channel"}};
     }
     case tid__custom: {
          t_any (*const type__fn)(void) = custom__get_fns(arg)->type;

          call_stack__push(thrd_data, owner);

          t_any result = type__fn();
          if (!(result.bytes[15] == tid__short_string || result.bytes[15] == tid__string || result.bytes[15] == tid__error)) [[clang::unlikely]]
               fail_with_call_stack(thrd_data, "The 'core.type' function returned a value with a type different from 'core.string'.", owner);

          call_stack__pop(thrd_data);

          ref_cnt__dec(thrd_data, arg);
          return result;
     }

     [[clang::unlikely]]
     case tid__error:
          return arg;
     default:
          unreachable;
     }
}
