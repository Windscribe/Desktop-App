#ifndef GLOBALCONSTANTS_H
#define GLOBALCONSTANTS_H

#include <QString>
#include <QStringList>
#include "utils/simplecrypt.h"

class GlobalConstants
{
public:

    static GlobalConstants &instance()
    {
        static GlobalConstants s;
        return s;
    }

    QString serverUrl() { return serverUrl_; }

private:
    GlobalConstants();

    QString serverUrl_;
};

#endif // GLOBALCONSTANTS_H
