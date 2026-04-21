#pragma once

#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/ref_cnt.h"
#include "../../common/type.h"
#include "../common/fn_struct.h"
#include "../error/basic.h"
#include "../obj/fn.h"
#include "basic.h"

static inline t_any custom__call__own(t_thrd_data* const thrd_data, t_any const mtd_tkn, u8 const result_type, t_any const custom, const t_any* const args, u8 const args_len, const char* const owner) {
     assert(mtd_tkn.bytes[15] == tid__token);
     assert(type_is_correct(result_type));
     assert(custom.bytes[15] == tid__custom);

     t_any const mtd_result = custom__get_fns(custom)->call_mtd(thrd_data, owner, mtd_tkn, args, args_len);
     if (mtd_result.bytes[15] == result_type) [[clang::likely]] return mtd_result;
     if (mtd_result.bytes[15] == tid__error) return mtd_result;

     ref_cnt__dec(thrd_data, mtd_result);
     return error__invalid_type(thrd_data, owner);
}

static inline t_any custom__call__any_result__own(t_thrd_data* const thrd_data, t_any const mtd_tkn, t_any const custom, const t_any* const args, u8 const args_len, const char* const owner) {
     assert(mtd_tkn.bytes[15] == tid__token);
     assert(custom.bytes[15] == tid__custom);

     return custom__get_fns(custom)->call_mtd(thrd_data, owner, mtd_tkn, args, args_len);
}

static inline t_any custom__try_call__own(t_thrd_data* const thrd_data, t_any const mtd_tkn, u8 const result_type, t_any const custom, const t_any* const args, u8 const args_len, const char* const owner, bool* const called) {
     assert(mtd_tkn.bytes[15] == tid__token);
     assert(type_is_correct(result_type));
     assert(custom.bytes[15] == tid__custom);

     *called = custom__get_fns(custom)->has_mtd(mtd_tkn);
     if (!*called) return null;

     t_any const mtd_result = custom__get_fns(custom)->call_mtd(thrd_data, owner, mtd_tkn, args, args_len);
     if (mtd_result.bytes[15] == result_type) [[clang::likely]] return mtd_result;
     if (mtd_result.bytes[15] == tid__error) return mtd_result;

     ref_cnt__dec(thrd_data, mtd_result);
     return error__invalid_type(thrd_data, owner);
}

static inline t_any custom__try_call__any_result__own(t_thrd_data* const thrd_data, t_any const mtd_tkn, t_any const custom, const t_any* const args, u8 const args_len, const char* const owner, bool* const called) {
     assert(mtd_tkn.bytes[15] == tid__token);
     assert(custom.bytes[15] == tid__custom);

     *called = custom__get_fns(custom)->has_mtd(mtd_tkn);
     if (!*called) return null;

     return custom__get_fns(custom)->call_mtd(thrd_data, owner, mtd_tkn, args, args_len);
}