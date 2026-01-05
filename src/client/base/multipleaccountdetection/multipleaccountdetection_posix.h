#pragma once

#include <QString>
#include <QDate>
#include "utils/simplecrypt.h"
#include "imultipleaccountdetection.h"

class MultipleAccountDetection_posix : public IMultipleAccountDetection
{
public:
    MultipleAccountDetection_posix();

    void userBecomeExpired(const QString &username, const QString &userId) override;
    bool entryIsPresent(QString &username, QString &userId) override;
    void removeEntry() override;

private:
    struct TEntry
    {
        QString username_;
        QString userId_;
        QDate date_;
    };

    bool readEntry(TEntry &entry);
    void writeEntry(const TEntry &entry);

    static const QString entryName_;
    SimpleCrypt crypt_;
};
