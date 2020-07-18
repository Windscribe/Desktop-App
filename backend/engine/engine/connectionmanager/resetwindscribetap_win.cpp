#include "resetwindscribetap_win.h"
#include <QProcess>
#include "Utils/logger.h"
#include "Engine/Helper/ihelper.h"

void ResetWindscribeTap_win::resetAdapter(IHelper *helper, const QString &tapName)
{
    qCDebug(LOG_BASIC) << "Reset TAP adapter";
    helper->executeResetTap(tapName);
}
