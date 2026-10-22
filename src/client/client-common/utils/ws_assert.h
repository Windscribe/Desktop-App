#pragma once

#include "log/categories.h"

// The macro is identical in every build type so that switching build types does not recompile its users;
// whether a failure is logged is decided in ws_assert.cpp.
#define WS_ASSERT(b) { \
  if (!(b)) { \
      ws_assert_failed(#b, __FILE__, __LINE__); \
  } \
}

void ws_assert_failed(const char *expression, const char *file, int line);
