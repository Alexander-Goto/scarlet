#pragma once

#include "../include/include.h"

t_any MstdFNrootB(t_thrd_data* const thrd_data, const t_any* const) {
     call_stack__pop_if_tail_call(thrd_data);
     return (const t_any){.bytes = {geteuid() == 0, [15] = tid__bool}};
}