#pragma once

#include "const.h"
#include "include.h"

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

union {
     void* pointer;
     u64   bits;
} typedef t_ptr64;

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

union {
     const u8* char_position;
     const u8* byte_position;
     struct {
          t_any* item_position;
          bool   is_mut;
     }         state;
} typedef t_zip_read_state;

union {
     const u8* char_position;
     const u8* byte_position;
     struct {
          t_any* item_position;
          bool   is_mut;
     }         array_state;
     struct {
          bool   is_fix; // false
          bool   is_mut;
          u16    chunk_size;
          t_any* items;
          u64    hash_seed;
          u64    last_idx;
          u64    items_len;
          u64    steps;
     }         hash_map_state;
     struct {
          bool   is_fix; // true
          bool   is_mut;
          u8     fix_idx_offset;
          u8     fix_idx_size;
          t_any* items;
     }         fix_obj_state;
} typedef t_zip_search_state;
