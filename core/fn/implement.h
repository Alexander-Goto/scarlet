#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

[[clang::noinline]]
core t_any McoreFNimplement(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.implement";

     t_any obj       = args[0];
     t_any interface = args[1];

     if (interface.bytes[15] == tid__array) {
          t_any       new_args[2];
          t_any const array = interface;

          u32          const slice_offset = slice__get_offset(array);
          u32          const slice_len    = slice__get_len(array);
          const t_any* const array_items  = &((const t_any*)slice_array__get_items(array))[slice_offset];
          u64          const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

          call_stack__push(thrd_data, owner);

          new_args[0] = obj;
          for (u64 idx = 0; idx < len; idx += 1) {
               new_args[1] = array_items[idx];
               if (new_args[0].bytes[15] == tid__error || new_args[1].bytes[15] != tid__obj) [[clang::unlikely]] {
                    if (new_args[0].bytes[15] != tid__error) {
                         ref_cnt__dec(thrd_data, new_args[0]);
                         new_args[0] = error__invalid_type(thrd_data, owner);
                    }

                    break;
               }

               ref_cnt__inc(thrd_data, new_args[1], owner);
               new_args[0] = McoreFNimplement(thrd_data, new_args);
          }
          ref_cnt__dec(thrd_data, array);

          call_stack__pop(thrd_data);

          return new_args[0];
     }

     if (obj.bytes[15] != tid__obj || interface.bytes[15] != tid__obj) [[clang::unlikely]] {
          if (obj.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, interface);
               return obj;
          }
          ref_cnt__dec(thrd_data, obj);

          if (interface.bytes[15] == tid__error) return interface;
          ref_cnt__dec(thrd_data, interface);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     u64 remain_qty = obj__get_fields_len(interface);
     if (remain_qty == 0) {
          ref_cnt__dec(thrd_data, interface);
          return obj;
     }

     call_stack__push(thrd_data, owner);

     if (get_ref_cnt(obj) != 1) obj = obj__dup__own(thrd_data, obj, owner);

     t_any* const mtds_ptr = obj__get_mtds(obj);
     if (mtds_ptr->bytes[15] == tid__null) {
          if (obj__get_mtds(interface)->bytes[15] == tid__null) {
               call_stack__pop(thrd_data);

               *mtds_ptr = interface;
               obj       = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
               return obj;
          }

          *mtds_ptr = obj__init(thrd_data, remain_qty, owner);
          obj       = hash_map__set_len(obj, hash_map__get_len(obj) + 1);
     } else if (get_ref_cnt(*mtds_ptr) != 1) *mtds_ptr = obj__dup__own(thrd_data, *mtds_ptr, owner);

     t_any              mtds     = *mtds_ptr;
     const t_any* const new_mtds = obj__get_fields(interface);
     for (u64 idx = 0; remain_qty != 0; idx += 2) {
          t_any const mtd_name = new_mtds[idx];
          if (mtd_name.bytes[15] != tid__token) continue;

          remain_qty -= 1;

#ifndef NDEBUG
          bool first_time = true;
#endif
          t_any* name_in_mtds_ptr;
          while (true) {
               name_in_mtds_ptr = obj__key_ptr(mtds, mtd_name);
               if (name_in_mtds_ptr != nullptr) break;

               mtds = obj__enlarge__own(thrd_data, mtds, owner);

#ifndef NDEBUG
               assert(first_time);
               first_time = false;
#endif
          }

          if (name_in_mtds_ptr->bytes[15] != tid__token) {
               t_any const new_mtd = new_mtds[idx + 1];
               ref_cnt__inc(thrd_data, new_mtd, owner);

               mtds                = hash_map__set_len(mtds, hash_map__get_len(mtds) + 1);
               name_in_mtds_ptr[0] = mtd_name;
               name_in_mtds_ptr[1] = new_mtd;
          }
     }

     *mtds_ptr = mtds;
     ref_cnt__dec(thrd_data, interface);

     call_stack__pop(thrd_data);

     return obj;
}