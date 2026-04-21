#pragma once

#include "../common/fn.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/share.h"
#include "../common/type.h"
#include "../multithread/fn.h"
#include "../multithread/type.h"
#include "../multithread/var.h"
#include "../struct/common/fn_struct.h"
#include "../struct/holder/basic.h"

[[clang::noinline]]
core t_any McoreFNthread(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.thread";

     call_stack__push(thrd_data, owner);

     if (!allow_thrds) [[clang::unlikely]] fail_with_call_stack(thrd_data, "At the stage of calculating global constants, it is forbidden to create threads.", owner);

     t_any fn = args[0];
     if ((fn.bytes[15] != tid__short_fn && fn.bytes[15] != tid__fn) || common_fn__get_params_len(fn) != 0) [[clang::unlikely]] {
          t_any result;
          if (fn.bytes[15] == tid__error)
               result = fn;
          else {
               ref_cnt__dec(thrd_data, fn);

               if (fn.bytes[15] != tid__short_fn && fn.bytes[15] != tid__fn)
                    result = error__invalid_type(thrd_data, owner);
               else result = error__wrong_num_fn_args(thrd_data, owner);
          }

          call_stack__pop(thrd_data);

          return result;
     }

     fn = to_shared__own(thrd_data, fn, sf__thread, false, owner);

     if (fn.bytes[15] == tid__error) [[clang::unlikely]] {
          call_stack__pop(thrd_data);
          return fn;
     }

     t_thrd_result* const thrd_result = aligned_alloc(alignof(t_thrd_result), sizeof(t_thrd_result));
     thrd_result->result              = (const t_any){.structure = {.value = ht__used_thrd_result, .type = tid__holder}};
     mtx_init(&thrd_result->mtx.mtx, mtx_plain);
     cnd_init(&thrd_result->cnd);

#ifdef CALL_STACK
     u64          const new_call_stack_cap = (thrd_data->call_stack.len | 7) + 1;
     const char** const new_call_stack      = malloc(new_call_stack_cap * sizeof(char*));
     u8*          const new_tail_call_flags = malloc(new_call_stack_cap >> 3);
     memcpy(new_call_stack, thrd_data->call_stack.stack, thrd_data->call_stack.len * sizeof(char*));
     memcpy(new_tail_call_flags, thrd_data->call_stack.tail_call_flags, (thrd_data->call_stack.len >> 3) + ((thrd_data->call_stack.len & 7) != 0));
     call_stack__pop(thrd_data);
#endif
     mtx_lock(&sub_thrds_mtx.mtx);
     if (active_sub_thrds != sub_thrds_len) {
          t_thrd_data* const new_thrd_data = sub_thrds_data[active_sub_thrds];
          active_sub_thrds                += 1;
          new_thrd_data->fn                = fn;
          new_thrd_data->result            = thrd_result;
          cnd_t*       const cnd           = &new_thrd_data->wait_cnd;

#ifdef CALL_STACK
          new_thrd_data->call_stack.len               = thrd_data->call_stack.len + 1;
          new_thrd_data->call_stack.cap               = new_call_stack_cap;
          new_thrd_data->call_stack.stack             = new_call_stack;
          new_thrd_data->call_stack.tail_call_flags   = new_tail_call_flags;
          new_thrd_data->call_stack.next_is_tail_call = false;
#endif

          mtx_unlock(&sub_thrds_mtx.mtx);
          cnd_signal(cnd);
          return (const t_any){.structure = {.value = (u64)thrd_result, .type = tid__thread}};
     }
     mtx_unlock(&sub_thrds_mtx.mtx);

     t_thrd_data* const new_thrd_data = aligned_alloc(alignof(t_thrd_data), sizeof(t_thrd_data));
     new_thrd_data->free_boxes        = boxes_mask;
     new_thrd_data->free_breakers     = breakers_mask;

     FILE* const file = fopen("/dev/urandom", "r");
     if (file == nullptr) [[clang::unlikely]] fail("Can't open the file \x22/dev/urandom\x22.");

     setvbuf(file, nullptr, _IONBF, 0);
     if (fread(&new_thrd_data->rnd_num_src, sizeof(u64), 4, file) != 4) [[clang::unlikely]] fail("Can't read the file \x22/dev/urandom\x22.");
     fclose(file);

     new_thrd_data->linear_allocator = (const t_linear_allocator){.mem = nullptr, .states = nullptr};

#ifdef CALL_STACK
     new_thrd_data->call_stack.len               = thrd_data->call_stack.len;
     new_thrd_data->call_stack.cap               = new_call_stack_cap;
     new_thrd_data->call_stack.stack             = new_call_stack;
     new_thrd_data->call_stack.tail_call_flags   = new_tail_call_flags;
     new_thrd_data->call_stack.next_is_tail_call = false;
#endif

     new_thrd_data->fn     = fn;
     new_thrd_data->result = thrd_result;
     cnd_init(&new_thrd_data->wait_cnd);

     mtx_lock(&sub_thrds_mtx.mtx);
     new_thrd_data->thrd_id = new_thrd_id;
     new_thrd_id           += 1;
     if (sub_thrds_len == sub_thrds_cap) {
          sub_thrds_cap  = (sub_thrds_len + 1) * 3 / 2;
          sub_thrds_data = realloc(sub_thrds_data, sub_thrds_cap * sizeof(t_thrd_data*));
     }

     if (sub_thrds_len != active_sub_thrds) {
          sub_thrds_data[sub_thrds_len]      = sub_thrds_data[active_sub_thrds];
          sub_thrds_data[sub_thrds_len]->idx = sub_thrds_len;
     }

     sub_thrds_data[active_sub_thrds] = new_thrd_data;
     new_thrd_data->idx               = active_sub_thrds;
     sub_thrds_len                   += 1;
     active_sub_thrds                += 1;
     mtx_unlock(&sub_thrds_mtx.mtx);

     thrd_t thrd;
     thrd_create(&thrd, &new_thrd_fn, new_thrd_data);
     thrd_detach(thrd);
     return (const t_any){.structure = {.value = (u64)thrd_result, .type = tid__thread}};
}