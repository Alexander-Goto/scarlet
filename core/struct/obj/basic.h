#pragma once

#include "../../common/macro.h"
#include "../../common/type.h"
#include "../common/hash_map.h"
#include "../int/fn.h"
#include "../null/basic.h"

static t_any const empty_obj = {.structure = {.type = tid__obj, .value = (u64)(const t_any[4]) {
     {.qwords={0xf000'0000'0000'0000ull, 0}},
     null,
     null,
     null,
}}};

[[clang::always_inline]]
static t_any* obj__get_mtds(t_any const obj) {
     assume(obj.bytes[15] == tid__obj);

     t_any* const mtds = &((t_any*)obj.qwords[0])[1];

     assume_aligned(mtds, 16);
     assume(mtds->bytes[15] == tid__null || mtds->bytes[15] == tid__obj);

     return mtds;
}

[[clang::always_inline]]
static t_any* obj__get_fields(t_any const obj) {
     assume(obj.bytes[15] == tid__obj);

     t_any* const fields = &((t_any*)obj.qwords[0])[2];

     assume_aligned(fields, 16);

     return fields;
}

[[clang::always_inline]]
static u64 obj__get_fields_len(t_any const obj) {
     assume(obj.bytes[15] == tid__obj);

     u64 len = hash_map__get_len(obj);
     len     = ((const t_any*)obj.qwords[0])[1].bytes[15] == tid__null ? len : len - 1;
     return len;
}

[[clang::always_inline]]
static u8 obj__get_fix_idx_offset(t_any const obj) {
     assume(obj.bytes[15] == tid__obj);

     u8 const offset = (((const u8*)obj.qwords[0])[7] >> 1);

     assume(offset <= 120);

     return offset;
}

[[clang::always_inline]]
static u8 obj__get_fix_idx_size(t_any const obj) {
     assume(obj.bytes[15] == tid__obj);

     u8 const size = __builtin_popcountll(hash_map__get_last_idx(obj));

     assume(size <= __builtin_popcountll(hash_map_max_chunks - 1));

     return size;
}

[[clang::always_inline]]
static bool obj__need_to_fit(t_any const obj) {
     assume(obj.bytes[15] == tid__obj);

     u8  const fix_idx_offset = obj__get_fix_idx_offset(obj);
     u64 const last_idx       = hash_map__get_last_idx(obj);
     u16 const chunk_size     = hash_map__get_chunk_size(obj);
     u64 const items_len      = (last_idx + 1) * chunk_size;
     u64 const fields_len     = obj__get_fields_len(obj);

     return fix_idx_offset == 120 && items_len >= fields_len * 4;
}

static t_any obj__init(t_thrd_data* const thrd_data, u64 const cap, const char* const owner) {
     if (cap > hash_map_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum capacity of a object has been exceeded.", owner);

     if (cap == 0) return empty_obj;

     u16 chunk_size;
     u64 max_chunks;
     if (cap < 5) {
          chunk_size = 1;
          max_chunks = cap == 3 ? 4 : cap;
     } else {
          chunk_size = (sizeof(long long) * 8 - 1 - __builtin_clzll(cap)) / 4 + 1;
          max_chunks = (cap + chunk_size) / chunk_size;
          max_chunks = __builtin_popcountll(max_chunks) == 1 ? max_chunks : (u64)1 << (sizeof(long long) * 8 - __builtin_clzll(max_chunks));
          max_chunks = max_chunks * chunk_size >= cap * 5 / 4 ? max_chunks : max_chunks * 2;
          if (max_chunks > hash_map_max_chunks) {
               max_chunks = hash_map_max_chunks;
               chunk_size = cap / max_chunks;
               chunk_size = chunk_size * max_chunks == cap ? chunk_size : chunk_size + 1;
          }

          assert(max_chunks * chunk_size >= cap);
     }

     t_any* const items = aligned_alloc(16, max_chunks * chunk_size * 32 + 32);
     t_any        obj   = {.structure = {.value = (u64)items, .type = tid__obj}};
     items[0]           = (const t_any){.qwords = {0xf000'0000'0000'0001ull, int__rnd(thrd_data)}};
     obj                = hash_map__set_metadata(obj, 0, max_chunks - 1, chunk_size);
     memset(&items[1], tid__null, max_chunks * chunk_size * 32 + 16);
     return obj;
}

static inline t_any obj__init_fix(u8 const fix_idx_offset, u8 const fix_idx_size) {
     assert(fix_idx_offset < 120);
     assert(fix_idx_size <= __builtin_popcountll(hash_map_max_chunks - 1));

     u64    const cap   = (u64)1 << fix_idx_size;
     t_any* const items = aligned_alloc(16, cap * 32 + 32);
     t_any        obj   = {.structure = {.value = (u64)items, .type = tid__obj}};
     items[0]           = (const t_any){.qwords = {1 | ((u64)fix_idx_offset << 57), 0}};
     obj                = hash_map__set_metadata(obj, 0, cap - 1, 1);
     memset(&items[1], tid__null, cap * 32 + 16);
     return obj;
}
