#pragma once

#include "../../common/const.h"
#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"

[[clang::always_inline]]
static u8 fn__get_params_cap(t_any const fn) {
     assume(fn.bytes[15] == tid__fn);

     return *(u16*)&fn.bytes[8] >> 5 & 0x1f;
}

[[clang::always_inline]]
static t_any fn__set_params_cap(t_any fn, u8 const cap) {
     assume(fn.bytes[15] == tid__fn);
     assume(cap <= 16);

     u16* const metadata_2_bytes = (u16*)&fn.bytes[8];
     *metadata_2_bytes           = *metadata_2_bytes & 0xfc1f | (u16)cap << 5;
     return fn;
}

[[clang::always_inline]]
static u64 fn__get_borrowed_len(t_any const fn) {
     assume(fn.bytes[15] == tid__fn);

     return fn.qwords[1] >> 10 & fn_borrowed_len_bit_mask;
}

[[clang::always_inline]]
static t_any fn__set_borrowed_len(t_any fn, u64 const borrowed_len) {
     assume(fn.bytes[15] == tid__fn);

     fn.qwords[1] = (fn.qwords[1] & ~((u64)fn_borrowed_len_bit_mask << 10)) | borrowed_len << 10;
     return fn;
}

[[clang::always_inline]]
static bool fn__is_linear_alloc(t_any const fn) {
     assume(fn.bytes[15] == tid__fn);

     return (fn.qwords[1] >> (10 + fn_borrowed_len_bits) & 1) == 1;
}

[[clang::always_inline]]
static t_any fn__set_is_linear_alloc_flag(t_any fn, bool const flag) {
     assume(fn.bytes[15] == tid__fn);

     u64 const flag_in_bits = (u64)1 << (10 + fn_borrowed_len_bits);
     fn.qwords[1] = (fn.qwords[1] | flag_in_bits) ^ (flag ? 0 : flag_in_bits);
     return fn;
}

[[clang::always_inline]]
static u16 fn__get_holders_mask(t_any const fn) {
     assume(fn.bytes[15] == tid__fn);

     return fn.qwords[1] >> (11 + fn_borrowed_len_bits) & 0xffffuwb;
}

[[clang::always_inline]]
static t_any fn__set_holders_mask(t_any fn, u16 const mask) {
     assume(fn.bytes[15] == tid__fn);

     fn.qwords[1] = fn.qwords[1] & ~((u64)0xffffuwb << (11 + fn_borrowed_len_bits)) | (u64)mask << (11 + fn_borrowed_len_bits);
     return fn;
}

[[clang::always_inline]]
static t_any fn__set_metadata(t_any fn, u8 const len, u8 const cap, u64 const borrowed_len, bool const is_linear_alloc, u16 const holders_mask) {
     assume(fn.bytes[15] == tid__fn);
     assume(len <= 16);
     assume(cap <= 16);

     fn.qwords[1] = ((u64)tid__fn << 56) | len | (u64)cap << 5 | borrowed_len << 10 | (is_linear_alloc ? (u64)1 << (10 + fn_borrowed_len_bits) : 0) | ((u64)holders_mask << (11 + fn_borrowed_len_bits));
     return fn;
}

[[clang::always_inline]]
static t_ptr64* fn__get_address(t_thrd_data* thrd_data, t_any const fn) {
     assume(fn.bytes[15] == tid__fn);

     t_any* const fn_data = fn__is_linear_alloc(fn) ? (t_any*)linear__get_mem_of_idx(&thrd_data->linear_allocator, fn.qwords[0]) : (t_any*)fn.qwords[0];

     return (t_ptr64*)&fn_data->qwords[1];
}

[[clang::always_inline]]
static t_any* fn__get_args(t_thrd_data* thrd_data, t_any const fn) {
     assume(fn.bytes[15] == tid__fn);

     t_any* const fn_data = fn__is_linear_alloc(fn) ? (t_any*)linear__get_mem_of_idx(&thrd_data->linear_allocator, fn.qwords[0]) : (t_any*)fn.qwords[0];
     t_any* const args    = &fn_data[1];

     assume_aligned(args, 16);

     return args;
}

[[clang::always_inline]]
static t_any* fn__get_borrowed(t_thrd_data* thrd_data, t_any const fn) {
     assume(fn.bytes[15] == tid__fn);

     t_any* const fn_data = fn__is_linear_alloc(fn) ? (t_any*)linear__get_mem_of_idx(&thrd_data->linear_allocator, fn.qwords[0]) : (t_any*)fn.qwords[0];
     t_any* const borrowed = &fn_data[1 + fn__get_params_cap(fn)];

     assume_aligned(borrowed, 16);

     return borrowed;
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