#ifndef WS_ASSERT_H
#define WS_ASSERT_H

#include "logger.h"

#define WS_ASSERT(b) { \
  if (!(b)) { \
      qCDebug(LOG_ASSERT) << "Assertion failed! (" << __FILE__ << ":" << __LINE__ << ")"; \
  } \
  Q_ASSERT(b); \
}

#endif //WS_ASSERT_H