#ifndef IMULTIPLEACCOUNTDETECTION_H
#define IMULTIPLEACCOUNTDETECTION_H

#include <QString>

class IMultipleAccountDetection
{
public:
    virtual ~IMultipleAccountDetection() {}
    virtual void userBecomeExpired(const QString &username) = 0;
    virtual bool entryIsPresent(QString &username) = 0;
    virtual void removeEntry() = 0;
};

#endif // IMULTIPLEACCOUNTDETECTION_H
