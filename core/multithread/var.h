#pragma once

#include "../common/include.h"
#include "../common/type.h"

static t_mtx         sub_thrds_mtx;
static cnd_t         main_thrd_wait_cnd;
static u64           new_thrd_id          = 1;
static t_thrd_data** sub_thrds_data       = nullptr;
static u32           sub_thrds_cap        = 0;
static u32           sub_thrds_len        = 0;
static u32           active_sub_thrds     = 0;
static u32           cpu_cores_num;
static u16           sub_thrd_wait_secs   = 60;
static bool          allow_thrds          = false;
static bool          main_thread_waiting  = false;
