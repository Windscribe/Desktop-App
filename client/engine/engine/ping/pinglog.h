#pragma once
#include <QString>

class PingLog
{
public:
    static void addLog(const QString &tag, const QString &str);
};

