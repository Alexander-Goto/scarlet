#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/table/fn.h"

core t_any McoreFNtable_to_obj(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.table_to_obj";

     call_stack__push(thrd_data, owner);

     t_any table = args[0];
     if (table.bytes[15] != tid__table) [[clang::unlikely]] {
          if (table.bytes[15] == tid__error) {
               call_stack__pop(thrd_data);
               return table;
          }

          ref_cnt__dec(thrd_data, table);

          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     u64 const ref_cnt = get_ref_cnt(table);
     if (ref_cnt > 1) set_ref_cnt(table, ref_cnt - 1);

     u64                remain_qty = hash_map__get_len(table);
     t_any              obj        = obj__init(thrd_data, remain_qty, owner);
     const t_any* const items      = table__get_kvs(table);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const key = items[idx];
          if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

          remain_qty -= 1;
          if (key.bytes[15] != tid__token) [[clang::unlikely]] {
               if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, table);
               ref_cnt__dec(thrd_data, obj);

               t_any const result = error__invalid_type(thrd_data, owner);
               call_stack__pop(thrd_data);

               return result;
          }

          t_any const value = items[idx + 1];
          ref_cnt__inc(thrd_data, key, owner);
          ref_cnt__inc(thrd_data, value, owner);
          obj = obj__builtin_unsafe_add_kv__own(thrd_data, obj, key, value, owner);

          assert(obj.bytes[15] == tid__obj);
     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, table);

     call_stack__pop(thrd_data);

     return obj;
}