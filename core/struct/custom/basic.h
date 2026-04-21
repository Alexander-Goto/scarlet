#pragma once

#include "../../common/type.h"
#include "../../multithread/type.h"

struct {
     t_any (*call_mtd)        (t_thrd_data* const, const char* const, t_any const, const t_any* const, u8 const);
     bool  (*has_mtd)         (t_any const);
     void  (*free_function)   (t_thrd_data* const, const t_any);
     t_any (*to_global_const) (t_thrd_data* const, t_any const, const char* const);
     t_any (*type)            (void);
     void  (*dump)            (t_thrd_data* const, t_any* const, t_any const, u64 const, u64 const, const char* const);
     t_any (*to_shared)       (t_thrd_data* const, t_any const, t_shared_fn const, bool const, const char* const);
} typedef t_custom_fns;

struct {
     u64     ref_cnt;
     t_ptr64 fns;
} typedef t_custom_types_data;

[[clang::always_inline]]
static t_custom_fns* custom__get_fns(t_any const custom) {
     assert(custom.bytes[15] == tid__custom);

     return ((t_custom_types_data*)custom.qwords[0])->fns.pointer;
}