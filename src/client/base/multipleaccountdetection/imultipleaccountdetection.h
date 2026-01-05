#pragma once

#include <QString>

class IMultipleAccountDetection
{
public:
    virtual ~IMultipleAccountDetection() {}
    virtual void userBecomeExpired(const QString &username, const QString &userId) = 0;
    virtual bool entryIsPresent(QString &username, QString &userId) = 0;
    virtual void removeEntry() = 0;
};
