#pragma once

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

#define hash_map_max_len                  0x80'0000'0000ull
#define hash_map_max_chunks               0x8000'0000ull
#define hash_map_max_chunk_size           256
#define hash_map_len_bits                 40
#define hash_map_last_idx_data_bits       5
#define hash_map_chunk_size_data_bits     8
#define hash_map_len_bit_mask             0xff'ffff'ffffull
#define hash_map_last_idx_data_bit_mask   0x1f
#define hash_map_chunk_size_data_bit_mask 0xff

#define fn_max_borrowed_len      0xffff
#define fn_borrowed_len_bits     16
#define fn_borrowed_len_bit_mask 0xffff

#define rabin_karp_q 0x7fff'ffffull

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

#if CACHE_LINE_SIZE == 64
#define ALIGN_FOR_CACHE_LINE 64
#elif
#define ALIGN_FOR_CACHE_LINE (__builtin_popcount(CACHE_LINE_SIZE)==1?CACHE_LINE_SIZE:1<<(sizeof(int)*8-__builtin_clz((unsigned int)CACHE_LINE_SIZE)))
#endif
