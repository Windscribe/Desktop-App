#pragma once

#include <QDate>
#include <QString>
#include <windows.h>
#include "Utils/simplecrypt.h"

// used in MultipleAccountDetection_win for store login value
class SecretValue_win
{
public:
    struct TEntry
    {
        QString username_;
        QString userId_;
        QDate date_;
    };

    SecretValue_win();

    void setValue(const TEntry &entry);
    bool isExistValue(TEntry &entry);
    void removeValue();

private:
    static constexpr qint32 versionForSerialization_ = 1;
    SimpleCrypt crypt_;
};
