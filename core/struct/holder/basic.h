#pragma once

#include "../../common/type.h"

enum : u8 {
     ht__untyped            = tid__holder,
     ht__fn_arg             = 1,
     ht__uninit_const       = 2,
     ht__now_init_const     = 3,
     ht__used_thrd_result   = 4,
     ht__unused_thrd_result = 5,
     ht__waiting_result     = 6,
} typedef t_holder_type;
