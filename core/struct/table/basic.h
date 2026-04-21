#pragma once

#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../../struct/common/hash_map.h"
#include "../../struct/int/fn.h"
#include "../../struct/null/basic.h"

static t_any const empty_table = {.structure = {.type = tid__table, .value = (u64)(const t_any[3]) {
     {},
     null,
     null,
}}};

[[clang::always_inline]]
static t_any* table__get_kvs(t_any const table) {
     assume(table.bytes[15] == tid__table);

     t_any* const kvs = &((t_any*)table.qwords[0])[1];

     assume_aligned(kvs, 16);

     return kvs;
}

[[clang::always_inline]]
static bool table__need_to_fit(t_any const table) {
     assume(table.bytes[15] == tid__table);

     u64 const last_idx   = hash_map__get_last_idx(table);
     u16 const chunk_size = hash_map__get_chunk_size(table);
     u64 const items_len  = (last_idx + 1) * chunk_size;
     u64 const len        = hash_map__get_len(table);

     return items_len >= len * 4;
}

static t_any table__init(t_thrd_data* const thrd_data, u64 const cap, const char* const owner) {
     if (cap > hash_map_max_len) [[clang::unlikely]] fail_with_call_stack(thrd_data, "The maximum capacity of a table has been exceeded.", owner);

     if (cap == 0) return empty_table;

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

     t_any* const items = aligned_alloc(16, max_chunks * chunk_size * 32 + 16);
     t_any        table = {.structure = {.value = (u64)items, .type = tid__table}};
     set_ref_cnt(table, 1);
     hash_map__set_hash_seed(table, int__rnd(thrd_data));
     table = hash_map__set_metadata(table, 0, max_chunks-1, chunk_size);
     memset(&items[1], tid__null, max_chunks * chunk_size * 32);
     return table;
}

core_basic t_any* table__get_kvs_and_len(t_any const table, u64* const len) {
     assert(table.bytes[15] == tid__table);

     *len = hash_map__get_len(table);
     return table__get_kvs(table);
}
