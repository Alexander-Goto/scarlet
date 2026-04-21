#pragma once

#define _GNU_SOURCE

#ifdef FULL_DEBUG
#define DEBUG
#else
#define NDEBUG
#endif

#ifdef DEBUG
#define CALL_STACK
#endif

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

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

#ifdef IMPORT_CORE
#ifndef IMPORT_CORE_BASIC
#define IMPORT_CORE_BASIC
#endif
#ifndef IMPORT_CORE_STRING
#define IMPORT_CORE_STRING
#endif
#ifndef IMPORT_CORE_ERROR
#define IMPORT_CORE_ERROR
#endif
#endif

#define boxes_max_qty 4
#define boxes_mask    0xf

#define breakers_max_qty 8
#define breakers_mask    0xff

#define ref_cnt_max      0xff'ffff'ffffull
#define ref_cnt_bits     40
#define ref_cnt_bit_mask 0xff'ffff'ffffull

#define array_max_len         0xfff'ffff'ffffull
#define slice_max_offset      0xfff'ffffull
#define slice_max_len         0xfff'fffeull
#define array_len_bits        44
#define slice_offset_bits     28
#define slice_len_bits        28
#define array_len_bit_mask    0xfff'ffff'ffffull
#define slice_offset_bit_mask 0xfff'ffffull
#define slice_len_bit_mask    0xfff'ffffull

#define fn_borrowed_len_bits 16

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

#if CACHE_LINE_SIZE == 64
#define ALIGN_FOR_CACHE_LINE 64
#elif
#define ALIGN_FOR_CACHE_LINE (__builtin_popcount(CACHE_LINE_SIZE)==1?CACHE_LINE_SIZE:1<<(sizeof(int)*8-__builtin_clz((unsigned int)CACHE_LINE_SIZE)))
#endif

#define u8   uint8_t
#define u16  uint16_t
#define u32  uint32_t
#define u64  uint64_t
#define u128 unsigned __int128
#define i128 __int128

[[gnu::aligned(1)]] typedef u16 ua_u16;
[[gnu::aligned(1)]] typedef u32 ua_u32;
[[gnu::aligned(1)]] typedef u64 ua_u64;

#define i8  int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

[[gnu::aligned(1)]] typedef i16  ua_i16;
[[gnu::aligned(1)]] typedef i32  ua_i32;
[[gnu::aligned(1)]] typedef i64  ua_i64;
[[gnu::aligned(1)]] typedef u128 ua_u128;
[[gnu::aligned(1)]] typedef i128 ua_i128;

enum : u8 {
     tid__short_string     = 0,
     tid__int              = 1,
     tid__float            = 2,
     tid__bool             = 3,
     tid__null             = 4,
     tid__char             = 5,
     tid__byte             = 6,
     tid__short_byte_array = 7,
     tid__time             = 8,
     tid__token            = 9,
     tid__short_fn         = 10,
     tid__holder           = 11,
     tid__obj              = 12,
     tid__string           = 13,
     tid__array            = 14,
     tid__table            = 15,
     tid__set              = 16,
     tid__byte_array       = 17,
     tid__custom           = 18,
     tid__box              = 19,
     tid__channel          = 20,
     tid__thread           = 21,
     tid__fn               = 22,
     tid__breaker          = 23,
     tid__error            = 24,
} typedef t_type_id;

typedef u8   v_any_u8 __attribute__((ext_vector_type(16), aligned(16)));
typedef char v_any_i8 __attribute__((ext_vector_type(16), aligned(16)));

[[gnu::aligned(16)]]
union {
     u8       bytes[16];
     u64      qwords[2];
     double   floats[2];
     u128     raw_bits;
     v_any_u8 vector;
     v_any_i8 chars_vec;
     struct {
          u64       value;
          u8        data[7];
          t_type_id type;
     }        structure;
} typedef t_any;

struct {
     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     t_any items[16];
} typedef t_box;

union {
     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     mtx_t mtx;
     u8    spacer[((sizeof(mtx_t) + ALIGN_FOR_CACHE_LINE - 1) / ALIGN_FOR_CACHE_LINE) * ALIGN_FOR_CACHE_LINE];
} typedef t_mtx;

struct {
     t_mtx mtx;
     cnd_t cnd;
     t_any result;
} typedef t_thrd_result;

typedef u64 v_rnd_src __attribute__((ext_vector_type(4), aligned(32)));

#ifdef CALL_STACK
struct {
     const char** stack;
     u8*          tail_call_flags;
     u64          len;
     u64          cap;
     bool         next_is_tail_call;
} typedef t_call_stack;
#endif

struct {
     void* mem;
     i64*  states;
     u64   mem_size;
     u64   mem_cap;
     u64   states_len;
     u64   states_cap;
} typedef t_linear_allocator;

struct t_thrd_data {
     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     t_any        fns_args[16];
     t_box        boxes[boxes_max_qty];
     v_rnd_src    rnd_num_src;
     u16          breakers_data[breakers_max_qty];
     u32          free_boxes;
     u32          free_breakers;
     u64          thrd_id;
     const t_any* borrowed_data;

     t_linear_allocator linear_allocator;

     #ifdef CALL_STACK
     t_call_stack call_stack;
     #endif

     t_any          fn;
     t_thrd_result* result;
     u64            idx;
     cnd_t          wait_cnd;
} typedef t_thrd_data;

typedef void t_char_codes;

enum : u8 {
     sf__thread = 1,
     sf__push   = 2,
} typedef t_shared_fn;

struct {
     t_any (*call_mtd)        (t_thrd_data* const, const char* const, t_any const, const t_any* const, u8 const);
     bool  (*has_mtd)         (t_any const);
     void  (*free_function)   (t_thrd_data* const, t_any const);
     t_any (*to_global_const) (t_thrd_data* const, t_any const, const char* const);
     t_any (*type)            (void);
     void  (*dump)            (t_thrd_data* const, t_any* const, t_any const, u64 const, u64 const, const char* const);
     t_any (*to_shared)       (t_thrd_data* const, t_any const, t_shared_fn const, bool const, const char* const);
} typedef t_custom_fns;

union {
     void* pointer;
     u64   bits;
} typedef t_ptr64;

struct {
     u64     ref_cnt;
     t_ptr64 fns;
} typedef t_custom_types_data;

struct {
     u64     ref_cnt;
     t_ptr64 owner;
     t_any   id;
     t_any   msg;
     t_any   data;

#ifdef CALL_STACK
     const char** call_stack;
     u64          call_stack__len;
#endif
} typedef t_error;

static t_any const bool__false = {.bytes = {[15] = tid__bool}};
static t_any const bool__true  = {.bytes = {1, [15] = tid__bool}};
static t_any const null        = {.bytes = {[15] = tid__null}};

#define comptime_tkn__equal     0x9b955b841d49390cc1cde10fede912buwb
#define comptime_tkn__great     0x9ea79e4c45fddcbafbd4b353d2f535fuwb
#define comptime_tkn__less      0x9e79e620503f2d16f246dc372ac1cb7uwb
#define comptime_tkn__nan_cmp   0x9e1b8d461ca75d16de399f20a6eb3a7uwb
#define comptime_tkn__not_equal 0x9b85ab539a264d16bea78a034a954f7uwb

static t_any const tkn__equal     = {.raw_bits = comptime_tkn__equal};
static t_any const tkn__great     = {.raw_bits = comptime_tkn__great};
static t_any const tkn__less      = {.raw_bits = comptime_tkn__less};
static t_any const tkn__nan_cmp   = {.raw_bits = comptime_tkn__nan_cmp};
static t_any const tkn__not_equal = {.raw_bits = comptime_tkn__not_equal};

[[clang::always_inline]]
static bool type_is_correct(t_type_id type) {return type <= tid__error;}

[[clang::always_inline]]
static bool type_is_val(t_type_id type) {return type <= tid__holder;}

[[clang::always_inline]]
static bool type_with_ref_cnt(t_type_id type) {
     return (type >= tid__obj) & (type != tid__box) & (type != tid__breaker) & (type != tid__thread);
}

[[clang::always_inline]]
static bool type_is_common(t_type_id type) {
     return (type != tid__null) & (type != tid__box) & (type != tid__breaker) & (type != tid__error);
}

[[clang::always_inline]]
static bool type_is_common_or_null(t_type_id type) {
     return (type != tid__box) & (type != tid__breaker) & (type != tid__error);
}

[[clang::always_inline]]
static bool type_is_common_or_error(t_type_id type) {
     return (type != tid__null) & (type != tid__box) & (type != tid__breaker);
}

[[clang::always_inline]]
static bool type_is_common_or_null_or_error(t_type_id type) {
     return (type != tid__box) & (type != tid__breaker);
}

[[clang::always_inline]]
static bool type_is_not_copyable_or_error(t_type_id type) {
     return (type == tid__box) | (type == tid__breaker) | (type == tid__thread) | (type == tid__error);
}

[[clang::always_inline]]
static bool type_is_eq_and_common(t_type_id type) {
     return (type != tid__null) & (type != tid__short_fn) & (type != tid__box) & (type != tid__breaker) & (type != tid__error) & (type != tid__fn) & (type != tid__thread) & (type != tid__channel);
}

[[gnu::cold, noreturn]]
static void fail(const char* const msg) {
     fprintf(stderr, "%s\n", msg);
     exit(EXIT_FAILURE);
}

[[clang::always_inline]]
static void memcpy_le_16(void* const dst, const void* const src, u64 const len) {
     assume(len < 17);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memcpy_le_32(void* const dst, const void* const src, u64 const len) {
     assume(len < 33);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 5:
          assert(len > 16 && len <= 32);

          memcpy_inline(dst, src, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &((const u8*)src)[len - 16], 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memcpy_le_64(void* const dst, const void* const src, u64 const len) {
     assume(len < 65);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 6:
          assert(len > 32 && len <= 64);

          memcpy_inline(dst, src, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &((const u8*)src)[len - 32], 32);
          return;
     case 5:
          assert(len > 16 && len <= 32);

          memcpy_inline(dst, src, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &((const u8*)src)[len - 16], 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memcpy_le_128(void* const dst, const void* const src, u64 const len) {
     assume(len < 129);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 7:
          assert(len > 64 && len <= 128);

          memcpy_inline(dst, src, 64);
          memcpy_inline(&((u8*)dst)[len - 64], &((const u8*)src)[len - 64], 64);
          return;
     case 6:
          assert(len > 32 && len <= 64);

          memcpy_inline(dst, src, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &((const u8*)src)[len - 32], 32);
          return;
     case 5:
          assert(len > 16 && len <= 32);

          memcpy_inline(dst, src, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &((const u8*)src)[len - 16], 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memcpy_le_256(void* const dst, const void* const src, u64 const len) {
     assume(len < 257);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 8:
          assert(len > 128 && len <= 256);

          memcpy_inline(dst, src, 128);
          memcpy_inline(&((u8*)dst)[len - 128], &((const u8*)src)[len - 128], 128);
          return;
     case 7:
          assert(len > 64 && len <= 128);

          memcpy_inline(dst, src, 64);
          memcpy_inline(&((u8*)dst)[len - 64], &((const u8*)src)[len - 64], 64);
          return;
     case 6:
          assert(len > 32 && len <= 64);

          memcpy_inline(dst, src, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &((const u8*)src)[len - 32], 32);
          return;
     case 5:
          assert(len > 16 && len <= 32);

          memcpy_inline(dst, src, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &((const u8*)src)[len - 16], 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_16(void* const dst, const void* const src, u64 const len) {
     assume(len < 17);

     [[gnu::aligned(alignof(u64))]]
     u8 buffer[16];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_32(void* const dst, const void* const src, u64 const len) {
     assume(len < 33);

     [[gnu::aligned(16)]]
     u8 buffer[32];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 5: {
          assert(len > 16 && len <= 32);

          memcpy_inline(buffer, src, 16);
          memcpy_inline(&buffer[16], &((const u8*)src)[len - 16], 16);
          memcpy_inline(dst, buffer, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &buffer[16], 16);
          return;
     }
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_64(void* const dst, const void* const src, u64 const len) {
     assume(len < 65);

     [[gnu::aligned(32)]]
     u8 buffer[64];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 6: {
          assert(len > 32 && len <= 64);

          memcpy_inline(buffer, src, 32);
          memcpy_inline(&buffer[32], &((const u8*)src)[len - 32], 32);
          memcpy_inline(dst, buffer, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &buffer[32], 32);
          return;
     }
     case 5: {
          assert(len > 16 && len <= 32);

          memcpy_inline(buffer, src, 16);
          memcpy_inline(&buffer[16], &((const u8*)src)[len - 16], 16);
          memcpy_inline(dst, buffer, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &buffer[16], 16);
          return;
     }
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_128(void* const dst, const void* const src, u64 const len) {
     assume(len < 129);

     [[gnu::aligned(64)]]
     u8 buffer[128];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 7: {
          assert(len > 64 && len <= 128);

          memcpy_inline(buffer, src, 64);
          memcpy_inline(&buffer[64], &((const u8*)src)[len - 64], 64);
          memcpy_inline(dst, buffer, 64);
          memcpy_inline(&((u8*)dst)[len - 64], &buffer[64], 64);
          return;
     }
     case 6: {
          assert(len > 32 && len <= 64);

          memcpy_inline(buffer, src, 32);
          memcpy_inline(&buffer[32], &((const u8*)src)[len - 32], 32);
          memcpy_inline(dst, buffer, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &buffer[32], 32);
          return;
     }
     case 5: {
          assert(len > 16 && len <= 32);

          memcpy_inline(buffer, src, 16);
          memcpy_inline(&buffer[16], &((const u8*)src)[len - 16], 16);
          memcpy_inline(dst, buffer, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &buffer[16], 16);
          return;
     }
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_256(void* const dst, const void* const src, u64 const len) {
     assume(len < 257);

     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     u8 buffer[256];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 8: {
          assert(len > 128 && len <= 256);

          memcpy_inline(buffer, src, 128);
          memcpy_inline(&buffer[128], &((const u8*)src)[len - 128], 128);
          memcpy_inline(dst, buffer, 128);
          memcpy_inline(&((u8*)dst)[len - 128], &buffer[128], 128);
          return;
     }
     case 7: {
          assert(len > 64 && len <= 128);

          memcpy_inline(buffer, src, 64);
          memcpy_inline(&buffer[64], &((const u8*)src)[len - 64], 64);
          memcpy_inline(dst, buffer, 64);
          memcpy_inline(&((u8*)dst)[len - 64], &buffer[64], 64);
          return;
     }
     case 6: {
          assert(len > 32 && len <= 64);

          memcpy_inline(buffer, src, 32);
          memcpy_inline(&buffer[32], &((const u8*)src)[len - 32], 32);
          memcpy_inline(dst, buffer, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &buffer[32], 32);
          return;
     }
     case 5: {
          assert(len > 16 && len <= 32);

          memcpy_inline(buffer, src, 16);
          memcpy_inline(&buffer[16], &((const u8*)src)[len - 16], 16);
          memcpy_inline(dst, buffer, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &buffer[16], 16);
          return;
     }
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

static inline void* safe_realloc(void* const pointer, u64 const size) {
     assume(size != 0);

     void* const result = realloc(pointer, size);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");
     return result;
}

static inline void* safe_aligned_realloc(u64 const align, void* const pointer, u64 const size) {
     assume(__builtin_popcountll(align) == 1 && align % sizeof(void*) == 0);
     assume(size != 0);
     assume_aligned(pointer, align);

     void* const result = realloc(pointer, size);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");

     if (result == pointer || (u64)result % align == 0) return result;

     void* const new_mem = aligned_alloc(align, size);

     assume_aligned(new_mem, align);

     memcpy(new_mem, result, size);
     free(result);
     return new_mem;
}

static inline void* safe_malloc(u64 const size) {
     assume(size != 0);

     void* const result = malloc(size);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");
     return result;
}

static inline void* safe_aligned_alloc(u64 const align, u64 const size) {
     assume(__builtin_popcountll(align) == 1 && align % sizeof(void*) == 0);
     assume(size != 0);

     void* const result = aligned_alloc(align, size);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");

     assume_aligned(result, align);

     return result;
}

static inline char* safe_strdup(const char* const string) {
     assume(string != nullptr);

     char* const result = __builtin_strdup(string);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");
     return result;
}

static inline int nointr_fclose(FILE* const stream) {
     #pragma nounroll
     while (true) {
          int const result = fclose(stream);
          if (result == 0 || errno != EINTR) return result;
          clearerr(stream);
     }
}

static inline int nointr_fflush(FILE* const stream) {
     #pragma nounroll
     while (true) {
          int const result = fflush(stream);
          if (result == 0 || errno != EINTR) return result;
          clearerr(stream);
     }
}

static inline FILE* nointr_fopen(const char* const pathname, const char* const mode) {
     #pragma nounroll
     while (true) {
          FILE* const result = fopen(pathname, mode);
          if (result != nullptr || errno != EINTR) return result;
     }
}

static inline size_t nointr_fread(void* const ptr, size_t const size, size_t const nitems, FILE* const stream) {
     size_t already_read_len = 0;

     #pragma nounroll
     while (true) {
          size_t const one_take_readed_len = fread(&ptr[already_read_len * size], size, nitems - already_read_len, stream);
          already_read_len                += one_take_readed_len;
          if (already_read_len == nitems || feof(stream) || errno != EINTR) return already_read_len;
          clearerr(stream);
     }
}

static inline int nointr_fseek(FILE* const stream, long const offset, int const whence) {
     #pragma nounroll
     while (true) {
          int const result = fseek(stream, offset, whence);
          if (result == 0 || errno != EINTR) return result;
          clearerr(stream);
     }
}

static inline size_t nointr_fwrite(const void* const ptr, size_t const size, size_t const nitems, FILE* const stream) {
     size_t already_written_len = 0;

     #pragma nounroll
     while (true) {
          size_t const one_take_readed_len = fwrite(&ptr[already_written_len * size], size, nitems - already_written_len, stream);
          already_written_len             += one_take_readed_len;
          if (already_written_len == nitems || feof(stream) || errno != EINTR) return already_written_len;
          clearerr(stream);
     }
}

static inline int nointr_thrd_sleep(const struct timespec* duration, struct timespec* const remain) {
     #pragma nounroll
     while (true) {
          int const result = thrd_sleep(duration, remain);
          if (result == 0 || errno != EINTR) return result;
          duration = remain;
     }
}

#define realloc(p, size)                      safe_realloc(p, size)
#define aligned_realloc(align, pointer, size) safe_aligned_realloc(align, pointer, size)
#define malloc(size)                          safe_malloc(size)
#define aligned_alloc(align, size)            safe_aligned_alloc(align, size)
#define strdup(string)                        safe_strdup(string)
#define fclose(stream)                        nointr_fclose(stream)
#define fflush(stream)                        nointr_fflush(stream)
#define fopen(pathname, mode)                 nointr_fopen(pathname, mode)
#define fread(ptr, size, nitems, stream)      nointr_fread(ptr, size, nitems, stream)
#define fseek(stream, offset, whence)         nointr_fseek(stream, offset, whence)
#define fwrite(ptr, size, nitems, stream)     nointr_fwrite(ptr, size, nitems, stream)
#define thrd_sleep(duration, remain)          nointr_thrd_sleep(duration, remain)

[[clang::always_inline]]
static u64 cast_double_to_u64(double const double_) {
     return *(const u64*)&double_;
}

[[clang::always_inline]]
static double cast_u64_to_double(u64 const u64_) {
     return *(const double*)&u64_;
}

[[clang::always_inline]]
static t_ptr64 ptr_to_ptr64(const void* const ptr) {
     return (const t_ptr64){.bits = (u64)ptr};
}

[[clang::always_inline]]
static u64 get_ref_cnt(t_any const arg) {
     return *(u64*)arg.qwords[0] & ref_cnt_bit_mask;
}

[[clang::always_inline]]
static void set_ref_cnt(t_any const arg, u64 const ref_cnt) {
     *(u64*)arg.qwords[0] = *(u64*)arg.qwords[0] & (u64)-1 << ref_cnt_bits | ref_cnt;
}

static inline u64 linear__alloc(t_linear_allocator* const allocator, u64 const size, u64 const align) {
     assert((i64)size > 0);
     assert(align < 17);
     assert(__builtin_popcountll(align) == 1);

     u64 const spacer_size  = allocator->mem == nullptr || ((u64)(allocator->mem) & align - 1) == 0 ? 0 : align - ((u64)(allocator->mem) & align - 1);
     u64 const new_mem_size = allocator->mem_size + size + spacer_size;

     assert(new_mem_size > allocator->mem_size);

     if (new_mem_size > allocator->mem_cap) {
          allocator->mem_cap = new_mem_size;
          allocator->mem     = aligned_realloc(16, allocator->mem, new_mem_size);
     }

     u64 const new_states_qty = spacer_size != 0 ? 2 : 1;
     u64 const idx            = allocator->states_len + new_states_qty - 1;
     if (idx >= allocator->states_cap) {
          u64 const new_cap     = allocator->states_cap + 4;
          allocator->states_cap = new_cap;
          allocator->states     = realloc(allocator->states, new_cap * sizeof(i64));
     }

     if (spacer_size != 0) {
          allocator->states[allocator->states_len] = allocator->mem_size | (u64)1 << 63;
          allocator->mem_size                     += spacer_size;
          allocator->states_len                   += 1;
     }

     allocator->states[idx] = allocator->mem_size;
     allocator->states_len += 1;
     allocator->mem_size    = new_mem_size;

     return idx;
}

[[clang::always_inline]]
static void* linear__get_mem_of_idx(const t_linear_allocator* const allocator, u64 const idx) {
     return &((u8*)allocator->mem)[allocator->states[idx]];
}

[[clang::always_inline]]
static void* linear__get_mem_of_last(const t_linear_allocator* const allocator) {
     return &((u8*)allocator->mem)[allocator->states[allocator->states_len - 1]];
}

[[clang::always_inline]]
static bool linear__idx_is_last(const t_linear_allocator* const allocator, u64 const idx) {
     return allocator->states_len - 1 == idx;
}

static inline void linear__last_realloc(t_linear_allocator* const allocator, u64 const size) {
     assert((i64)size > 0);

     u64 const last_idx     = allocator->states_len - 1;
     u64 const new_mem_size = allocator->states[last_idx] + size;
     if (new_mem_size > allocator->mem_cap) {
          allocator->mem_cap = new_mem_size;
          allocator->mem     = aligned_realloc(16, allocator->mem, new_mem_size);
     }
     allocator->mem_size = new_mem_size;
}

static inline void linear__free(t_linear_allocator* const allocator, u64 const idx) {
     assert(idx < allocator->states_len);

     if (allocator->states_len - 1 == idx) {
          allocator->mem_size = allocator->states[idx];
          i64 look_idx        = idx - 1;
          for (; look_idx != -1 && allocator->states[look_idx] < 0; allocator->mem_size = allocator->states[look_idx] & (u64)-1 >> 1, look_idx -= 1);
          allocator->states_len = look_idx + 1;
     } else allocator->states[idx] |= (u64)1 << 63;
}

#ifdef CALL_STACK
[[clang::always_inline]]
static void call_stack__next_is_tail_call(t_thrd_data* const thrd_data) {thrd_data->call_stack.next_is_tail_call = true;}

static inline void call_stack__push(t_thrd_data* const thrd_data, const char* const item) {
     u64 const stack_len = thrd_data->call_stack.len;
     if (stack_len == thrd_data->call_stack.cap) {
          u64 const cap = thrd_data->call_stack.cap * 2 | 8;
          thrd_data->call_stack.cap             = cap;
          thrd_data->call_stack.stack           = realloc(thrd_data->call_stack.stack, cap * sizeof(char*));
          thrd_data->call_stack.tail_call_flags = realloc(thrd_data->call_stack.tail_call_flags, cap >> 3);
     }

     u64 const tail_call_bucket_idx = stack_len >> 3;
     u8  const tail_call_mask       = (u8)1 << ((u8)stack_len & (u8)7);
     if (thrd_data->call_stack.next_is_tail_call) {
          thrd_data->call_stack.tail_call_flags[tail_call_bucket_idx] |= tail_call_mask;
          thrd_data->call_stack.next_is_tail_call                      = false;
     } else thrd_data->call_stack.tail_call_flags[tail_call_bucket_idx] &= ~tail_call_mask;

     thrd_data->call_stack.stack[stack_len] = item;
     thrd_data->call_stack.len              = stack_len + 1;
}

static inline void call_stack__pop(t_thrd_data* const thrd_data) {
     u64 stack_len = thrd_data->call_stack.len;

     #pragma nounroll
     while (true) {
          stack_len -= 1;

          u64  const tail_call_bucket_idx = stack_len >> 3;
          u8   const tail_call_mask       = (u8)1 << ((u8)stack_len & (u8)7);
          bool const is_not_tail_call     = (thrd_data->call_stack.tail_call_flags[tail_call_bucket_idx] & tail_call_mask) == 0;
          if (is_not_tail_call) {
               thrd_data->call_stack.len = stack_len;
               return;
          }
     }
}

[[clang::always_inline]]
static void call_stack__pop_if_tail_call(t_thrd_data* const thrd_data) {
     bool const is_tail_call = thrd_data->call_stack.next_is_tail_call;
     if (is_tail_call) {
          thrd_data->call_stack.next_is_tail_call = false;
          call_stack__pop(thrd_data);
     }
}

[[clang::noinline]]
static void call_stack__show(u64 const stack_len, const char* const* const stack, FILE* const out) {
     assert(stack_len != 0);

     fprintf(out, "[CALL STACK]\n%s\n", stack[0]);
     for (u64 idx = 1; idx < stack_len; idx += 1)
          fprintf(out, "-> %s\n", stack[idx]);
     fprintf(out, "\n");
}
#else
#define call_stack__next_is_tail_call(thrd_data) ((void)0)
#define call_stack__push(thrd_data, item) ((void)0)
#define call_stack__pop(thrd_data) ((void)0)
#define call_stack__pop_if_tail_call(thrd_data) ((void)0)
#define call_stack__show(stack_len, stack, out) ((void)0)
#endif

[[gnu::cold, noreturn]]
static void fail_with_call_stack(t_thrd_data* const thrd_data, const char* const msg, const char* const owner) {
     #ifdef CALL_STACK
     call_stack__show(thrd_data->call_stack.len, thrd_data->call_stack.stack, stderr);
     #endif

     fprintf(stderr, "%s (%s)\n", msg, owner);
     exit(EXIT_FAILURE);
}

[[clang::always_inline]]
static t_custom_fns* custom__get_fns(t_any const custom) {
     assert(custom.bytes[15] == tid__custom);

     return ((t_custom_types_data*)custom.qwords[0])->fns.pointer;
}

[[clang::always_inline]]
static u32 slice__get_offset(t_any const slice) {
     assert(slice.bytes[15] == tid__byte_array || slice.bytes[15] == tid__string || slice.bytes[15] == tid__array);

     return slice.qwords[1] & slice_offset_bit_mask;
}

[[clang::always_inline]]
static u32 slice__get_len(t_any const slice) {
     assert(slice.bytes[15] == tid__byte_array || slice.bytes[15] == tid__string || slice.bytes[15] == tid__array);

     return (slice.qwords[1] >> slice_offset_bits) & slice_len_bit_mask;
}

[[clang::always_inline]]
static t_any slice__set_metadata(t_any slice, u32 const offset, u32 const len) {
     assert(slice.bytes[15] == tid__byte_array || slice.bytes[15] == tid__string || slice.bytes[15] == tid__array);
     assume(offset <= slice_max_offset);
     assume(len <= slice_max_len + 1);

     slice.qwords[1] = slice.qwords[1] & (u64)0xff << 56 | (u64)offset | (u64)len << slice_offset_bits;
     return slice;
}

[[clang::always_inline]]
static u64 slice_array__get_len(t_any const slice) {
     assert(slice.bytes[15] == tid__byte_array || slice.bytes[15] == tid__string || slice.bytes[15] == tid__array);

     return *(u128*)slice.qwords[0] >> ref_cnt_bits & array_len_bit_mask;
}

[[clang::always_inline]]
static void slice_array__set_len(t_any const slice, u64 const len) {
     assert(slice.bytes[15] == tid__byte_array || slice.bytes[15] == tid__string || slice.bytes[15] == tid__array);
     assert(get_ref_cnt(slice) != 0);
     assume(len <= array_max_len);

     u128* const fields = (u128*)slice.qwords[0];
     *fields = *fields & ((u128)-1 ^ (u128)array_len_bit_mask << ref_cnt_bits) | (u128)len << ref_cnt_bits;
}

[[clang::always_inline]]
static u64 slice_array__get_cap(t_any const slice) {
     assert(slice.bytes[15] == tid__byte_array || slice.bytes[15] == tid__string || slice.bytes[15] == tid__array);

     return *(u128*)slice.qwords[0] >> (ref_cnt_bits + array_len_bits) & array_len_bit_mask;
}

[[clang::always_inline]]
static void slice_array__set_cap(t_any const slice, u64 const cap) {
     assert(slice.bytes[15] == tid__byte_array || slice.bytes[15] == tid__string || slice.bytes[15] == tid__array);
     assert(get_ref_cnt(slice) != 0);
     assume(cap <= array_max_len);

     u128* const fields = (u128*)slice.qwords[0];
     *fields = *fields & ((u128)-1 ^ (u128)array_len_bit_mask << (ref_cnt_bits + array_len_bits)) | (u128)cap << (ref_cnt_bits + array_len_bits);
}

[[clang::always_inline]]
static void* slice_array__get_items(t_any const slice) {
     assert(slice.bytes[15] == tid__byte_array || slice.bytes[15] == tid__string || slice.bytes[15] == tid__array);

     return &((u64*)slice.qwords[0])[2];
}

static t_any const empty_box = {.structure = {.data = {32}, .type = tid__box}};

[[clang::always_inline]]
static u8 box__get_len(t_any const box) {
     assume(box.bytes[15] == tid__box);

     u8 const len = box.bytes[8] & 0x1f;

     assume(len <= 16);

     return len;
}

[[clang::always_inline]]
static bool box__is_global(t_any const box) {
     assume(box.bytes[15] == tid__box);

     return (box.bytes[8] & 0x20) == 0x20;
}

[[clang::always_inline]]
static t_any* box__get_items(t_any const box) {
     assume(box.bytes[15] == tid__box);

     t_any* const items = (t_any*)box.qwords[0];

     assume_aligned(items, 16);

     return items;
}

[[clang::always_inline]]
static u8 short_byte_array__get_len(t_any const array) {
     assert(array.bytes[15] == tid__short_byte_array);

     u8 const len = array.bytes[14];

     assume(len < 15);

     return len;
}

[[clang::always_inline]]
static t_any short_byte_array__set_len(t_any array, u8 const len) {
     assert(array.bytes[15] == tid__short_byte_array);
     assert(len < 15);

     array.bytes[14] = len;
     return array;
}

static t_any long_byte_array__new(u64 const cap) {
     assume(cap <= array_max_len);

     t_any array = {.structure = {.value = (u64)malloc(cap + 16), .type = tid__byte_array}};
     set_ref_cnt(array, 1);
     slice_array__set_len(array, 0);
     slice_array__set_cap(array, cap);
     return array;
}

[[clang::always_inline]]
static u8 short_string__get_len(t_any const string) {
     assert(string.bytes[15] == tid__short_string);
     u8 const len = -__builtin_reduce_add((string.vector < 192) & (string.vector != 0));
     assume(len < 16);
     return len;
}

[[clang::always_inline]]
static u8 short_string__get_size(t_any const string) {
     assert(string.bytes[15] == tid__short_string);
     u8 const size = -__builtin_reduce_add(string.vector != 0);
     assume(size < 16);
     return size;
}

static t_any long_string__new(u64 const cap) {
     assume(cap <= array_max_len);

     t_any string = {.structure = {.value = (u64)malloc(cap * 3 + 16), .type = tid__string}};
     set_ref_cnt(string, 1);
     slice_array__set_len(string, 0);
     slice_array__set_cap(string, cap);

     return string;
}

#ifdef IMPORT_CORE_BASIC
enum : u8 {
     log_lvl__minimum               = 1,
     log_lvl__hint                  = 2,
     log_lvl__may_require_attention = 3,
     log_lvl__require_attention     = 4,
     log_lvl__important             = 5,
     log_lvl__very_important        = 6,
     log_lvl__maximum               = 7,
} typedef t_log_level;

extern t_any const empty_array_data;
static t_any const empty_array = {.structure = {.value = (u64)&empty_array_data, .type = tid__array}};

static t_any array__init(t_thrd_data* const thrd_data, u64 const cap, const char* const owner) {
     if (cap > array_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum array size has been exceeded.", owner);

     if (cap == 0) return empty_array;

     t_any array = {.structure = {.value = (u64)aligned_alloc(16, (cap + 1) * 16), .type = tid__array}};
     set_ref_cnt(array, 1);
     slice_array__set_len(array, 0);
     slice_array__set_cap(array, cap);
     return array;
}

extern t_any to_global_const(t_thrd_data* const thrd_data, t_any const arg, const char* const owner);
extern u64 get_hash(const u8* const data, u64 const size, u64 const seed);
extern const char* get_app_path();
extern const char* const* get_c_env_vars(u64* const len);
extern void log__msg(t_log_level const level, const char* const msg);
extern void log__debug_msg(t_log_level const level, const char* const msg);
extern t_any box__new(t_thrd_data* const thrd_data, u8 const len, const char* const owner);
extern t_any breaker__get_result__own(t_thrd_data* const thrd_data, t_any breaker);
extern t_any breaker__new(t_thrd_data* const thrd_data, t_any result, const char* const owner);
extern t_any Onew__set(t_thrd_data* const thrd_data, t_any container, u64 const keys_len, const t_any* const keys, t_any const value, bool const return_old_value, const char* const owner);
extern t_any Onew__apply(t_thrd_data* const thrd_data, t_any container, u64 const keys_len, const t_any* const keys, t_any const fn, bool const return_old_value, const char* const owner);
extern t_any common_fn__partial_app__own(t_thrd_data* const thrd_data, t_any const fn, const t_any* const args, u8 const args_len, const char* const owner);
extern t_any common_fn__call__own(t_thrd_data* const thrd_data, t_any const fn, const t_any* const args, u8 const args_len, const char* const owner);
extern t_any common_fn__call__half_own(t_thrd_data* const thrd_data, t_any const fn, const t_any* const args, u8 const args_len, const char* const owner);
extern t_any token_from_ascii_chars(u64 const len, const char* const chars);
extern void token_to_ascii_chars(t_any const token, u8* const len, char* const chars);
extern t_any* table__get_kvs_and_len(t_any const table, u64* const len);
extern t_any* set__get_items_and_len(t_any const set, u64* const len);
extern void dump__add_string__own(t_thrd_data* const thrd_data, t_any* const result, t_any const string, const char* const owner);
extern t_any dump__level_spaces(t_thrd_data* const thrd_data, u64 const level, const char* const owner);
extern void dump__half_own(t_thrd_data* const thrd_data, t_any* const result__own, t_any const arg, u64 const level, u64 const sub_level, const char* const owner);
extern t_any get_obj_field__own(t_thrd_data* const thrd_data, t_any const obj, t_any const field_name);
extern u64 look_2_bytes_from_begin(const u8* const bytes, u64 const bytes_len, u16 const looked_bytes);
extern u64 look_2_bytes_from_end(const u8* const bytes, u64 const bytes_len, u16 const looked_bytes);
extern u64 look_byte_from_begin(const u8* const bytes, u64 const bytes_len, u8 const looked_byte);
extern u64 look_byte_from_end(const u8* const bytes, u64 const bytes_len, u8 const looked_byte);

[[clang::always_inline]]
static bool fn__is_linear_alloc(t_any const fn) {
     assume(fn.bytes[15] == tid__fn);

     return (fn.qwords[1] >> (10 + fn_borrowed_len_bits) & 1) == 1;
}

[[clang::always_inline]]
static u64 fn__get_ref_cnt(t_thrd_data* thrd_data, t_any const fn) {
     assume(fn.bytes[15] == tid__fn);

     u64* const fn_data = fn__is_linear_alloc(fn) ? (u64*)linear__get_mem_of_idx(&thrd_data->linear_allocator, fn.qwords[0]) : (u64*)fn.qwords[0];

     return *fn_data & ref_cnt_bit_mask;
}

[[clang::always_inline]]
static void fn__set_ref_cnt(t_thrd_data* thrd_data, t_any const fn, u64 const ref_cnt) {
     assume(fn.bytes[15] == tid__fn);

     u64* const fn_data = fn__is_linear_alloc(fn) ? (u64*)linear__get_mem_of_idx(&thrd_data->linear_allocator, fn.qwords[0]) : (u64*)fn.qwords[0];

     *fn_data = *fn_data & (u64)-1 << ref_cnt_bits | ref_cnt;
}

static void ref_cnt__add(t_thrd_data* const thrd_data, t_any const arg, u64 const val, const char* const owner) {
     u8 const type = arg.bytes[15];
     assert(type_is_correct(type));

     if (type_is_val(type) || val == 0) return;

     if (type_with_ref_cnt(type)) [[clang::likely]] {
          u64 ref_cnt = type == tid__fn ? fn__get_ref_cnt(thrd_data, arg) : get_ref_cnt(arg);
          if (ref_cnt == 0) return;

          bool const overflow = __builtin_add_overflow(ref_cnt, val, &ref_cnt);
          if (ref_cnt > ref_cnt_max || overflow) [[clang::unlikely]] fail_with_call_stack(thrd_data, "Reference counter overflow.", owner);

          type == tid__fn ? fn__set_ref_cnt(thrd_data, arg, ref_cnt) : set_ref_cnt(arg, ref_cnt);
          return;
     }

     if (type == tid__thread || type == tid__breaker) [[clang::unlikely]]
          fail_with_call_stack(thrd_data, type == tid__thread ? "Attempting to copy a thread." : "Attempting to duplicate a breaker.", owner);

     if (!box__is_global(arg)) [[clang::unlikely]] fail_with_call_stack(thrd_data, "Attempting to duplicate a box.", owner);
}

static inline void ref_cnt__inc(t_thrd_data* const thrd_data, t_any const arg, const char* const owner) {
     u8 const type = arg.bytes[15];
     assert(type_is_correct(type));

     if (type_is_val(type)) return;

     if (type_with_ref_cnt(type)) [[clang::likely]] {
          u64 const ref_cnt = type == tid__fn ? fn__get_ref_cnt(thrd_data, arg) : get_ref_cnt(arg);
          if (ref_cnt == 0) return;
          if (ref_cnt == ref_cnt_max) [[clang::unlikely]] fail_with_call_stack(thrd_data, "Reference counter overflow.", owner);

          assume(ref_cnt < ref_cnt_max);

          type == tid__fn ? fn__set_ref_cnt(thrd_data, arg, ref_cnt + 1) : set_ref_cnt(arg, ref_cnt + 1);
          return;
     }

     if (type == tid__thread || type == tid__breaker) [[clang::unlikely]]
          fail_with_call_stack(thrd_data, type == tid__thread ? "Attempting to copy a thread." : "Attempting to duplicate a breaker.", owner);

     if (!box__is_global(arg)) [[clang::unlikely]] fail_with_call_stack(thrd_data, "Attempting to duplicate a box.", owner);
}

[[gnu::hot]]
extern void ref_cnt__dec__noinlined_part(t_thrd_data* const thrd_data, t_any const arg);

[[clang::always_inline]]
static void ref_cnt__dec(t_thrd_data* const thrd_data, t_any const arg) {
     u8 const type = arg.bytes[15];
     assert(type_is_correct(type));

     if (type_is_val(type)) return;

     if (type_with_ref_cnt(type)) {
          u64 const ref_cnt = type == tid__fn ? fn__get_ref_cnt(thrd_data, arg) : get_ref_cnt(arg);
          if (ref_cnt != 1) {
               if (ref_cnt != 0) type == tid__fn ? fn__set_ref_cnt(thrd_data, arg, ref_cnt - 1) : set_ref_cnt(arg, ref_cnt - 1);
               return;
          }
     }
     ref_cnt__dec__noinlined_part(thrd_data, arg);
}

extern t_any to_shared__noinlined_part__own(t_thrd_data* const thrd_data, t_any const arg, t_shared_fn const shared_fn, bool const nested, const char* const owner);

static inline t_any to_shared__own(t_thrd_data* const thrd_data, t_any const arg, t_shared_fn const shared_fn, bool const nested, const char* const owner) {
     if (type_is_val(arg.bytes[15]) && arg.bytes[15] != tid__null) {
          assert(arg.bytes[15] != tid__holder || nested);
          return arg;
     }

     return to_shared__noinlined_part__own(thrd_data, arg, shared_fn, nested, owner);
}
#endif

#ifdef IMPORT_CORE_STRING
enum : u64 {
     str_cvt_err__encoding    = (u64)-1,
     str_cvt_err__recoding    = (u64)-2,
     str_cvt_err__sys_escape  = (u64)-3,
     str_cvt_sys_is_utf8      = (u64)-4,
};

struct {
     u64 src_offset;
     u64 dst_offset;
} typedef t_str_cvt_result;

extern const t_char_codes* get_sys_char_codes(void);
extern u32 corsar_code_to_unicode(u32 const corsar_code);
extern u32 unicode_to_corsar_code(u32 const unicode, const t_char_codes* const lang_codes);
extern t_any short_string_from_chars(const u8* const chars, u8 const len);
extern t_any string_from_ascii(const char* const string);
extern t_any string_from_n_len_sysstr(t_thrd_data* const thrd_data, u64 const sysstr_len, const char* const sysstr, const char* const owner);
extern t_any string_from_ze_sysstr(t_thrd_data* const thrd_data, const char* const sysstr, const char* const owner);
extern u64 check_corsar_chars(u64 const size, const u8* const chars);
extern u64 check_ctf8_chars(u64 const size, const u8* const chars);
extern u64 check_utf8_chars(u64 const size, const u8* const chars);
extern t_str_cvt_result corsar_chars_to_ctf8_chars(u64 const corsar_chars_size, const u8* const corsar_chars, u64 const ctf8_chars_cap_size, u8* const ctf8_chars);
extern t_str_cvt_result corsar_chars_to_sys_chars(u64 const corsar_chars_size, const u8* const corsar_chars, u64 const sys_chars_cap_size, u8* const sys_chars);
extern t_str_cvt_result corsar_chars_to_utf8_chars(u64 const corsar_chars_size, const u8* const corsar_chars, u64 const utf8_chars_cap_size, u8* const utf8_chars);
extern t_str_cvt_result ctf8_chars_to_corsar_chars(u64 const ctf8_chars_size, const u8* const ctf8_chars, u64 const corsar_chars_cap_size, u8* const corsar_chars);
extern t_str_cvt_result ctf8_chars_to_sys_chars(u64 const ctf8_chars_size, const u8* const ctf8_chars, u64 const sys_chars_cap_size, u8* const sys_chars);
extern t_str_cvt_result ctf8_chars_to_utf8_chars(u64 const ctf8_chars_size, const u8* const ctf8_chars, u64 const utf8_chars_cap_size, u8* const utf8_chars);
extern t_str_cvt_result sys_chars_to_corsar_chars(u64 const sys_chars_size, const u8* const sys_chars, u64 const corsar_chars_cap_size, u8* const corsar_chars);
extern t_str_cvt_result sys_chars_to_ctf8_chars(u64 const sys_chars_size, const u8* const sys_chars, u64 const ctf8_chars_cap_size, u8* const ctf8_chars);
extern t_str_cvt_result sys_chars_to_utf8_chars(u64 const sys_chars_size, const u8* const sys_chars, u64 const utf8_chars_cap_size, u8* const utf8_chars);
extern t_str_cvt_result utf8_chars_to_corsar_chars(u64 const utf8_chars_size, const u8* const utf8_chars, u64 const corsar_chars_cap_size, u8* const corsar_chars, const t_char_codes* const lang_codes);
extern t_str_cvt_result utf8_chars_to_ctf8_chars(u64 const utf8_chars_size, const u8* const utf8_chars, u64 const ctf8_chars_cap_size, u8* const ctf8_chars, const t_char_codes* const lang_codes);
extern t_str_cvt_result utf8_chars_to_sys_chars(u64 const utf8_chars_size, const u8* const utf8_chars, u64 const sys_chars_cap_size, u8* const sys_chars);
#endif

#ifdef IMPORT_CORE_ERROR
extern t_any McoreFNget_error_data(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNget_error_id(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNget_error_msg(t_thrd_data* const thrd_data, const t_any* const args);

extern t_any builtin_error__new(t_thrd_data* const thrd_data, const char* const owner, const char* const id_ascii, const char* const msg_ascii);
extern t_any error__new__own(t_thrd_data* const thrd_data, const char* const owner, t_any const id, t_any const msg, t_any const data);
extern void error__show_and_exit(t_thrd_data* const thrd_data, t_any const error_arg);

[[gnu::cold, clang::noinline]]
extern t_any error__invalid_type(t_thrd_data* const thrd_data, const char* const owner);
[[gnu::cold, clang::noinline]]
extern t_any error__invalid_enc(t_thrd_data* const thrd_data, const char* const owner);
[[gnu::cold, clang::noinline]]
extern t_any error__fail_text_recoding(t_thrd_data* const thrd_data, const char* const owner);
[[gnu::cold, clang::noinline]]
extern t_any error__wrong_num_fn_args(t_thrd_data* const thrd_data, const char* const owner);
[[gnu::cold, clang::noinline]]
extern t_any error__out_of_bounds(t_thrd_data* const thrd_data, const char* const owner);
#endif

#ifdef IMPORT_CORE
extern t_any McoreFN__add__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__and__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__ashr__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__concat__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__div__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__equal__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__get_item__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__great__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__great_or_eq__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__less__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__less_or_eq__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__lshr__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__mod__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__mul__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__not_equal__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__or__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__shl__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__slice__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__strong_equal__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__strong_not_equal__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__sub__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFN__xor__(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNabs(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNacos(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNall_kv(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNall(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNany_kv(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNany(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNappend_new(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNappend(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_ctrlB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_digitB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_graphicB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_letterB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_lowerB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_spaceB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_symbolB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_to_lower(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_to_upper(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNascii_upperB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNasciiB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNasin(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNassemble_time(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNatan(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNbox_len(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNbreak(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNcap(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNceil(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNchar_lang(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNchar_to_uchar(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNclear(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNcmp(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNcopy(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNcos(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNctrlB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNday(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNdelete(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNdeserialize(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNdigitB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNdisassemble_time(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNdrop_while(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNdrop(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNdump(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNduration(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNeach_to_each_kv(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNeach_to_each(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNempty_val(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNemptyB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNevenB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNexit(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNexp(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNexp2(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfail(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfilter_kv(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfilter(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfirst(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfloor(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfoldl_kv(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfoldl(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfoldl1(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfoldr_kv(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfoldr(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNfoldr1(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNformat_int(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNgraphicB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNhash(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNhour(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNimplement(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNin_rangeB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNinB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNinfB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNinherits_from(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNinit(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNinsert_part_to(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNinsert_to(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNint_to_time(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNjoin(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNkey_ofB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlast(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlen(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNletterB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlog__debug_msg(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlog__msg(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlog(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlog2(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlog10(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlook_from_end_in(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlook_in(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlook_other_from_end_in(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlook_other_in(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlook_part_from_end_in(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlook_part_in(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNloop(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNlowerB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmake_channel(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmap_kv_with_state(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmap_kv(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmap_with_state(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmap(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmax_val(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmax(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmin_val(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmin(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNminute(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmonth(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNmove(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNnanB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNneg_bits(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNneg_infB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNneg(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNnot(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNnullB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNobj_to_table(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNoddB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNokA(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNparse_int(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNpart_ofB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNpop(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNpower(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNprefix_ofB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNprint(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNprintln(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNpush(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNqty_0_bits(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNqty_1_bits(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNqty_leading_0_bits(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNqty_leading_1_bits(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNqty_trailing_0_bits(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNqty_trailing_1_bits(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNqty(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNread_from_memAAA(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNremove(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNrepeat(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNreplace_part(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNreplace(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNreserve(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNreverse_bits(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNreverse_bytes(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNreverse(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNrnd(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNrotate_left(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNrotate_right(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNround(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNscarlet__joinAAA(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNscarlet__splitAAA(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsecond(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNself_copy(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNserialize(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNset_log_level(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNshort_typeB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNshuffle_box(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsin(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsleep(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsort_fn(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsort(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNspaceB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsplit_by_part(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsplit(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsqr(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsqrt(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNstable_sort_fn(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNstable_sort(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNstr_to_tkn(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNstr_to_ustr(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsuffix_ofB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNsymbolB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtable_to_obj(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtake_one_kv(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtake_one(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtake_while(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtake(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtan(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNthread(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtime_limit_pop(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtime_limit_wait(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtime(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtimezone(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_array(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_bool(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_box(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_byte(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_char(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_float(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_int(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_lower(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_string(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNto_upper(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtrim(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtrunc(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNtype(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNuchar_to_ustr(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNuncopyable_errorB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNunslice(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNupperB(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNuprint(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNuprintln(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNustr_to_str(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNwait(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNwrite_to_memAAA(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNyear(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNyield(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNzip_n(t_thrd_data* const thrd_data, const t_any* const args);
extern t_any McoreFNzip(t_thrd_data* const thrd_data, const t_any* const args);
#endif
