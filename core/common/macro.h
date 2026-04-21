#pragma once

#include "include.h"

#define memchr        __builtin_memchr
#define memcpy        __builtin_memcpy
#define memcpy_inline __builtin_memcpy_inline
#define memmove       __builtin_memmove
#define memset        __builtin_memset
#define memset_inline __builtin_memset_inline
#define strchr        __builtin_strchr
#define strlen        __builtin_strlen

#define MACRO_STR__HELPER(arg) #arg
#define MACRO_STR(arg) MACRO_STR__HELPER(arg)

#ifdef NDEBUG
#define assume_aligned(mem, align) __builtin_assume((u64)(mem) % (align) == 0)
#define assume(cond) __builtin_assume(cond)
#define unreachable __builtin_unreachable()
#else
#define assume_aligned(mem, align) assert((u64)(mem) % (align) == 0)
#define assume(cond) assert(cond)
#define unreachable fail("unreachable: ("__FILE__", "MACRO_STR(__LINE__)")")
#endif

#ifdef EXPORT_CORE
#define core extern
#define core_basic extern
#define core_string extern
#define core_error extern
#else
#define core static

#ifdef EXPORT_CORE_BASIC
#define core_basic extern
#else
#define core_basic static
#endif

#ifdef EXPORT_CORE_STRING
#define core_string extern
#else
#define core_string static
#endif

#ifdef EXPORT_CORE_ERROR
#define core_error extern
#else
#define core_error static
#endif

#endif