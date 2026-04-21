#pragma once

#include "../../common/const.h"
#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"

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