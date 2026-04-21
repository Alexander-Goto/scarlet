#pragma once

#include "../../common/type.h"

static t_any const bool__false = {.bytes = {[15] = tid__bool}};
static t_any const bool__true  = {.bytes = {1, [15] = tid__bool}};