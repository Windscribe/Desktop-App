#include "checkadapterenable.h"

#include "utils/log/categories.h"

bool CheckAdapterEnable::isAdapterDisabled(Helper *helper, const QString &adapterName)
{
    QString str = helper->executeWmicGetConfigManagerErrorCode(adapterName);
    QStringList list = str.split("\n");
    if (list.count() < 2)
    {
        qCCritical(LOG_BASIC) << "CheckAdapterEnable::isAdapterDisabled(), wmic command return incorrect output: " << str;
        return false;
    }
    QString par1 = list[0].trimmed();
    QString par2 = list[1].trimmed();
    if (par1 != "ConfigManagerErrorCode")
    {
        qCCritical(LOG_BASIC) << "CheckAdapterEnable::isAdapterDisabled(), wmic command return incorrect output: " << str;
        return false;
    }
    bool bOk;
    int code = par2.toInt(&bOk);
    if (!bOk)
    {
        qCCritical(LOG_BASIC) << "CheckAdapterEnable::isAdapterDisabled(), wmic command return incorrect output: " << str;
        return false;
    }
    return code != 0;
}
