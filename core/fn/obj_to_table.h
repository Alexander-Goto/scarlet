#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/table/fn.h"

core t_any McoreFNobj_to_table(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.obj_to_table";

     t_any obj = args[0];
     if (obj.bytes[15] != tid__obj) [[clang::unlikely]] {
          if (obj.bytes[15] == tid__error) return obj;

          ref_cnt__dec(thrd_data, obj);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     u64 const ref_cnt = get_ref_cnt(obj);
     if (ref_cnt > 1) set_ref_cnt(obj, ref_cnt - 1);

     call_stack__push(thrd_data, owner);

     t_any table = table__init(thrd_data, hash_map__get_len(obj), owner);

     t_any* const mtds = obj__get_mtds(obj);
     if (mtds->bytes[15] != tid__null) {
          ref_cnt__inc(thrd_data, *mtds, owner);
          table = table__builtin_add_kv__own(thrd_data, table, mtds_token, *mtds, owner);

          assert(table.bytes[15] == tid__table);
     }

     const t_any* const fields = &mtds[1];
     u64                remain_qty = obj__get_fields_len(obj);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const name = fields[idx];
          if (name.bytes[15] != tid__token) continue;

          remain_qty          -= 1;
          t_any const field = fields[idx + 1];
          ref_cnt__inc(thrd_data, field, owner);
          table = table__builtin_add_kv__own(thrd_data, table, name, field, owner);

          assert(table.bytes[15] == tid__table);

     }

     if (ref_cnt == 1) ref_cnt__dec__noinlined_part(thrd_data, obj);

     call_stack__pop(thrd_data);

     return table;
}