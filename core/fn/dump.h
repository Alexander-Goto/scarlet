#pragma once

#include "../common/fn.h"
#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/box/basic.h"
#include "../struct/channel/basic.h"
#include "../struct/common/fn_struct.h"
#include "../struct/custom/fn.h"
#include "../struct/obj/fn.h"
#include "../struct/obj/mtd.h"
#include "../struct/string/fn.h"
#include "../struct/table/basic.h"
#include "../struct/time/fn.h"
#include "../struct/token/basic.h"
#include "to_string.h"

[[clang::noinline, clang::minsize]]
core_basic void dump__add_string__own(t_thrd_data* const thrd_data, t_any* const result, t_any const string, const char* const owner) {
     if (string.bytes[15] == tid__short_string)
          *result = result->bytes[15] == tid__short_string ? concat__short_str__short_str(*result, string) : concat__long_str__short_str__own(thrd_data, *result, string, owner);
     else
          *result = result->bytes[15] == tid__short_string ? concat__short_str__long_str__own(thrd_data, *result, string, owner) : concat__long_str__long_str__own(thrd_data, *result, string, owner);
}

[[clang::noinline, clang::minsize]]
core_basic t_any dump__level_spaces(t_thrd_data* const thrd_data, u64 const level, const char* const owner) {
     t_any const result = short_string__repeat(thrd_data, (const t_any){.bytes = "  "}, level, owner);

     if (result.bytes[15] == tid__error) [[clang::unlikely]]
          error__show_and_exit(thrd_data, result);

     return result;
}

[[clang::noinline, clang::minsize]]
core_basic void dump__half_own(t_thrd_data* const thrd_data, t_any* const result__own, t_any const arg, u64 const level, u64 const sub_level, const char* const owner) {
     assert(result__own->bytes[15] == tid__short_string || result__own->bytes[15] == tid__string);

     dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, level, owner), owner);

     t_any const new_line_string = {.bytes = {'\n'}};
     switch (arg.bytes[15]) {
     case tid__short_string:
          short_string__dump__half_own(thrd_data, result__own, arg, owner);
          return;
     case tid__null:
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "null\n"}, owner);
          return;
     case tid__token:
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = ":"}, owner);
          *result__own = to_string__half_own(thrd_data, result__own, arg, owner);
          dump__add_string__own(thrd_data, result__own, new_line_string, owner);
          return;
     case tid__bool:
          dump__add_string__own(thrd_data, result__own, arg.bytes[0] == 1 ? (const t_any){.bytes = "true\n"} : (const t_any){.bytes = "false\n"}, owner);
          return;
     case tid__byte: {
          t_any byte_string    = {.bytes = "0bff\n"};
          u8 const byte        = arg.bytes[0];
          byte_string.bytes[2] = "0123456789abcdef"[byte >> 4];
          byte_string.bytes[3] = "0123456789abcdef"[byte & 0xf];
          dump__add_string__own(thrd_data, result__own, byte_string, owner);
          return;
     }
     case tid__char: {
          u32 const corsar_code = char_to_code(arg.bytes);
          t_any char_string = {.bytes = {'\''}};
          u8 offset;
          switch (corsar_code) {
          case '\t':
               memcpy_inline(&char_string.bytes[1], "\\t", 2);
               offset = 3;
               break;
          case '\n':
               memcpy_inline(&char_string.bytes[1], "\\n", 2);
               offset = 3;
               break;
          case '\\':
               memcpy_inline(&char_string.bytes[1], "\\\\", 2);
               offset = 3;
               break;
          case '\'':
               memcpy_inline(&char_string.bytes[1], "\\'", 2);
               offset = 3;
               break;
          default:
               if (char_is_graphic(corsar_code) && (corsar_code == ' ' || !char_is_space(corsar_code)))
                    offset = 1 + corsar_code_to_ctf8_char(corsar_code, &char_string.bytes[1]);
               else {
                    char_string.bytes[1] = '\\';
                    for (u8 idx = 1; idx < 4; idx += 1) {
                         u8  const byte                 = (corsar_code >> (24 - idx * 8)) & 0xff;
                         char_string.bytes[idx * 2]     = "0123456789abcdef"[byte >> 4];
                         char_string.bytes[idx * 2 + 1] = "0123456789abcdef"[byte & 0xf];
                    }
                    offset = 8;
               }
          }
          memcpy_inline(&char_string.bytes[offset], "\'\n", 2);
          dump__add_string__own(thrd_data, result__own, char_string, owner);
          return;
     }
     case tid__int: case tid__time: case tid__float:
          *result__own = to_string__half_own(thrd_data, result__own, arg, owner);
          dump__add_string__own(thrd_data, result__own, new_line_string, owner);
          return;
     case tid__short_fn: {
          u8 const params_len = common_fn__get_params_len(arg);
          if (params_len == 0)
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[fn()]\n"}, owner);
          else {
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[fn(_"}, owner);
               for (u8 counter = 1; counter < params_len; counter += 1)
                    dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = ", _"}, owner);
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = ")]\n"}, owner);
          }
          return;
     }
     case tid__short_byte_array: {
          u8 const len = short_byte_array__get_len(arg);
          switch (len) {
          case 0:
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[byte_array]\n"}, owner);
               return;
          case 1: case 2: case 3: case 4: {
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[byte_array "}, owner);
               t_any byte_string    = {.bytes = "0bff"};
               u8    byte           = arg.bytes[0];
               byte_string.bytes[2] = "0123456789abcdef"[byte >> 4];
               byte_string.bytes[3] = "0123456789abcdef"[byte & 0xf];
               dump__add_string__own(thrd_data, result__own, byte_string, owner);

               memcpy_inline(byte_string.bytes, ", 0bff", 6);
               for (u8 idx = 1; idx < len; idx += 1) {
                    byte                 = arg.bytes[idx];
                    byte_string.bytes[4] = "0123456789abcdef"[byte >> 4];
                    byte_string.bytes[5] = "0123456789abcdef"[byte & 0xf];
                    dump__add_string__own(thrd_data, result__own, byte_string, owner);
               }

               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);
               return;
          }
          default: {
               t_any       byte_string      = {.bytes = "0bff\n"};
               t_any const sub_level_spaces = dump__level_spaces(thrd_data, sub_level, owner);
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[byte_array\n"}, owner);
               for (u8 idx = 0; idx < len; idx += 1) {
                    ref_cnt__inc(thrd_data, sub_level_spaces, owner);
                    dump__add_string__own(thrd_data, result__own, sub_level_spaces, owner);
                    u8 const byte        = arg.bytes[idx];
                    byte_string.bytes[2] = "0123456789abcdef"[byte >> 4];
                    byte_string.bytes[3] = "0123456789abcdef"[byte & 0xf];
                    dump__add_string__own(thrd_data, result__own, byte_string, owner);
               }
               ref_cnt__dec(thrd_data, sub_level_spaces);

               dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level - 1, owner), owner);
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);
               return;
          }}
     }
     case tid__box: case tid__array: {
          u64          len;
          const t_any* items;
          bool const   is_array = arg.bytes[15] == tid__array;
          if (is_array) {
               u32 const slice_offset = slice__get_offset(arg);
               u32 const slice_len    = slice__get_len(arg);
               len                    = slice_len <= slice_max_len ? slice_len : slice_array__get_len(arg);
               items                  = &((const t_any*)slice_array__get_items(arg))[slice_offset];
          } else {
               len   = box__get_len(arg);
               items = box__get_items(arg);
          }

          if (len == 0) {
               dump__add_string__own(thrd_data, result__own, is_array ? (const t_any){.bytes = "[array]\n"} : (const t_any){.bytes = "[box]\n"}, owner);
               return;
          }

          dump__add_string__own(thrd_data, result__own, is_array ? (const t_any){.bytes = "[array\n"} : (const t_any){.bytes = "[box\n"}, owner);
          for (u64 idx = 0; idx < len; idx += 1)
               dump__half_own(thrd_data, result__own, items[idx], sub_level, sub_level + 1, owner);

          dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level - 1, owner), owner);
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);
          return;
     }
     case tid__breaker: {
          u8 const idx  = arg.bytes[14];
          t_any    item = arg;
          memcpy_inline(&item.bytes[14], &thrd_data->breakers_data[idx], 2);

          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[breaker\n"}, owner);
          dump__half_own(thrd_data, result__own, item, sub_level, sub_level + 1, owner);
          dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level - 1, owner), owner);
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);
          return;
     }
     case tid__error: {
          const t_error* const error_data = (const t_error*)arg.qwords[0];
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[error\n"}, owner);
          dump__half_own(thrd_data, result__own, error_data->id, sub_level, sub_level + 1, owner);
          dump__half_own(thrd_data, result__own, error_data->msg, sub_level, sub_level + 1, owner);
          dump__half_own(thrd_data, result__own, error_data->data, sub_level, sub_level + 1, owner);
          dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level - 1, owner), owner);
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);
          return;
     }
     case tid__obj: case tid__table: {
          bool const is_table = arg.bytes[15] == tid__table;

          u64 remain_qty = hash_map__get_len(arg);
          if (remain_qty == 0) {
               dump__add_string__own(thrd_data, result__own, is_table ? (const t_any){.bytes = "[table]\n"} : (const t_any){.bytes = "[obj]\n"}, owner);
               return;
          }

          dump__add_string__own(thrd_data, result__own, is_table ? (const t_any){.bytes = "[table\n"} : (const t_any){.bytes = "[obj\n"}, owner);
          if (!is_table) {
               t_any const mtds = *obj__get_mtds(arg);
               if (mtds.bytes[15] != tid__null) {
                    remain_qty -= 1;

                    dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level, owner), owner);
                    dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "key   = _\n"}, owner);
                    dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level, owner), owner);
                    dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "value = "}, owner);
                    dump__half_own(thrd_data, result__own, mtds, 0, sub_level + 1, owner);
               }
          }

          const t_any* const items = is_table ? table__get_kvs(arg) : obj__get_fields(arg);
          for (u64 idx = 0; remain_qty != 0; idx += 2) {
               t_any const key = items[idx];
               if (key.bytes[15] == tid__null || key.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level, owner), owner);
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "key   = "}, owner);
               dump__half_own(thrd_data, result__own, key, 0, sub_level + 1, owner);
               dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level, owner), owner);
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "value = "}, owner);
               dump__half_own(thrd_data, result__own, items[idx + 1], 0, sub_level + 1, owner);

          }

          dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level - 1, owner), owner);
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);
          return;
     }
     case tid__set: {
          u64 remain_qty = hash_map__get_len(arg);
          if (remain_qty == 0) {
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[set]\n"}, owner);
               return;
          }

          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[set\n"}, owner);
          const t_any* const items = set__get_items(arg);
          for (u64 idx = 0; remain_qty != 0; idx += 1) {
               t_any const item = items[idx];
               if (item.bytes[15] == tid__null || item.bytes[15] == tid__holder) continue;

               remain_qty -= 1;
               dump__half_own(thrd_data, result__own, item, sub_level, sub_level + 1, owner);
          }

          dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level - 1, owner), owner);
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);
          return;
     }
     case tid__byte_array: {
          u32 const slice_offset = slice__get_offset(arg);
          u32 const slice_len    = slice__get_len(arg);
          u64 const len          = slice_len <= slice_max_len ? slice_len : slice_array__get_len(arg);
          u8* const bytes        = &((u8*)slice_array__get_items(arg))[slice_offset];

          t_any       byte_string      = {.bytes = "0bff\n"};
          t_any const sub_level_spaces = dump__level_spaces(thrd_data, sub_level, owner);
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[byte_array\n"}, owner);
          for (u64 idx = 0; idx < len; idx += 1) {
               ref_cnt__inc(thrd_data, sub_level_spaces, owner);
               dump__add_string__own(thrd_data, result__own, sub_level_spaces, owner);
               u8 const byte        = bytes[idx];
               byte_string.bytes[2] = "0123456789abcdef"[byte >> 4];
               byte_string.bytes[3] = "0123456789abcdef"[byte & 0xf];
               dump__add_string__own(thrd_data, result__own, byte_string, owner);
          }
          ref_cnt__dec(thrd_data, sub_level_spaces);

          dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level - 1, owner), owner);
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);
          return;
     }
     case tid__string:
          long_string__dump__half_own(thrd_data, result__own, arg, owner);
          return;
     case tid__fn: {
          u8           const params_cap   = fn__get_params_cap(arg);
          u8           const params_len   = common_fn__get_params_len(arg);
          const t_any* const args         = fn__get_args(thrd_data, arg);
          u16          const borrowed_len = fn__get_borrowed_len(arg);
          const t_any* const borrowed     = fn__get_borrowed(thrd_data, arg);

          if (params_cap == 0) {
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[fn()\n"}, owner);
          } else if (params_cap == params_len) {
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[fn(_"}, owner);
               for (u8 counter = 1; counter < params_len; counter += 1)
                    dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = ", _"}, owner);
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = ")\n"}, owner);
          } else {
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[fn\n"}, owner);
               dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level, owner), owner);
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "(\n"}, owner);
               for (u8 idx = 0; idx < params_cap; idx += 1)
                    if (args[idx].bytes[15] == tid__holder) {
                         dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level + 1, owner), owner);
                         dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "_\n"}, owner);
                    } else dump__half_own(thrd_data, result__own, args[idx], sub_level + 1, sub_level + 2, owner);
               dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level, owner), owner);
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = ")\n"}, owner);
          }

          for (u16 idx = 0; idx < borrowed_len; idx += 1)
               dump__half_own(thrd_data, result__own, borrowed[idx], sub_level, sub_level + 1, owner);

          dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level - 1, owner), owner);
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);

          return;
     }
     case tid__thread:
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[thread]\n"}, owner);
          return;
     case tid__channel: {
          const t_channel_ptr* const channel_ptr = (const t_channel_ptr*)arg.qwords[0];
          t_channel*           const channel     = channel_ptr->channel;
          mtx_lock(&channel->mtx.mtx);
          u64 const len = channel->items_len;
          if (len == 0)
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[channel]\n"}, owner);
          else {
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[channel\n"}, owner);
               const t_any* const items = &channel->items[channel->idx_of_first_item];
               for (u64 idx = 0; idx < len; idx += 1)
                    dump__half_own(thrd_data, result__own, items[idx], sub_level, sub_level + 1, owner);

               dump__add_string__own(thrd_data, result__own, dump__level_spaces(thrd_data, sub_level - 1, owner), owner);
               dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "]\n"}, owner);
          }
          mtx_unlock(&channel->mtx.mtx);
          return;
     }
     case tid__custom: {
          void (*const dump__fn)(t_thrd_data* const, t_any* const, t_any const, u64 const, u64 const, const char* const) = custom__get_fns(arg)->dump;
          dump__fn(thrd_data, result__own, arg, level, sub_level, owner);

          if (result__own->bytes[15] != tid__short_string && result__own->bytes[15] != tid__string ) [[clang::unlikely]] {
               if (result__own->bytes[15] == tid__error) error__show_and_exit(thrd_data, *result__own);

               fail_with_call_stack(thrd_data, "The dump function did not return a string.", owner);
          }

          return;
     }

     [[clang::unlikely]]
     case tid__holder:
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[system_type]\n"}, owner);
          return;
     [[clang::unlikely]]
     default:
          dump__add_string__own(thrd_data, result__own, (const t_any){.bytes = "[unknown]\n"}, owner);
          return;
     }
}

core t_any McoreFNdump(t_thrd_data* const thrd_data, const t_any* const args) {
     const char* const owner = "function - core.dump";

     t_any const arg = args[0];

     t_any result = {};
     call_stack__push(thrd_data, owner);
     dump__half_own(thrd_data, &result, arg, 0, 1, owner);
     call_stack__pop(thrd_data);

     ref_cnt__dec(thrd_data, arg);
     return result;
}
