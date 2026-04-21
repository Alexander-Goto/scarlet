#pragma once

#include "include.h"

static const t_any tkn__access       = {.raw_bits = 0x9e780bdd575bfae4f1e7ad1d8b795f3uwb};
static const t_any tkn__change_dir   = {.raw_bits = 0x9e5954b32f3cf50aeb5ec5dc907d32fuwb};
static const t_any tkn__ctf8         = {.raw_bits = 0x9bb9e97438b2178e5d29745df701957uwb};
static const t_any tkn__cut          = {.raw_bits = 0x9e9a40617e3f1254f9247a6762a96f3uwb};
static const t_any tkn__delete       = {.raw_bits = 0x9aa20b7e96f500f68e5b4afb821f6c7uwb};
static const t_any tkn__encoding     = {.raw_bits = 0x9ae8ff8f2c285dd69d582379fd02217uwb};
static const t_any tkn__exists       = {.raw_bits = 0x9e7da0570ea2cfa8f30650d0cd8eb53uwb};
static const t_any tkn__flush        = {.raw_bits = 0x9b0885a478d1e7d0a40adc9dc46bbb3uwb};
static const t_any tkn__fs_limits    = {.raw_bits = 0x9e80615917ab3f4af393efe7795f91fuwb};
static const t_any tkn__list         = {.raw_bits = 0x9e9fc27c81b4ebb0fa3f9c923f4c1ebuwb};
static const t_any tkn__long         = {.raw_bits = 0x9ae90100a0ac05589d586681d9ea88buwb};
static const t_any tkn__make         = {.raw_bits = 0x9a97aae4a9f452bc8c4685a966e1ecbuwb};
static const t_any tkn__max_open     = {.raw_bits = 0x9bca1b13ae002f90cd225af8f7422b3uwb};
static const t_any tkn__not_dir      = {.raw_bits = 0x9e65560fc608c82cedc7c69557e0913uwb};
static const t_any tkn__not_empty    = {.raw_bits = 0x9a2ff798c9dda5f809fdce176adfabbuwb};
static const t_any tkn__not_exist    = {.raw_bits = 0x9e9774633ee792a0f894bf0926c2acbuwb};
static const t_any tkn__not_file     = {.raw_bits = 0x9a96c63403e7808a8c188461944b95fuwb};
static const t_any tkn__open         = {.raw_bits = 0x9bd8dfc3e3787dfcd0196649f2b4073uwb};
static const t_any tkn__read         = {.raw_bits = 0x9a78f2b273dba56085bd6eaef1b8d1buwb};
static const t_any tkn__roots_parent = {.raw_bits = 0x973ae9ff4241ec40f88ea09d44ef5afuwb};
static const t_any tkn__seek         = {.raw_bits = 0x9b6837c648ce9fa0b85ebac344a2fe3uwb};
static const t_any tkn__space        = {.raw_bits = 0x9aa552df1501346c8f04e0486b51bf3uwb};
static const t_any tkn__sys          = {.raw_bits = 0x9e8602c50c0d74bef4b789e4386c367uwb};

static const char* const msg__wrong_num = "Wrong number of function arguments.";

[[clang::always_inline]]
static t_custom_types_data* custom__data(t_any const custom) {return (t_custom_types_data*)custom.qwords[0];}

[[clang::always_inline]]
static void* custom__outside_data(t_any const custom) {return (void*)(custom.qwords[0] + sizeof(t_custom_types_data));}

[[gnu::cold, clang::noinline]]
static t_any handle_args_in_error_mtd(t_thrd_data* const thrd_data, const t_any* const args, u8 const args_len) {
     ref_cnt__dec(thrd_data, args[0]);

     t_any error = null;
     for (u8 idx = 1; idx < args_len; idx += 1) {
          t_any const arg = args[idx];
          if (arg.bytes[15] == tid__error && error.bytes[15] == tid__null)
               error = arg;
          else ref_cnt__dec(thrd_data, args[idx]);
     }

     return error;
}
