#ifndef MULTIPLEACCOUNTDETECTION_LINUX_H
#define MULTIPLEACCOUNTDETECTION_LINUX_H

#include <QString>
#include <QDate>
#include "utils/simplecrypt.h"
#include "imultipleaccountdetection.h"

class MultipleAccountDetection_linux : public IMultipleAccountDetection
{
public:
    MultipleAccountDetection_linux();

    void userBecomeExpired(const QString &username) override;
    bool entryIsPresent(QString &username) override;
    void removeEntry() override;
};

#endif // MULTIPLEACCOUNTDETECTION_LINUX_H
