#pragma once

#include <QString>

class IHelper;

class CheckAdapterEnable
{
public:
    static bool isAdapterDisabled(IHelper *helper, const QString &adapterName);
};
