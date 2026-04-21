#pragma once

#ifdef __linux__

#include <sys/sysinfo.h>

static unsigned int platform__get_cpu_cure_num() {
     return get_nprocs_conf();
}

#else

static unsigned int platform__get_cpu_cure_num() {
     __builtin_unreachable();
}

#endif