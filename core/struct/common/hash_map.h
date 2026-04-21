#pragma once

#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"

[[clang::always_inline]]
static u64 hash_map__get_len(t_any const map) {
     assert(map.bytes[15] == tid__obj || map.bytes[15] == tid__table || map.bytes[15] == tid__set);

     return map.qwords[1] & hash_map_len_bit_mask;
}

[[clang::always_inline]]
static t_any hash_map__set_len(t_any map, u64 const len) {
     assert(map.bytes[15] == tid__obj || map.bytes[15] == tid__table || map.bytes[15] == tid__set);
     assert(len <= hash_map_max_len);

     map.qwords[1] = map.qwords[1] & (u64)-1 << hash_map_len_bits | len;
     return map;
}

[[clang::always_inline]]
static u64 hash_map__get_last_idx(t_any const map) {
     assert(map.bytes[15] == tid__obj || map.bytes[15] == tid__table || map.bytes[15] == tid__set);

     u64 const last_idx_data = map.qwords[1] >> hash_map_len_bits & hash_map_last_idx_data_bit_mask;
     return ((u64)1 << last_idx_data) - 1;
}

[[clang::always_inline]]
static u16 hash_map__get_chunk_size(t_any const map) {
     assert(map.bytes[15] == tid__obj || map.bytes[15] == tid__table || map.bytes[15] == tid__set);

     return (map.qwords[1] >> (hash_map_len_bits + hash_map_last_idx_data_bits) & hash_map_chunk_size_data_bit_mask) + 1;
}

[[clang::always_inline]]
static t_any hash_map__set_metadata(t_any map, u64 const len, u64 const last_idx, u16 const chunk_size) {
     assert(map.bytes[15] == tid__obj || map.bytes[15] == tid__table || map.bytes[15] == tid__set);
     assert((last_idx + 1) * chunk_size >= len);
     assume(len <= hash_map_max_len);
     assume(__builtin_popcountll(last_idx + 1) == 1);
     assume((u64)__builtin_popcountll(last_idx) < ((u64)1 << hash_map_last_idx_data_bits));
     assume(chunk_size >= 1 && chunk_size <= hash_map_max_chunk_size);

     map.qwords[1] = map.qwords[1] & ((u64)0xff << 56) | len | (u64)__builtin_popcountll(last_idx) << hash_map_len_bits | (u64)(chunk_size - 1) << (hash_map_len_bits + hash_map_last_idx_data_bits);
     return map;
}

[[clang::always_inline]]
static u64 hash_map__get_hash_seed(t_any const map) {
     assert(map.bytes[15] == tid__obj || map.bytes[15] == tid__table || map.bytes[15] == tid__set);

     return ((u64*)map.qwords[0])[1];
}

[[clang::always_inline]]
static void hash_map__set_hash_seed(t_any const map, u64 const seed) {
     assert(map.bytes[15] == tid__obj || map.bytes[15] == tid__table || map.bytes[15] == tid__set);
     assert(get_ref_cnt(map) != 0);

     ((u64*)map.qwords[0])[1] = seed;
}