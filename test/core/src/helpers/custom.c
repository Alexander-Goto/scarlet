#define IMPORT_CORE

#include "../../../../core/core_h/core.h"

bool custom__has_mtd(t_any const) {return true;}

t_any custom__call_mtd(t_thrd_data* const thrd_data, const char* const owner, t_any const mtd_tkn, const t_any* const args, u8 const args_len) {
     t_any custom_arg = {};
     if (mtd_tkn.raw_bits == 0x9bc9f7da2dd47998cd1bada6ba2b92buwb)
          custom_arg = box__get_items(args[0])[0];
     else for (u8 idx = 0; idx < args_len; idx += 1)
          if (args[idx].bytes[15] == tid__custom) {
               custom_arg = args[idx];
               break;
          }

     const t_custom_types_data* const custom        = (t_custom_types_data*)custom_arg.qwords[0];
     const t_any*               const mtds_and_data = (const t_any*)&custom[1];

     t_any const mtds = mtds_and_data[0];
     ref_cnt__inc(thrd_data, mtds, owner);
     t_any const mtd = get_obj_property__own(thrd_data, mtds, mtd_tkn);
     return common_fn__call__own(thrd_data, mtd, args, args_len, owner);
}

void custom__free_function(t_thrd_data* const thrd_data, t_any const custom_arg) {
     t_custom_types_data* const custom        = (t_custom_types_data*)custom_arg.qwords[0];
     const t_any*         const mtds_and_data = (const t_any*)&custom[1];

     ref_cnt__dec(thrd_data, mtds_and_data[0]);
     ref_cnt__dec(thrd_data, mtds_and_data[1]);
     free(custom);
}

t_any custom__to_global_const(t_thrd_data* const thrd_data, t_any const custom_arg, const char* const owner) {
     t_custom_types_data* const custom        = (t_custom_types_data*)custom_arg.qwords[0];
     t_any*               const mtds_and_data = (t_any*)&custom[1];

     if (custom->ref_cnt != 0) {
          mtds_and_data[0] = to_global_const(thrd_data, mtds_and_data[0], owner);
          mtds_and_data[1] = to_global_const(thrd_data, mtds_and_data[1], owner);

          custom->ref_cnt = 0;
     }

     return custom_arg;
}

t_any custom__type(void) {return (const t_any){.bytes = "main.custom"};}

void custom__dump(t_thrd_data* const thrd_data, t_any* const result, t_any const custom_arg, u64 const level, u64 const sub_level, const char* const owner) {
     const t_custom_types_data* const custom        = (const t_custom_types_data*)custom_arg.qwords[0];
     const t_any*               const mtds_and_data = (const t_any*)&custom[1];
     dump__half_own(thrd_data, result, mtds_and_data[1], level, sub_level, owner);
}

t_any custom__to_shared(t_thrd_data* const thrd_data, t_any const custom_arg, t_shared_fn const shared_fn, bool const nested, const char* const owner) {
     t_custom_types_data* const custom = (t_custom_types_data*)custom_arg.qwords[0];
     switch (custom->ref_cnt) {
     case 0:
          unreachable;
     case 1: {
          t_any* const mtds_and_data = (t_any*)&custom[1];

          t_any const new_mtds = to_shared__own(thrd_data, mtds_and_data[0], shared_fn, nested, owner);
          if (new_mtds.bytes[15] == tid__error) [[clang::unlikely]] {
               mtds_and_data[0].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, custom_arg);

               return new_mtds;
          }
          mtds_and_data[0] = new_mtds;

          t_any const new_data = to_shared__own(thrd_data, mtds_and_data[1], shared_fn, nested, owner);
          if (new_data.bytes[15] == tid__error) [[clang::unlikely]] {
               mtds_and_data[1].raw_bits = 0;
               ref_cnt__dec__noinlined_part(thrd_data, custom_arg);

               return new_data;
          }
          mtds_and_data[1] = new_data;

          return custom_arg;
     }
     default: {
          custom->ref_cnt -= 1;

          const t_any*         const mtds_and_data     = (const t_any*)&custom[1];
          t_custom_types_data* const new_custom        = aligned_alloc(16, sizeof(t_custom_types_data) + 32);
          t_any*               const new_mtds_and_data = (t_any*)&new_custom[1];

          new_custom->ref_cnt = 1;
          new_custom->fns     = custom->fns;

          ref_cnt__inc(thrd_data, mtds_and_data[0], owner);
          t_any const new_mtds = to_shared__own(thrd_data, mtds_and_data[0], shared_fn, nested, owner);
          if (new_mtds.bytes[15] == tid__error) [[clang::unlikely]] {
               free(new_custom);
               return new_mtds;
          }
          new_mtds_and_data[0] = new_mtds;

          ref_cnt__inc(thrd_data, mtds_and_data[1], owner);
          t_any const new_data = to_shared__own(thrd_data, mtds_and_data[1], shared_fn, nested, owner);
          if (new_data.bytes[15] == tid__error) [[clang::unlikely]] {
               ref_cnt__dec(thrd_data, new_mtds);
               free(new_custom);
               return new_data;
          }
          new_mtds_and_data[1] = new_data;

          return (const t_any){.structure = {.value = (u64)new_custom, .type = tid__custom}};
     }}
}

static t_custom_fns const custom_fns = {
     .call_mtd        = custom__call_mtd,
     .has_mtd         = custom__has_mtd,
     .free_function   = custom__free_function,
     .to_global_const = custom__to_global_const,
     .type            = custom__type,
     .dump            = custom__dump,
     .to_shared       = custom__to_shared,
};

t_any MmainFNcustom__create(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function -  main.custom__create";

     t_any const data = args[0];
     t_any const mtds = args[1];

     if (!type_is_common_or_null(data.bytes[15]) || mtds.bytes[15] != tid__obj) [[clang::unlikely]] {
          if (data.bytes[15] == tid__error) {
               ref_cnt__dec(thrd_data, mtds);
               return data;
          }
          ref_cnt__dec(thrd_data, data);

          if (mtds.bytes[15] == tid__error) return mtds;
          ref_cnt__dec(thrd_data, mtds);

          call_stack__push(thrd_data, owner);
          t_any const error = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);
          return error;
     }

     t_custom_types_data* const custom        = aligned_alloc(16, sizeof(t_custom_types_data) + 32);
     t_any*               const mtds_and_data = (t_any*)&custom[1];

     custom->ref_cnt     = 1;
     custom->fns.pointer = (void*)&custom_fns;
     mtds_and_data[0]    = mtds;
     mtds_and_data[1]    = data;

     call_stack__pop_if_tail_call(thrd_data);
     return (const t_any){.structure = {.value = (u64)custom, .type = tid__custom}};
}

t_any MmainFNcustom__get_data(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - main.custom__get_data";

     t_any const custom = args[0];
     if (custom.bytes[15] != tid__custom) [[clang::unlikely]] {
          if (custom.bytes[15] == tid__error) return custom;

          ref_cnt__dec(thrd_data, custom);

          call_stack__push(thrd_data, owner);
          t_any const error = builtin_error__new(thrd_data, owner, "invalid_type", "Invalid type.");
          call_stack__pop(thrd_data);

          return error;
     }

     t_any* const mtds_and_data = (t_any*)&((const t_custom_types_data*)custom.qwords[0])[1];
     t_any  const data          = mtds_and_data[1];

     call_stack__push(thrd_data, owner);
     ref_cnt__inc(thrd_data, data, owner);
     call_stack__pop(thrd_data);

     ref_cnt__dec(thrd_data, custom);
     return data;
}