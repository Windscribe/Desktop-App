#pragma once

#include <sys/resource.h>

bool raiseOpenFileLimit(rlim_t target, rlim_t &result);
