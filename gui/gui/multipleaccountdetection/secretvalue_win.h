#ifndef SECRETVALUE_WIN_H
#define SECRETVALUE_WIN_H

#include <QDate>
#include <QString>
#include <windows.h>
#include "Utils/simplecrypt.h"

// used in MultipleAccountDetection_win for store login value
class SecretValue_win
{
public:
    SecretValue_win();

    void setValue(const QString &value);
    bool isExistValue(QString &value);
    void removeValue();

private:
    SimpleCrypt crypt_;

    struct TOldEntry
    {
        QString username_;
        QDate date_;
    };

    void readOldEntry(TOldEntry &entry);
};

#endif // SECRETVALUE_WIN_H
