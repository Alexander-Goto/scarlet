#pragma once

#include "../app/env/var.h"
#include "../common/fn.h"
#include "../common/include.h"
#include "../common/type.h"

#define fake_rnd_num_0 0xadb294741c7b36c5uwb
#define fake_rnd_num_1 0xcf396a4dc8a70756uwb
#define fake_rnd_num_2 0x9ee702757c3fad2fuwb
#define fake_rnd_num_3 0x43bcf00422367272uwb
#define fake_rnd_num_4 0xe4b8f7639f83b7b2uwb

static void platform__init_rnd_nums(u64* const rnd_nums, u8 const len) {
     assert(len != 0);

     if (fake_rnd_enable) {
          rnd_nums[0] = fake_rnd_num_0;
          if (len >= 4) {
               rnd_nums[1] = fake_rnd_num_1;
               rnd_nums[2] = fake_rnd_num_3;
               rnd_nums[3] = fake_rnd_num_3;
               if (len == 5) rnd_nums[4] = fake_rnd_num_4;
               else for (u8 idx = 5; idx < len; rnd_nums[idx] = __builtin_bswap64(__builtin_rotateleft64(rnd_nums[idx - 5], 11)), idx += 1);
          } else if (len <= 2) {
               if (len == 2) rnd_nums[1] = fake_rnd_num_1;
          } else {
               rnd_nums[1] = fake_rnd_num_1;
               rnd_nums[2] = fake_rnd_num_2;
          }

          return;
     }

#ifdef __linux__
     FILE* const file = fopen("/dev/urandom", "r");
     if (file == nullptr) [[clang::unlikely]] fail("Can't open the file \x22/dev/urandom\x22.");

     setvbuf(file, nullptr, _IONBF, 0);
     if (fread(rnd_nums, sizeof(u64), len, file) != len) [[clang::unlikely]] fail("Can't read the file \x22/dev/urandom\x22.");
     fclose(file);
#else
     __builtin_unreachable();
#endif
}
