#pragma once

#ifdef __linux__

#include "../common/fn.h"
#include "../common/include.h"
#include "../common/type.h"

static void platform__init_rnd_nums(u64* const rnd_nums) {
     FILE* const file = fopen("/dev/urandom", "r");
     if (file == nullptr) [[clang::unlikely]] fail("Can't open the file \x22/dev/urandom\x22.");

     setvbuf(file, nullptr, _IONBF, 0);
     if (fread(rnd_nums, sizeof(u64), 5, file) != 5) [[clang::unlikely]] fail("Can't read the file \x22/dev/urandom\x22.");
     fclose(file);
}

#else

static void platform__init_rnd_nums(void*) {
     __builtin_unreachable();
}

#endif