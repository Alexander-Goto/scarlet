#pragma once

#include "../../common/macro.h"
#include "../../common/type.h"

struct {
     t_mtx  mtx;
     cnd_t  cnd;
     u64    main_ref_cnt;
     u64    writers_len;
     u64    readers_len;
     u64    waiting_thrds_len;
     u64    items_len;
     u64    items_cap;
     u64    idx_of_first_item;
     t_any* items;
} typedef t_channel;

struct {
     u64        local_ref_cnt;
     t_channel* channel;
} typedef t_channel_ptr;

[[clang::always_inline]]
static u8 channel__allow_read(t_any const channel) {
     assume(channel.bytes[15] == tid__channel);

     return (channel.bytes[8] & 1) == 1;
}

[[clang::always_inline]]
static t_any channel__set_read_flag(t_any channel, bool const flag) {
     assume(channel.bytes[15] == tid__channel);

     channel.bytes[8] = (channel.bytes[8] | 1) ^ (flag ? 0 : 1);
     return channel;
}

[[clang::always_inline]]
static u8 channel__allow_write(t_any const channel) {
     assume(channel.bytes[15] == tid__channel);

     return (channel.bytes[8] & 2) == 2;
}

[[clang::always_inline]]
static t_any channel__set_write_flag(t_any channel, bool const flag) {
     assume(channel.bytes[15] == tid__channel);

     channel.bytes[8] = (channel.bytes[8] | 2) ^ (flag ? 0 : 2);
     return channel;
}