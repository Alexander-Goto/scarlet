#pragma once

#include "../../common/fn.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../../struct/null/basic.h"
#include "../../struct/token/basic.h"

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

core_string t_any string_from_ascii(const char* const string);

[[clang::noinline]]
core_error t_any builtin_error__new(t_thrd_data* const thrd_data, const char* const owner, const char* const id_ascii, const char* const msg_ascii) {
     u64   const len = strlen(id_ascii);
     t_any const id  = token_from_ascii_chars(len, id_ascii);
     t_any const msg = string_from_ascii(msg_ascii);

     assert(id.bytes[15] == tid__token);

     t_error* const error = aligned_alloc(16, sizeof(t_error));
     *error               = (const t_error) {
          .ref_cnt = 1,
          .owner     = ptr_to_ptr64(owner),
          .id        = id,
          .msg       = msg,
          .data      = null
     };

#ifdef CALL_STACK
     error->call_stack__len = thrd_data->call_stack.len;
     error->call_stack      = malloc(error->call_stack__len * sizeof(char*));
     memcpy(error->call_stack, thrd_data->call_stack.stack, thrd_data->call_stack.len * sizeof(char*));
#endif

     return (const t_any){.structure = {.value = (u64)error, .type = tid__error}};
}

[[gnu::cold, clang::noinline]]
core_error t_any error__invalid_type(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "invalid_type", "Invalid type.");}

[[gnu::cold, clang::noinline]]
core_error t_any error__invalid_enc(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "text_encoding", "Invalid string encoding.");}

[[gnu::cold, clang::noinline]]
core_error t_any error__fail_text_recoding(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "text_recoding", "When encoding a string from one encoding to another, a character was detected that was not in the target encoding.");}

[[gnu::cold, clang::noinline]]
core_error t_any error__wrong_num_fn_args(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "wrong_num_fn_args", "Wrong number of function arguments.");}

[[gnu::cold, clang::noinline]]
core_error t_any error__out_of_bounds(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "out_of_bounds", "Going beyond boundaries.");}

[[gnu::cold, clang::noinline]]
static t_any error__cant_print(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "print", "Failed to print anything.");}

[[gnu::cold, clang::noinline]]
static t_any error__cant_get_current_time(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "get_time", "Could not get the current time.");}

[[gnu::cold, clang::noinline]]
static t_any error__bits_shift(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "bits_shift", "Incorrect number of bit shifts.");}

[[gnu::cold, clang::noinline]]
static t_any error__div_by_0(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "div_by_0", "Division by zero.");}

[[gnu::cold, clang::noinline]]
static t_any error__nan_cmp(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "nan_cmp", "Comparsion with NaN.");}

[[gnu::cold, clang::noinline]]
static t_any error__invalid_range(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "invalid_range", "Invalid range.");}

[[gnu::cold, clang::noinline]]
static t_any error__not_uniq_key(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "not_uniq_key", "Using the same keys when initializing an object, table or set.");}

[[gnu::cold, clang::noinline]]
static t_any error__unpacking_not_enough_items(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "unpk_not_enough_items", "When unpacking a constant, it contained fewer items than expected.");}

[[gnu::cold, clang::noinline]]
static t_any error__invalid_items_num(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "invalid_items_num", "Invalid numbers of items.");}

[[gnu::cold, clang::noinline]]
static t_any error__shuffle_dup(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "shuffle_dup", "When shuffling the box, the same index is indicated several times.");}

[[gnu::cold, clang::noinline]]
static t_any error__sqrt_from_neg(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "sqrt_from_neg", "Calculating the square root of a negative number.");}

[[gnu::cold, clang::noinline]]
static t_any error__cant_ord(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "cant_ord", "Items cannot be ordered.");}

[[gnu::cold, clang::noinline]]
static t_any error__share_invalid_type(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "share_invalid_type", "Sharing from thread to thread of data of an invalid type.");}

[[gnu::cold, clang::noinline]]
static t_any error__cant_read_from_channel(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "cant_read_from_channel", "Can't read from a channel.");}

[[gnu::cold, clang::noinline]]
static t_any error__cant_write_to_channel(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "cant_write_to_channel", "Can't write to a channel.");}

[[gnu::cold, clang::noinline]]
static t_any error__no_writing_channel(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "no_writing_channel", "Reading from a channel that has no items and to which no one will add them.");}

[[gnu::cold, clang::noinline]]
static t_any error__no_reading_channel(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "no_reading_channel", "Writing to a channel that no one reads.");}

[[gnu::cold, clang::noinline]]
static t_any error__deserialization_from_incorrect_data(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "incorrect_data", "Deserialization from incorrect data.");}

[[gnu::cold, clang::noinline]]
static t_any error__ret_incorrect_value(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "ret_incorrect_value", "A function returned a constant of the correct type, but its value is incorrect.");}

[[gnu::cold, clang::noinline]]
static t_any error__insomnia(t_thrd_data* const thrd_data, const char* const owner) {return builtin_error__new(thrd_data, owner, "insomnia", "Failed to put a thread to sleep.");}
