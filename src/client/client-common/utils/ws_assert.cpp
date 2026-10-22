#include "ws_assert.h"

#include <QtGlobal>

#include "version/ws_version.h"

void ws_assert_failed(const char *expression, const char *file, int line)
{
#if defined(WINDSCRIBE_IS_BETA) || defined(WINDSCRIBE_IS_GUINEA_PIG)
    qCritical(LOG_ASSERT) << "Assertion failed! (" << file << ":" << line << ")";
#ifndef QT_NO_DEBUG
    qt_assert(expression, file, line);
#endif
#endif
    Q_UNUSED(expression);
    Q_UNUSED(file);
    Q_UNUSED(line);
}
