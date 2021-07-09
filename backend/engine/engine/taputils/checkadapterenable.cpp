#include "checkadapterenable.h"
#include "engine/helper/helper_win.h"
#include "utils/logger.h"

void CheckAdapterEnable::enable(IHelper *helper, const QString &adapterName)
{
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
    Q_ASSERT(helper_win);
    qCDebug(LOG_BASIC) << QString("%1 adapter seems disabled, try enable it").arg(adapterName);
    QString str = helper_win->executeWmicEnable(adapterName);
    qCDebug(LOG_BASIC) << "wmic enable command output:" << str;
}

void CheckAdapterEnable::enableIfNeed(IHelper *helper, const QString &adapterName)
{
    bool isDisabled = isAdapterDisabled(helper, adapterName);
    if (isDisabled)
    {
        enable(helper, adapterName);
    }
    else
    {
        qCDebug(LOG_BASIC) << QString("%1 adapter already enabled").arg(adapterName);
    }
}

bool CheckAdapterEnable::isAdapterDisabled(IHelper *helper, const QString &adapterName)
{
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
    Q_ASSERT(helper_win);
    QString str = helper_win->executeWmicGetConfigManagerErrorCode(adapterName);
    QStringList list = str.split("\n");
    if (list.count() < 2)
    {
        qCDebug(LOG_BASIC) << "CheckAdapterEnable::isAdapterDisabled(), wmic command return incorrect output: " << str;
        return false;
    }
    QString par1 = list[0].trimmed();
    QString par2 = list[1].trimmed();
    if (par1 != "ConfigManagerErrorCode")
    {
        qCDebug(LOG_BASIC) << "CheckAdapterEnable::isAdapterDisabled(), wmic command return incorrect output: " << str;
        return false;
    }
    bool bOk;
    int code = par2.toInt(&bOk);
    if (!bOk)
    {
        qCDebug(LOG_BASIC) << "CheckAdapterEnable::isAdapterDisabled(), wmic command return incorrect output: " << str;
        return false;
    }
    return code != 0;
}
