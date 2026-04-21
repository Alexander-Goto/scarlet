#pragma once

#include "const.h"
#include "fn.h"
#include "macro.h"
#include "type.h"

[[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
static u64 const xxh3_secret[24] = {
     0xbe4ba423396cfeb8ull, 0x1cad21f72c81017cull, 0xdb979083e96dd4deull, 0x1f67b3b7a4a44072ull,
     0x78e5c0cc4ee679cbull, 0x2172ffcc7dd05a82ull, 0x8e2443f7744608b8ull, 0x4c263a81e69035e0ull,
     0xcb00c391bb52283cull, 0xa32e531b8b65d088ull, 0x4ef90da297486471ull, 0xd8acdea946ef1938ull,
     0x3f349ce33f76faa8ull, 0x1d4f0bc7c7bbdcf9ull, 0x3159b4cd4be0518aull, 0x647378d9c97e9fc8ull,
     0xc3ebd33483acc5eaull, 0xeb6313faffa081c5ull, 0x49daf0b751dd0d17ull, 0x9e68d429265516d3ull,
     0xfca1477d58be162bull, 0xce31d07ad1b8f88full, 0x280416958f3acb45ull, 0x7e404bbbcafbd7afull,
};

[[clang::always_inline]]
static u64 xxh64_avalanche(u64 hash) {
     hash ^= hash >> 33;
     hash *= 0xc2b2ae3d27d4eb4full;
     hash ^= hash >> 29;
     hash *= 0x165667b19e3779f9ull;
     hash ^= hash >> 32;
     return hash;
}

[[clang::always_inline]]
static u64 xxh3_rrmxmx(u64 hash, u64 const size) {
     hash ^= __builtin_rotateleft64(hash, 49) ^ __builtin_rotateleft64(hash, 24);
     hash *= 0x9fb21c651e98df25ull;
     hash ^= (hash >> 35) + size;
     hash *= 0x9fb21c651e98df25ull;
     hash ^= hash >> 28;
     return hash;
}

[[clang::always_inline]]
static u64 xxh3_avalanche(u64 hash) {
     hash ^= hash >> 37;
     hash *= 0x165667919e3779f9ull;
     hash ^= hash >> 32;
     return hash;
}

[[clang::always_inline]]
static u64 xxh3_mix(const u8* const data, const ua_u64* const secret, u64 const seed) {
     u64 const  low  = *(ua_u64*)data ^ (secret[0] + seed);
     u64 const  high = ((ua_u64*)data)[1] ^ (secret[1] - seed);
     u128 const mul  = (u128)low * (u128)high;

     return (u64)mul ^ (u64)(mul >> 64);
}

[[clang::always_inline]]
static void init_secret(u64* const secret, u64 const seed) {
     if (seed == 0) [[clang::unlikely]] memcpy_inline(secret, xxh3_secret, sizeof(xxh3_secret));

     typedef u64 t_vec2 __attribute__((ext_vector_type(2)));
     typedef u64 t_vec8 __attribute__((ext_vector_type(8), aligned(64)));

     t_vec8 const seed_vec = (const t_vec2){seed, -seed}.s01010101;

     *(t_vec8*) secret       = *(const t_vec8*)xxh3_secret + seed_vec;
     *(t_vec8*)(secret + 8)  = *(const t_vec8*)(xxh3_secret + 8) + seed_vec;
     *(t_vec8*)(secret + 16) = *(const t_vec8*)(xxh3_secret + 16) + seed_vec;
}

[[clang::always_inline]]
static void xxh3_accumulate_512(u64* const acc, const u8* const data, const ua_u64* const secret) {
     typedef u64 t_a_vec __attribute__((ext_vector_type(8), aligned(64)));
     typedef u64 t_ua_vec __attribute__((ext_vector_type(8), aligned(1)));

     t_a_vec const data_vec = *(const t_ua_vec*)data;
     t_a_vec const data_key = data_vec ^ *(const t_ua_vec*)secret;

     *(t_a_vec*)acc += (data_key & 0xffffffffull) * (data_key >> 32) + data_vec.s10325476;
}

[[clang::always_inline]]
static void xxh3_accumulate(u64* const acc, const u8* const data, const ua_u64* const secret, u64 const stripes_len) {
     for (
          u64 stripe_idx = 0;
          stripe_idx != stripes_len;

          xxh3_accumulate_512(acc, data + stripe_idx * 64, secret + stripe_idx),
          stripe_idx += 1
     );
}

[[clang::always_inline]]
static void xxh3_scramble(u64* const acc, const ua_u64* const secret) {
     typedef u64 t_vec __attribute__((ext_vector_type(8), aligned(64)));

     t_vec const prime    = 0x9e3779b1ull;
     t_vec const acc_vec  = *(t_vec*)acc;
     t_vec const data_key = (acc_vec >> 47) ^ acc_vec ^ *(const t_vec*)secret;

     *(t_vec*)acc = (data_key & 0xffffffffull) * prime + (((data_key >> 32) * prime) << 32);
}

static void xxh3_hash_long_loop(u64* const acc, const u8* const data, u64 const size, const ua_u64* const secret) {
     u64 const blocks_len = (size - 1) >> 10;
     for (
          u64 block_idx = 0;
          block_idx != blocks_len;

          xxh3_accumulate(acc, data + (block_idx << 10), secret, 16),
          xxh3_scramble(acc, secret + 16),
          block_idx += 1
     );

     u64 const stripes_len = (size - (blocks_len << 10) - 1) >> 6;
     xxh3_accumulate(acc, data + (blocks_len << 10), secret, stripes_len);
     xxh3_accumulate_512(acc, data + size - 64, (const ua_u64*)((const u8*)secret + 121));
}

[[clang::always_inline]]
static u64 xxh3_merge_accs(u64* const acc, const ua_u64* const secret, u64 hash) {
     for (u64 idx = 0; idx < 8; idx += 2) {
          u128 const mul = (u128)(acc[idx] ^ secret[idx]) * (u128)(acc[idx + 1] ^ secret[idx + 1]);
          hash          += (u64)mul ^ (u64)(mul >> 64);
     }

     return xxh3_avalanche(hash);
}

static u64 xxh3_hash_long(const u8* const data, u64 const size, const u64* const secret) {
     [[gnu::aligned(64)]]
     u64 acc[8] = {
          0xc2b2ae3dull,         0x9e3779b185ebca87ull,
          0xc2b2ae3d27d4eb4full, 0x165667b19e3779f9ull,
          0x85ebca77c2b2ae63ull, 0x85ebca77ull,
          0x27d4eb2f165667c5ull, 0x9e3779b1ull,
     };

     xxh3_hash_long_loop(acc, data, size, secret);
     return xxh3_merge_accs(acc, (const ua_u64*)((const u8*)secret + 11), size * 0x9e3779b185ebca87ull);
}

static inline u64 xxh3_get_hash(const u8* const data, u64 const size, u64 const seed) {
     typedef u64 t_vec __attribute__((ext_vector_type(8)));

     switch (-__builtin_reduce_add((t_vec)size > (const t_vec){0, 3, 8 ,16, 128, 240, -1, -1})) {
     case 0:
          assert(size == 0);
          return xxh64_avalanche(seed ^ xxh3_secret[7] ^ xxh3_secret[8]);
     case 1:
          assume(size >= 1 && size <= 3);

          return xxh64_avalanche(
               (u64)(
                    ((u32)data[0] << 16) |
                    ((u32)data[size >> 1] << 24) |
                    (u32)data[size - 1] |
                    ((u32)size << 8)
               ) ^ ((((xxh3_secret[0] << 32) ^ xxh3_secret[0]) >> 32) + seed)
          );
     case 2:
          assume(size >= 4 && size <= 8);

          return xxh3_rrmxmx(
               (
                    (u64)*(const ua_u32*)(data + size - 4) +
                    ((u64)*(const ua_u32*)data << 32)
               ) ^ (
                    (xxh3_secret[1] ^ xxh3_secret[2]) -
                    (seed ^ ((u64)__builtin_bswap32(seed) << 32))
               ),

               size
          );
     case 3: {
          assume(size >= 9 && size <= 16);

          u64 const low = *(const ua_u64*)data ^ ((xxh3_secret[3] ^ xxh3_secret[4]) + seed);
          u64 const high = *(const ua_u64*)(data + size - 8) ^ ((xxh3_secret[5] ^ xxh3_secret[6]) - seed);
          u128 const mul = (u128)low * (u128)high;
          return xxh3_avalanche(
               size +
               __builtin_bswap64(low) +
               high +
               ((u64)mul ^ (u64)(mul >> 64))
          );
     }
     case 4: {
          assume(size >= 17 && size <= 128);

          u64 hash = size * 0x9e3779b185ebca87ull;
          switch (
               (u8)(size > 32) +
               (u8)(size > 64) +
               (u8)(size > 96)
          ) {
          case 3: hash +=
               xxh3_mix(data + 48, xxh3_secret + 12, seed) +
               xxh3_mix(data + size - 64, xxh3_secret + 14, seed);
          case 2: hash +=
               xxh3_mix(data + 32, xxh3_secret + 8, seed) +
               xxh3_mix(data + size - 48, xxh3_secret + 10, seed);
          case 1: hash +=
               xxh3_mix(data + 16, xxh3_secret + 4, seed) +
               xxh3_mix(data + size - 32, xxh3_secret + 6, seed);
          case 0: hash +=
               xxh3_mix(data, xxh3_secret, seed) +
               xxh3_mix(data + size - 16, xxh3_secret + 2, seed);
               return xxh3_avalanche(hash);
          default: unreachable;
          }
     }
     case 5: {
          assume(size >= 129 && size <= 240);

          u64 hash = size * 0x9e3779b185ebca87ull;
          for (
               u64 counter = 0;
               counter < 8;

               hash += xxh3_mix(data + 16 * counter, xxh3_secret + 2 * counter, seed),
               counter += 1
          );

          hash = xxh3_avalanche(hash);
          hash += xxh3_mix(data + size - 16, (const ua_u64*)((const u8*)xxh3_secret + 119), seed);
          switch ((u8)size >> 4) {
          case 15: hash += xxh3_mix(data + 224, (const ua_u64*)((const u8*)xxh3_secret + 99), seed);
          case 14: hash += xxh3_mix(data + 208, (const ua_u64*)((const u8*)xxh3_secret + 83), seed);
          case 13: hash += xxh3_mix(data + 192, (const ua_u64*)((const u8*)xxh3_secret + 67), seed);
          case 12: hash += xxh3_mix(data + 176, (const ua_u64*)((const u8*)xxh3_secret + 51), seed);
          case 11: hash += xxh3_mix(data + 160, (const ua_u64*)((const u8*)xxh3_secret + 35), seed);
          case 10: hash += xxh3_mix(data + 144, (const ua_u64*)((const u8*)xxh3_secret + 19), seed);
          case 9:  hash += xxh3_mix(data + 128, (const ua_u64*)((const u8*)xxh3_secret + 3), seed);
          case 8:  return xxh3_avalanche(hash);
          default: unreachable;
          }
     }
     case 6: {
          assume(size > 240);

          [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
          u64 secret[24];
          init_secret(secret, seed);
          return xxh3_hash_long(data, size, secret);
     }
     default:
          unreachable;
     }
}

[[gnu::hot]]
core_basic u64 get_hash(const u8* const data, u64 const size, u64 const seed) {
     return xxh3_get_hash(data, size, seed);
}
