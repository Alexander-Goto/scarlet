#pragma once

#define IMPORT_CORE_BASIC
#define IMPORT_CORE_ERROR
#define IMPORT_CORE_STRING
#include "core.h"

#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "nointr.h"