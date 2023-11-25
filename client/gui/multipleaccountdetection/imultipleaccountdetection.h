#pragma once

#include <QString>

class IMultipleAccountDetection
{
public:
    virtual ~IMultipleAccountDetection() {}
    virtual void userBecomeExpired(const QString &username) = 0;
    virtual bool entryIsPresent(QString &username) = 0;
    virtual void removeEntry() = 0;
};
