#ifndef CHECKADAPTERENABLED_H
#define CHECKADAPTERENABLED_H

#include <QString>

class IHelper;

class CheckAdapterEnable
{
public:    
    static void enable(IHelper *helper, const QString &adapterName);
    static void enableIfNeed(IHelper *helper, const QString &adapterName);
    static bool isAdapterDisabled(IHelper *helper, const QString &adapterName);
};

#endif // CHECKADAPTERENABLED_H
