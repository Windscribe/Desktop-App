#pragma once

#include <QString>
#include <QSet>

class UniqueIpList
{
public:
    void add(const QString &ip);
    QSet<QString> get() const;

private:
    QSet<QString> set_;
};
