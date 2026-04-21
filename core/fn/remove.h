#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/custom/fn.h"
#include "../struct/error/basic.h"
#include "../struct/holder/basic.h"
#include "../struct/obj/fn.h"
#include "../struct/set/fn.h"

[[gnu::hot, clang::noinline]]
static t_any hash(t_thrd_data* const thrd_data, t_any const arg, u64 seed, const char* const owner);

core t_any McoreFNremove(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.remove";

     t_any       set  = args[0];
     t_any const item = args[1];
     if (set.bytes[15] == tid__error || item.bytes[15] == tid__error) [[clang::unlikely]] {
          if (set.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, item);
               return set;
          }

          ref_cnt__dec(thrd_data, set);
          return item;
     }

     switch (set.bytes[15]) {
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__call__own(thrd_data, mtd__core_remove, tid__obj, set, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__set: {
          call_stack__push(thrd_data, owner);

          if (get_ref_cnt(set) != 1) set = set__dup__own(thrd_data, set, owner);

          u64   const seed     = hash_map__get_hash_seed(set);
          t_any const key_hash = hash(thrd_data, item, seed, owner);
          if (key_hash.bytes[15] == tid__error) [[clang::unlikely]] {
               call_stack__pop(thrd_data);

               ref_cnt__dec(thrd_data, set);
               ref_cnt__dec(thrd_data, item);
               return key_hash;
          }

          assert(key_hash.bytes[15] == tid__int);

          t_any* const key_ptr = set__key_ptr(thrd_data, set, item, key_hash.qwords[0], owner);
          ref_cnt__dec(thrd_data, item);
          if (key_ptr == nullptr || key_ptr->bytes[15] == tid__null || key_ptr->bytes[15] == tid__holder) {
               call_stack__pop(thrd_data);
               return set;
          }

          ref_cnt__dec(thrd_data, *key_ptr);
          key_ptr->bytes[15] = tid__holder;

          set = hash_map__set_len(set, hash_map__get_len(set) - 1);
          if (set__need_to_fit(set)) set = set__fit__own(thrd_data, set, owner);

          call_stack__pop(thrd_data);

          return set;
     }
     case tid__custom: {
          call_stack__push(thrd_data, owner);
          t_any const result = custom__call__own(thrd_data, mtd__core_remove, tid__custom, set, args, 2, owner);
          call_stack__pop(thrd_data);

          return result;
     }

     [[clang::unlikely]] default: {
          ref_cnt__dec(thrd_data, set);
          ref_cnt__dec(thrd_data, item);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}