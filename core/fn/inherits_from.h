#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/fn.h"
#include "../struct/obj/fn.h"

[[clang::noinline]]
core t_any McoreFNinherits_from(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.inherits_from";

     t_any child  = args[0];
     t_any parent = args[1];

     if (parent.bytes[15] == tid__array) {
          t_any       new_args[2];
          t_any const array = parent;

          u32          const slice_offset = slice__get_offset(array);
          u32          const slice_len    = slice__get_len(array);
          const t_any* const array_items  = &((const t_any*)slice_array__get_items(array))[slice_offset];
          u64          const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(array);

          call_stack__push(thrd_data, owner);

          new_args[0] = child;
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
               new_args[0] = McoreFNinherits_from(thrd_data, new_args);
          }
          ref_cnt__dec(thrd_data, array);

          call_stack__pop(thrd_data);

          return new_args[0];
     }

     if (child.bytes[15] != tid__obj || parent.bytes[15] != tid__obj) [[clang::unlikely]] {
          if (child.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, parent);
               return child;
          }
          ref_cnt__dec(thrd_data, child);

          if (parent.bytes[15] == tid__error) return parent;
          ref_cnt__dec(thrd_data, parent);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     t_any const parent_mtds_obj = *obj__get_mtds(parent);

     u64 remain_mtds_qty       = parent_mtds_obj.bytes[15] == tid__null ? 0 : obj__get_fields_len(parent_mtds_obj);
     u64 remain_fields_qty = obj__get_fields_len(parent);
     if ((remain_mtds_qty | remain_fields_qty) == 0) {
          ref_cnt__dec(thrd_data, parent);
          return child;
     }

     call_stack__push(thrd_data, owner);

     if (get_ref_cnt(child) != 1) child = obj__dup__own(thrd_data, child, owner);

     const t_any* const parent_fields = obj__get_fields(parent);
     for (u64 idx = 0; remain_fields_qty != 0; idx += 2) {
          t_any const field_name = parent_fields[idx];
          if (field_name.bytes[15] != tid__token) continue;

          remain_fields_qty -= 1;

#ifndef NDEBUG
          bool first_time = true;
#endif
          t_any* name_in_child;
          while (true) {
               name_in_child = obj__key_ptr(child, field_name);
               if (name_in_child != nullptr) break;

               child = obj__enlarge__own(thrd_data, child, owner);

#ifndef NDEBUG
               assert(first_time);
               first_time = false;
#endif
          }

          if (name_in_child->bytes[15] != tid__token) {
               t_any const parent_field = parent_fields[idx + 1];
               ref_cnt__inc(thrd_data, parent_field, owner);

               child            = hash_map__set_len(child, hash_map__get_len(child) + 1);
               name_in_child[0] = field_name;
               name_in_child[1] = parent_field;
          }
     }

     if (remain_mtds_qty != 0) {
          t_any* const child_mtds_ptr = obj__get_mtds(child);
          if (child_mtds_ptr->bytes[15] == tid__null) {
               if (obj__get_mtds(parent_mtds_obj)->bytes[15] == tid__null) {
                    ref_cnt__inc(thrd_data, parent_mtds_obj, owner);
                    ref_cnt__dec(thrd_data, parent);

                    *child_mtds_ptr = parent_mtds_obj;
                    child           = hash_map__set_len(child, hash_map__get_len(child) + 1);

                    call_stack__pop(thrd_data);

                    return child;
               }

               *child_mtds_ptr = obj__init(thrd_data, remain_mtds_qty, owner);
               child           = hash_map__set_len(child, hash_map__get_len(child) + 1);
          } else if (get_ref_cnt(*child_mtds_ptr) != 1) *child_mtds_ptr = obj__dup__own(thrd_data, *child_mtds_ptr, owner);

          t_any              child_mtds  = *child_mtds_ptr;
          const t_any* const parent_mtds = obj__get_fields(parent_mtds_obj);
          for (u64 idx = 0; remain_mtds_qty != 0; idx += 2) {
               t_any const mtd_name = parent_mtds[idx];
               if (mtd_name.bytes[15] != tid__token) continue;

               remain_mtds_qty -= 1;

#ifndef NDEBUG
               bool first_time = true;
#endif
               t_any* name_in_child;
               while (true) {
                    name_in_child = obj__key_ptr(child_mtds, mtd_name);
                    if (name_in_child != nullptr) break;

                    child_mtds = obj__enlarge__own(thrd_data, child_mtds, owner);

#ifndef NDEBUG
                    assert(first_time);
                    first_time = false;
#endif
               }

               if (name_in_child->bytes[15] != tid__token) {
                    t_any const parent_mtd = parent_mtds[idx + 1];
                    ref_cnt__inc(thrd_data, parent_mtd, owner);

                    child_mtds       = hash_map__set_len(child_mtds, hash_map__get_len(child_mtds) + 1);
                    name_in_child[0] = mtd_name;
                    name_in_child[1] = parent_mtd;
               }
          }

          *child_mtds_ptr = child_mtds;
     }
     ref_cnt__dec(thrd_data, parent);

     call_stack__pop(thrd_data);

     return child;
}