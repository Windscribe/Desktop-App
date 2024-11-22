#include "pinglog.h"
#include "utils/log/categories.h"
#include "utils/extraconfig.h"

void PingLog::addLog(const QString &tag, const QString &str)
{
    if (ExtraConfig::instance().getLogPings())
        qCDebug(LOG_PING) << tag << "   " << str;
}
