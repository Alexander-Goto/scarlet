#pragma once

#define _GNU_SOURCE

#ifdef FULL_DEBUG
#define DEBUG
#else
#define NDEBUG
#endif

#ifdef DEBUG
#define CALL_STACK
#endif

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <uchar.h>