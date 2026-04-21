#pragma once

#include "../app/env/var.h"
#include "include.h"
#include "macro.h"
#include "type.h"

enum : u8 {
     log_lvl__minimum               = 1,
     log_lvl__hint                  = 2,
     log_lvl__may_require_attention = 3,
     log_lvl__require_attention     = 4,
     log_lvl__important             = 5,
     log_lvl__very_important        = 6,
     log_lvl__maximum               = 7,
} typedef t_log_level;

static _Atomic u8 log_level =
#ifdef DEBUG
log_lvl__minimum;
#else
log_lvl__require_attention;
#endif

core_basic void log__msg(t_log_level const level, const char* const msg) {
     assert(level >= log_lvl__minimum && level <= log_lvl__maximum);

     if (level < log_level) return;

     FILE* const out = level < log_lvl__require_attention ? stdout : stderr;
     fprintf(out, "[LOG]: %s\n", msg);
}

core_basic void log__debug_msg(t_log_level const level, const char* const msg) {
#ifdef DEBUG
     log__msg(level, msg);
#endif
}
