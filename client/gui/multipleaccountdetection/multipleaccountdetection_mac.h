#ifndef MULTIPLEACCOUNTDETECTION_MAC_H
#define MULTIPLEACCOUNTDETECTION_MAC_H

#include <QString>
#include <QDate>
#include "utils/simplecrypt.h"
#include "imultipleaccountdetection.h"

class MultipleAccountDetection_mac : public IMultipleAccountDetection
{
public:
    MultipleAccountDetection_mac();

    void userBecomeExpired(const QString &username) override;
    bool entryIsPresent(QString &username) override;
    void removeEntry() override;

private:
    struct TEntry
    {
        QString username_;
        QDate date_;
    };

    bool readEntry(TEntry &entry);
    void writeEntry(const TEntry &entry);

    static const QString entryName_;
    SimpleCrypt crypt_;
};

#endif // MULTIPLEACCOUNTDETECTION_MAC_H
