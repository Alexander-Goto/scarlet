#pragma once

#include "../../common/type.h"
#include "../../common/macro.h"

static u64                app_args_len;
static const char* const* app_args;
static u64                c_env_vars_len;
static const char* const* c_env_vars;
static u64                static_rnd_num;
static char*              sys_locale;
static u32                sys_lang;
static bool               sys_enc_is_utf8 = false;